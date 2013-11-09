// LwTx.cpp
//
// LightwaveRF 434MHz tx interface for Arduino
// 
// Author: Bob Tidey (robert@tideys.net)

#include <LwTx.h>

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

// These set the pulse durations in ticks
static byte tx_low_count = 7; // total number of ticks in a low (980 uSec)
static byte tx_high_count = 4; // total number of ticks in a high (560 uSec)
static byte tx_trail_count = 2; //tick count to set line low (280 uSec)
static byte tx_gap_count = 72; // Inter-message gap count (10.8 msec)

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
         break;
       case tx_state_gapstart:
         tx_toggle_count = tx_gap_count;
         tx_state = tx_state_gapend;
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
  Set things up to transmit LighWaveRF 434Mhz messages
**/
void lwtx_setup(int pin, byte repeats, byte invert, int uSec) {
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

