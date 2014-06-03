#include <LwTx.h>
#if EEPROM_EN == 1
#include <EEPROM.h>
#endif
#define echo true

#define feedback
#ifdef feedback
  #define pr(x) Serial.print(x)
  #define prln(x) Serial.println(x)
#else
  #define pr(x)
  #define prln(x)
#endif

//Address of transmitter
byte addr[] = {0,0,0,0,0};

// Command values
byte command = 0;
byte parameter = 0;
byte device = 7;
byte room = 7;

//Basic init values
byte invert = 0;
int uSec = 140;
int txpin = 7;
byte repeats = 25;

//Default values for tick count widths for experimentation
byte lowCount = 7;
byte highCount = 4;
byte trailCount = 2;
byte gapCount = 72;

//Serial message input
const byte maxvalues = 8;
byte indexQ = 0;
boolean newvalue;
int invalues[maxvalues];

void setup() {
   Serial.begin(9600);
   Serial.setTimeout(1000 * 60);
   lwtx_setup(txpin, repeats, invert, uSec);
   prln("LwTx initial setup complete");
   indexQ = 0;
   invalues[0] = 0;
   newvalue = false;
}

void loop() {
   //collect any incoming command message and execute when complete
   if(getMessage()){
      // wait for any previous command to complete
      if (!lwtx_free()) {
         prln("Wait for last msg to complete");
         do {
            delay(100);
         } while (!lwtx_free());
         prln("Wait finished");
      }
      switch(invalues[0]) {
         case 1: // init,repeats,uSec,invert
            if(indexQ > 1) repeats = invalues[1];
            if(indexQ > 2) uSec = invalues[2];
            if(indexQ > 3) invert = invalues[3];
            lwtx_setup(txpin, repeats, invert, uSec);
            pr("LwTx setup. repeats=");pr(repeats);
            pr(" uSec=");pr(uSec);
            pr(" invert=");prln(invert);
           break;
         case 2: // counts,lowCount,highCount,trailCount,gapCount
            if(indexQ > 1) lowCount = invalues[1];
            if(indexQ > 2) highCount = invalues[2];
            if(indexQ > 3) trailCount = invalues[3];
            if(indexQ > 4) gapCount = invalues[4];
            lwtx_setTickCounts(lowCount, highCount, trailCount, gapCount);
            pr("LwTx Counts. low=");pr(lowCount);
            pr(" high=");pr(highCount);
            pr(" trail=");pr(trailCount);
            pr(" gap=");prln(gapCount);
            break;
         case 3: // address,ad1,ad2,ad3,ad4,ad5
            pr("Address set to ");
            for (byte i = 0; i < 5; i++) {
               if(indexQ >= 6) {
                  addr[i] = invalues[i+1];
               }
               pr(addr[i]);pr("-");
            }
            prln();
            lwtx_setaddr(addr);
            if(indexQ >= 6) {
               prln("Address updated");
            }
            break;
         case 4: // send message,cmd,par,room,device
            if(indexQ > 1) command = invalues[1];
            if(indexQ > 2) parameter = invalues[2];
            if(indexQ > 3) room = invalues[3];
            if(indexQ > 4) device = invalues[4];
            lwtx_cmd(command, parameter, room, device);
            prln("LwTx command sent.");
            break;
         case 5: // send gapMultiplier
            if(indexQ > 1) {
               lwtx_setGapMultiplier(invalues[1]);
               pr("LwTx gap multiplier ");
               prln(invalues[1]);
            } else {
               prln("No parameter for Gap multiplier. Ignored.");
            }
            break;
         default:
            help();
            break;
      }
      indexQ = 0;
      invalues[0] = 0;
   }
   delay(100);
}

boolean getMessage() {
   int inchar;
   
   if(Serial.available()) {
      inchar = Serial.read();
      if (echo) Serial.write(inchar);
      if(inchar == 10 || inchar == 13) {
         if (newvalue) indexQ++;
         newvalue = false;
         if (echo && inchar != 10) Serial.println();
         return true;
      } else if ((indexQ < maxvalues) && inchar >= 48 && inchar <= 57) {
         invalues[indexQ] = invalues[indexQ] * 10 + (inchar - 48);
         newvalue = true;
      } else if (indexQ < (maxvalues - 1)) {
         indexQ++;
         invalues[indexQ] = 0;
         newvalue = false;
      }
   }
   return false;
}

void help() {
   Serial.println("Commands:");
   Serial.println("  1:init  1,repeats,[clocktick],[invert]");
   Serial.println("  2:tick  2,tlow,thigh,ttrail,tgap");
   Serial.println("  3:addr  3,ad1,ad2,ad3,ad4,ad5");
   Serial.println("  4:send  4,cmd,par,[room],[device]");
   Serial.println("  5:gapm  5,gapMultiplier");
   Serial.println("[] Defaults to last value if not entered");
  
}
