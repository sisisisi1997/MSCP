#ifndef MSCPCOMMONSERIAL_H
#define MSCPCOMMONSERIAL_H

#define DEBUG

#include <Arduino.h>

class MSCPCommonSerial
{
	public:
		static			uint16_t	SerialOpenWaitMillis;
		static			byte		CONN_STATE_DISCONNECTED,
									CONN_STATE_CONNECTED,
									CONN_STATE_ERROR;

		static			byte		REPLY_STATE_NOREPLY,
									REPLY_STATE_WAITING,
									REPLY_STATE_HASREPLY,
									REPLY_STATE_NO_ORIGINAL;
								
		static			byte		BEGIN_SUCCESS,
									BEGIN_ERROR_ZERONODE,
									BEGIN_ERROR_SERIALOPEN;
		
		static const	byte		LED_PIN						= 13;
};

#endif