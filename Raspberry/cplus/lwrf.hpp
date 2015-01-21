#ifndef LWRF_HPP
#define LWRF_HPP

#include <stdint.h>
#include <queue>
#include <string>

#define LWRF_MSGLEN 10
#define LWRF_MAXMSGS 100
#define RX_MSG_TIMEOUT 1000000

#define RX_STATE_IDLE 0
#define RX_STATE_MSGSTARTFOUND 1
#define RX_STATE_BYTESTARTFOUND 2
#define RX_STATE_GETBYTE 3

#define TX_HIGH 280
#define TX_LOW 980
#define TX_GAP 10800

int sym2nibble(int symbol);
std::string makeMsg(char* rMessage);

class LwrfTX
{
   /* 
   For transmitting LW messages
   */
   int txBusy;
   int mygpio;
   int dbg;

   public:
   LwrfTX(int gpio);
   int Busy();
   void Put(std::string msg, int repeat=1);
   int Debug();
};

class LwrfRX
{
   /*
   This class receives from a 433MHz RX and tries to decode LWRF messages
   */

   int mygpio, myRepeat;
   std::queue<std::string> msgqueue;
   char rMessage[10];
   std::string rMsg;
   int rData, rByte, rBit, rState, rDuplicate, repeatCount;
   uint32_t lastTick, lastMessageTick;
   int dbg;

   void _callback(int gpio, int level, uint32_t tick);

   /* Need a static callback to link with C. */
   static void _callbackExt(int gpio, int level, uint32_t tick, void *user);

   public:
   LwrfRX(int gpio, int repeat=1);
   int Ready();
   std::string Get();
   int Debug();
};

#endif

