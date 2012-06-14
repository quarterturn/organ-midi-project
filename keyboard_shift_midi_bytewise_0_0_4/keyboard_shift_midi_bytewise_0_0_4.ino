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
 HP Entertainment Center VFD display
 and old bussbar-type organ keyboard
 
 Git repository: https://github.com/quarterturn/organ-midi-project
 */

// external libraries
// button debouncing
#include <Bounce.h>
// fluxamasynth
#include "Fluxamasynth.h"
// hp media center vfd
#include <HpDecVfd.h>
// for using atmega eeprom
#include <EEPROM.h>

// constants
// 8 bits all '1'
#define BITS_8_ON 0xFF

// version
#define VERSION "0.0.4"

// debounce interval - 10 ms
#define DEBOUNCE 10
// debounce/jitter interval for pots
#define POT_DEBOUNCE 100

/* NOTE
   SoftwareSerial uses pin 4
   so don't go using it for anything else, ok?
*/

// buttons
#define BUTTON_BANK 2
#define BUTTON_RANK 3

// analog inputs
#define I_POT_VOICE A0
#define I_POT_VELOCITY A1
#define I_POT_CUTOFF A2
#define I_POT_RESONANCE A3

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
#define DEFAULT_VELOCITY 100

// turn on debugging output via Serial
#define DEBUG 0

// eeprom memory locations
#define EE_U_VOICE 0
#define EE_L_VOICE 1
#define EE_U_VELOCITY 2
#define EE_L_VELOCITY 3
#define EE_U_CUTOFF 4
#define EE_L_CUTOFF 5
#define EE_U_RESONANCE 6
#define EE_L_RESONANCE 7
#define EE_U_BANK 8
#define EE_L_BANK 9
#define EE_NEWCHIP1 127
#define EE_NEWCHIP2 128

// values to test if chip has been programmed
#define NEWCHIP1_VAL 0xaa
#define NEWCHIP2_VAL 0x55

// bounce objects
Bounce buttonMode = Bounce(BUTTON_BANK, DEBOUNCE);
Bounce buttonRank = Bounce(BUTTON_RANK, DEBOUNCE);

// create a vfd object based on HpDecVfd class
// pass the pins to use to the constructor
HpDecVfd vfd(6, 7);

// create a synth object
Fluxamasynth synth;

// global variables

// an array of button states
// 2 means just released
// 0 means nothing happened
// 1 means just pressed
// takes up 1024 bytes of RAM total
byte pressStateLower[NUM_KEYS] = {0};
byte pressStateUpper[NUM_KEYS] = {0};

// 8-bit unsigned int to hold the note select mask
// initialize with a 1 in the LSB; this represents key 1 the leftmost key
uint8_t noteSel = 1;

// channel for upper manual
byte upperChannel = 0;
// channel for lower manual
byte lowerChannel = 1;
// master volume
byte masterVolume = 110;

// analog input values
// we store previous and current
byte potVoicePrevious = 0;
byte potVoiceCurrent = 0;
byte potVelocityPrevious = 0;
byte potVelocityCurrent = 0;
byte potCutoffPrevious = 0;
byte potCutoffCurrent = 0;
byte potResonancePrevious = 0;
byte potResonanceCurrent = 0;

// voice parameters
// bank for upper manual
byte upperBank = 0;
// bank for lower manual
byte lowerBank = 0;
// voice for upper manual
// default is piano
byte upperVoice = 0;
// voice for lower manual
// default is string ensemble
byte lowerVoice = 49;
// master volume
// velocity
byte upperVelocity = 110;
byte lowerVelocity = 110;
// resonance
byte upperResonance = 0;
byte lowerResonance = 0;
// cutoff
byte upperCutoff = 127;
byte lowerCutoff = 127;


// note index
byte index;

// note value
byte theNote;

// for tracking time in delay loops
unsigned long previousMillis;

// track which keyboard we are setting values on
byte setUpper = 1;

