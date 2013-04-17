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
static const byte tx_gapcount = 15; // Inter-message gap count 15 * 640uSec = 9.6msec

static boolean tx_msg_active = false; //set true to activate message sending
static boolean tx_translate = true; // Set false to send raw data

static byte tx_buf[tx_msglen]; // the message buffer during reception
static byte tx_repeat = 0; //counter for repeats
static byte tx_gap = 0; // counter for gap
static byte tx_state = 0;
static boolean tx_toggle_low = false;
static const byte tx_state_idle = 0;
static const byte tx_state_msgstart = 1;
static const byte tx_state_bytestart = 2;
static const byte tx_state_sendbyte = 3;
static const byte tx_state_msgend = 4;
static const byte tx_state_gap = 5;

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
   //Set low every other interrupt
   if (tx_toggle_low) {
      digitalWrite(tx_pin, LOW);
   } else {
     switch (tx_state) {
       case tx_state_idle:
         if(tx_msg_active) {
           tx_repeat = 0;
           tx_state = tx_state_msgstart;
         }
         break;
       case tx_state_msgstart:
         digitalWrite(tx_pin, HIGH);
         tx_num_bytes = 0;
         tx_state = tx_state_bytestart;
         break;
       case tx_state_bytestart:
         digitalWrite(tx_pin, HIGH);
         tx_bit_mask = 0x80;
         tx_state = tx_state_sendbyte;
         break;
       case tx_state_sendbyte:
         if(tx_buf[tx_num_bytes] & tx_bit_mask) {
           digitalWrite(tx_pin, HIGH);
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
         digitalWrite(tx_pin, HIGH);
         tx_gap = 0;
         tx_state = tx_state_gap;
         break;
       case tx_state_gap:
         tx_gap++;
         if(tx_gap >= tx_gapcount) {
            tx_repeat++;
            if(tx_repeat >= tx_repeats) {
              //disable timer 2 interrupt
              TIMSK2 &= ~(1 << OCIE2A);
              tx_msg_active = false;
              tx_state = tx_state_idle;
            } else {
              tx_state = tx_state_msgstart;
            }
         }
         break;
     }
   }
   tx_toggle_low = !tx_toggle_low;
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
void lwtx_send(byte* msg) {
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
void lwtx_setup(int pin, byte repeats) {
  if(pin !=0) {
    tx_pin = pin;
  }
  if(repeats > 0 && repeats < 40) {
    tx_repeats = repeats;
  }
  pinMode(tx_pin,OUTPUT);
  digitalWrite(tx_pin, LOW);
  cli();//stop interrupts

  //set timer2 interrupt at 320uSec
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 320 uSec
  OCR2A = 79;// = 16MHz Prescale to 4 uSec * 80
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS11 bit for 64 prescaler
  TCCR2B |= (1 << CS22);   
  // disable timer compare interrupt
  TIMSK2 &= ~(1 << OCIE2A);
  sei();//enable interrupts
}

