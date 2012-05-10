/*
  HpDecVfd Library - Unit Tests
 
 Demonstrates the use a VFD from a HP z500 series 
 Digital Entertainment Center.
 
 This sketch runs a number of tests on the library.
 The display is quite fussy around timing and the library
 is quite complex in this regard so the unit tests focus on that.
 
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

void doRandomDelay(long startRange, long endRange)
{
  if (endRange > 10000L)
   {
     delay(random(startRange/1000L, endRange/1000L));
   }
   else
   {
     delayMicroseconds(random(startRange, endRange));
   }
}
 
// 0-9A-F sequence should be correctly displayed on bottom row.
// There will be random delays between drawing of characters.
void performRandomDelayTest()
{
  static const long START_DELAY_RANGE = 1L; // 1 microsecond.
  static const long END_DELAY_RANGE = 10L * 1000L * 1000L; // 10 microseconds.
  static const long DELAY_SCALE_UP = 2L;
  static const int NUM_PASSES_PER_RANGE = 2;
  static const int NUM_CHARACTERS_TO_DRAW = 16;
  static const char *charactersToDraw = "0123456789ABCDEF";
  
  vfd.clear();
  vfd.print("Random delays");
  delay(2);
  
  for (long delayRange = START_DELAY_RANGE; delayRange <= END_DELAY_RANGE; delayRange*=DELAY_SCALE_UP)
  {
    for (int pass = 1; pass <= NUM_PASSES_PER_RANGE; pass++)
    {
      // Print what test we are doing.
      vfd.clear();
      vfd.print("p=");
      vfd.print(pass);
      vfd.print(" dr=");

      if (delayRange < 1000L)
      {
        // Microseconds.
        vfd.print((int)delayRange);
        vfd.print("us");
      }
      else if (delayRange < (1000L*1000L))
      {
        //Milliseconds.
        vfd.print((int)(delayRange / 1000L));
        vfd.print("ms");
      }
      else
      {
        //Seconds.
        vfd.print((int)(delayRange / (1000L*1000L)));
        vfd.print("s");
      }
    
      // Locate cursor.
      doRandomDelay(delayRange/DELAY_SCALE_UP, delayRange);
      vfd.setCursor(0, 1);
    
      // Print random length fragments of our string with random delays in between.
      for (int charIndex = 0; charIndex < NUM_CHARACTERS_TO_DRAW;)
      {
        int charsThisLoop = random(charIndex - NUM_CHARACTERS_TO_DRAW) + 1;

        doRandomDelay(delayRange/DELAY_SCALE_UP, delayRange);
      
        for (int charCount = 0; 
             (charCount < charsThisLoop) && (charIndex < NUM_CHARACTERS_TO_DRAW);
             charCount++, charIndex++)
        {
          vfd.write(charactersToDraw[charIndex]);
        }
      }

      // Hold result on the screen for a while.
      delay(1000);    
    }
  }
}
 
void setup() {
  // start the library/displaying to the vfd 
  vfd.begin();
}

void loop() {

  // The only test for now.
  performRandomDelayTest();
}

