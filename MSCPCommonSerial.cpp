#include "MSCPCommonSerial.h"

byte MSCPCommonSerial::CONN_STATE_DISCONNECTED  = 0;
byte MSCPCommonSerial::CONN_STATE_CONNECTED     = 1;
byte MSCPCommonSerial::CONN_STATE_ERROR         = 2;

byte MSCPCommonSerial::REPLY_STATE_NOREPLY      = 0;
byte MSCPCommonSerial::REPLY_STATE_WAITING      = 1;
byte MSCPCommonSerial::REPLY_STATE_HASREPLY     = 2;
byte MSCPCommonSerial::REPLY_STATE_NO_ORIGINAL  = 3;

byte MSCPCommonSerial::BEGIN_SUCCESS			= 0;
byte MSCPCommonSerial::BEGIN_ERROR_ZERONODE		= 1;
byte MSCPCommonSerial::BEGIN_ERROR_SERIALOPEN	= 2;

uint16_t MSCPCommonSerial::SerialOpenWaitMillis	= 500;