// LxTx.h
//
// LightwaveRF 434MHz tx interface for Arduino
// 
// Author: Bob Tidey (robert@tideys.net)

#include <Arduino.h>

//Sets up basic parameters must be called at least once
extern void lwtx_setup(int pin, byte repeats, byte invert, int uSec);

//Allows changing basic tick counts from their defaults
extern void lwtx_setTickCounts( byte lowCount, byte highCount, byte trailCount, byte gapCount);

//Allws multiplying the gap period for creating very large gaps
extern void lwtx_setGapMultiplier(byte gapMultiplier);

// determines whether incoming data or should be translated from nibble data
extern void lwtx_settranslate(boolean txtranslate);

//Checks whether tx is free to accept a new message
extern boolean lwtx_free();

//Basic send of new 10 char message, not normally needed if setaddr and cmd are used.
extern void lwtx_send(byte* msg);

//Sets up 5 char address which will be used to form messages for lwtx_cmd
extern void lwtx_setaddr(byte* addr);

//Send Command
extern void lwtx_cmd(byte command, byte parameter, byte room, byte device);
