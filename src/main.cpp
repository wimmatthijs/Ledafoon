#include <Arduino.h>
#include "AudioFileSourceSD.h"
#include "AudioOutputI2S.h"
#include "AudioGeneratorMP3.h"
#include "AudioFileSourceID3.h"
#include <WiFiManager.h>
#include "datatypes.h"
#include "LFSOperations.h"
#include "WiFiManagerApp.h"
#include <time.h>   //for doing time stuff
#include <TZ.h>      //timezones
#include "Wire.h"
#include "PhoneKeypad.h"


//******************************************************************
//Function Declatarions (for Platform IO compatibilty)
//******************************************************************
//Functions related to WiFi Manager
void ConnectToWifi(); //Does what it says, goes to deepsleep if unsuccessful
void RunWiFiManager(); //runs in case of hard reset (5 seconds) or first boot (no files in filesystem)
void saveParamCallback(); //If custom parameters are saved on the webserver this function will be called
void ResetWifiRoutine(); //
//Time-keeping functions
void SetupTime(); //configs the time and requests time from server (automatic through time library)
void NTP_Sync_Callback(); //gets called if UDP timepacket arrives (asynchronous callback)
//Keypad functiond
void keyChanged();
//Program Functions
void setup();
void loop();

//******************************************************************
// Defines
//******************************************************************
#define MAXSLEEPWITHOUTSYNC 24*60* 60 //standard setting Maximum 24 hours without a timesync. If this time gets exceeded a timesync will be forced. 
#define TIMEZONE TZ_Europe_Brussels

// You may need a fast SD card. Set this as high as it will work (40MHz max).
#define SPI_SPEED SD_SCK_MHZ(20)
#define SPI_CS_PIN D0

#define WIFI_RESET_KEY 's' //button to reset microcontroller to reset WiFiManger
#define LONGPRESS_TIME_SECONDS 5 //how long the reset button needs to be pushed in order for the reset routine to be triggered



//******************************************************************
// global variables
//******************************************************************
//Variables for WiFi
WiFiSecrets wiFiSecrets;
unsigned long wiFiConnectStartMillis;
//timekeeping variables
volatile bool rtc_synced = false;   //keeps track if timesync already occured or not.
static time_t now;
String timeString, dateString; // strings to hold time 
int StartTime, CurrentHour = 0, CurrentMin = 0, CurrentSec = 0;
long CurrentMinuteCounter;
File dir;
//Audioplayback variables
AudioFileSourceSD *source = NULL;
AudioFileSourceID3 *id3;
AudioFileSource *mp3;
AudioOutputI2S *output = NULL;
AudioGeneratorMP3 *decoder = NULL;
//Keypad variables
const uint8_t KEYPAD_ADDRESS = 0x20;
PhoneKeypad keyPad(KEYPAD_ADDRESS);
char keys[] = "1A23798B465CsD0#NF@";  // N = NoKey, F = Fail (e.g. >1 keys pressed), @ = Bounced
volatile bool keyChange = false; // for interrupt in case of a keychange
bool samplePlaying = false;





//******************************************************************
// Program
//******************************************************************


//interruptroutine for keypad
IRAM_ATTR void keyChanged()
{
  keyChange = true;
}


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

void ConnectToWifi(){
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  initFS();
  if (!LittleFS.exists(F("/WiFiSecrets.txt"))){
    //TODO: audio signal that wifi was not saved, run the wifimanager
    Serial.println("No WiFi credentials found, starting Ledafoon Accesspoint");
    RunWiFiManager();
  }
  wiFiSecrets = RecoverWiFiSecrets();
  WiFi.begin(wiFiSecrets.SSID, wiFiSecrets.Pass);
}

void RunWiFiManager(){
  //recover all current settings from the installed App's.
  WiFiManagerApp wiFiManagerApp;
  if(wiFiManagerApp.Run()){
    ESP.restart();
  }
  else{
    //TODO: put some audio feedback here that wifi manager app didnt finish successfullly
    ESP.deepSleep(ESP.deepSleepMax());
  }
}

void ResetWifiRoutine(){
  deleteAllFiles();
  RunWiFiManager();
}

void SetupTime() {
  if(!rtc_synced){
    configTime(TIMEZONE, "be.pool.ntp.org");
    settimeofday_cb(NTP_Sync_Callback);
  }
  return;
}

