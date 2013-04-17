// LwRx.cpp
//
// LightwaveRF 434MHz receiver interface for Arduino
// 
// Author: Bob Tidey (robert@tideys.net)

#include <LwRx.h>

static byte rx_nibble[] = {0xF6,0xEE,0xED,0xEB,0xDE,0xDD,0xDB,0xBE,0xBD,0xBB,0xB7,0x7E,0x7D,0x7B,0x77,0x6F};

static int rx_pin = 2;
static const byte rx_msglen = 10; // expected length of rx message

//Receive mode constants and variables
static byte rx_msg[rx_msglen]; // message received
static byte rx_buf[rx_msglen]; // message buffer during reception

static unsigned long rx_prev; // time of previous pulse edge

static boolean rx_msgcomplete = false; //set high when message available
static boolean rx_translate = true; // Set false to get raw data

static byte rx_state = 0;
static const byte rx_state_idle = 0;
static const byte rx_state_msgstartfound = 1;
static const byte rx_state_bytestartfound = 2;
static const byte rx_state_getbyte = 3;

static byte rx_num_bits = 0; // number of bits in the current byte
static byte rx_num_bytes = 0; // number of bytes received 


/**
  Pin change interrupt routine that identifies 1 and 0 LightwaveRF bits
  and constructs a message when a valid packet of data is received.
**/
void rx_process_bits() { 
  byte event = digitalRead(rx_pin); // start setting event to the current value
  unsigned long curr = micros(); // the current time in microseconds
  
  unsigned int dur = (curr-rx_prev);
  rx_prev = curr;
  //set event based on input and duration of previous pulse
  if (dur < 160) { //160 very short
  } else if (dur < 500) { // normal short pulse
    event +=2;
  } else if (dur < 1500) { // normal long pulse
    event +=4;
  } else if (dur > 5000){ // gap between messages
    event +=6;
  } else { //1500 > 5000
    event = 8; //illegal gap
  } 
  //state machine transitions 
  switch(rx_state) {
    case rx_state_idle:
      switch(event) {
        case 7: //1 after a message gap
          rx_state = rx_state_msgstartfound;
          break;
      }
      break;
    case rx_state_msgstartfound:
      switch(event) {
        case 2: //0 160->500
          //nothing to do wait for next positive edge
          break;
        case 3: //1 160->500
          rx_num_bytes = 0;
          rx_state = rx_state_bytestartfound;
          break;
        default:
          //not good start again
          rx_state = rx_state_idle;
          break;
      }
      break;
    case rx_state_bytestartfound:
      switch(event) {
        case 2: //0 160->500
          //nothing to do wait for next positive edge
          break;
        case 3: //1 160->500
          rx_state = rx_state_getbyte;
          rx_num_bits = 0;
          break;
        case 5: //0 500->1500
          rx_state = rx_state_getbyte;
          // Starts with 0 so put this into byte
          rx_num_bits = 1;
          rx_buf[rx_num_bytes] = 0;
          break;
        default: 
          //not good start again
          rx_state = rx_state_idle;
          break;
      }
      break;
    case rx_state_getbyte:
      switch(event) {
        case 2: //0 160->500
          //nothing to do wait for next positive edge
          break;
        case 3: //1 160->500
          // a single 1
          rx_buf[rx_num_bytes] = rx_buf[rx_num_bytes] << 1 | 1;
          rx_num_bits++;
          break;
        case 5: //1 500->1500
          // a 1 followed by a 0
          rx_buf[rx_num_bytes] = rx_buf[rx_num_bytes] << 2 | 2;
          rx_num_bits++;
          rx_num_bits++;
          break;
        default:
          //not good start again
          rx_state = rx_state_idle;
          break;
      }
      if (rx_num_bits >= 8) {
        rx_num_bytes++;
        rx_num_bits = 0;
        if (rx_num_bytes >= rx_msglen) {
          //If last message hasn't been read it gets overwritten
          memcpy(rx_msg, rx_buf, rx_msglen); 
          rx_msgcomplete = true;
          // And cycle round for next one
          rx_state = rx_state_idle;
        } else {
          rx_state = rx_state_bytestartfound;
        }
      }
      break;
  }
}

/**
  Test if a message has arrived
**/
boolean lwrx_message()
{
    return (rx_msgcomplete);
}

/**
  Set translate mode
**/
void lwrx_settranslate(boolean rxtranslate)
{
    rx_translate = rxtranslate;
}

/**
  Transfer a message to user buffer
**/
boolean lwrx_getmessage(byte  *buf, byte *len) {
  boolean ret = true;
  if (lwrx_message()) {
    if (rx_translate) {
      for(byte i=0; i < rx_msglen && ret; i++) {
        // Assume false for this nibble
        ret = false;
        for(byte j=0; j < 16; j++) {
          if (rx_nibble[j] == rx_msg[i]) {
           //found nibble, translate and set ret true for this byte
           rx_msg[i] = j;
           ret = true;
           break;
          }
        }
      }
    }
    byte rxLen = rx_msglen;
    if (*len > rxLen) {
      *len = rxLen;
    }
    memcpy(buf, rx_msg, *len);
    rx_msgcomplete= false; 
  } else {
    ret = false;
  }
  return ret;
}

/**
  Set things up to receive LighWaveRF 434Mhz messages
  pin must be 2 or 3 to trigger interrupts
**/
void lwrx_setup(int pin) {
  if(pin == 3) {
    rx_pin = pin;
  } else {
    rx_pin = 2;
  }
  pinMode(rx_pin,INPUT);
  attachInterrupt(rx_pin - 2, rx_process_bits, CHANGE);
}

