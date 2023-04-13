#ifndef RTC_RAM_H
#define RTC_RAM_H

extern "C" {
  #include "user_interface.h" //for being able to access the RTC RAM
}
#include <CRC32.h>    //to check if the data saved in RTC RAM is valid or gobbledeegook
#include "datatypes.h"

//number of bytes to be saved in RTC memory, excluding 4 byte CRC
//We will insert the variables that need to be retained here. in the current conscept 112 bytes. (28 buckets of 4);
//For good working of the functions the length should alwas be a multipe of 4.
#define DEEPSLEEPBACKUPLENGTH 12 //3 ints
#define DEEPSLEEPSETTINGSADDRESS 100 //place where the information should be written in RTC RAM, every place is a bucket of 4 bytes
#define PROGRAMBACKUPLENGTH 16 //3 bools and 2 longs
#define PROGRAMSETTINGSADDRESS 104 //place where the information should be written in RTC RAM

uint32_t calculateChecksum(uint8_t byteBuffer[], uint8_t bufferSize);
void WriteToRTCRAM (int address, uint8_t byteBuffer[], uint8_t bufferSize);
bool ReadFromRTCRAMAndCheck(int address, uint8_t byteBuffer[], uint8_t bufferSize);

#endif