void NTP_Sync_Callback(){
  rtc_synced=true;
  char output[30], day_output[30];
  now = time(nullptr);
  const tm* tm = localtime(&now);
  CurrentHour = tm->tm_hour;
  CurrentMin  = tm->tm_min;
  CurrentSec  = tm->tm_sec;
  CurrentMinuteCounter = CurrentHour*60 + CurrentMin;

  //Serial.print("Current minute couter: ");
  //Serial.println(CurrentMinuteCounter);
  #ifdef METRIC
    strftime(day_output, 30, "%d-%b-%y", tm);     // Displays: 24-Jun-17
    strftime(output, 30, "%H:%M", tm);            // Creates: '14:05'
  #else 
    strftime(day_output, 30, "%b-%d-%y", tm);     // Creates: Jun-24-17
    strftime(output, 30, "%I:%M%p", tm);          // Creates: '2:05pm'
  #endif
  dateString = day_output;
  timeString = output;
  Serial.println(dateString);
  Serial.println(timeString);
}

bool playMP3FromPath(String path){
  if (SD.exists(path)){
      source->close();
      if ((decoder) && (decoder->isRunning())){
        decoder->stop();
      }
      File file = SD.open(path);
      if (source->open(file.name())){
        Serial.printf_P(PSTR("Playing '%s' from SD card...\n"), file.name());
        id3 = new AudioFileSourceID3(source);
        decoder->begin(id3, output);
        return true;
      }
      else {
          Serial.printf_P(PSTR("Error opening '%s'\n"), file.name());
          return false;
      }
    }
    else {
      Serial.printf_P(PSTR("No file '%s' found on SD card...\n"), path);
      return false;
    }
  return false;
}
    

void setup() {
   
  //TODO: this should be one of the i2C buttons on the phone and not a pin on the ESP
  //pinMode(BUTTON_PIN, INPUT);  
  //If reset button remains pushed, reset the WiFi
  //if (digitalRead(BUTTON_PIN) == HIGH){
    //ResetWifiRoutine();
  //}

  Serial.begin(74880); //Same as ESP8266 bootloader
  delay(1000);
  Serial.println("Serial started");

  audioLogger = &Serial;  
  source = new AudioFileSourceSD();
  output = new AudioOutputI2S();
  decoder = new AudioGeneratorMP3();
 
  // NOTE: SD.begin(...) should be called AFTER AudioOutput...()
  //       to takover the the SPI pins if they share some with I2S
  //       (i.e. D8 on Wemos D1 mini is both I2S BCK and SPI SS)
  SD.begin(SPI_CS_PIN, SPI_SPEED);
  dir = SD.open("/");

  Serial.println("Connecting to WiFi");
  wiFiConnectStartMillis = millis();
  ConnectToWifi();

    // NOTE: PCF8574 will generate an interrupt on key press and release.
  pinMode(D3, INPUT_PULLUP);
  attachInterrupt(D3, keyChanged, FALLING);
  keyChange = false;
  Wire.setClock(400000);
  Wire.begin();
  keyPad.loadKeyMap(keys);
  keyPad.setLatestCharsDepth(20);
  keyPad.setDebounce(250);
  if (keyPad.begin() == false)
  {
    Serial.println("\nERROR: cannot communicate to keypad.\nRebooting.\n");
    delay(5000);
    //ESP.restart();
  }
}


bool rtcSyncStarted = false;
uint8_t numberLength = 0;
void loop() {
  
  //wifi and rtc management
  if(!rtcSyncStarted && !rtc_synced && WiFi.status()==WL_CONNECTED){
    Serial.println("WiFi Connected, synchronising time");
    rtcSyncStarted = true;
    SetupTime();
  }
  if(rtc_synced && WiFi.status()==WL_CONNECTION_LOST){
    rtc_synced=false;
  }

  //keypad management
  if (keyChange)
  {
    uint8_t index = keyPad.readKey();
    keyChange = false;
    if (index < 16)
    {
      String path = "/s.mp3";
      path[1]=keyPad.getChar();
      if(index == 12 || index == 15){  //star or pound clears it for now, should become horn down
        keyPad.clearLatestChars();
      } 
      samplePlaying=false;
      playMP3FromPath(path);
    }
  }
  if(keyPad.isPressed() && keyPad.getPressLengthMillis() > LONGPRESS_TIME_SECONDS*1000){
    //perform special reset-functions on longpresses
    Serial.println("Longpress detected");
    if(keyPad.getChar()==WIFI_RESET_KEY){
      Serial.println("Resetting WiFi network");
      ResetWifiRoutine();
    }
  }
  
  //decode management
  if ((decoder) && (decoder->isRunning()))
  {
    if (!decoder->loop()) decoder->stop();
  }
  else {
    if(!samplePlaying && keyPad.getLatestCharsLength() > 2 && keyPad.getLatestCharsLength() > numberLength){
      numberLength = keyPad.getLatestCharsLength();
      String samplePath = "/" + keyPad.getLatestChars() + ".mp3";
      if(SD.exists(samplePath)){
        samplePlaying=true;
        //TODO: resetting the state should be a separate function
        keyPad.clearLatestChars();
        numberLength = 0;
        playMP3FromPath(samplePath);
      }
    }
  }
}





