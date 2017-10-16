#include "MSCPSlaveSerial.h"
#include "MSCPCommonSerial.h"

/* Inicializálja a slave-et.
 */
MSCPSlaveSerial::MSCPSlaveSerial()
{
	// még nem lett meghívva a begin(), kapcsolat állapota: nincs csatlakoztatva
	this->_hasBegun 		= false;
	this->_connectionState	= MSCPCommonSerial::CONN_STATE_DISCONNECTED;
	// alapból nincs hosszú üzenet ID, csak ha a kapcsolat másképp mondja
	this->_longMsgID		= false;
	// még nem kaptunk üzenetet, éppen nincs csomag, nincs mire válaszolni
	this->_lastMessageTime	= 0;
	this->_hasCurrentPkg	= false;
	this->_manualReplyState	= MSCPCommonSerial::REPLY_STATE_NO_ORIGINAL;
	this->_replyNeededCount	= 0;

	pinMode(MSCPCommonSerial::LED_PIN, OUTPUT);
}

/* - felszabadítja a jelenlegi csomagot mielőtt az objektum megsemmisül, ha van ilyen
 * - beállítja, hogy nem kezdődött el a jelenlegi objektum működése
 */
MSCPSlaveSerial::~MSCPSlaveSerial()
{
	disposeCurrentPackage();
	_hasBegun = false;
}

/* Elkezdi a slave működését.
 * beginSerial:	Megnyissa-e a begin() metódus a soros vonalat, vagy ne.
 * baud:		Ha megnyitja a soros vonalat, ekkora sebességgel. Ha nem, ezt ignorálja.
 * nodeID:		Zzt az ID-t kapja a slave node. Nem lehet 0.
 * maxMemory:	Ekkora buffert foglal le a memóriából a könyvtár, amiben később tárolja majd
 *				a csomagokat és egyéb előre nem tudható méretű adatokat (így megakadályozva a
 *				memória töredezését). 0 érték esetén nem lesz buffer.
 * Returns:		A metódus eredménye:
 *				- sikeres a megnyitás VAGY már eleve meg volt nyitva: MSCPCommonSerial::BEGIN_SUCCESS
 *				- hiba, a nodeID nem lehet 0: MSCPCommonSerial::BEGIN_ERROR_ZERONODE
 *				- hiba, a Serial port nem nyílt meg időben: MSCPCommonSerial::BEGIN_ERROR_SERIALOPEN
 */
byte MSCPSlaveSerial::begin(bool beginSerial, long baud, byte nodeID, long maxMemory) // UNDONE: maxMemory implementációja
{
	if(this->_hasBegun)
		return MSCPCommonSerial::BEGIN_SUCCESS;
	
	if(nodeID == 0)
		return MSCPCommonSerial::BEGIN_ERROR_ZERONODE;
	this->_nodeID = nodeID;
	
	if(beginSerial)
	{
		unsigned long m = millis();
		Serial.begin(baud);
		while(!Serial && millis() - m < MSCPCommonSerial::SerialOpenWaitMillis);
		if(!Serial) // A várakozás után még mindig nem nyílt meg
			return MSCPCommonSerial::BEGIN_ERROR_SERIALOPEN;
	}
	
	this->_hasBegun = true;
	return MSCPCommonSerial::BEGIN_SUCCESS;
}

/* Visszatér ennek a slave-nek a node ID-jával.
 * Ezt a user adta meg a begin() metódus paramétereként.
 */
byte MSCPSlaveSerial::getNodeID()
{
	return this->_nodeID;
}

/* Ha jelenleg van eltárolva bejövő csomag, felszabadítja az általa elfoglalt memóriaterületet.
 * Ezzel törli is a csomagot.
 */
void MSCPSlaveSerial::disposeCurrentPackage()
{
	if(_hasCurrentPkg)
	{
		free(_currentPkgStart);
		_hasCurrentPkg = false;
	}
}

/* Beállítja a jelenlegi csomag kezdetét és hosszát. Fejjel együtt megadandó.
 * A JELENLEGI CSOMAGOT TÖRLI, HA VAN ILYEN.
 */
void MSCPSlaveSerial::setCurrentPackage(byte* start, byte length)
{
	disposeCurrentPackage(); // ha van csomag jelenleg, ne maradjon a memóriában

	_currentPkgStart	= start;
	_currentPkgLength	= length;
	_hasCurrentPkg		= true;
}

