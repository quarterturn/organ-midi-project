/*
  HpDecVfd Library - Declaration file
  
  Originally created 25 March 2011

  This file is in the public domain.  
*/

#ifndef HP_DEC_VFD_H
#define HP_DEC_VFD_H

#include <inttypes.h>
#include "Print.h"

// API is designed to be as close to the standard LiquidCrystal library as possible.
class HpDecVfd : public Print {

public:

  enum Icon
  {
    ICON_ANTENNA,
    ICON_SIGNAL_BAR_1,
    ICON_SIGNAL_BAR_2,
    ICON_SIGNAL_BAR_3,
    ICON_SIGNAL_BAR_4,
    ICON_SIGNAL_BAR_5,
    ICON_REC,
    ICON_LEFT_TRACK_HORSESHOE,
    ICON_LEFT_TRACK_ARROW,
    ICON_RIGHT_TRACK_ARROW,
    ICON_RIGHT_TRACK_HORSESHOE,
    ICON_FAST_LOOP,
    ICON_PLAY_TRIANGLE,
    ICON_PAUSE_BARS,
    ICON_SPEAKER,
    ICON_NO_SYMBOL, // Slashed circle as used in no smoking sign.
    ICON_TRACK_SUBTITLE,
    ICON_TOTAL_SUBTITLE,
    ICON_TITLE_SUBTITLE,
    ICON_CHAPTER_SUBTITLE,
    ICON_HOUR_SUBTITLE,
    ICON_MIN_SUBTITLE,
    ICON_SEC_SUBTITLE,
  
    NUM_ICONS
  };

  HpDecVfd(uint8_t clockPin, uint8_t dataPin);
  ~HpDecVfd();
  
  // Methods consistent with LiquidCrystal.
  void begin(bool clearDisplay = true);

  void clear();
  void home();

  void noDisplay();
  void display();

  void setCursor(uint8_t column, uint8_t row); 
  size_t write(uint8_t character);

  // Methods new to our class.
  void resetDisplay();
  void setBrightness(uint8_t level); // Level is 0 to 15.
  
  void clearIcons();
  void setIcon(Icon icon, bool enable);
  bool isIconSet(Icon icon);
    
  // These LiquidCrystal methods are not implemented.
//  void noBlink();
//  void blink();
//  void noCursor();
//  void cursor();
//  void scrollDisplayLeft();
//  void scrollDisplayRight();
//  void leftToRight();
//  void rightToLeft();
//  void autoscroll();
//  void noAutoscroll();
//  void createChar(uint8_t characterIndex, uint8_t pixels[] /* 8 bytes, 1 for each row */);/
//  void command(uint8_t commandCode);
  
private:

  // Command list.
  static const uint8_t COMMAND_EXTENDED_COMMANDS = 0x00;  
  static const uint8_t EXTENDED_COMMAND_CLEAR = 0x00;
  static const uint8_t EXTENDED_COMMAND_SET_CURSOR = 0x80;
  static const uint8_t EXTENDED_COMMAND_BLANK_DISPLAY = 0x08;
  static const uint8_t EXTENDED_COMMAND_UNBLANK_DISPLAY = 0x0C;
  static const uint8_t COMMAND_DRAW_TEXT = 0x02;
  static const uint8_t COMMAND_SET_ICON_STATE = 0x40;
  static const uint8_t COMMAND_SET_BRIGHTNESS = 0xA0;
  static const uint8_t COMMAND_RESET_DISPLAY = 0xFA;

  // Timing data.
  static const uint8_t HALF_CLOCK_PERIOD_IN_MICROSECONDS = 28; // Period is 56 microseconds.
  static const unsigned long MAX_TEXT_CHAIN_TIME_IN_MICROSECONDS = 1000; // Who knows, 1ms seems to work.
  static const unsigned long DEFAULT_COMMAND_EXECUTION_TIME_IN_MICROSECONDS = 6000; // Measured at around 3ms.
  static const unsigned long RESET_COMMAND_EXECUTION_TIME_IN_MICROSECONDS = 100000; // Measured at around 65ms.

  static const uint8_t ICON_STATE_SIZE = 3;

  uint8_t _clockPin;
  uint8_t _dataPin;
  
  uint8_t _iconState[ICON_STATE_SIZE];

  unsigned long _lastSendTime;
  unsigned long _lastCommandExecuteTimeInMicroseconds;

  // We do batching of character sends into one command.
  // In addition to timing considerations, this does not
  // update the cursor address properly so we keep track and
  // update the cursor only when absolutely necessary.
  bool _sendingText;
  uint8_t _desiredCursorAddress;
  uint8_t _actualCursorAddress;
  
  uint8_t getIconBitMask(Icon icon, uint8_t &iconStateIndexOut);
  void sendIconState();

  void setCursorAddress(uint8_t address);
  
  void beginCommand(uint8_t command);
  void endCommand(unsigned long executionTimeInMicroseconds = DEFAULT_COMMAND_EXECUTION_TIME_IN_MICROSECONDS);
  void sendByte(uint8_t byteToSend);
  void endText();
  void waitForPreviousCommandToExecute();
  bool timingOkToChainCharacter();  
};

#endif // HP_DEC_VFD_H

