// LwTx.cpp
//
// LightwaveRF 434MHz tx interface for Arduino
// 
// Author: Bob Tidey (robert@tideys.net)

#include <LwTx.h>

//define EEPROMaddr to location to store message addr or -1 to skip EEPROM
#define EEPROMaddr 0

static byte tx_nibble[] = {0xF6,0xEE,0xED,0xEB,0xDE,0xDD,0xDB,0xBE,0xBD,0xBB,0xB7,0x7E,0x7D,0x7B,0x77,0x6F};

static int tx_pin = 3;
static const byte tx_msglen = 10; // the expected length of the message

//Transmit mode constants and variables

static byte tx_repeats = 12; // Number of repeats of message sent
static byte txon = 1;
static byte txoff = 0;
static boolean tx_msg_active = false; //set true to activate message sending
static boolean tx_translate = true; // Set false to send raw data

static byte tx_buf[tx_msglen]; // the message buffer during reception
static byte tx_repeat = 0; //counter for repeats
static byte tx_state = 0;
static byte tx_toggle_count = 3;
static unsigned int tx_gap_repeat = 0;

// These set the pulse durations in ticks
static byte tx_low_count = 7; // total number of ticks in a low (980 uSec)
static byte tx_high_count = 4; // total number of ticks in a high (560 uSec)
static byte tx_trail_count = 2; //tick count to set line low (280 uSec)
// Use with low repeat counts
static byte tx_gap_count = 72; // Inter-message gap count (10.8 msec)
//Gap multiplier byte is used to multiply gap if longer periods are needed for experimentation
//If gap is 255 (35msec) then this to give a max of 9 seconds
//Used with low repeat counts to find if device times out
static byte tx_gap_multiplier = 0; //Gap extension byte 

static const byte tx_state_idle = 0;
static const byte tx_state_msgstart = 1;
static const byte tx_state_bytestart = 2;
static const byte tx_state_sendbyte = 3;
static const byte tx_state_msgend = 4;
static const byte tx_state_gapstart = 5;
static const byte tx_state_gapend = 6;

static byte tx_bit_mask = 0; // bit mask in current byte
static byte tx_num_bytes = 0; // number of bytes sent 

/**
  Set translate mode
**/
void lwtx_settranslate(boolean txtranslate)
{
    tx_translate = txtranslate;
}

ISR(TIMER2_COMPA_vect){
   //Set low after toggle count interrupts
   tx_toggle_count--;
   if (tx_toggle_count == tx_trail_count) {
      digitalWrite(tx_pin, txoff);
   } else if (tx_toggle_count == 0) {
     tx_toggle_count = tx_high_count; //default high pulse duration
     switch (tx_state) {
       case tx_state_idle:
         if(tx_msg_active) {
           tx_repeat = 0;
           tx_state = tx_state_msgstart;
         }
         break;
       case tx_state_msgstart:
         digitalWrite(tx_pin, txon);
         tx_num_bytes = 0;
         tx_state = tx_state_bytestart;
         break;
       case tx_state_bytestart:
         digitalWrite(tx_pin, txon);
         tx_bit_mask = 0x80;
         tx_state = tx_state_sendbyte;
         break;
       case tx_state_sendbyte:
         if(tx_buf[tx_num_bytes] & tx_bit_mask) {
           digitalWrite(tx_pin, txon);
         } else {
           // toggle count for the 0 pulse
           tx_toggle_count = tx_low_count;
         }
         tx_bit_mask >>=1;
         if(tx_bit_mask == 0) {
           tx_num_bytes++;
           if(tx_num_bytes >= tx_msglen) {
             tx_state = tx_state_msgend;
           } else {
             tx_state = tx_state_bytestart;
           }
         }
         break;
       case tx_state_msgend:
         digitalWrite(tx_pin, txon);
         tx_state = tx_state_gapstart;
         tx_gap_repeat = tx_gap_multiplier;
         break;
       case tx_state_gapstart:
         tx_toggle_count = tx_gap_count;
         if (tx_gap_repeat == 0) {
            tx_state = tx_state_gapend;
         } else {
            tx_gap_repeat--;
         }
         break;
       case tx_state_gapend:
         tx_repeat++;
         if(tx_repeat >= tx_repeats) {
           //disable timer 2 interrupt
           TIMSK2 &= ~(1 << OCIE2A);
           tx_msg_active = false;
           tx_state = tx_state_idle;
         } else {
            tx_state = tx_state_msgstart;
         }
         break;
     }
   }
}

