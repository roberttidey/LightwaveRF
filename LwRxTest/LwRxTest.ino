#include <LwRx.h>
#include <EEPROM.h>

#define feedback
#ifdef feedback
  #define pr(x) Serial.print(x)
  #define prln(x) Serial.println(x)
#else
  #define pr(x)
  #define prln(x)
#endif
 
#define echo true
//define EEPROMaddr to location to store pair data or -1 to skip EEPROM
//First byte is pair count followed by 8 byte pair addresses (device,dummy,5*addr,room)
#define EEPROMaddr 16

unsigned long millisOffset = millis();

//Msg data
byte msg[10];
byte msglen = 10;

//Repeats data
static byte repeats = 0;
static byte timeout = 20;

//Serial message input
const byte maxvalues = 10;
byte index;
boolean newvalue;
int invalues[maxvalues];

void setup() {
   // set up with rx into pin 2
   lwrx_setup(2);
   Serial.begin(9600);
   restoreEEPROMPairing();
   prln("Set up completed and stats enabled");
   //Serial.println("Set up completed and stats enabled");
   index = 0;
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
            if (index >=8) {
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
               saveEEPROMPair(pair);
            } else {
               prln("Insufficient pair data.");
            }
            break;
         case 5: // Clear pair
            lwrx_clearpairing();
            clearEEPROMPairing();
            prln("LwRx pairing cleared.");
            break;
         case 6: // Set repeat filter
            if( index > 1) repeats = invalues[1];
            if (index > 2) timeout = invalues[2];
            lwrx_setfilter(repeats, timeout);
            pr("LwRx set filter repeat=");
            pr(repeats);
            pr(" timeout=");
            prln(timeout);
            break;
         case 7: // Set message display
            if (index > 1) msglen = invalues[1];
            pr("Set message display len=");
            pr(msglen);
            if (index > 2) {
               lwrx_settranslate(invalues[2] !=0);
               pr(" translate ");
               pr(invalues[2] !=0);
            }
            prln();
            break;
         default:
            help();
            break;
      }
      index = 0;
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
  unsigned int stats[rx_stat_count];
  
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
         if (newvalue) index++;
         newvalue = false;
         if (echo && inchar != 10) Serial.println();
         return true;
      } else if ((index < maxvalues) && inchar >= 48 && inchar <= 57) {
         invalues[index] = invalues[index] * 10 + (inchar - 48);
         newvalue = true;
      } else if (index < (maxvalues - 1)) {
         index++;
         invalues[index] = 0;
         newvalue = false;
      }
   }
   return false;
}

/**
   Save pair data to EEPROM if used
**/
void saveEEPROMPair(byte *pair) {
   if(EEPROMaddr >= 0) {
      byte pairCount = EEPROM.read(EEPROMaddr);
      if(pairCount > rx_maxpairs) {
         pairCount= 0;
         EEPROM.write(EEPROMaddr, 0);
         prln("Reset pair count");
      }
      if(pairCount < rx_maxpairs) {
         for (byte i = 0; i <= 7; i++) {
            EEPROM.write(EEPROMaddr + 1 + 8 * pairCount + i, pair[i]);
         }
         pairCount++;
         EEPROM.write(EEPROMaddr, pairCount);
         pr("Saved pair to EEPROM. Count=");
         prln(pairCount);
      } else {
         prln("Pairs limit reached");
      }
   }
}

/**
   Retrieve and set up pairing data from EEPROM if used
**/
void restoreEEPROMPairing() {
   if(EEPROMaddr >= 0) {
      byte pairCount = EEPROM.read(EEPROMaddr);
      if(pairCount < rx_maxpairs) { 
         byte pair[8];
         byte i=0;
         for( byte i=0; i < pairCount; i++) {
            pr("Restore pair ");
            pr(i);
            for(byte j=0; j<8; j++) {
               pair[j] = EEPROM.read(EEPROMaddr + 1 + 8 * i + j);
               pr(" ");
               pr(pair[j]);
            }
            prln();
            lwrx_addpair(pair);
         }
      }
   }
}

/**
   Clear pairing data from EEPROM if used
**/
void clearEEPROMPairing() {
   if(EEPROMaddr >= 0) {
      EEPROM.write(EEPROMaddr, 0);
   }
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
   Serial.println("[] Defaults to last value if not entered");
}

