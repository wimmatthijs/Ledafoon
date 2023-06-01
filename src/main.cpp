#include <Arduino.h>
#include "AudioFileSourceSD.h"
#include "AudioOutputI2S.h"
#include "AudioGeneratorMP3.h"
#include "AudioFileSourceID3.h"
#include "datatypes.h"
#include "FSOperations.h"
#include "WiFiManager/defines.h"
#include "WiFiManager/Credentials.h"
#include "WiFiManager/dynamicParams.h"
#include <time.h>   //for doing time stuff
#include <TZ.h>      //timezones
#include "Wire.h"
#include "PhoneKeypad.h"
#include "PCF8574.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>


//******************************************************************
//Function Declatarions (for Platform IO compatibilty)
//******************************************************************
//Functions related to WiFi Manager
void ConnectToWifi(); //Does what it says, goes to deepsleep if unsuccessful
void RunWiFiManager(); //runs in case of hard reset (5 seconds) or first boot (no files in filesystem)
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
#define SPI_SPEED SD_SCK_MHZ(10)
#define SPI_CS_PIN D0

#define WIFI_RESET_KEY 's' //button to reset microcontroller to reset WiFiManger
#define OTA_KEY '#' //button to open OTA webserver on port 80
#define LONGPRESS_TIME_SECONDS 5 //how long the reset button needs to be pushed in order for the reset routine to be triggered



//******************************************************************
// global variables
//******************************************************************
//Variables for WiFi
ESPAsync_WiFiManager_Lite* ESPAsync_WiFiManager;
WiFiSecrets wiFiSecrets;
unsigned long wiFiConnectStartMillis;
//variables for elegantOTA
AsyncWebServer* server = NULL;
bool OTAServerStarted = false;
//timekeeping variables
volatile bool rtc_synced = false;   //keeps track if timesync already occured or not.
bool rtcSyncStarted = false;
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
char keys[] = "#AH30582R69Cs471NF@";  // N = NoKey, F = Fail (e.g. >1 keys pressed), @ = Bounced
volatile bool keyChange = false; // for interrupt in case of a keychange
bool hornDown = true;
bool samplePlaying = false;
//GPIO EXPANDER
const uint8_t GPIO_ADDRESS = 0x21;
PCF8574 pcf8574(GPIO_ADDRESS);
const uint8_t LEDPIN = 7;




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
  if (!SD.exists("/WiFiSecrets.txt")){
    //TODO: audio signal that wifi was not saved, run the wifimanager
    Serial.println("No WiFi credentials found, starting Ledafoon Accesspoint");
    RunWiFiManager();
  }
  wiFiSecrets = RecoverWiFiSecrets();
  WiFi.begin(wiFiSecrets.SSID, wiFiSecrets.Pass);
}

void heartBeatPrint()
{
  static int num = 1;

  if (WiFi.status() == WL_CONNECTED)
    Serial.print("H");        // H means connected to WiFi
  else
  {
    if (ESPAsync_WiFiManager->isConfigMode())
      Serial.print("C");        // C means in Config Mode
    else
      Serial.print("F");        // F means not connected to WiFi
  }

  if (num == 80)
  {
    Serial.println();
    num = 1;
  }
  else if (num++ % 10 == 0)
  {
    Serial.print(F(" "));
  }
}

