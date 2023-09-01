//
//    FILE: PhoneKeypad.cpp
//  AUTHOR: Wim Matthijs
// VERSION: 1.0.0
// PURPOSE: Arduino library for 4x4 KeyPad connected to an I2C PCF8574


#include "PhoneKeypad.h"


PhoneKeypad::PhoneKeypad(const uint8_t deviceAddress, TwoWire *wire)
{
  _lastKey = I2C_KEYPAD_NOKEY;
  _address = deviceAddress;
  _wire    = wire;
  _mode    = I2C_KEYPAD_4x4;
}



#if defined(ESP8266) || defined(ESP32)
#ifdef Ledafoon1
bool PhoneKeypad::begin(uint8_t sda, uint8_t scl)
{
  _wire->begin(sda, scl);
  //  enable interrupts
  _read(0xF0);
  return isConnected();
}
#else
bool PhoneKeypad::begin(uint8_t sda, uint8_t scl)
{
  _wire->begin(sda, scl);
  //  enable interrupts
  _read(0x9C);
  return isConnected();
}
#endif
#endif


bool PhoneKeypad::begin()
{
  _wire->begin();
  //  enable interrupts
  #ifdef Ledafoon1
  _read(0xF0);
  #else
  _read(0x9C);
  #endif
  return isConnected();
}

void PhoneKeypad::setDebounce(uint8_t debounce)
{
    _debounceTimeMillis = debounce;
}

uint8_t PhoneKeypad::getDebounce()
{
    return _debounceTimeMillis;
}

uint8_t PhoneKeypad::readKey()
{
  //check if debounce conditions met for another read
  if (_mode == I2C_KEYPAD_5x3) _lastKey =  _readKey5x3();
  if (_mode == I2C_KEYPAD_6x2) _lastKey = _readKey6x2();
  if (_mode == I2C_KEYPAD_8x1) _lastKey = _readKey8x1();
  else _lastKey = _readKey4x4();
  if (_lastKey==I2C_KEYPAD_NOKEY){
    _isPressed = false;
    return _lastKey;
  }
  if(millis()-_lastPressMillis<_debounceTimeMillis) return I2C_KEYPAD_BOUNCE;
  if (_latestCharsDepth && _lastKey<16){
    _lastPressMillis=millis();
    _latestChars += _keyMap[_lastKey];
    _isPressed = true;
    if (_latestChars.length()>_latestCharsDepth) _latestChars.remove(0,1);
  }
  return _lastKey;
}


uint8_t PhoneKeypad::getLastKey()   
{ 
  return _lastKey;
};


//  to check "press any key"
bool PhoneKeypad::isPressed()
{
  return _isPressed;
}


bool PhoneKeypad::isConnected()
{
  _wire->beginTransmission(_address);
  return (_wire->endTransmission() == 0);
}


char PhoneKeypad::getChar()
{ 
  return _keyMap[_lastKey]; 
}


void PhoneKeypad::setLatestCharsDepth(uint8_t depth)
{
    _latestCharsDepth = depth;
}

uint8_t PhoneKeypad::getLatestCharsDepth()
{
    return _latestCharsDepth;
}

String PhoneKeypad::getLatestChars(){
    return _latestChars;
}

uint8_t PhoneKeypad::getLatestCharsLength(){
  return (uint8_t)_latestChars.length();
}

void PhoneKeypad::clearLatestChars(){
  _latestChars="";
}

uint16_t PhoneKeypad::getPressLengthMillis(){
  return (uint16_t)(millis()-_lastPressMillis);
}


void PhoneKeypad::loadKeyMap(char * keyMap)
{
  _keyMap = keyMap;
}


void PhoneKeypad::setKeyPadMode(uint8_t mode)
{
  if ((mode == I2C_KEYPAD_5x3) || 
      (mode == I2C_KEYPAD_6x2) ||
      (mode == I2C_KEYPAD_8x1))
  {
    _mode = mode;
    return;
  }
  _mode = I2C_KEYPAD_4x4;
}


uint8_t PhoneKeypad::getKeyPadMode()
{
  return _mode;
}


//////////////////////////////////////////////////////
//
//  PROTECTED
//
uint8_t PhoneKeypad::_read(uint8_t mask)
{
  //  improve the odds that IO will not interrupted.
  yield();

  _wire->beginTransmission(_address);
  _wire->write(mask);
  if (_wire->endTransmission() != 0)
  {
    //  set communication error
    return 0xFF;
  }
  _wire->requestFrom(_address, (uint8_t)1);
  return _wire->read();
}


