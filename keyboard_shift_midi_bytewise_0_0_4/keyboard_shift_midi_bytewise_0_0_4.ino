/* vintage organ keyboard to midi converter
 Author: Alexander Davis
 Date: 5/10/2012
 Purpose: Scans a bussbar-type organ keyboard via chained shift registers and outputs MIDI note commands.
 Also sends the MIDI commands to a Fluxamasynth single-chip MIDI synth for audio output.
 
 Built for the 2012 Raleigh, NC Maker Faire
 
 Hardware: ATMEGA328p @ 16 mHz on Modern Device RBBB
 Modern Device Fluxamasynth board
 Wicked Device RBBB shield board (Vcc to Vin pin connected)
 5v 1A regulated switchmode power supply
 8x 74ls165 shift registers
 and old bussbar-type organ keyboard
 
 Git repository: https://github.com/quarterturn/organ-midi-project
 */

// external libraries
// fluxamasynth
#include "Fluxamasynth.h"

// constants
// 8 bits all '1'
#define BITS_8_ON 0xFF

// version
#define VERSION "0.0.4"

// debounce interval - 10 ms
#define DEBOUNCE 10

// keyboard busses
#define BUS_LOWER 9
#define BUS_UPPER 10

// shift register pins
#define SHIFT_LOAD 13
#define SHIFT_DATA 11
#define SHIFT_CLOCK 12

// shift register setup
#define NUMBER_OF_SHIFT_CHIPS 8
#define DATA_WIDTH 8
#define PULSE_WIDTH_USEC 5
#define POLL_DELAY_MSEC 1

// this decides what note the leftmost key will sound
#define LOWEST_NOTE 36

// how many keys
#define NUM_KEYS 64

// default velocity
#define DEFAULT_VELOCITY 110

// turn on debugging output via Serial
#define DEBUG 0

// create a synth object
Fluxamasynth synth;

// global variables

// an array of button states
// 2 means just released
// 0 means nothing happened
// 1 means just pressed
// takes up 1024 bytes of RAM total
// this could be done better - maybe store 2 nybbles per byte
// as they say, this is an exercise left to the student
byte pressStateLower[NUM_KEYS] = {0};
byte pressStateUpper[NUM_KEYS] = {0};

// 8-bit unsigned int to hold the note select mask
// initialize with a 1 in the LSB; this represents key 1 the leftmost key
uint8_t noteSel = 1;


// channel for upper manual
byte upperChannel = 0;
// channel for lower manual
byte lowerChannel = 1;
// bank for upper manual
byte upperBank = 0;
// bank for lower manual
byte lowerBank = 0;
// instrument for upper manual
// drawbar organ for testing
byte upperInstrument = 0;
// instrument for lower manual
// synth bass for testing
byte lowerInstrument = 50;
// master volume
byte masterVolume = 120;

// note index
byte index;

// note value
byte theNote;

// for tracking time in delay loops
unsigned long previousMillis;

void setup()
{
  // bus setup
  // start off with both busses inactive
  // a bus is active if it is an output and low
  pinMode(BUS_LOWER, INPUT);
  //digitalWrite(BUS_LOWER, LOW);
  // a bus is inactive if it is an input
  pinMode(BUS_UPPER, INPUT);
  
  // shift register pin setup
  pinMode(SHIFT_LOAD, OUTPUT);
  digitalWrite(SHIFT_LOAD, HIGH);
  pinMode(SHIFT_CLOCK, OUTPUT);
  digitalWrite(SHIFT_CLOCK, LOW);
  pinMode(SHIFT_DATA, INPUT);

  // uart serial setup
  Serial.begin(9600);
  synth.midiReset(); 

  // enable chorus
  synth.setChorus(0, 1, 30, 30, 10);
  // max. master volume
  synth.setMasterVolume(100);

  // configure the voices
  synth.programChange(upperBank * 127, upperChannel, upperInstrument - 1);
  synth.programChange(lowerBank * 127, lowerChannel, lowerInstrument - 1);
  
  
}

