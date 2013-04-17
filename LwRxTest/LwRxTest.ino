#include <LwRx.h>

void setup() {
  // set up with rx into pin 2
  lwrx_setup(2);
  Serial.begin(9600);
  Serial.println("Set up completed");
}

void loop() {
  byte msg[10];
  byte len = 10;
 
  if (lwrx_message()) {
     lwrx_getmessage(msg,&len);
     printMsg(msg, len);
  }
}

void printMsg(byte *msg, byte len) {
  Serial.print(millis());
  Serial.print(" ");
  for(int i=0;i<len;i++) {
    Serial.print(msg[i],HEX);
    Serial.print(" ");
  }
  Serial.println();
}