bool MSCPSlaveSerial::packageAvailable()
{
	return _hasCurrentPkg;
}

bool MSCPSlaveSerial::packageAvailable(byte** dataStart, byte* dataLength)
{
	if(_hasCurrentPkg)
	{
		byte headSize = (_longMsgID ? 4 : 1) + 2;
		*dataStart	= _currentPkgStart	+ headSize;
		*dataLength	= _currentPkgLength	- headSize;
	}
	return _hasCurrentPkg;
}

/* Elvégzi azokat a háttérbeli műveleteket, amibe a felhasználónak nem kell beavatkoznia.
 * - fogadja a bejövő csomagokat (ezzel törli is az előzőleg eltároltat, ha van ilyen!)
 * - beállítja a kapcsolat állapotát
 */
void MSCPSlaveSerial::maintain(bool checkNewMsg)
{
	if(!this->_hasBegun)
		return;
	
	// üzenet fogadása
	if(checkNewMsg)
	{
		disposeCurrentPackage();

		byte* start; byte length;
		if(receive(&start, &length)) // kaptunk csomagot
		{
			setCurrentPackage(start, length);
		}
	}

	// ellenőrizzük a kapcsolat állapotát
	#ifdef DEBUG
	if(millis() - _lastMessageTime < 5000 && _connectionState == MSCPCommonSerial::CONN_STATE_CONNECTED)
	{
		digitalWrite(MSCPCommonSerial::LED_PIN, HIGH);
	}
	#endif

	// régebben kaptunk üzenetet, mint 5 sec, a kapcsolat hibás
	if(millis() - _lastMessageTime > 5000 && (_connectionState == MSCPCommonSerial::CONN_STATE_CONNECTED || _connectionState == MSCPCommonSerial::CONN_STATE_ERROR))
	{
		_connectionState = MSCPCommonSerial::CONN_STATE_ERROR;
		#ifdef DEBUG
		digitalWrite(MSCPCommonSerial::LED_PIN, ((millis() - _lastMessageTime) % 200 < 100) ? HIGH : LOW);
		#endif
	}

	// régebben kaptunk üzenetet, mint 20 sec, a kapcsolat felbomlott
	if(millis() - _lastMessageTime > 20000 && _connectionState != MSCPCommonSerial::CONN_STATE_DISCONNECTED)
	{
		_connectionState = MSCPCommonSerial::CONN_STATE_DISCONNECTED;
		#ifdef DEBUG
		digitalWrite(MSCPCommonSerial::LED_PIN, LOW);
		#endif
	}
}

// TODO: Tartozzon bele a csomagba a csomagazonosító byte és a fej is!
/* Fogad egy csomagot, elküldi rá az autoACK-t, és feldolgozza a csomagot.
 * start: ebben a kimeneti paraméterben lesz eltárolva a pointer a fogadott adatokhoz.
 * length: ebben a kimeneti paraméterben lesz eltárolva az adatok hossza byte-ban.
 * visszatérés: true, ha van adat a két paraméterben; false, ha nincs
 */
