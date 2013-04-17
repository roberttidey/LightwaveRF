// LwRx.h
//
// LightwaveRF 434MHz receiver for Arduino
// 
// Author: Bob Tidey (robert@tideys.net)

#include <Arduino.h>

extern void lwrx_setup(int pin);

extern void lwrx_settranslate(boolean translate);

extern boolean lwrx_message();

extern boolean lwrx_getmessage(byte* buf, byte* len);

