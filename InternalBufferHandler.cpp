#include "InternalBufferHandler.h"

/*
 * Létrehozás és megsemmisítés
 */
void IBH::begin(unsigned long sizeInBytes)
{
	_usedCount			= 0;					// nincs nyitott puffer
	_bigBufferLength	= sizeInBytes;			// ekkora a teljes lefoglalt terület
	_bigBufferStart		= (uint8_t*)malloc(_bigBufferLength);// lefoglaljuk a megadott területet
}

void IBH::end()
{
	// végignézzük a puffer "indexeket", és lezárjuk amelyik nyitva van
	for(uint8_t bufferIndex = 0; bufferIndex < IBH::MAX_BUFFER_COUNT; ++ bufferIndex)
	{
		if(_bufferLength[bufferIndex] != 0) // a jelenleg vizsgált puffer nyitva van
		{
			ReleaseBuffer(bufferIndex);
		}
	}
}

/*
 *
 * Kisebb "al"pufferek kezelése
 *
 */

uint8_t* IBH::GetBufferStart(uint8_t index)
{
	return _bufferStart[index];
}

size_t IBH::GetBufferLength(uint8_t index)
{
	return _bufferLength[index];
}

/* A CreateBuffer metódus lefoglal egy memóriaterületet és visszatér az indexével.
 */
// TODO: Ellenőrizni, hogy ez jól működik-e!!!
uint8_t IBH::CreateBuffer(size_t size)
{
	int index = GetFreeBufferIndex();
	if(index == IBH::MAX_BUFFER_COUNT) // nem lehet létrehozni új puffert
		return index;
	
	uint8_t* newBuffer = GetFreeMemoryPointer(size);
	if(newBuffer == 0) // new tudtunk lefoglalni neki a kért helyet
		return IBH::MAX_BUFFER_COUNT;
		
	_bufferStart[index] = newBuffer;
	_bufferLength[index] = size;
	
	uint8_t* last = _bigBufferStart;
	for(uint8_t i = 0; i < IBH::MAX_BUFFER_COUNT; ++ i)
	{
		if(newBuffer >= last && newBuffer < _bufferStart[_pointerOrder[i]]) // megtaláltuk, hogy hova kell beszúrni
		{
			uint8_t tmpIndex = index, tmpIndex2 = 0; // TODO: Ez súlyos optimalizálásra szorul...
			for(uint8_t j = i; j < IBH::MAX_BUFFER_COUNT; ++ j) // Ezzel a módszerrel az utolsó elveszik, de ha kaptunk szabad indexet, ott úgyis 0 volt
			{
				tmpIndex2 = _pointerOrder[j];
				_pointerOrder[j] = tmpIndex;
				tmpIndex = tmpIndex2;
			}
			break;
		}
	}
	++ _usedCount;
	return index;
}

// TODO: Ellenőrizni, hogy ez jól működik-e!!!
void IBH::ReleaseBuffer(uint8_t bufferIndex)
{
	_bufferStart[bufferIndex] = 0;
	_bufferLength[bufferIndex] = 0;
	for(uint8_t i = 0; i < _usedCount; ++ i)
	{
		if(_pointerOrder[i] == bufferIndex)
		{
			for(uint8_t j = i; j < _usedCount - 1; ++ j)
			{
				_pointerOrder[j] = _pointerOrder[j + 1];
			}
			_pointerOrder[_usedCount - 1] = 0;
			break;
		}
	}
	
	-- _usedCount;
}

/* Keres egy nem használt indexet.
 * Visszatérés:
 *  - egy 0-[MAX_BUFFER_COUNT - 1] közötti szám (mindkettőt beleértve), ha van szabad index,
 *  - MAX_BUFFER_COUNT, ha nincs.
 */
uint8_t IBH::GetFreeBufferIndex()
{
	for(uint8_t i = 0; i < IBH::MAX_BUFFER_COUNT; ++ i)
	{
		if(_bufferLength[i] == 0)
			return i;
	}
	return IBH::MAX_BUFFER_COUNT;
}

/* Megkeresi a legkisebb lyukat a nagy pufferben, ahova elfér egy megadott méretű adathalmaz.
 * Visszatérés: a pointer, ami ennek a területnek a kezdetére mutat, vagy 0, ha nincs ilyen
 */
// TODO: Ellenőrizni, hogy ez jól működik-e!!!
// TODO: _usedCount használata
uint8_t* IBH::GetFreeMemoryPointer(size_t size)
{
	uint8_t usedCount = 0;
	uint8_t usedIndices[IBH::MAX_BUFFER_COUNT];
	for(int i = 0; i < IBH::MAX_BUFFER_COUNT; ++ i)
	{
		if(_pointerOrder[i] != 0) // ezen az indexen van valami
		{
			usedIndices[usedCount ++] = _bufferStart[_pointerOrder[i]];
		}
	}
	
	uint32_t minSize	= _bigBufferLength;
	uint8_t* minStart	= 0; 
	if(_bufferStart[usedIndices[0]] - _bigBufferStart >= size) // A nagy puffer kezdete és az első bejegyzés között elég hely van
	{
		minSize		= _bufferStart[usedIndices[0]] - _bigBufferStart;
		minStart	= _bigBufferStart;
	}
	for(uint8_t i = 1; i < usedCount; ++ i)
	{
		size_t curSize = _bufferStart[usedIndices[i]] - _bufferStart[usedIndices[i - 1]] - _bufferLength[usedIndices[i - 1]];
		if(curSize >= size && curSize < minSize)
		{
			minSize = curSize;
			minStart = _bufferStart[usedIndices[i - 1]] + _bufferLength[usedIndices[i - 1]];
		}
	}
	size_t curSize = _bigBufferStart + _bigBufferLength - _bufferStart[usedIndices[usedCount - 1]] - _bufferLength[usedIndices[usedCount - 1]];
	if(curSize >= size && curSize < minSize)
	{
		minSize = curSize;
		minStart = _bufferStart[usedIndices[usedCount - 1]] + _bufferLength[usedIndices[usedCount - 1]];
	}
	
	return minStart;
}