#ifdef Ledafoon1
uint8_t PhoneKeypad::_readKey4x4()
{
  //  key = row + 4 x col
  uint8_t key = 0;

  //  mask = 4 rows as input pull up, 4 columns as output
  uint8_t rows = _read(0xF0);
  //  check if single line has gone low.
  if (rows == 0xF0)      return I2C_KEYPAD_NOKEY;
  else if (rows == 0xE0) key = 0;
  else if (rows == 0xD0) key = 1;
  else if (rows == 0xB0) key = 2;
  else if (rows == 0x70) key = 3;
  else return I2C_KEYPAD_FAIL;

  // 4 columns as input pull up, 4 rows as output
  uint8_t cols = _read(0x0F);
  // check if single line has gone low.
  if (cols == 0x0F)      return I2C_KEYPAD_NOKEY;
  else if (cols == 0x0E) key += 0;
  else if (cols == 0x0D) key += 4;
  else if (cols == 0x0B) key += 8;
  else if (cols == 0x07) key += 12;
  else return I2C_KEYPAD_FAIL;

  _lastKey = key;

  return key;   // 0..15
}
#else
uint8_t PhoneKeypad::_readKey4x4()
{
  //  key = row + 4 x col
  uint8_t key = 0;

  //  mask = 4 rows as input pull up, 4 columns as output
  uint8_t rows = _read(0x39);
  //  check if single line has gone low.
  if (rows == 0x39)      return I2C_KEYPAD_NOKEY;
  else if (rows == 25) key = 0;
  else if (rows == 41) key = 1;
  else if (rows == 49) key = 2;
  // else if (rows == 0x3B) key = 3; Not connected in this phone
  else return I2C_KEYPAD_FAIL;

  // 4 columns as input pull up, 4 rows as output
  uint8_t cols = _read(0xC6);
  // check if single line has gone low.
  if (cols == 0xC6)      return I2C_KEYPAD_NOKEY;
  else if (cols == 134) key += 0;
  else if (cols == 194) key += 4;
  else if (cols == 70) key += 8;
  else if (cols == 196) key += 12;
  else return I2C_KEYPAD_FAIL;

  _lastKey = key;

  return key;   // 0..15
}
#endif


//  not tested
uint8_t PhoneKeypad::_readKey5x3()
{
  //  key = row + 5 x col
  uint8_t key = 0;

  //  mask = 5 rows as input pull up, 3 columns as output
  uint8_t rows = _read(0xF8);
  //  check if single line has gone low.
  if (rows == 0xF8)      return I2C_KEYPAD_NOKEY;
  else if (rows == 0xF0) key = 0;
  else if (rows == 0xE8) key = 1;
  else if (rows == 0xD8) key = 2;
  else if (rows == 0xB8) key = 3;
  else if (rows == 0x78) key = 4;
  else return I2C_KEYPAD_FAIL;

  // 3 columns as input pull up, 5 rows as output
  uint8_t cols = _read(0x07);
  // check if single line has gone low.
  if (cols == 0x07)      return I2C_KEYPAD_NOKEY;
  else if (cols == 0x06) key += 0;
  else if (cols == 0x05) key += 5;
  else if (cols == 0x03) key += 10;
  else return I2C_KEYPAD_FAIL;

  _lastKey = key;

  return key;   // 0..14
}


//  not tested
uint8_t PhoneKeypad::_readKey6x2()
{
  //  key = row + 6 x col
  uint8_t key = 0;

  //  mask = 6 rows as input pull up, 2 columns as output
  uint8_t rows = _read(0xFC);
  //  check if single line has gone low.
  if (rows == 0xFC)      return I2C_KEYPAD_NOKEY;
  else if (rows == 0xF8) key = 0;
  else if (rows == 0xF4) key = 1;
  else if (rows == 0xEC) key = 2;
  else if (rows == 0xDC) key = 3;
  else if (rows == 0xBC) key = 4;
  else if (rows == 0x7C) key = 5;
  else return I2C_KEYPAD_FAIL;

  // 2 columns as input pull up, 6 rows as output
  uint8_t cols = _read(0x03);
  // check if single line has gone low.
  if (cols == 0x03)      return I2C_KEYPAD_NOKEY;
  else if (cols == 0x02) key += 0;
  else if (cols == 0x01) key += 6;
  else return I2C_KEYPAD_FAIL;

  _lastKey = key;

  return key;   // 0..11
}


//  not tested
uint8_t PhoneKeypad::_readKey8x1()
{
  //  key = row
  uint8_t key = 0;

  //  mask = 8 rows as input pull up, 0 columns as output
  uint8_t rows = _read(0xFF);
  //  check if single line has gone low.
  if (rows == 0xFF)      return I2C_KEYPAD_NOKEY;
  else if (rows == 0xFE) key = 0;
  else if (rows == 0xFD) key = 1;
  else if (rows == 0xFB) key = 2;
  else if (rows == 0xF7) key = 3;
  else if (rows == 0xEF) key = 4;
  else if (rows == 0xDF) key = 5;
  else if (rows == 0xBF) key = 6;
  else if (rows == 0x7F) key = 7;
  else return I2C_KEYPAD_FAIL;

  _lastKey = key;

  return key;   // 0..7
}


// -- END OF FILE --

