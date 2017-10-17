#ifndef MSCPSLAVESERIAL_H
#define MSCPSLAVESERIAL_H

#include <Arduino.h>
// #include "InternalBufferHandler.h"

class MSCPSlaveSerial
{
	public:
						MSCPSlaveSerial();
						~MSCPSlaveSerial();
		byte			begin(bool, long, byte, long);
		byte			getNodeID();

		void			maintain(bool);
		void			disposeCurrentPackage();
		bool			packageAvailable();
		bool			packageAvailable(byte**, byte*);
		bool			manualReply(byte* start, byte length, uint32_t originalMsgID);
		bool			noManualReply(uint32_t msgID);
	private:
		// példány specifikus
		bool			_hasBegun;
		byte			_nodeID;
		unsigned long	_memoryBufferSize;
		bool			_hadConnection;
		
		// csomagspecifikus
		byte*			_currentPkgStart;
		byte			_currentPkgLength;
		bool			_hasCurrentPkg;

		byte*			_manualReplyStart;
		byte			_manualReplyLength;
		uint32_t		_manualReplyOriginalMsgID;
		byte			_manualReplyState;
		
		uint8_t			_maxMessageCount;	// maximum ennyi üzenetet tárolunk el egyszerre
		uint8_t			_replyNeededCount;	// ennyi elem van jelenleg az alábbi tömbökben
		uint8_t*		_origIDs8;			// ez a tömb tárolja el a kapott üzenetek ID-ját ha nincs long message ID
		uint32_t*		_origIDs32;			// ez a tömb tárolja el a kapott üzenetek ID-ját ha van long message ID
		uint8_t*		_replyLengths;		// ez a tömb tárolja a válaszok hosszait
		uint8_t**		_replyStarts;
		uint8_t*		_replyStates;		// ez a tömb tárolja el a kapott üzenetek állapotát
		bool*			_isUsed;

		// kapcsolat specifikus
		byte			_connectionState;
		byte			_standardSize;
		bool			_longMsgID;
		unsigned long	_lastMessageTime;

		// metódusok
		bool			receive(byte**, byte*);
		void			localConnect(byte* pkg, byte length);
		void			setCurrentPackage(byte* start, byte length);
		void			digestPoll(byte* pkg, byte length);
		uint8_t			getReplyIndex(uint32_t msgID, bool* found);
		uint8_t			getFreeReplyIndex(bool* found);
};

#endif