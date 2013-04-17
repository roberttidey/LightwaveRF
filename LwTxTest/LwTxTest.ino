#include <LwTx.h>

byte msg[] = {4,0,0,0,3,1,7,9,6,1};
long timeout = 0;

void setup() {
  Serial.begin(9600);
  //Transmit on pin 7, 10 repeats
  lwtx_setup(7, 10);
}

void loop() {
  if (lwtx_free()) {
    lwtx_send(msg);
    timeout = millis();
    Serial.print(timeout);
    Serial.println(" msg start");
  }
  while(!lwtx_free() && millis() < (timeout + 1000)) {
    delay(10);
  }
  timeout = millis() - timeout;
  Serial.print(millis());
  Serial.print(" msg sent:");
  Serial.println(timeout);
  delay(100);
}

