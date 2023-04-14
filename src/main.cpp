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
#include "I2CKeyPad.h"

#define RESET_KEY '*' //button to reset microcontroller to reset WiFiManger
#define RESET_KEY_TIME_SECONDS 5 //how long the reset button needs to be pushed in order for the reset routine to be triggered



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




//******************************************************************
// global variables
//******************************************************************
//Variables for WiFi
WiFiSecrets wiFiSecrets;
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
I2CKeyPad keyPad(KEYPAD_ADDRESS);
char keys[] = "1A23798B465CsD0#NF";  // N = NoKey, F = Fail (e.g. >1 keys pressed)
String lastkeys = "NNNNNNNNNN"; //no keys yet pressed
volatile bool keyChange = false; // for interrupt in case of a keychange
bool samplePlayed = false;




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
    RunWiFiManager();
  }
  wiFiSecrets = RecoverWiFiSecrets();
  WiFi.begin(wiFiSecrets.SSID, wiFiSecrets.Pass);
  uint counter = millis();
  while (WiFi.status() != WL_CONNECTED) {
    //do something usefull while connecting ;)
    //TODO : can there be more useful stuff here instead of just waiting?
    yield();
    delay(100);
    //Serial.print('.');
  }
  if(millis()>counter+10000){ //If WiFi not connected after 10 seconds, stop waiting for connection
    //TODO: audio feedback that wifi is not connected
    
  }
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
    const int NumberOfSeconds = 3;
    //Serial.print("Resetting WiFi credentials");
    while(digitalRead(RESET_KEY) == HIGH && millis()<StartTime+NumberOfSeconds*1000UL){
        //Serial.print('.');
        yield();
        delay(100);
    }
    //Serial.println();
    if (millis()>StartTime+NumberOfSeconds*1000UL){
        deleteAllFiles();
        RunWiFiManager();
    }
}

void SetupTime() {
  if(!rtc_synced){
    configTime(TIMEZONE, "be.pool.ntp.org");
    settimeofday_cb(NTP_Sync_Callback);
  }
  else{
    //Serial.println("time was already synced, using RTC");
  }
  return;
}


void WaitforNTPCallbak(){
  uint timenow = millis();
  while(!rtc_synced && millis()<timenow+5000){ //wait for 5 seconds more for the time to come in...
    //Serial.println("Waiting for timeserver");
    yield();
    delay(500);
  }
  if(!rtc_synced){
    //Not able to set up time correctly, go to sleep and try again 3 hours;
    ESP.deepSleep(ESP.deepSleepMax());
  }
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
      Serial.printf_P(PSTR("Playing '%s' from SD card...\n"), path);
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
  StartTime = millis();
  //Serial.begin(74880); //Same as ESP8266 bootloader
  
  //TODO: this should be one of the i2C buttons on the phone and not a pin on the ESP
  //pinMode(BUTTON_PIN, INPUT);  
  //If reset button remains pushed, reset the WiFi
  //if (digitalRead(BUTTON_PIN) == HIGH){
    //ResetWifiRoutine();
  //}

  Serial.begin(115200);
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

  Serial.println("Connect to WiFi");
  ConnectToWifi();
  //Serial.println("Calling SetupTime");
  SetupTime();
  if (!rtc_synced){
    WaitforNTPCallbak();
  }
    // NOTE: PCF8574 will generate an interrupt on key press and release.
  pinMode(D3, INPUT_PULLUP);
  attachInterrupt(D3, keyChanged, FALLING);
  keyChange = false;
  Wire.setClock(400000);
  Wire.begin();
  if (keyPad.begin() == false)
  {
    Serial.println("\nERROR: cannot communicate to keypad.\nPlease reboot.\n");
    while (1);
  }
}


void loop() {
  if (keyChange)
  {
    uint8_t index = keyPad.getKey();
    // only after keyChange is handled it is time reset the flag
    keyChange = false;
    if (index != 16)
    {
      Serial.print("press: ");
      Serial.println(keys[index]);
      String path = "/f.mp3";
      path[1]=keys[index];
      lastkeys = lastkeys.substring(1,9)+keys[index];
      samplePlayed=false;
      Serial.println(lastkeys);
      playMP3FromPath(path);
    }
    else
    {
      Serial.println("release");
    }
  }

  if ((decoder) && (decoder->isRunning()))
  {
    if (!decoder->loop()) decoder->stop();
  }
  else {
    if(!samplePlayed && (lastkeys.substring(3,9) == "797204")){
      playMP3FromPath("/797204.mp3");
      samplePlayed=true;
    }
  }

  // else {
    // File file = dir.openNextFile();
    // if (file) {      
    //   if (String(file.name()).endsWith(".mp3")) {
    //     source->close();
    //     if (source->open(file.name())) {
    //       Serial.printf_P(PSTR("Playing '%s' from SD card...\n"), file.name());
    //       id3 = new AudioFileSourceID3(source);
    //       decoder->begin(id3, output);
    //     } else {
    //       Serial.printf_P(PSTR("Error opening '%s'\n"), file.name());
    //     }
    //   }
    // }
    // else {
    //   dir=SD.open("/");
    // }     
  // }
}





