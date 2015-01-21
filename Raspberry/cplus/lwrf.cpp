#include "pigpio.h"

#include "lwrf.hpp"

int SYMBOLS[16] = {0xF6,0xEE,0xED,0xEB,0xDE,0xDD,0xDB,0xBE,0xBD,0xBB,0xB7,0x7E,0x7D,0x7B,0x77,0x6F};
char NIBBLES[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

int sym2nibble(int symbol)
{
   /*
      Translate a symbol byte to nibble
   */
   int i = 15;
   while(i > 0 && symbol != SYMBOLS[i])
   {
      i--;
   }
   return i;
}

std::string makeMsg(char* rMessage)
{
   /*
      Make message string from message nibble array
   */
   std::string msg = "";
   for(int i = 0; i < LWRF_MSGLEN; i++)
   {
      msg += NIBBLES[rMessage[i]];
   }
   return msg;
}

void LwrfTX::Put(std::string msg, int repeat)
{
   /*
      Sets up a message waveform and sends it
   */
   gpioPulse_t wf[200];
   uint32_t tPulse, txId;
   uint32_t txbit = (1<<mygpio);
   txBusy = 0;
   gpioSetMode(mygpio, PI_OUTPUT);

   if(msg.size() >= LWRF_MSGLEN)
   {
      txBusy = 1;
      gpioWaveTxStop();
      //Send a high for a bit to train agc
      gpioWrite(mygpio, 1);
      gpioSleep(PI_TIME_RELATIVE, 0, TX_GAP);
      tPulse = 0;
      //PreMessage Gap
      wf[tPulse].gpioOn = 0; wf[tPulse].gpioOff = txbit; wf[tPulse].usDelay = TX_GAP; tPulse++; 
      //Message start pulse
      wf[tPulse].gpioOn = txbit; wf[tPulse].gpioOff = 0; wf[tPulse].usDelay = TX_HIGH; tPulse++; 
      wf[tPulse].gpioOn = 0; wf[tPulse].gpioOff = txbit; wf[tPulse].usDelay = TX_HIGH; tPulse++; 
      for(int i = 0; i < LWRF_MSGLEN; i++)
      {
         //Byte start pulse
         wf[tPulse].gpioOn = txbit; wf[tPulse].gpioOff = 0; wf[tPulse].usDelay = TX_HIGH; tPulse++; 
         wf[tPulse].gpioOn = 0; wf[tPulse].gpioOff = txbit; wf[tPulse].usDelay = TX_HIGH; tPulse++;
         char symc = msg.at(i);
         int sym = (symc >= 'A') ? (symc - 'A' + 10) : (symc - '0');
         sym = SYMBOLS[sym & 0xF];
         for(int j = 0; j < 8; j++)
         {
            if(sym & (0x80>>j))
            {
               wf[tPulse].gpioOn = txbit; wf[tPulse].gpioOff = 0; wf[tPulse].usDelay = TX_HIGH; tPulse++; 
               wf[tPulse].gpioOn = 0; wf[tPulse].gpioOff = txbit; wf[tPulse].usDelay = TX_HIGH; tPulse++;
            }
            else
            {
               wf[tPulse].gpioOn = 0; wf[tPulse].gpioOff = 0; wf[tPulse].usDelay = TX_LOW; tPulse++;
            }
         }
      }
      //Message end pulse
      wf[tPulse].gpioOn = txbit; wf[tPulse].gpioOff = 0; wf[tPulse].usDelay = TX_HIGH; tPulse++; 
      wf[tPulse].gpioOn = 0; wf[tPulse].gpioOff = txbit; wf[tPulse].usDelay = TX_HIGH; tPulse++; 
      gpioWaveAddGeneric(tPulse, wf);
      txId = gpioWaveCreate();
      if(txId == 0)
      {
         for(int r = 0; r < repeat; r++)
         {
            gpioWaveTxSend(txId, PI_WAVE_MODE_ONE_SHOT);
            while(gpioWaveTxBusy() == 1)
            {
               gpioSleep(PI_TIME_RELATIVE, 0, 1000);
            }
         }
      }
   txBusy = 0;
   }
}

int LwrfTX::Busy()
{
   /*
      Returns 0 if clear or 1 if still sending last message
   */
   if(gpioWaveTxBusy() || txBusy)
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

int LwrfTX::Debug()
{
   /*
      Returns debug int
   */
   return dbg;
}


LwrfTX::LwrfTX(int gpio)
{
   /*
      Initialises an LwrfTX transmitter on a gpio.
   */
   mygpio = gpio;
}


void LwrfRX::_callback(int gpio, int level, uint32_t tick)
{
   if (level != PI_TIMEOUT)
   {
      // Get microseconds since last change
      uint32_t pulse = tick - lastTick;
      lastTick = tick;
      int trans = 0;
      if (pulse > 150)
      {
         if (pulse < 500)
         {
            trans = level + 2; //normal short pulse
         }
         else if (pulse < 2000)
         {
            trans = level + 4; //normal long pulse
         }
         else if (pulse > 5000)
         {
            trans = level + 6; //gap between messages
         }
         else
         {
            trans = 8;
         }
         //State machine
         switch(rState)
         {
            case RX_STATE_IDLE:
               switch(trans)
               {
                  case 7:  //1 after a message gap
                     rState = RX_STATE_MSGSTARTFOUND;
                     rDuplicate = 1;
                     break;
               }
               break;
            case RX_STATE_MSGSTARTFOUND:
               switch(trans)
               {
                  case 2:  //nothing to do wait for next 1
                     break;
                  case 3:  //start of a byte detected
                     rByte = 0;
                     rState = RX_STATE_BYTESTARTFOUND;
                     break;
                  default: //not good start again
                     rState = RX_STATE_IDLE;
                     break;
               }
               break;
            case RX_STATE_BYTESTARTFOUND:
               switch(trans)
               {
                  case 2:  //nothing to do wait for next 1
                     break;
                  case 3:  //1 bit detected
                     rData = 0;
                     rBit = 0;
                     rState = RX_STATE_GETBYTE;
                     break;
                  case 5:  //starts with 0 so enter it
                     rData = 0;
                     rBit = 1;
                     rState = RX_STATE_GETBYTE;
                     break;
                  default: //not good start again
                     rState = RX_STATE_IDLE;
                     break;
               }
               break;
            case RX_STATE_GETBYTE:
               switch(trans)
               {
                  case 2:  //nothing to do wait for next 1
                     break;
                  case 3:  //1 bit detected
                     rData = rData << 1 | 1;
                     rBit++;
                     break;
                  case 5:  //0 so enter it
                     rData = rData << 2 | 2;
                     rBit++;
                     rBit++;
                     break;
                  default: //not good start again
                     rState = RX_STATE_IDLE;
                     break;
               }
               //Check if byte complete
               if(rBit >= 8)
               {
                  //Translate symbols to nibbles and enter message
                  rData = sym2nibble(rData);
                  if (rData != rMessage[rByte]) //# Is it same as last packet
                  {
                     rDuplicate = 0;
                     repeatCount = 0;
                  }
                  rMessage[rByte] = rData;
                  rByte++;
                  rBit = 0;
                  if (rByte >= LWRF_MSGLEN) //Is packet complete
                  {
                     if (((lastTick - lastMessageTick) > RX_MSG_TIMEOUT) || lastMessageTick == 0)
                     {
                        //Long time since last message so reset the counter
                        repeatCount = 0;
                     }
                     else if (rDuplicate > 0)
                     {
                        repeatCount++;
                     }
                     if (myRepeat == 0 || repeatCount == myRepeat)
                     {
                        msgqueue.push(makeMsg(rMessage));
                     }
                     rState = RX_STATE_IDLE;
                     lastMessageTick = lastTick;
                  }
                  else
                  {
                     rState = RX_STATE_BYTESTARTFOUND;
                  }
               }
               break;
         }
      }
   }
   else
   {
      gpioSetWatchdog(mygpio, 0);
   }
}

void LwrfRX::_callbackExt(int gpio, int level, uint32_t tick, void *user)
{
   /*
      Need a static callback to link with C.
   */
   LwrfRX *mySelf = (LwrfRX *) user;
   mySelf->_callback(gpio, level, tick); /* Call the instance callback. */
}

int LwrfRX::Ready()
{
   /*
      Returns count of messages on Queue
   */
   return msgqueue.size();
}

std::string LwrfRX::Get()
{
   /*
      Returns next message on Queue
   */
   if (msgqueue.size() > 0)
   {
      std::string msg = msgqueue.front();
      msgqueue.pop();
      return msg;
   }
   else
   {
   return "";
   }
}

int LwrfRX::Debug()
{
   /*
      Returns debug int
   */
   return dbg;
}


LwrfRX::LwrfRX(int gpio, int repeat)
{
   /*
      Initialises an LwrfRX monitor on a gpio.
   */
   mygpio     = gpio;
   myRepeat   = repeat;
   
   rDuplicate = 0;
   repeatCount = 0;
   lastMessageTick = 0;
   rBit = 0;
   rByte = 0;
   rState = RX_STATE_IDLE;
   rData = 0;
   lastTick = gpioTick();
   dbg = 0;

   gpioSetMode(gpio, PI_INPUT);
   gpioSetAlertFuncEx(gpio, _callbackExt, (void *)this);
}

