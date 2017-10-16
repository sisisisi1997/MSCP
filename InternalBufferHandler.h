/* Ez az osztály azt a célt szolgálja, hogy a memóriát úgy tudjuk dinamikusan kiosztani, hogy ne töredezzen fel.
 * Ezt úgy érjük el, hogy egyetlen malloc()-kal lefoglalunk egy nagy területet, majd ezt manuálisan a malloc-nál
 * valamivel intelingensebben kezeljük, további kisebb területeket innen osztunk ki.
 */

#ifndef INTERNALBUFFERHANDLER_H
#define INTERNALBUFFERHANDLER_H

#include <Arduino.h>

class IBH
{
	public:
		static			void				begin(unsigned long sizeInBytes);
		static			void				end();

		static			uint8_t*			GetBufferStart(uint8_t index);
		static			size_t				GetBufferLength(uint8_t index);
		static			uint8_t				CreateBuffer(size_t size);
		static			void				ReleaseBuffer(uint8_t buffer);
		static			void				optimise(); // Not implemented
		
		static const	uint8_t				MAX_BUFFER_COUNT					= 16;
	
	private:
		static			uint8_t*			_bufferStart[MAX_BUFFER_COUNT];			// indexek sorrendjében a címeket tartalmazza
		static			size_t				_bufferLength[MAX_BUFFER_COUNT];			// indexek sorrendjében a hosszakat tartalmazza
		static			uint8_t				_pointerOrder[MAX_BUFFER_COUNT];	// a pufferindexeket tartalmazza
		static			uint8_t				_usedCount; // TODO: Ezt is fel kéne használni

		static			uint8_t*			_bigBufferStart;
		static			size_t				_bigBufferLength;

		static			uint8_t				GetFreeBufferIndex();
		static			uint8_t*			GetFreeMemoryPointer(size_t size);
};

#endif