bool MSCPSlaveSerial::receive(byte** start, byte* length)
{
	// ellenőrizzük, hogy érkezett-e újabb üzenet
	if(Serial.available()) // több, mint 0 byte van a Serial bufferében
	{
		// beolvassuk a csomagazonosító byte-ot
		byte		first				= Serial.read(),
					tLength,			// tLength: hátralévő byte-ok száma
					fullLength;
		uint16_t	protocolVersion;
		first >>= 3;

		// nem üdvözlő csomag ÉS nincs csatlakoztatva
		if(first != 1 && _connectionState == MSCPCommonSerial::CONN_STATE_DISCONNECTED)
			return false;

		// beállítjuk a fogadandó csomag teljes hosszát
		unsigned long	current	= millis();	// case 2
		bool			canGo	= false;	// case 2
		switch(first)
		{
			case 0: // standard csomag
				tLength = _standardSize;
				break;
			case 1: // üdvözlő csomag
				while(millis() - current < 50 && !canGo) // adunk 50ms-ot a következő 3 byte érkezésére
				{
					if(Serial.available() >= 3)
					{
						protocolVersion	= Serial.read();
						protocolVersion	<<= 8;
						protocolVersion	+= Serial.read();
						fullLength		= Serial.read();
						tLength			= fullLength;
						canGo			= true;
					}
				}
				if(!canGo) // hibás csomag?
				{
					return false;
				}
				break;
			case 2: // kapcsolatbontó csomag
				tLength = 2;
				break;
			case 3: // kapcsolatfenntartó csomag
				tLength = 2;
				break;
			case 4: // poll csomag [master --> slave] irányban
				tLength = 2 + (_longMsgID ? 4 : 1);
				break;
		}

		// beállítjuk a fogadáshoz szükséges változók értékét
				tLength		-= 1; // a csomag azonosító byte is benne volt ebben...
				fullLength	= tLength; // eltároljuk a csomagazonosító nélküli hosszot
		byte*	package		= (byte*)malloc(tLength - (first == 1 ? 3 : 0));
	//	byte	pkgLength	= tLength;
		byte	received	= 0;
		if(first == 1) // üdvözlő csomag
		{
			*package		= (byte)((protocolVersion >> 8) % 256);
			*(package + 1)	= (byte)((protocolVersion) % 256);
			*(package + 2)	= fullLength;
			tLength			-= 3;
			received		+= 3;
		}
		
		// ténylegesen fogadjuk a csomagot
		current = millis();
		while(millis() - current < 50 && tLength) // 50ms-et várunk az adatok beérkeztére, de csak ha még várunk adatot
		{
			int available = Serial.available();
			if(!available) // nincs mit fogadni
				continue;
			
			if(available < tLength)
			{
				Serial.readBytes(package + received, available);
				received += available;
				tLength -= available;
			}
			else
			{
				Serial.readBytes(package + received, tLength);
				received += tLength;
				tLength = 0;
			}
		}
		
		// ellenőrizzük, hogy nekünk jött-e az üzenet
		byte slaveNodePos = 0;
		switch(first)
		{
			case 0: // standard csomag
				slaveNodePos = _longMsgID ? 4 : 0;
				break;
			case 1: // üdvözlő csomag
				slaveNodePos = 3;
				break;
			case 2: // kapcsolatbontó csomag
				slaveNodePos = 1;
				break;
			case 3: // kapcsolatfenntartó csomag
				slaveNodePos = 1;
				break;
			case 4: // poll csomag
				slaveNodePos = 1;
				break;
		}
		
		if(*(package + slaveNodePos) != _nodeID && *(package + slaveNodePos) != 0) // a 0 a broadcast üzenet
			return false;
		
		// beállítjuk az üzenet fogadásának idejét, hogy a kapcsolatállapotot megállapíthassuk vele
		_lastMessageTime = millis(); // beállítjuk, hogy most kaptunk üzenetet utoljára
		_connectionState = MSCPCommonSerial::CONN_STATE_CONNECTED;

		// elküldjük az üzenetre az automatikus választ (autoACK)
		byte reply[2];

		reply[0] = (byte)(5 << 3);
		reply[1] = _nodeID;

		Serial.write(reply, 2);
		
		// felodolgozzuk az üzenetet
		// kapcsolatfenntartó csomag esetén nincs teendő a switchben
		// ACK csomagot nem kapunk, csak mi küldünk
		switch(first)
		{
			case 0: // standard csomag
				*start	= package;
				*length	= fullLength;
				if(_manualReplyState == MSCPCommonSerial::REPLY_STATE_NO_ORIGINAL)
				{
					_manualReplyState = MSCPCommonSerial::REPLY_STATE_WAITING;
				}
				else
				{
					++ _replyNeededCount;
				}
				return true;
			case 1: // üdvözlő csomag
				localConnect(package, received);
				break;
			case 2: // kapcsolatbontó csomag
				_connectionState = MSCPCommonSerial::CONN_STATE_DISCONNECTED;
				#ifdef DEBUG
				digitalWrite(MSCPCommonSerial::LED_PIN, LOW);
				#endif
				break;
			case 4: // poll csomag
				digestPoll(package, received);
				break;
		}

		// nem kell továbbítani a felhaszáló felé a csomagot
		free(package);
		return false;
	}
}

