/*
  HpDecVfd Library - Hello World
 
 Demonstrates the use a VFD from a HP z500 series 
 Digital Entertainment Center.
 
 This sketch prints "Hello World!" to the VFD
 and shows the time.
 
  The circuit:
  
 * Connect display pin 12 to GND
 * Connect display pin 11 to +5v
 * Connect display pin 6 to Arduino pin 6
 * Connect display pin 7 to Arduino pin 7
 * All other display pins can be left unconnected.
 
 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 8 Feb 2010
 by Tom Igoe
 modified 27 Feb 2011 - ported to PrimeVfd library
 modified 25 Mar 2011 - ported to HpDecVfd Library
 
 This example code is in the public domain.

 http://arduino.cc/playground/Main/HpDecVfd
*/

// include the library code:
#include <HpDecVfd.h> 

// initialize the library with the numbers of the interface pins
HpDecVfd vfd(6, 7);

void setup() {
  // start the library/displaying to the vfd 
  vfd.begin();
  // Print a message to the VFD.
  vfd.print("hello, world!");
}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  vfd.setCursor(0, 1);
  // print the number of seconds since reset:
  vfd.print(millis()/1000);
}

