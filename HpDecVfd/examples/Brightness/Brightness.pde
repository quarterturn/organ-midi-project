/*
  HpDecVfd Library - setBrightness()
 
 Demonstrates the use a VFD from a HP z500 series 
 Digital Entertainment Center.
 
 This sketch prints text and changes the display
 brightness setting.
 
  The circuit:
  
 * Connect display pin 12 to GND
 * Connect display pin 11 to +5v
 * Connect display pin 6 to Arduino pin 6
 * Connect display pin 7 to Arduino pin 7
 * All other display pins can be left unconnected.
 
 Created 26 Mar 2011
 
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
  vfd.print("setBrightness()");
}

void loop() {

  for (int level = 0; level < 16; level++)
  {
    vfd.setCursor(0, 1);
    vfd.print("level=");
    vfd.print(level);
    vfd.write(' ');
    
    vfd.setBrightness(level);
    
    delay(250);
  }
}

