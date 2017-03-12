#include <LwRx.h>
#if EEPROM_EN == 1
#include <EEPROM.h>
#endif

#define feedback
#ifdef feedback
  #define pr(x) Serial.print(x)
  #define prln(x) Serial.println(x)
#else
  #define pr(x)
  #define prln(x)
#endif
 
#define echo true

//ESP12 tested with pin 13
#define RX_PIN 2

unsigned long millisOffset = millis();

//Msg data
byte msg[10];
byte msglen = 10;

//Repeats data
static byte repeats = 0;
static byte timeout = 20;

//pair data
static byte pairtimeout = 50;
static byte pairEnforce = 0;
static byte pairBaseOnly = 0;

//Serial message input
const byte maxvalues = 10;
byte indexQ;
boolean newvalue;
int invalues[maxvalues];

void setup() {
   // set up with rx pin
   lwrx_setup(RX_PIN);
   Serial.begin(115200);
   prln("Set up completed and stats enabled");
   indexQ = 0;
   invalues[0] = 0;
   newvalue = false;
}

void loop() {
   //collect any incoming command message and execute when complete
   if(getMessage()){
      switch(invalues[0]) {
         case 1: // Get Stats
            printStats();
            break;
         case 2: // Reset Stats
            lwrx_setstatsenable(false);
            lwrx_setstatsenable(true);
            prln("Stats reset and enabled");
            break;
         case 3: // Disable Stats
            lwrx_setstatsenable(false);
            prln("Stats disabled");
            break;
         case 4: // Add pair
            if (indexQ >=8) {
               byte pair[8];
               pair[0] = invalues[1];
               pr(pair[0]);
               for (byte i = 2; i <= 7; i++) {
                  pair[i] = invalues[i];
                  pr("-");
                  pr(pair[i]);
               }
               prln(" Pair added");
               lwrx_addpair(pair);
            } else {
               prln("Insufficient pair data.");
            }
            break;
         case 5: // Clear pair
            lwrx_clearpairing();
            prln("LwRx pairing cleared.");
            break;
         case 6: // Set repeat filter
            if( indexQ > 1) repeats = invalues[1];
            if (indexQ > 2) timeout = invalues[2];
            lwrx_setfilter(repeats, timeout);
            pr("LwRx set filter repeat=");
            pr(repeats);
            pr(" timeout=");
            prln(timeout);
            break;
         case 7: // Set message display
            if (indexQ > 1) msglen = invalues[1];
            pr("Set message display len=");
            pr(msglen);
            if (indexQ > 2) {
               lwrx_settranslate(invalues[2] !=0);
               pr(" translate ");
               pr(invalues[2] !=0);
            }
            prln();
            break;
          case 8: // Put in pairing mode
            if (indexQ > 1) pairtimeout = invalues[1];
            pr("Set into pair mode for 100ms * ");
            prln(pairtimeout);
            lwrx_makepair(pairtimeout);
            break;
         case 9: // Get and display a pair
            byte pair[8];
            byte paircount;
            paircount = lwrx_getpair(pair, invalues[1]);
            pr("Paircount=");
            pr(paircount);
            if (invalues[1] < paircount) {
               pr(" Pair addr");
               for(byte i = 0; i< 8; i++) {
                  if(i != 1) {
                     pr(" ");
                     pr(pair[i]);
                  }
               }
            }
            prln();
            break;
         case 10: // Time since last packet
            pr("Last packet delay mS=");
            prln(lwrx_packetinterval());
            break;
         case 11: // Pair controls
            if( indexQ > 1) pairEnforce = invalues[1];
            if (indexQ > 2) pairBaseOnly = invalues[2];
            lwrx_setPairMode(pairEnforce, pairBaseOnly);
            pr("Pair mode enforce=");
            pr(pairEnforce);
            pr(" baseOnly=");
            prln(pairBaseOnly);
            break;
       default:
            help();
            break;
      }
      indexQ = 0;
      invalues[0] = 0;
   }
   if (lwrx_message()) {
      lwrx_getmessage(msg, msglen);
      printMsg(msg, msglen);
   }
   delay(5);
}

/**
   Retrieve and print out received message
**/
void printMsg(byte *msg, byte len) {
   Serial.print(millis() - millisOffset);
   Serial.print(" ");
   for(int i=0;i<len;i++) {
      Serial.print(msg[i],HEX);
      Serial.print(" ");
   }
   Serial.println();
}

/**
   Retrieve and print out current pulse width stats
**/
void printStats() {
  uint16_t stats[rx_stat_count];
  
  if (lwrx_getstats(stats)) {
    Serial.print("Stats, ");
    for (byte i=0; i<rx_stat_count; i++) {
      // Ave values 0,3,6 are 16 x
      if (i % 3 == 0) {
        Serial.print(stats[i] >> 4);
      } else {
        Serial.print(stats[i]);
      }
      Serial.print(",");
    }
    Serial.println();
  } else {
    Serial.println("No stats available");
  }
}

/**
   Check and build input command and paramters from serial input
**/
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
   Serial.println("  1:gets  1,  Get Stats");
   Serial.println("  2:ress  2,  Reset stats");
   Serial.println("  3:diss  3,  Disable stats");
   Serial.println("  4:addp  4,dev,ad1,ad2,ad3,ad4,ad5,room Add pair");
   Serial.println("  5:resp  5,  Reset pairs");
   Serial.println("  6:srep  6,repeats,timeout  Set repeat filter");
   Serial.println("  7:msgl  7,n,t Message display len(2,4,10),translate");
   Serial.println("  8:prmd  8,n Put into pairing mode for up to n * 100mSec");
   Serial.println("  9:prdt  9,n Get pairdata n");
   Serial.println(" 10:time 10,  Time in mS since last packet");
   Serial.println(" 11:prct 11,e,b  Pair controls e=enforce pairing, b=address only");
   Serial.println("[] Defaults to last value if not entered");
}

