// LwRx.h
//
// LightwaveRF 434MHz receiver for Arduino
// 
// Author: Bob Tidey (robert@tideys.net)

#include <Arduino.h>
#include <../EEPROM/EEPROM.h>
#define rx_stat_high_ave 0
#define rx_stat_high_max 1
#define rx_stat_high_min 2
#define rx_stat_low0_ave 3
#define rx_stat_low0_max 4
#define rx_stat_low0_min 5
#define rx_stat_low1_ave 6
#define rx_stat_low1_max 7
#define rx_stat_low1_min 8
#define rx_stat_count 9

#define rx_maxpairs 10


//Setup must be called once, set up pin used to receive data
extern void lwrx_setup(int pin);

//Set translate to determine whether translating from nibbles to bytes in message
//Translate off only applies to 10char message returns
extern void lwrx_settranslate(boolean translate);

// Check to see whether message available
extern boolean lwrx_message();

//Get a message, len controls format (2 cmd+param, 4 cmd+param+room+device),10 full message
extern boolean lwrx_getmessage(byte* buf, byte len);

//Setup repeat filter
extern void lwrx_setfilter(byte repeats, byte timeout);

//Add pair, if no pairing set then all messages are received, returns number of pairs
extern byte lwrx_addpair(byte* pairdata);

// Get pair data into buffer  for the pairnumber. Returns current paircount
// Use pairnumber 255 to just get current paircount
extern byte lwrx_getpair(byte* pairdata, byte pairnumber);

//Make a pair from next message received within timeout 100mSec
//This call returns immediately whilst message checking continues
extern void lwrx_makepair(byte timeout);

//Returns time from last packet received in msec
// Can be used to determine if Rx may be still receiving repeats
extern unsigned long lwrx_packetinterval();

extern void lwrx_clearpairing();

//Return stats on pulse timings
extern boolean lwrx_getstats(unsigned int* stats);

//Enable collection of stats on pulse timings
extern void lwrx_setstatsenable(boolean rx_stats_enable);

int rx_findNibble(byte data);
void rx_addpairfrommsg();
void rx_paircommit();
boolean rx_checkPairs(byte *buf);
void restoreEEPROMPairing();