/**
  Check for send free
**/
boolean lwtx_free() {
  return !tx_msg_active;
}

/**
  Send a LightwaveRF message (10 nibbles in bytes)
**/
void lwtx_send(byte *msg) {
  if (tx_translate) {
    for (byte i=0; i < tx_msglen; i++) {
      tx_buf[i] = tx_nibble[msg[i] & 0xF];
    }
  } else {
    memcpy(tx_buf, msg, tx_msglen);
  }
  //enable timer 2 interrupts
  TIMSK2 |= (1 << OCIE2A);
  tx_msg_active = true;
}

/**
  Set 5 char address for future messages
**/
void lwtx_setaddr(byte *addr) {
   for (byte i=0; i < 5; i++) {
      tx_buf[i+4] = tx_nibble[addr[i] & 0xF];
      if(EEPROMaddr >= 0) {
         EEPROM.write(EEPROMaddr + i, tx_buf[i+4]);
      }
   }
}

/**
  Send a LightwaveRF message (10 nibbles in bytes)
**/
void lwtx_cmd(byte command, byte parameter, byte room, byte device) {
  //enable timer 2 interrupts
  tx_buf[0] = tx_nibble[parameter >> 4];
  tx_buf[1] = tx_nibble[parameter  & 0xF];
  tx_buf[2] = tx_nibble[device  & 0xF];
  tx_buf[3] = tx_nibble[command  & 0xF];
  tx_buf[9] = tx_nibble[room  & 0xF];
   //enable timer 2 interrupts
  TIMSK2 |= (1 << OCIE2A);
  tx_msg_active = true;
}

/**
  Set things up to transmit LightWaveRF 434Mhz messages
**/
void lwtx_setup(int pin, byte repeats, byte invert, int uSec) {
   if(EEPROMaddr >= 0) {
      for(int i=0; i<5; i++) {
        tx_buf[i+4] = EEPROM.read(EEPROMaddr+i);
      }
   }
  if(pin !=0) {
    tx_pin = pin;
  }
  if(repeats > 0 && repeats < 40) {
    tx_repeats = repeats;
  }
  if(invert != 0) {
    txon = 0;
    txoff = 1;
  } else {
    txon = 1;
    txoff = 0;
  }

  byte clock;
  if (uSec > 32 && uSec < 1000) {
    clock = (uSec / 4) - 1;
  } else {
    //default 140 uSec
    clock = 34;
  }
  pinMode(tx_pin,OUTPUT);
  digitalWrite(tx_pin, txoff);
  cli();//stop interrupts

  //set timer2 interrupt at  clock uSec (default 140)
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for clock uSec
  OCR2A = clock;// = 16MHz Prescale to 4 uSec * (counter+1)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS11 bit for 64 prescaler
  TCCR2B |= (1 << CS22);   
  // disable timer compare interrupt
  TIMSK2 &= ~(1 << OCIE2A);
  sei();//enable interrupts
}

void lwtx_setTickCounts( byte lowCount, byte highCount, byte trailCount, byte gapCount) {
     tx_low_count = lowCount;
     tx_high_count = highCount;
     tx_trail_count = trailCount;
     tx_gap_count = gapCount;
}

void lwtx_setGapMultiplier(byte gapMultiplier) {
   tx_gap_multiplier = gapMultiplier;
}

