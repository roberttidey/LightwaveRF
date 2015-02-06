#ifndef CUSTOM_LWRF_XX
#define CUSTOM_LWRF_XX

//Interface commands for LWRF Pigpio gpioCustom_2
#define CUSTOM_LWRF 7287
#define CUSTOM_LWRF_TX_INIT   1
#define CUSTOM_LWRF_TX_BUSY   2
#define CUSTOM_LWRF_TX_PUT    3
#define CUSTOM_LWRF_TX_CANCEL 4
#define CUSTOM_LWRF_TX_DEBUG  5
#define CUSTOM_LWRF_RX_INIT   10
#define CUSTOM_LWRF_RX_CLOSE  11
#define CUSTOM_LWRF_RX_READY  12
#define CUSTOM_LWRF_RX_GET    13
#define CUSTOM_LWRF_RX_DEBUG  14

//Lightwave constants
#define LWRF_MSGLEN 10
#define LWRF_MAXMESSAGES 16
#define LWRF_MAXREPEAT 25
#define LWRX_MSG_TIMEOUT 1000000

#define LWRX_STATE_IDLE 0
#define LWRX_STATE_MSGSTARTFOUND 1
#define LWRX_STATE_BYTESTARTFOUND 2
#define LWRX_STATE_GETBYTE 3

#define LWTX_HIGH 280
#define LWTX_LOW 980
#define LWTX_GAP 10800

/* 
For transmitting LW messages
*/
int lwtxGpio;
char lwtxDebug[64];

int lwtxPut(char* msg, char repeat);

/*
Receives from a 433MHz RX and tries to decode LWRF messages
*/
int lwrxGpio, lwrxRepeat;
char lwrxMsgQueue[LWRF_MAXMESSAGES][LWRF_MSGLEN + 1];
int lwrxQueueHead;
int lwrxQueueTail;
char lwrxMessage[LWRF_MSGLEN + 1];
int lwrxData, lwrxByte, lwrxBit, lwrxState, lwrxDuplicate, lwrxRepeatCount;
uint32_t lwrxLastTick, lwrxLastMessageTick;
char lwrxDebug[64];

void _lwrxCallback(int gpio, int level, uint32_t tick);
int lwrxQueueSize();
#endif

