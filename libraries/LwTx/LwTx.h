// LxTx.h
//
// LightwaveRF 434MHz tx interface for Arduino
// 
// Author: Bob Tidey (robert@tideys.net)
//Choose environment to compile for. Only one should be defined
//For SparkCore the SparkIntervalTimer Library code needs to be present
//For Due the DueTimer library code needs to be present
//#define SPARK_CORE 1
//#define DUE 1
#define AVR328 1

//Choose whether to include EEPROM support, comment or set to 0 to disable
#define EEPROM_EN 1

//Include basic library header and set default TX pin
#ifdef SPARK_CORE
#include "application.h"
#define TX_PIN_DEFAULT D3
#elif DUE
#include <Arduino.h>
#define TX_PIN_DEFAULT 3
#else
#include <Arduino.h>
#define TX_PIN_DEFAULT 3
#endif

//Include EEPROM if required to include storing device paramters in EEPROM
#if EEPROM_EN
#include <../EEPROM/EEPROM.h>
//define EEPROMaddr to location to store message addr
#define EEPROMaddr 0
#endif


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

//Genralised timer routines go here
//Sets up timer and the callback to the interrupt service routine
extern void lw_timer_Setup(void (*isrCallback)(), int period);

//Allows changing basic tick counts from their defaults
extern void lw_timer_Start();

//Allws multiplying the gap period for creating very large gaps
extern void lw_timer_Stop();
