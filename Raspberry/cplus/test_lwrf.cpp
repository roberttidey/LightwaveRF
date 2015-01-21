#include <iostream>
#include <stdio.h>
#include <time.h>
#include <string>
#include <sstream>

#include "pigpio.h"

#include "lwrf.hpp"
/*
REQUIRES
An 433MHz receiver data pin connected to a Pi gpio (input).
An 433MHz transmitter data pin connected to a Pi gpio (output).
TO BUILD
g++ -o test_lwrf test_lwrf.cpp lwrf.cpp -L. -lpigpio -lpthread -lrt

TO RUN
sudo ./test_lwrf
*/

// GPIO Pins
int RX = 24;
int RX_REPEAT = 0;
int TX = 25;
int TX_REPEAT = 10;

int main(int argc, char *argv[])
{
   time_t starttime = time(NULL);
   /* Can't instantiate lwrf classes before pigpio is initialised. */
   if (gpioInitialise() >= 0)
   {
      std::string TX_TEST = "0001F15AC2";
      LwrfRX rx(RX, RX_REPEAT);
      LwrfTX tx(TX);
      
      tx.Put(TX_TEST, TX_REPEAT);

      do
      {
         if (rx.Ready() > 0)
         {
            std::cout << rx.Get() << std::endl;
         }
         else
         {
            //No messages, pause for 50msec breather
            usleep(50000);
         }
      } while ((time(NULL) - starttime) < 30);
      std::cout << "Rx debug " << rx.Debug() << std::endl;
      std::cout << "Tx debug " << tx.Debug() << std::endl;
   }
}