// a csomagot a csomagazonosító byte nélkül kapja meg
void MSCPSlaveSerial::localConnect(byte* pkg, byte length)
{
	// a protokoll verziót egyenlőre skippelhetjük
	// a hosszt megkapjuk
	// a slave node ID-t már hamarabb leellenőrizzük
	byte currentIndex = 4;
	
	_standardSize = *(pkg + currentIndex);
	++ currentIndex;

	_longMsgID = ((*(pkg + currentIndex) % 2) == 1); // az utolsó bit

	_connectionState = MSCPCommonSerial::CONN_STATE_CONNECTED;
}

// a csomagot a csomagazonosító byte nélkül kapja meg
// TODO: ellenőrizni, hogy elég rövid a válasz hogy beleférjen a csomagba!
void MSCPSlaveSerial::digestPoll(byte* pkg, byte length)
{
	// [0]: slave node ID
	// [1] vagy [1-4]: original Msg ID

	unsigned long origMsgID = 0;
	if(_longMsgID)
	{
		for(byte i = 4; i > 0; -- i)
		{
			origMsgID <<= 8;
			origMsgID += *(pkg + i);
		}
	}
	else
		origMsgID = *(pkg + 1);
	
	// küldünk választ
	if(_manualReplyState != MSCPCommonSerial::REPLY_STATE_NO_ORIGINAL && origMsgID == _manualReplyOriginalMsgID)
	{
		byte size = 2 + (_longMsgID ? 4 : 1) + 2 + _manualReplyLength;
		byte reply[_standardSize];
		byte currentIndex = 0;
		
		reply[currentIndex ++] = 4 << 3; // csomagazonosító: ez egy poll csomag
		reply[currentIndex ++] = _nodeID; // a választ ettől a node-tól kapja
		
		if(_longMsgID) // hosszú üzenet ID beállítása
		{
			for(byte i = 0, max = 4; i < max; ++ i)
			{
				// helyiértékeket megőrizve byte-ra daraboljuk az ID-t
				reply[currentIndex ++] = (_manualReplyOriginalMsgID >> (8 * (max - i - 1))) % 256;
			}
		}
		else // rövid üzenet ID beállítása
		{
			reply[currentIndex ++] = _manualReplyOriginalMsgID;
		}

		reply[currentIndex ++] = _manualReplyState;
		if(_manualReplyState == MSCPCommonSerial::REPLY_STATE_HASREPLY)
		{
			reply[currentIndex ++] = _manualReplyLength;
			for(byte i = 0; i < _manualReplyLength; ++ i)
			{
				reply[currentIndex ++] = *(_manualReplyStart + i);
			}
			free(_manualReplyStart); // lemásoltuk, innentől nem kell
		}

		while(currentIndex < _standardSize)
		{
			reply[currentIndex ++] = 0;
		}

		Serial.write(reply, _standardSize); // a metódus végén a reply[] felszabadul automatikusan, mivel tömb

		if(_replyNeededCount != 0)
		{
			_manualReplyState = MSCPCommonSerial::REPLY_STATE_WAITING;
		}
		else
		{
			_manualReplyState = MSCPCommonSerial::REPLY_STATE_NO_ORIGINAL;
		}
	}
}

bool MSCPSlaveSerial::manualReply(byte* start, byte length, unsigned long originalMsgID)
{
	if(_manualReplyState == MSCPCommonSerial::REPLY_STATE_WAITING)
	{
		_manualReplyStart = (byte*)malloc(length);
		_manualReplyLength = length;
		for(byte i = 0; i < length; ++ i) // lemásoljuk a választ, hogy a függvény visszatérése után a hívó rögtön törölhesse
		{
			*(_manualReplyStart + i) = *(start + i);
		}
		
		_manualReplyOriginalMsgID = originalMsgID;
		_manualReplyState = MSCPCommonSerial::REPLY_STATE_HASREPLY;
		-- _replyNeededCount;
		return true;
	}
	else
	{
		return false;
	}
}

bool MSCPSlaveSerial::noManualReply()
{
	if(_manualReplyState == MSCPCommonSerial::REPLY_STATE_WAITING)
	{
		-- _replyNeededCount;
		_manualReplyState = MSCPCommonSerial::REPLY_STATE_NOREPLY;
		return true;
	}
	else
	{
		return false;
	}
}