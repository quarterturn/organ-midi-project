/*
  HpDecVfd Library - Icons
 
 Demonstrates the use a VFD from a HP z500 series 
 Digital Entertainment Center.
 
 This sketch turns icons on and off in various ways.
 
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
  vfd.print("Icons demo");
}

void loop() {

  static const int DEMO_ITEM_LOOP_COUNT = 4;
  
  // Demo 1. Flash the antenna icon on and off.
  vfd.setCursor(0, 1);
  vfd.print("Flash antenna   "); 
  for (int i=0; i< DEMO_ITEM_LOOP_COUNT; i++)
  {
    // Turn on antenna icon. For more icon definitions, see HpDecVfd.h
    vfd.setIcon(HpDecVfd::ICON_ANTENNA, true);
    
    delay(500);    
  
    // Turn off antenna icon.
    vfd.setIcon(HpDecVfd::ICON_ANTENNA, false);
    
    delay(500);    
  } 

  // Demo 2. Flash the play triangle icon on and off.
  vfd.setCursor(0, 1);
  vfd.print("Flash play icon "); 
  for (int i=0; i< DEMO_ITEM_LOOP_COUNT; i++)
  {
    // Turn on play icon. For more icon definitions, see HpDecVfd.h
    vfd.setIcon(HpDecVfd::ICON_PLAY_TRIANGLE, true);
    
    delay(500);    
  
    // Turn off play icon.
    vfd.setIcon(HpDecVfd::ICON_PLAY_TRIANGLE, false);
    
    delay(500);    
  } 

  // Demo 3. All icons on, then clear.
  vfd.setCursor(0, 1);
  vfd.print("All on -> clear "); 
  for (int i=0; i< DEMO_ITEM_LOOP_COUNT; i++)
  {
    for (int j=0; j < HpDecVfd::NUM_ICONS; j++)
    {
      // Turn on icon.
      vfd.setIcon((HpDecVfd::Icon)j, true);
    }
    
    delay(500);
    
    vfd.clearIcons();
    
    delay(500);
  } 

  // Demo 4. All icons on and off, one at a time.
  vfd.setCursor(0, 1);
  vfd.print("All, 1 at a time"); 
  for (int i=0; i< DEMO_ITEM_LOOP_COUNT; i++)
  {
    for (int j=0; j < HpDecVfd::NUM_ICONS; j++)
    {
      // Turn on icon.
      vfd.setIcon((HpDecVfd::Icon)j, true);
    
      delay(100);    
  
      // Turn off icon.
      vfd.setIcon((HpDecVfd::Icon)j, false);
    
      delay(100);    
    }
  } 

  // Demo 5. All icons on and then all off.
  vfd.setCursor(0, 1);
  vfd.print("All on, then off"); 
  for (int i=0; i< DEMO_ITEM_LOOP_COUNT; i++)
  {
    for (int j=0; j < HpDecVfd::NUM_ICONS; j++)
    {
      // Turn on icon.
      vfd.setIcon((HpDecVfd::Icon)j, true);
    
      delay(100);    
    }
    for (int j=0; j < HpDecVfd::NUM_ICONS; j++)
    {
      // Turn off icon.
      vfd.setIcon((HpDecVfd::Icon)j, false);
    
      delay(100);    
    }
  } 
}

