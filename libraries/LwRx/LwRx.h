// LwRx.h
//
// LightwaveRF 434MHz receiver for Arduino
// 
// Author: Bob Tidey (robert@tideys.net)

#include <Arduino.h>
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

extern void lwrx_clearpairing();

//Return stats on pulse timings
extern boolean lwrx_getstats(unsigned int* stats);

//Enable collection of stats on pulse timings
extern void lwrx_setstatsenable(boolean rx_stats_enable);

boolean rx_checkPairs();
int rx_findNibble(byte data);

