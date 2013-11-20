Arduino libraries for Lightwave RX and TX at 433MHz

LxRx - a library for LightwaveRF receive
 Reception is interrupt driven
 Reception is buffered, Foreground should read an available messsge before next one is received
 lwrx_setup(int pin) - initialise reception, pin can be 2 or 3
 lwrx_settranslate(boolean translate) translate true (default) formats message as 10 nibbles
                                      translate false messages as 10 raw bytes
 boolean lwrx_message() returns true if message available
 boolean lwrx_getmessage(byte* buf, byte* len) returns 10 byte message into buf

LxTx - a library for LightwaveRF transmit
 Transmission is interrupt driven
 Foreground sends a message which is buffered and sent in background
 lwtx_setup(int pin, byte repeatcount) - initialise transmission, pin can be any pin
 lwtx_settranslate(boolean translate) translate true (default) formats message as 10 nibbles
                                      translate false messages as 10 raw bytes
 boolean lwtx_free() returns true if message can be sent, false if one still in progress
 boolean lwtx_send(byte* buf) 10 byte message in buf
 void lwtx_setTickCounts( byte lowCount, byte highCount, byte trailCount, byte gapCount)
 
 LxTxTest - simple Test sketch for LwTx lib, sends one messages repeatedly
 LxRxTest - simple Test sketch for LwRx lib, monitors and prints messages on serial port
 LxTxTestEx - extended Test sketch for LwTx lib, responds to serial coomands to send message and adjust protocol values

Notes: As these libraries use interrupt service routines they may have issues with other
services using interrupts like Serial. The time spent in the isrs is kept very small to minimise
risks. The LxTx library also uses Timer2 and assumes 16MHz clock rates.

Updates 9 Nov 2013
LxRx library window for 0 detection widened

LxTx rework to allow better control of pulse widths. Interrupt tick is now a default of 140uSec
A 1 is encoded as 2 ticks high followed by 2 ticks low (280uSec high + 280uSec low)
A 0 is encoded as 7 ticks low (980uSec low)
A 1 followed by 0 therefore is 280uSec high followed by 1260uSec low
Added the function lwtx_setCounts to allow tuning of the tick counts for experimentation

Added new sketch LwTxTestEx for experimentation

Update 11 Nov 2013
Added stats gathering onto RX library and test sketch so that timings can be measured.

Update 19 Nov 2013
Added pairing and filtering to rx library
Added set address to tx library and short cmd method after address set
Reworked LwTxTestEx and LwRxTest sketches to respond to serial port commands
Sketches can save pairing and address details to EEPROM
NOTE interface to getmessage in RX library has changed, len is passed as a value rather than a pointer.

Updated 20 Nov 2013
EEPROM functions moved to library
Pairing functions enhanced
Auto pairing mode where a pair can be created from a received message
