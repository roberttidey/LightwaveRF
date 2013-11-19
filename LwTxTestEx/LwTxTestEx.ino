#include <LwTx.h>

//Replace these with appropriate test messages
byte msgon[] = {1,15,3,1,5,9,3,0,1,2};
byte msgoff[] = {4,0,3,0,5,9,3,0,1,2};
byte invert = 0;
int uSec = 140;
int txpin = 7;
byte repeats = 25;

//Default values for tick count widths for experimentation
byte lowCount = 7;
byte highCount = 4;
byte trailCount = 2;
byte gapCount = 72;

void setup() {
  Serial.begin(9600);
  lwtx_setup(txpin, repeats, invert, uSec);
}

void loop() {
  int inchar;
  if(Serial.available()) {
    inchar = Serial.read();
    Serial.print("Received:");
    Serial.print(inchar);
    if (lwtx_free()) {
      if (inchar == 48) { //0
        Serial.println("  - Sending Off");
        lwtx_send(msgoff);
      } else if (inchar == 49) { //1
        Serial.println("  - Sending On");
        lwtx_send(msgon);
      } else if (inchar == 50) { //2
        Serial.print("  - Toggle invert to ");
        invert = invert ^ 1;
        lwtx_setup(txpin, repeats, invert, uSec);
        Serial.println(invert);
      } else if (inchar == 51) { //3
        Serial.print("  - Decrease clock uSec to ");
        uSec = uSec - 4;
        lwtx_setup(txpin, repeats, invert, uSec);
        Serial.println(uSec);
      } else if (inchar == 52) { //4
        Serial.print("  - Increase clock uSec to ");
        uSec = uSec + 4;
        lwtx_setup(txpin, repeats, invert, uSec);
        Serial.println(uSec);
      } else if (inchar == 53) { //5
        Serial.print("  - Decrease clock uSec to ");
        uSec = uSec - 20;
        lwtx_setup(txpin, repeats, invert, uSec);
        Serial.println(uSec);
      } else if (inchar == 54) { //6
        Serial.print("  - Increase clock uSec to ");
        uSec = uSec + 20;
        lwtx_setup(txpin, repeats, invert, uSec);
        Serial.println(uSec);
      } else if (inchar == 55) { //7
        Serial.print("  - Increase lowCount to ");
        lowCount++;
        lwtx_setTickCounts(lowCount, highCount, trailCount, gapCount);
        Serial.println(lowCount);
      } else if (inchar == 56) { //8
        Serial.print("  - Decrease lowCount to ");
        lowCount--;
        lwtx_setTickCounts(lowCount, highCount, trailCount, gapCount);
        Serial.println(lowCount);
      } else {
        Serial.println("  - Null");
      }
    }
  }
  delay(100);
}

