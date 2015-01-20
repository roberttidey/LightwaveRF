#!/usr/bin/env python
"""
This module provides a Raspberry Pi 433MHz radio interface compatible
with LightwaveRF modules.

It is a combination of LightwaveRF libraries developed for Arduinos
and the pigpio DMA library for real time IO.
Used the Virtual Wire example as starting point and replaced the protocol.
"""
# 2015-01-17
# lwrf.py

import time
import pigpio

MESSAGE_BYTES=10

RX_STATE_IDLE = 0
RX_STATE_MSGSTARTFOUND = 1
RX_STATE_BYTESTARTFOUND = 2
RX_STATE_GETBYTE = 3
RX_MSG_TIMEOUT = 1000000

TX_HIGH = 280
TX_LOW = 980
TX_GAP = 10800

_SYMBOL = [0xF6,0xEE,0xED,0xEB,0xDE,0xDD,0xDB,0xBE,0xBD,0xBB,0xB7,0x7E,0x7D,0x7B,0x77,0x6F]


def _sym2nibble(symbol):
   for nibble in range(16):
      if symbol == _SYMBOL[nibble]:
         return nibble
   return 0

class tx():

   def __init__(self, pi, txgpio):
      """
      Instantiate a transmitter with the Pi and the transmit gpio.
      """
      self.pi = pi
      self.txgpio = txgpio
      self.txbit = (1<<txgpio)
      self.wave_id = None
      pi.wave_add_new()
      pi.set_mode(txgpio, pigpio.OUTPUT)

   def put(self, data, repeat):
      for r in range(repeat):
         put1(self, data)

   def put(self, data, repeat=1):
      """
      Transmit a message repeat times
      0 is returned if message transmission has successfully started.
      Negative number indicates an error.
      """
      ret = 0
      if len(data) <> MESSAGE_BYTES:
         return -1
      self.cancel()
      #Set TX high and wait to get agc of RX trained
      self.pi.write(self.txgpio, 1)
      time.sleep(TX_GAP / 1000000)
      #Define a single message waveform
      self.wf = []
      # Pre Message low gap
      self.wf.append(pigpio.pulse(0, self.txbit, TX_GAP))
      # Message start pulse
      self.wf.append(pigpio.pulse(self.txbit, 0, TX_HIGH))
      self.wf.append(pigpio.pulse(0, self.txbit, TX_HIGH))
      
      for i in data:
         # Byte start pulse
         self.wf.append(pigpio.pulse(self.txbit, 0, TX_HIGH))
         self.wf.append(pigpio.pulse(0, self.txbit, TX_HIGH))
         self.byte = _SYMBOL[i & 0x0F]
         for j in range(8):
            if self.byte & (0x80>>j):
               self.wf.append(pigpio.pulse(self.txbit, 0, TX_HIGH))
               self.wf.append(pigpio.pulse(0, self.txbit, TX_HIGH))
            else:
               self.wf.append(pigpio.pulse(0, 0, TX_LOW))
         
      # Message end pulse
      self.wf.append(pigpio.pulse(self.txbit, 0, TX_HIGH))
      self.wf.append(pigpio.pulse(0, self.txbit, TX_HIGH))

      self.pi.wave_add_generic(self.wf)
      self.wave_id = self.pi.wave_create()
      if self.wave_id >= 0:
         for r in range(repeat):
            self.pi.wave_send_once(self.wave_id)
            while self.pi.wave_tx_busy(): # wait for waveform to be sent
               time.sleep(0.001)
      else:
         return -2
      return ret


   def ready(self):
      """
      Returns True if a new message may be transmitted.
      """
      return not self.pi.wave_tx_busy()

   def cancel(self):
      """
      Cancels the wireless transmitter, aborting any message
      in progress.
      """
      if self.wave_id is not None:
         self.pi.wave_tx_stop()
         self.pi.wave_delete(self.wave_id)
         self.pi.wave_add_new()

      self.wave_id = None

