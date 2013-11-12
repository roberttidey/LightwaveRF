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

extern void lwrx_setup(int pin);

extern void lwrx_settranslate(boolean translate);

extern boolean lwrx_message();

extern boolean lwrx_getmessage(byte* buf, byte* len);

extern boolean lwrx_getstats(unsigned int* stats);

extern void lwrx_setstatsenable(boolean rx_stats_enable);

