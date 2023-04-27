#pragma once
//
//    FILE: PhoneKeyPad.h
//  AUTHOR: Wim Matthijs forked from PhoneKeypad by Rob Tillaart
// VERSION: 1.0.0
// PURPOSE: Arduino library for 4x4 KeyPad connected to an I2C PCF8574


#include "Arduino.h"
#include "Wire.h"

#define I2C_KEYPAD_LIB_VERSION    (F("1.0.0"))

#define I2C_KEYPAD_NOKEY          16
#define I2C_KEYPAD_FAIL           17
#define I2C_KEYPAD_BOUNCE         18

#define I2C_KEYPAD_4x4            44
#define I2C_KEYPAD_5x3            53
#define I2C_KEYPAD_6x2            62
#define I2C_KEYPAD_8x1            81


class PhoneKeypad
{
public:
  PhoneKeypad(const uint8_t deviceAddress, TwoWire *wire = &Wire);

#if defined(ESP8266) || defined(ESP32)
  bool    begin(uint8_t sda, uint8_t scl);
#endif
  bool    begin();

  // set the debounce in millis for keypresses
  void setDebounce(uint8_t debounce);
  uint8_t getDebounce();
  
  //  get raw key's 0..15
  uint8_t readKey();
  uint8_t getLastKey();

  bool    isPressed();
  bool    isConnected();

  //  get 'translated' keys
  //  user must load KeyMap, there is no check.
  void    loadKeyMap(char * keyMap);   //  char[19]
  char getChar();
  void setLatestCharsDepth(uint8_t depth);
  uint8_t getLatestCharsDepth();
  String getLatestChars();
  uint8_t getLatestCharsLength();
  void clearLatestChars();
  
  //get lenght of current keypress in millis
  uint16_t getPressLengthMillis();

  //  mode functions - experimental
  void    setKeyPadMode(uint8_t mode = I2C_KEYPAD_4x4);
  uint8_t getKeyPadMode();


protected:
  bool _isPressed;
  uint8_t _address;
  uint8_t _lastKey;
  uint8_t _mode;
  uint8_t _read(uint8_t mask);
  uint8_t _readKey4x4();
  uint8_t _debounceTimeMillis;
  long _lastPressMillis;

  //  experimental - could be public ?!
  uint8_t _readKey5x3();
  uint8_t _readKey6x2();
  uint8_t _readKey8x1();

  TwoWire* _wire;

  char *  _keyMap = NULL;
  uint8_t _latestCharsDepth = 0;
  String _latestChars = "";
};


// -- END OF FILE --