class rx():

   def __init__(self, pi, rxgpio, repeat):
      """
      Instantiate a LightwaveRF receiver with the Pi, the receive gpio, and
      Repeat count sets number of identical messages before a report 
      A repeat count > 0 also filters duplicates
      """
      self.pi = pi
      self.rxgpio = rxgpio
      self.messages = []
      self.duplicate = False
      self.repeat = repeat
      self.repeatCount = 0
      self.messageTick = 0
      self.bit = 0
      self.byte = 0
      self.message = [0]*(MESSAGE_BYTES)
      self.state = RX_STATE_IDLE;
      self.data = 0

      pi.set_mode(rxgpio, pigpio.INPUT)
      self.lastTick = self.pi.get_current_tick()
      self.cb = pi.callback(rxgpio, pigpio.EITHER_EDGE, self._cb)

   def _cb(self, gpio, level, tick):
      if level <> pigpio.TIMEOUT:
         # Get microseconds since last change
         pulse = tick - self.lastTick
         self.lastTick = tick
         if pulse < 150: #very short pulse so ignore it
            return
         elif self.state == RX_STATE_IDLE and pulse <= 5000: #quick check to see worth proceeding
            return
         elif pulse < 500: #normal short pulse
            trans = level + 2
         elif pulse < 2000: #normal long pulse
            trans = level + 4
         elif pulse > 5000: # gap between messages
            trans = level + 6
         else:
            trans = 8
         #State machine using nested ifs
         if self.state == RX_STATE_IDLE:
            if trans == 7: # 1 after a message gap
               self.state = RX_STATE_MSGSTARTFOUND
               self.duplicate = True
         elif self.state == RX_STATE_MSGSTARTFOUND:
            if trans == 2: # nothing to do wait for next 1
               trans = trans
            elif trans == 3: # start of a byte detected
               self.byte = 0
               self.state = RX_STATE_BYTESTARTFOUND
            else:
               self.state = RX_STATE_IDLE
         elif self.state == RX_STATE_BYTESTARTFOUND:
            if trans == 2: # nothing to do wait for next 1
               trans = trans
            elif trans == 3: # 1 160->500
               self.data = 0
               self.bit = 0
               self.state = RX_STATE_GETBYTE
            elif trans == 5: # 0 500->1500 starts with a 0 so enter it
               self.data = 0
               self.bit = 1
               self.state = RX_STATE_GETBYTE
            else:
               self.state = RX_STATE_IDLE
         elif self.state == RX_STATE_GETBYTE:
            if trans == 2: # nothing to do wait for next 1
               trans = trans
            elif trans == 3: # 1 160->500
               self.data = self.data << 1 | 1
               self.bit +=1
            elif trans == 5: # 500 - 1500 a 1 followed by a 0
               self.data = self.data << 2 | 2
               self.bit +=2
            else:
               self.state = RX_STATE_IDLE
            # Check if byte complete
            if self.bit >= 8:
               # Translate symbols to nibbles and enter message
               self.data = _sym2nibble(self.data)
               if self.data <> self.message[self.byte]: # Is it same as last packet
                  self.duplicate = False
                  self.repeatCount = 0
               self.message[self.byte] = self.data
               self.byte +=1
               self.bit = 0
               if self.byte >= MESSAGE_BYTES: # Is packet complete
                  if pigpio.tickDiff(self.messageTick, self.lastTick) > RX_MSG_TIMEOUT or self.messageTick == 0:
                     # Long time since last message so reset the counter
                     self.repeatCount = 0
                  elif self.duplicate:
                     self.repeatCount +=1
                  if self.repeat == 0 or self.repeatCount == self.repeat:
                     self.messages.append(self.message[0:MESSAGE_BYTES])
                  self.state = RX_STATE_IDLE
                  self.messageTick = self.messageTick
               else:
                  self.state = RX_STATE_BYTESTARTFOUND
      else:
         self.pi.set_watchdog(self.rxgpio, 0) # Switch watchdog off.

   def get(self):
      """
      Returns the next unread message, or None if none is available.
      """
      if len(self.messages):
         return self.messages.pop(0)
      else:
         return None

   def ready(self):
      """
      Returns True if there is a message available to be read.
      """
      return len(self.messages)

   def cancel(self):
      """
      Cancels the wireless receiver.
      """
      if self.cb is not None:
         self.cb.cancel()
         self.pi.set_watchdog(self.rxgpio, 0)
      self.cb = None

"""
Test main routine
Sends out 1 test TX message and reports RX messages for 30 seconds
"""

if __name__ == "__main__":

   import time
   import pigpio
   import lwrf

   RX=24
   TX=25
   TX_TEST = [0,0,0,1,15,1,5,10,12,2]
   TX_REPEAT = 10
   RX_REPEAT = 0

   pi = pigpio.pi() # Connect to local Pi.

   rx = lwrf.rx(pi, RX, RX_REPEAT) # Specify Pi, rx gpio, and repeat.
   tx = lwrf.tx(pi, TX) # Specify Pi, tx gpio, and baud.

   print "Transmit test sending", TX_TEST, TX_REPEAT, "times"
   tx.put(TX_TEST, TX_REPEAT)
      
   start = time.time()
   
   while (time.time()-start) < 30:
      if rx.ready():
         print "Received","".join("%01X" % x for x in rx.get())
      time.sleep(0.010)
   rx.cancel()
   tx.cancel()
   pi.stop()
   time.sleep(2)