//---------------------------------------------------------------------------------------------//
// main loop
//---------------------------------------------------------------------------------------------//
void loop()
{
  // get the keystate
  getKeystate();
  
  // start of showing press state
  if (DEBUG == 1)
  {
    Serial.println();
    Serial.print("P");
  }
  
  // do something if a key was pressed or released
  for (index = 0; index < NUM_KEYS; index++)
  {
    theNote = index + LOWEST_NOTE;
    // keydown on lower - send note on
    if ((pressStateLower[index]) == 1)
    {
      synth.noteOn(lowerChannel, theNote, DEFAULT_VELOCITY);
    }
    // keyup on lower - send note off
    if ((pressStateLower[index]) == 2)
    {
      synth.noteOff(lowerChannel, theNote);
    }
    
    // keydown on upper - send note on
    if ((pressStateUpper[index]) == 1)
    {
      synth.noteOn(upperChannel, theNote, DEFAULT_VELOCITY);
    }
    // keyup on upper - send note off
    if ((pressStateUpper[index]) == 2)
    {
      synth.noteOff(upperChannel, theNote);
    }
    
    // print a . every eight press states to make it more readable
    if (DEBUG == 1)
    {    
      if ((index % 8) == 0)
      {
        Serial.print(".");
      }
    }
    
    // print each press state
    if (DEBUG == 1)
    {
      Serial.print(pressStateLower[index], DEC);
    }
  }
  
  // delay for testing
  if (DEBUG == 1)
  {
    delay(1000);
  }
}