void check_status()
{
  static unsigned long checkstatus_timeout = 0;

  //KH
#define HEARTBEAT_INTERVAL    20000L
  // Print hearbeat every HEARTBEAT_INTERVAL (20) seconds.
  if ((millis() > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    heartBeatPrint();
    checkstatus_timeout = millis() + HEARTBEAT_INTERVAL;
  }
}

#if USING_CUSTOMS_STYLE
const char NewCustomsStyle[] PROGMEM =
  "<style>div,input{padding:5px;font-size:1em;}input{width:95%;}body{text-align: center;}"\
  "button{background-color:blue;color:white;line-height:2.4rem;font-size:1.2rem;width:100%;}fieldset{border-radius:0.3rem;margin:0px;}</style>";
#endif



void RunWiFiManager(){
  //recover all current settings from the installed App's.
  Serial.print(F("\nStarting ESPAsync_WiFi using "));
  Serial.print(FS_Name);
  Serial.print(F(" on "));
  Serial.println(ARDUINO_BOARD);
  Serial.println(ESP_ASYNC_WIFI_MANAGER_LITE_VERSION);
#if USING_MRD
  Serial.println(ESP_MULTI_RESET_DETECTOR_VERSION);
#else
  Serial.println(ESP_DOUBLE_RESET_DETECTOR_VERSION);
#endif
  ESPAsync_WiFiManager = new ESPAsync_WiFiManager_Lite();
  String AP_SSID = "Ledafoon";
  String AP_PWD  = "Leda12345";
  // Set customized AP SSID and PWD
  ESPAsync_WiFiManager->setConfigPortal(AP_SSID, AP_PWD);

  // Optional to change default AP IP(192.168.4.1) and channel(10)
  //ESPAsync_WiFiManager->setConfigPortalIP(IPAddress(192, 168, 120, 1));
  ESPAsync_WiFiManager->setConfigPortalChannel(0);

#if USING_CUSTOMS_STYLE
  ESPAsync_WiFiManager->setCustomsStyle(NewCustomsStyle);
#endif

#if USING_CUSTOMS_HEAD_ELEMENT
  ESPAsync_WiFiManager->setCustomsHeadElement(PSTR("<style>html{filter: invert(10%);}</style>"));
#endif

#if USING_CORS_FEATURE
  ESPAsync_WiFiManager->setCORSHeader(PSTR("Your Access-Control-Allow-Origin"));
#endif

  // Set customized DHCP HostName
  ESPAsync_WiFiManager->begin(HOST_NAME);
  while (WiFi.status() != WL_CONNECTED)
  { 
    ESPAsync_WiFiManager->run();
    check_status();
    Serial.print('.');
    yield();
    delay(1000);
  }
  String SSID = ESPAsync_WiFiManager->getWiFiSSID(0);
  String PWD = ESPAsync_WiFiManager->getWiFiPW(0);
  Serial.print("SSID: ");
  Serial.println(SSID);
  wiFiSecrets.Pass = PWD;
  wiFiSecrets.SSID = SSID;
  StoreSettings(&wiFiSecrets);
  Serial.println("Wifi information stored, rebooting...\n");
  delay(5000);
  ESP.restart();
  return;
  
  //Or use default Hostname "ESP_XXXXXX"
  //ESPAsync_WiFiManager->begin();
  //WiFiManagerApp wiFiManagerApp;
  //if(wiFiManagerApp.Run()){
    //ESP.restart();
  //}
  //else{
    //TODO: put some audio feedback here that wifi manager app didnt finish successfullly
    ESP.deepSleep(UINT64_MAX,RF_DISABLED);
  //}
}

void ResetWifiRoutine(){
  if (SD.exists("/WiFiSecrets.txt")) SD.remove("/WiFiSecrets.txt");
  RunWiFiManager();
}

void SetupTime() {
  if(!rtc_synced){
    configTime(TIMEZONE, "be.pool.ntp.org");
    settimeofday_cb(NTP_Sync_Callback);
  }
  return;
}

void OTAUpdate(){
  server = new AsyncWebServer(80);
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is a sample response.");
  });
  AsyncElegantOTA.begin(server);    // Start AsyncElegantOTA
  server->begin();
  Serial.println("OTA server started");
  OTAServerStarted = true;
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

void resetState(){
  if(decoder && decoder->isRunning()){
    decoder->stop();
  }
  samplePlaying=false;
  keyPad.clearLatestChars();
  pcf8574.digitalWrite(LEDPIN, HIGH);
}
    

void setup() {
   
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
  if(!SD.begin(SPI_CS_PIN, SPI_SPEED)){
    Serial.println("Communication with SD card Failed");
    ESP.deepSleep(ESP.deepSleepMax());
    ESP.restart();
  }
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
    ESP.restart();
  }
  
   // Set pinMode to OUTPUT, ALL unused pins must be set to output (datasheet)
  pcf8574.pinMode(P0, INPUT);
  for(int i=1;i<8;i++) {
    pcf8574.pinMode(i, OUTPUT);
  }
	Serial.print("Init pcf8574...");
	if (pcf8574.begin()){
		Serial.println("OK");
	}else{
		Serial.println("NOK, rebooting.");
    delay(5000);
    ESP.restart();
	}
}

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
    //read the extra GPIO
    PCF8574::DigitalInput val = pcf8574.digitalReadAll();
    if (val.p0==HIGH){
      //Serial.println("Horn down");
      hornDown = true;
      resetState();
    }
    else{
      hornDown=false;
      pcf8574.digitalWrite(LEDPIN, LOW);
    }
    
    if(!hornDown && !samplePlaying){
      //read the keypad
      uint8_t index = keyPad.readKey();
      if (index < 16)
      {
        String path = "/s.mp3";
        path[1]=keyPad.getChar();
        samplePlaying=false;
        playMP3FromPath(path);
      }
      Serial.print(keyPad.getLatestCharsLength());
      Serial.print(": ");
      Serial.println(keyPad.getLatestChars());
    }
    keyChange = false;
  }
  if(keyPad.isPressed() && keyPad.getPressLengthMillis() > LONGPRESS_TIME_SECONDS*1000){
    //perform special reset-functions on longpresses
    Serial.println("Longpress detected");
    if(keyPad.getChar()==WIFI_RESET_KEY){
      Serial.println("Resetting WiFi network");
      ResetWifiRoutine();
    }
    if(keyPad.getChar()==OTA_KEY && !OTAServerStarted){
      Serial.println("Opening Server for OTA Update");
      OTAUpdate();
    }
  }
  
  //decode management
  if ((decoder) && (decoder->isRunning()))
  {
    pcf8574.digitalWrite(LEDPIN, LOW);
    if (!decoder->loop()){
      decoder->stop();
      samplePlaying=false;
      pcf8574.digitalWrite(LEDPIN, HIGH);
    }
  }
  else {
    if(keyPad.getLatestCharsLength()>2){
      String samplePath = "/" + keyPad.getLatestChars() + ".mp3";
      if(SD.exists(samplePath)){
        samplePlaying=true;
        playMP3FromPath(samplePath);
        keyPad.clearLatestChars();
      }
    }
  }
}





