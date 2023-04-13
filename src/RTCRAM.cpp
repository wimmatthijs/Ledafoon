#include "RTCRAM.h"

/* void BackupStateToRTCRAM(DeepsleepSettings settings){
  uint8_t byteBuffer[DEEPSLEEPBACKUPLENGTH+4];
  memcpy(&byteBuffer[0], &settings.sleptFor, sizeof(int));
  memcpy(&byteBuffer[4], &settings.deepSleepTimer, sizeof(int));
  memcpy(&byteBuffer[8], &settings.maxDeepSleepWithoutSync, sizeof(int));
  uint32_t checksum = calculateChecksum(byteBuffer, DEEPSLEEPBACKUPLENGTH+4);
  memcpy(&byteBuffer[DEEPSLEEPBACKUPLENGTH], &checksum, sizeof(int));
  WriteToRTCRAM (DEEPSLEEPSETTINGSADDRESS, byteBuffer, DEEPSLEEPBACKUPLENGTH+4);
}

DeepsleepSettings ReadDeepsleepStateFromRTCRAM(){
  uint8_t byteBuffer[DEEPSLEEPBACKUPLENGTH+4];
  DeepsleepSettings	 settings;
  if(ReadFromRTCRAMAndCheck(DEEPSLEEPSETTINGSADDRESS,byteBuffer, DEEPSLEEPBACKUPLENGTH+4)){
    memcpy(&settings.sleptFor, &byteBuffer[0], sizeof(int));
    memcpy(&settings.deepSleepTimer, &byteBuffer[4], sizeof(int));
    memcpy(&settings.maxDeepSleepWithoutSync,&byteBuffer[8], sizeof(int));
  }
  else{
    //Serial.println("Error reading deepsleep settings, putting defaults (sleptFor 0 deepSleepTimer 0)");
    settings.sleptFor = 0;
    settings.deepSleepTimer = 0;
  }
  return settings;
} */


uint32_t calculateChecksum(uint8_t byteBuffer[], uint8_t bufferSize){
  CRC32 crc;
  size_t numBytes = bufferSize - 4; // deduct last byte of the byte buffer, the checsum will be stored here
  for (size_t i = 0; i < numBytes; i++)
  {
    crc.update(byteBuffer[i]);
  }
  return crc.finalize();
}

void WriteToRTCRAM (int address, uint8_t byteBuffer[], uint8_t bufferSize){
  //Serial.println("Bytebuffer to write: ");
  //Serial.print("Size of bytebufer: ");
  //Serial.println(bufferSize);
  for (int i=0;i<bufferSize;i++){
      //Serial.println(byteBuffer[i]);
  }
  int buckets = bufferSize/4;
  if (buckets == 0) buckets = 1;
  int temp;
  for (int i = 0; i<buckets;i++){
    system_rtc_mem_write(address+i, &byteBuffer[i*4], 4);
    //Serial.print("written: ");
    memcpy(&temp, &byteBuffer[i*4], sizeof(int));
    //Serial.println(temp);
  }
}

bool ReadFromRTCRAMAndCheck(int address, uint8_t byteBuffer[], uint8_t bufferSize){
  int buckets = bufferSize/4;
  if (buckets == 0) buckets = 1;
  int temp;
  for (int i = 0; i<buckets;i++){
    system_rtc_mem_read(address+i, &byteBuffer[i*4], 4);
    //Serial.print("read: ");
    memcpy(&temp, &byteBuffer[i*4], sizeof(int));
    //Serial.println(temp);
  }
  uint32_t calculatedChecksum = calculateChecksum(byteBuffer, bufferSize);
  //Serial.print("Checksum calculated by CRCLib: ");
  //Serial.println(calculatedChecksum);
  uint32_t readChecksum;
  memcpy(&readChecksum, &byteBuffer[bufferSize-4], sizeof(uint32_t));
  //Serial.print("Checsum as found in buffer: ");
  //Serial.println(readChecksum);
  if(calculatedChecksum == readChecksum){
    //Serial.println("These checksums are the same, amazing! Restoring program state");
    return true;
  }
  //Serial.println("CRC Error");
  return false;
}