//---------------------------------------------------------------------------------------------//
// function getkeystate()
// gets the state of the keys, pressed or released
//---------------------------------------------------------------------------------------------//
void getKeystate()
{
  static long lastTime;
  static uint8_t previousStateUpper[NUMBER_OF_SHIFT_CHIPS] = {0};
  static uint8_t previousStateLower[NUMBER_OF_SHIFT_CHIPS] = {0};
  // track selecting upper or lower bus
  // first time through select the lower bus
  static uint8_t isUpper = 0;
  uint8_t bitVal = 0;
  uint8_t byteVal = 0;
  uint8_t bitSelect;
  uint8_t bitsChanged = 0;
  uint8_t noteIndex;
  uint8_t previousByte;
  
  // handle when millis counter overflows
  if (millis() <  lastTime)
  {
    lastTime = millis();
  }
  
  // the debounce period has not elapsed yet
  // go back and handle other stuff in the main loop
  if ((lastTime + DEBOUNCE) > millis())
  {
    return;
  }
  
  // otherwise DEBOUNCE has elapsed so reset the timer
  lastTime = millis();
  
  // select a bus
  // upper bus
  if (isUpper == 1)
  {
    // set lower to high-z
    pinMode(BUS_LOWER, INPUT);
    // set upper to output
    pinMode(BUS_UPPER, OUTPUT);
    // make upper low
    digitalWrite(BUS_UPPER, LOW);
  }
  // lower bus
  else
  {
    // set upper to high-z
    pinMode(BUS_UPPER, INPUT);
    // set lower to output
    pinMode(BUS_LOWER, OUTPUT);
    // make lower low
    digitalWrite(BUS_LOWER, LOW);
  }

  // trigger a parallel load to latch the state of all the data lines
  digitalWrite(SHIFT_LOAD, LOW);
  delayMicroseconds(PULSE_WIDTH_USEC);
  digitalWrite(SHIFT_LOAD, HIGH);
  
  // reset the note index
  // have to count down because we start with the last byte or group of keys
  noteIndex = 63;
  
  // loop through the number of shift chips and shift in a byte at a time
  // count down because we get the last byte from the shift register first
  for (int i = (NUMBER_OF_SHIFT_CHIPS - 1); i >= 0; i--)
  {
    // get the previous byte
    // upper
    if (isUpper == 1)
    {
      previousByte = previousStateUpper[i];
    }
    // lower
    else
    {
      previousByte = previousStateLower[i];
    }
    
    // loop through each bit in each shift chip byte
    // do it backwards to get the byte in the correct order
    for (int j = (DATA_WIDTH - 1); j >= 0 ; j--)
    {
      bitVal = digitalRead(SHIFT_DATA);
  
      // set the corresponding bit in the selected byte of currentState
      // byteVal is the current state of the selected byte or chip
      byteVal = byteVal + (bitVal << j);
  
      // pulse the Clock to get the next bit (rising edge shifts the next bit)
      digitalWrite(SHIFT_CLOCK, HIGH);
      delayMicroseconds(PULSE_WIDTH_USEC);
      digitalWrite(SHIFT_CLOCK, LOW);    
    }
    
    // XOR current and previous to find the difference
    bitsChanged = (byteVal ^ previousByte);
    
    // see if anything changed for the selected byte between states
    if (bitsChanged != 0)
    { 

      // find each bit that changed      
      // start with the MSB and work downwards
      // first byte out of the register is the last byte of keys (rightmost)
      // keys are numbered 0 -> 63
      // so the last key of the last byte is the MSB
      bitSelect = 128;
      for (int k = 0; k < DATA_WIDTH; k++)
      {        
        // if the selected bit is '1' it has changed
        // determine the press state of the note
        if ((bitsChanged & bitSelect) == bitSelect)
        {
          
          // if the selected bit in byteVal is '1'
          // the key has been pressed so the press state is '1'
          if ((byteVal & bitSelect) == bitSelect)
          {
            if (isUpper == 1)
            {
              pressStateUpper[noteIndex] = 1;
            }
            else
            {
              pressStateLower[noteIndex] = 1;
            }
          }
          // otherwise the press state is '2'
          else
          {
            if (isUpper == 1)
            {
              pressStateUpper[noteIndex] = 2;
            }
            else
            {
              pressStateLower[noteIndex] = 2;
            }
          }
        }
        // nothing changed so set the press state to '0'
        else
        {
          if (isUpper == 1)
          {
            pressStateUpper[noteIndex] = 0;
          }
          else
          {
            pressStateLower[noteIndex] = 0;
          }
        }
        
        // decrement the note index by 1
        noteIndex--;
        // shift bitSelect one to the right
        bitSelect = bitSelect >> 1;
      }
      
      // since there was a change, copy byteVal to the current indexed byte of previousState
      if (isUpper == 1)
      {
        previousStateUpper[i] = byteVal;
      }
      else
      {
        previousStateLower[i] = byteVal;
      }
    }
    // nothing changed
    else
    {     
      // no notes changes need to be sent
      // so set the next eight notes to the left as '0'
      if (isUpper == 1)
      {
        pressStateUpper[noteIndex] = 0;
        noteIndex--;
        pressStateUpper[noteIndex] = 0;
        noteIndex--;
        pressStateUpper[noteIndex] = 0;
        noteIndex--;
        pressStateUpper[noteIndex] = 0;
        noteIndex--;
        pressStateUpper[noteIndex] = 0;
        noteIndex--;
        pressStateUpper[noteIndex] = 0;
        noteIndex--;
        pressStateUpper[noteIndex] = 0;
        noteIndex--;
        pressStateUpper[noteIndex] = 0;
        noteIndex--;
      }
      else
      {
        pressStateLower[noteIndex] = 0;
        noteIndex--;
        pressStateLower[noteIndex] = 0;
        noteIndex--;
        pressStateLower[noteIndex] = 0;
        noteIndex--;
        pressStateLower[noteIndex] = 0;
        noteIndex--;
        pressStateLower[noteIndex] = 0;
        noteIndex--;
        pressStateLower[noteIndex] = 0;
        noteIndex--;
        pressStateLower[noteIndex] = 0;
        noteIndex--;
        pressStateLower[noteIndex] = 0;
        noteIndex--;
      }
   
      // update previous state
      if (isUpper == 1)
      {
        previousStateUpper[i] = byteVal;
      }
      else
      {
        previousStateLower[i] = byteVal;
      }
    }
     
    // clear the byte
    byteVal = 0;
  }
  
  // switch busses
  if (isUpper == 1)
  {
    isUpper = 0;
  }
  else
  {
    isUpper = 1;
  }
  
  return;
}
  

