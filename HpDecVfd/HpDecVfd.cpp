/*
  HpDecVfd Library - Implementation

  Originally created 25 March 2011

  This file is in the public domain.  
*/

#include "HpDecVfd.h"
#include "Arduino.h"

HpDecVfd::HpDecVfd(uint8_t clockPin, uint8_t dataPin)
{
  _clockPin = clockPin;
  _dataPin = dataPin;
 
  // Zero out icon state.
  for (uint8_t i=0; i<ICON_STATE_SIZE; i++)
  {
    _iconState[i] = 0;
  }

  // Assume no wait required for first command.
  _lastSendTime = 0;
  _lastCommandExecuteTimeInMicroseconds = 0;

  _sendingText = false;
  _desiredCursorAddress = 0;
  _actualCursorAddress = 0;

  // Configure pins.
  digitalWrite(clockPin, HIGH);
  pinMode(clockPin, OUTPUT);
  
  digitalWrite(dataPin, LOW);
  pinMode(dataPin, OUTPUT);  
}
  
HpDecVfd::~HpDecVfd()
{
}
  
void HpDecVfd::begin(bool clearDisplay)
{
  // Reset the display.
  resetDisplay();
  
  if (clearDisplay)
  {
    clear();
    clearIcons();
  }
  else
  {
    // Must have the cursor at a known location even if we don't clear.
    home();
  }
}
  
void HpDecVfd::clear()
{
  // Send clear command.
  beginCommand(COMMAND_EXTENDED_COMMANDS);
  sendByte(EXTENDED_COMMAND_CLEAR);
  endCommand();
  
  // Clear does home the cursor but the display 
  // starts on the bottom line so we correct for that 
  // oddity in our setCursor/home implementation.
  home();
}
  
void HpDecVfd::home()
{
  setCursorAddress(0b01000000); // Display uses bottom row as first row so invert to be normal.
}
  
void HpDecVfd::noDisplay()
{
  beginCommand(COMMAND_EXTENDED_COMMANDS);
  sendByte(EXTENDED_COMMAND_BLANK_DISPLAY);
  endCommand();
}
  
void HpDecVfd::display()
{
  beginCommand(COMMAND_EXTENDED_COMMANDS);
  sendByte(EXTENDED_COMMAND_UNBLANK_DISPLAY);
  endCommand();
}
  
void HpDecVfd::setCursor(uint8_t column, uint8_t row)
{
  uint8_t address = row * 64 + column; // Compute address.
  address ^= 0b01000000; // Display uses bottom row as first row so invert to be normal.

  setCursorAddress(address);
}
 
// Writes a text character to the display.  
/*virtual*/ 
size_t HpDecVfd::write(uint8_t character)
{
  if (_sendingText && timingOkToChainCharacter())
  {
    // Chain onto the current draw text command.
    sendByte(character);
    _desiredCursorAddress = (_desiredCursorAddress + 1) & 0b01111111;
  }
  else
  {
    // Need to start a new text command.
    
    // Fixup curosr location if necessary.
    if  (_desiredCursorAddress != _actualCursorAddress)
    {
      setCursorAddress(_desiredCursorAddress);
    }
    
    // Start text and draw the first character
    beginCommand(COMMAND_DRAW_TEXT);
    sendByte(character);
    
    // The actual cursor address will increment by 1 position only so 
    // desired and actual match after the first character.
    _desiredCursorAddress = _actualCursorAddress = (_actualCursorAddress + 1) & 0b01111111;
  
    // Set flag so next character can chain.
    _sendingText = true;
  }  
}

void HpDecVfd::resetDisplay()
{
  beginCommand(COMMAND_RESET_DISPLAY);
  endCommand(RESET_COMMAND_EXECUTION_TIME_IN_MICROSECONDS);
}

void HpDecVfd::setBrightness(uint8_t level) // Level is 0 to 15.
{
  if (level > 15)
  {
    level = 15;
  }
  
  beginCommand(COMMAND_SET_BRIGHTNESS | level);
  endCommand();
}

void HpDecVfd::clearIcons()
{
  for (uint8_t i=0; i<ICON_STATE_SIZE; i++)
  {
    _iconState[i] = 0;
  }

  sendIconState();  
}

void HpDecVfd::setIcon(Icon icon, bool enable)
{
  uint8_t stateIndex;
  uint8_t bitMask = getIconBitMask(icon, stateIndex);
    
  if (enable)
  {
    _iconState[stateIndex] |= bitMask;
  }
  else
  {
    _iconState[stateIndex] &= ~bitMask;
  }
  
  sendIconState();
}
  
bool HpDecVfd::isIconSet(Icon icon)
{
  uint8_t stateIndex;
  uint8_t bitMask = getIconBitMask(icon, stateIndex);
  
  return (_iconState[stateIndex] & bitMask) ? true : false;
}


uint8_t HpDecVfd::getIconBitMask(Icon icon, uint8_t &iconStateIndexOut)
{
  if (icon >= 0 && icon < NUM_ICONS)
  {
    uint8_t bitNum = icon;
    uint8_t stateIndex = 0;

    while (bitNum >= 8)
    {
      bitNum -= 8;;
      stateIndex ++;
    }
    
    iconStateIndexOut = stateIndex;
    return 1 << bitNum;
  }
  else
  {
    // Return the unused bit.
    iconStateIndexOut = 2;
    return 0b10000000;
  }    
}

void HpDecVfd::sendIconState()
{
  // Set icon state command is 4 bytes.
  beginCommand(COMMAND_SET_ICON_STATE);
  for (uint8_t i=0; i<ICON_STATE_SIZE; i++)
  {
    sendByte(_iconState[i]);
  }
  endCommand();
}

void HpDecVfd::setCursorAddress(uint8_t address)
{
  address &= 0b01111111; // Clamp to valid range.

  beginCommand(COMMAND_EXTENDED_COMMANDS);
  sendByte(EXTENDED_COMMAND_SET_CURSOR | address);
  endCommand();
  
  _desiredCursorAddress = _actualCursorAddress = address;
}

void HpDecVfd::beginCommand(uint8_t command)
{
  if (_sendingText)
  {
    endText();
  }
 
  waitForPreviousCommandToExecute();
  
  sendByte(command);
}

void HpDecVfd::endCommand(unsigned long executionTimeInMicroseconds)
{
  _lastCommandExecuteTimeInMicroseconds = executionTimeInMicroseconds;  
}


bool HpDecVfd::timingOkToChainCharacter()
{
  return ((micros() - _lastSendTime) < MAX_TEXT_CHAIN_TIME_IN_MICROSECONDS);
}

void HpDecVfd::endText()
{
  _sendingText = false;
  endCommand();
}
  
void HpDecVfd::waitForPreviousCommandToExecute()
{
  while ( (micros() - _lastSendTime) < _lastCommandExecuteTimeInMicroseconds )
  {
  }
}

void HpDecVfd::sendByte(uint8_t byteToSend)
{
  for (uint8_t mask = 0x80; mask; mask>>=1)
  {
    digitalWrite(_dataPin, (byteToSend & mask) ? HIGH : LOW);
    delayMicroseconds(HALF_CLOCK_PERIOD_IN_MICROSECONDS);
    digitalWrite(_clockPin, LOW);
    delayMicroseconds(HALF_CLOCK_PERIOD_IN_MICROSECONDS);
    digitalWrite(_clockPin, HIGH);
  }      

  _lastSendTime = micros();
}

