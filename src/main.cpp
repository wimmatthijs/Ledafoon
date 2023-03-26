#include <Arduino.h>
#ifdef ARDUINO_ARCH_RP2040
void setup() {}
void loop() {}
#else
#include "AudioFileSourceSD.h"
#include "AudioOutputI2S.h"
#include "AudioGeneratorMP3.h"
#include "AudioFileSourceID3.h"


// For this sketch, you need connected SD card with '.flac' music files in the root
// directory. Some samples with various sampling rates are available from i.e.
// Espressif Audio Development Framework at:
// https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/audio-samples.html
//
// On ESP8266 you might need to re-encode FLAC files with max '-2' compression level
// (i.e. 1152 maximum block size) or you will run out of memory. FLAC files will be
// slightly bigger but you don't loose audio quality with reencoding (lossles codec).


// You may need a fast SD card. Set this as high as it will work (40MHz max).
#define SPI_SPEED SD_SCK_MHZ(40)
#define SPI_CS_PIN D0


File dir;
AudioFileSourceSD *source = NULL;
AudioFileSourceID3 *id3;
AudioOutputI2S *output = NULL;
AudioGeneratorMP3 *decoder = NULL;


// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.)
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  Serial.printf("ID3 callback for: %s = '", type);


  if (isUnicode) {
    string += 2;
  }
 
  while (*string) {
    char a = *(string++);
    if (isUnicode) {
      string++;
    }
    Serial.printf("%c", a);
  }
  Serial.printf("'\n");
  Serial.flush();
}






void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(10000);


  audioLogger = &Serial;  
  source = new AudioFileSourceSD();
  output = new AudioOutputI2S();
  decoder = new AudioGeneratorMP3();
 


  // NOTE: SD.begin(...) should be called AFTER AudioOutput...()
  //       to takover the the SPI pins if they share some with I2S
  //       (i.e. D8 on Wemos D1 mini is both I2S BCK and SPI SS)
  #if defined(ESP8266)
    SD.begin(SPI_CS_PIN, SPI_SPEED);
  #else
    SD.begin();
  #endif
  dir = SD.open("/");
}


void loop() {
  if ((decoder) && (decoder->isRunning())) {
    if (!decoder->loop()) decoder->stop();
  } else {
    File file = dir.openNextFile();
    if (file) {      
      if (String(file.name()).endsWith(".mp3")) {
        source->close();
        if (source->open(file.name())) {
          Serial.printf_P(PSTR("Playing '%s' from SD card...\n"), file.name());
          id3 = new AudioFileSourceID3(source);
          id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
          decoder->begin(id3, output);
        } else {
          Serial.printf_P(PSTR("Error opening '%s'\n"), file.name());
        }
      }
    } else {
      Serial.println(F("Playback from SD card done\n"));
      delay(1000);
    }      
  }
}
#endif





