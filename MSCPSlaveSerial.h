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
		bool			manualReply(byte* start, byte length, unsigned long originalMsgID);
		bool			noManualReply();
	private:
		// példány specifikus
		bool			_hasBegun;
		byte			_nodeID;
		unsigned long	_memoryBufferSize;

		
		// csomagspecifikus
		byte*			_currentPkgStart;
		byte			_currentPkgLength;
		bool			_hasCurrentPkg;

		byte*			_manualReplyStart;
		byte			_manualReplyLength;
		unsigned long	_manualReplyOriginalMsgID;
		byte			_manualReplyState;
		uint8_t			_replyNeededCount;

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
};

#endif