void setup()
{

  // button setup
  // buttons are active low
  // pull-ups enabled
  pinMode(BUTTON_BANK, INPUT);
  digitalWrite(BUTTON_BANK, HIGH);
  pinMode(BUTTON_RANK, INPUT);
  digitalWrite(BUTTON_RANK, HIGH);
  
  // analog inputs configured by default

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

  // initialize and clear the display
  vfd.begin(1);
  vfd.setCursor(0, 0);
  vfd.print("SHIFT-IN DEMO");
  vfd.setCursor(0, 1);
  vfd.print("V. 0.0.4 6/12");
  delay(5000);
  vfd.clear();

  // uart serial setup
  Serial.begin(9600);
  synth.midiReset();
  
  // check the eeprom to see if it has been programmed
  if ((EEPROM.read(EE_NEWCHIP1) + EEPROM.read(EE_NEWCHIP2)) == 0xff)
  {
    // the eeprom has been programmed so read in the values
    upperVoice = EEPROM.read(EE_U_VOICE);
    lowerVoice = EEPROM.read(EE_L_VOICE);
    upperVelocity = EEPROM.read(EE_U_VELOCITY);
    lowerVelocity = EEPROM.read(EE_L_VELOCITY);
    upperCutoff = EEPROM.read(EE_U_CUTOFF);
    lowerCutoff = EEPROM.read(EE_L_CUTOFF);
    upperResonance = EEPROM.read(EE_U_RESONANCE);
    lowerResonance = EEPROM.read(EE_L_RESONANCE);
    upperBank = EEPROM.read(EE_U_BANK);
    lowerBank = EEPROM.read(EE_L_BANK);
  }
  else
  {
    // write the defaults to the eeprom
    EEPROM.write(EE_U_VOICE, upperVoice);
    EEPROM.write(EE_L_VOICE, lowerVoice);
    EEPROM.write(EE_U_VELOCITY, upperVelocity);
    EEPROM.write(EE_L_VELOCITY, lowerVelocity);
    EEPROM.write(EE_U_CUTOFF, upperCutoff);
    EEPROM.write(EE_L_CUTOFF, lowerCutoff);
    EEPROM.write(EE_U_RESONANCE, upperResonance);
    EEPROM.write(EE_L_RESONANCE, lowerResonance);
    EEPROM.write(EE_U_BANK, upperBank);
    EEPROM.write(EE_L_BANK, lowerBank);
    EEPROM.write(EE_NEWCHIP1, NEWCHIP1_VAL);
    EEPROM.write(EE_NEWCHIP2, NEWCHIP2_VAL);
  }
  
  // enable chorus
  synth.setChorus(0, 1, 30, 30, 10);

  synth.setMasterVolume(100);	// max. master volume
//  synth.setChannelVolume(channel, 60);
//  synth.setMasterPan(pan1, pan2);


  // configure the voices
  synth.programChange(upperBank * 127, upperChannel, upperVoice);
  synth.programChange(lowerBank * 127, lowerChannel, lowerVoice);
  synth.setChannelVolume(upperChannel, upperVelocity);
  synth.setChannelVolume(lowerChannel, lowerVelocity);
  synth.setTVFCutoff(upperChannel, upperCutoff);
  synth.setTVFCutoff(lowerChannel, lowerCutoff);
  synth.setTVFResonance(upperChannel, upperResonance);
  synth.setTVFResonance(lowerChannel, lowerResonance);
 
  // display the initial values
  updateDisplay();
  
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
  
  // check the buttons and update the display
  if (checkButtons() > 0)
  {
    updateDisplay();
  }
  
  // check the pots and update the display
  if (getPots() == 1)
  {
    updateDisplay(); 
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
    // for (int j = 0; j < DATA_WIDTH; j++)
    {
      bitVal = digitalRead(SHIFT_DATA);
  
      // set the corresponding bit in the selected byte of currentState
      // byteVal is the current state of the selected byte or chip
      // byteVal |= (bitVal << ((DATA_WIDTH - 1) - j));
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
  
//---------------------------------------------------------------------------------------------//
// function getPots()
// gets the state of the analog inputs connected to the potentiometers
// returns 1 if values changed, 0 if no change
//---------------------------------------------------------------------------------------------//
byte getPots()
{
  byte potChanged = 0;
  
  static long lastPotTime;
  
  // handle when millis counter overflows
  if (millis() <  lastPotTime)
  {
    lastPotTime = millis();
  }
  
  // the debounce period has not elapsed yet
  // go back and handle other stuff in the main loop
  if ((lastPotTime + POT_DEBOUNCE) > millis())
  {
    return 0;
  }
  
  // otherwise DEBOUNCE has elapsed so reset the timer
  lastPotTime = millis();
  
  // read the analog inputs and scale to 0 - 127
  potVoiceCurrent = map(analogRead(I_POT_VOICE), 40, 960, 0, 127);
  potVelocityCurrent = map(analogRead(I_POT_VELOCITY), 40, 960, 0, 127);
  potCutoffCurrent = map(analogRead(I_POT_CUTOFF), 40, 960, 0, 127);
  potResonanceCurrent = map(analogRead(I_POT_RESONANCE), 40, 960, 0, 127);
  
  // something crazy is going on with map
  // values are going higher than 127
  // cheap fix
  if (potVoiceCurrent > 127)
  {
    potVoiceCurrent = 127;
  }
  if (potVelocityCurrent > 127)
  {
    potVelocityCurrent = 127;
  }
  if (potCutoffCurrent > 127)
  {
    potCutoffCurrent = 127;
  }
  if (potResonanceCurrent > 127)
  {
    potResonanceCurrent = 127;
  }
  
  // see if the values have changed
  // send the midi value if there is a change
  if (potVoiceCurrent != potVoicePrevious)
  {
    potVoicePrevious = potVoiceCurrent;
    potChanged = 1;
    if (setUpper == 1)
    {
      upperVoice = potVoiceCurrent;
      synth.programChange(upperBank * 127, upperChannel, upperVoice);
    }
    else
    {
      lowerVoice = potVoiceCurrent;
      synth.programChange(lowerBank * 127, lowerChannel, lowerVoice);
    }
  }
  if (potVelocityCurrent != potVelocityPrevious)
  {
    potVelocityPrevious = potVelocityCurrent;
    potChanged = 1;
    if (setUpper == 1)
    {
      upperVelocity = potVelocityCurrent;
      // I decided to use volume not velocity so it could be changed while notes are held down
      synth.setChannelVolume(upperChannel, upperVelocity);
    }
    else
    {
      lowerVelocity = potVelocityCurrent;
      synth.setChannelVolume(lowerChannel, lowerVelocity);
    }
  }
  if (potCutoffCurrent != potCutoffPrevious)
  {
    potCutoffPrevious = potCutoffCurrent;
    potChanged = 1;
    if (setUpper == 1)
    {
      upperCutoff = potCutoffCurrent;
      synth.setTVFCutoff(upperChannel, upperCutoff);
    }
    else
    {
      lowerCutoff = potCutoffCurrent;
      synth.setTVFCutoff(lowerChannel, lowerCutoff);
    }
  }
  if (potResonanceCurrent != potResonancePrevious)
  {
    potResonancePrevious = potResonanceCurrent;
    potChanged = 1;
    if (setUpper == 1)
    {
      upperResonance = potResonanceCurrent;
      synth.setTVFResonance(upperChannel, upperResonance);
    }
    else
    {
      lowerResonance = potResonanceCurrent;
      synth.setTVFResonance(lowerChannel, lowerResonance);
    }
  }
  
  // return with potChanged status
  return potChanged;
}

//---------------------------------------------------------------------------------------------//
// function checkButtons()
// gets the state of the buttons
// returns 1 if values changed, 0 if no change
//---------------------------------------------------------------------------------------------//
byte checkButtons()
{
  byte modeState;
  
  // upper/lower pressed
  if (buttonRank.update())
  {
    if (buttonRank.fallingEdge())
    {
      // toggle upper and lower
      if (setUpper == 1)
      {
        setUpper = 0;
      }
      else
      {
        setUpper = 1;
      }
      return 1;
    }
  }
  
  // bank (set) pressed
  if (buttonMode.update())
  {
    // get the button state
    modeState = buttonMode.read();
    if (modeState == LOW);
    {
      // if it has been low for more than 1 sec write the settingd to eeprom
      if (buttonMode.duration() > 1000)
      {
        // test first so we dont wear out the eeprom
        Serial.println("writing values to eeprom");
      }
      else
      {
        if (setUpper == 1)
        {
          if (upperBank == 1)
          {
            upperBank = 0;
          }
          else
          {
            upperBank = 1;
          }
          synth.programChange(upperBank * 127, upperChannel, upperVoice);
        }
        else
        {
          if (lowerBank == 1)
          {
            lowerBank = 0;
          }
          else
          {
            lowerBank = 1;
          }
          synth.programChange(lowerBank * 127, lowerChannel, lowerVoice);
        }
      }
    }
    return 1;
  }
  
  // otherwise nothing pressed
  return 0;
}     
          
//---------------------------------------------------------------------------------------------//
// function updateDisplay()
// updated the display
//---------------------------------------------------------------------------------------------//
void updateDisplay()
{
  
  // cursor to top row and leftmost character
  vfd.setCursor(0, 0);
  // B indicates bank
  vfd.print("B");
  // show the current bank
  if (setUpper == 1)
  {
    vfd.print(upperBank);
  }
  else
  {
    vfd.print(lowerBank);
  }
  // print a space
  vfd.print(" ");
  // P indicates program
  vfd.print("P");
  // show the current program
  if (setUpper == 1)
  {
    // format the values
    // pad with spaces as appropriate
    if (upperVoice < 10)
    {
      vfd.print("  ");
    }
    else if (upperVoice < 100)
    {
      vfd.print(" ");
    }
    vfd.print(upperVoice);
  }
  else
  {
    // format the values
    // pad with spaces as appropriate
    if (lowerVoice < 10)
    {
      vfd.print("  ");
    }
    else if (lowerVoice < 100)
    {
      vfd.print(" ");
    }
    vfd.print(lowerVoice);
  }
  // print a space
  vfd.print(" ");
  // P indicates program
  vfd.print("V");
  // show the current velocity
  if (setUpper == 1)
  {
    // format the values
    // pad with spaces as appropriate
    if (upperVelocity < 10)
    {
      vfd.print("  ");
    }
    else if (upperVelocity < 100)
    {
      vfd.print(" ");
    }
    vfd.print(upperVelocity);
  }
  else
  {
    // format the values
    // pad with spaces as appropriate
    if (lowerVelocity < 10)
    {
      vfd.print("  ");
    }
    else if (lowerVelocity < 100)
    {
      vfd.print(" ");
    }
    vfd.print(lowerVelocity);
  }
  // cursor to bottom row and leftmost character
  vfd.setCursor(0, 1);
  // show upper or lower setting
  if (setUpper == 1)
  {
    vfd.print("U");
  }
  else
  {
    vfd.print("L");
  }
  // print two spaces
  vfd.print("  ");
  // F indicates cutoff frequency
  vfd.print("F");
  // show the current frequency
  if (setUpper == 1)
  {
    // format the values
    // pad with spaces as appropriate
    if (upperCutoff < 10)
    {
      vfd.print("  ");
    }
    else if (upperCutoff < 100)
    {
      vfd.print(" ");
    }
    vfd.print(upperCutoff);
  }
  else
  {
    // format the values
    // pad with spaces as appropriate
    if (lowerCutoff < 10)
    {
      vfd.print("  ");
    }
    else if (lowerCutoff < 100)
    {
      vfd.print(" ");
    }
    vfd.print(lowerCutoff);
  }
  // print a space
  vfd.print(" ");
  // Q indicates resonance
  vfd.print("Q");
  // show the current resonance
  if (setUpper == 1)
  {
    // format the values
    // pad with spaces as appropriate
    if (upperResonance < 10)
    {
      vfd.print("  ");
    }
    else if (upperResonance < 100)
    {
      vfd.print(" ");
    }
    vfd.print(upperResonance);
  }
  else
  {
    // format the values
    // pad with spaces as appropriate
    if (lowerResonance < 10)
    {
      vfd.print("  ");
    }
    else if (lowerResonance < 100)
    {
      vfd.print(" ");
    }
    vfd.print(lowerResonance);
  }
  // that's it
  return;
}
