#include "WiFiManagerApp.h"

WiFiManagerApp::WiFiManagerApp(){
  wm.setDebugOutput(false);
}

bool WiFiManagerApp::Run(){
    //Serial.println("Resetting WiFi config");
    wm.resetSettings();
    std::vector<const char *> menu = {"wifi","info","param","exit"};
    wm.setMenu(menu);
    //Serial.println("inverting");
    // set dark theme
    wm.setClass("invert");
    //set static ip
    // wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
    // wm.setShowStaticFields(true); // force show static ip fields
    // wm.setShowDnsFields(true);    // force show dns field always
    // wm.setConnectTimeout(20); // how long to try to connect for before continuing
    //Serial.println("setting config Portal Timeout");
    wm.setConfigPortalTimeout(150); // auto close configportal after n seconds
    // wm.setCaptivePortalEnable(false); // disable captive portal redirection
    // wm.setAPClientCheck(true); // avoid timeout if client connected to softap
    // wifi scan settings
    // wm.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
    // wm.setMinimumSignalQuality(20);  // set min RSSI (percentage) to show in scans, null = 8%
    // wm.setShowInfoErase(false);      // do not show erase button on info page
    // wm.setScanDispPerc(true);       // show RSSI as percentage not graph icons
    // wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fail
    bool res;
    
    // res = wm.autoConnect(); // auto generated AP name from chipid
    //Serial.println("Calling autoConnect");
    res = wm.autoConnect("Ledafoon"); // anonymous ap
    // res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
    

    if(!res) {
        return false;
        //Serial.println("Failed to connect or hit timeout, going into deepsleep and try again in a few hours...");
    } 
    else {
        //if you get here you have connected to the WiFi    
        //Serial.println("connected...yeey :), restarting the app"); //If i don't restart there is not enough memory in the heap to run the apps., Wifimanager doesn't clean up...
        WiFiSecrets wiFiSecrets;
        wiFiSecrets.SSID = wm.getWiFiSSID();
        wiFiSecrets.Pass = wm.getWiFiPass();
        StoreSettings(&wiFiSecrets);
        return true;
    }
}

void WiFiManagerApp::AddParameters(){
    //custom parameters input through WiFiManager 
    //Serial.println("creating new sections");
    goldAPI_selection = new WiFiManagerParameter(checkboxGoldAPI_str);
    goldAPI_hours = new WiFiManagerParameter(goldAPI_hours_str);
    goldAPI_minutes = new WiFiManagerParameter(goldAPI_minutes_str);
    weatherAPI_selection = new WiFiManagerParameter(checkboxWeatherAPI_str);
    weatherAPI_hours = new WiFiManagerParameter(weatherAPI_hours_str);
    weatherAPI_minutes = new WiFiManagerParameter(weatherAPI_minutes_str);
    logo_selection = new WiFiManagerParameter(checkboxLogo_str);
    logo_hours = new WiFiManagerParameter(logo_hours_str);
    logo_minutes = new WiFiManagerParameter(logo_minutes_str);
    Gold_API_key_inputfield = new WiFiManagerParameter("GOLD_API_key_inputfield", "", "", 30, "placeholder='GoldAPI.IO API key'");
    Gold_cert_thumbprint_inputfield = new WiFiManagerParameter("cert_thumbprint_inputfield", "", "", 40, "placeholder='GoldAPI.IO Certificate Thumbprint'");
    Weather_API_key_inputfield = new WiFiManagerParameter("Weather_API_key_inputfield", "", "", 30, "placeholder='GoldAPI.IO API key'");

    //Serial.println("Adding paramaters to wifimanager");
    wm.addParameter(goldAPI_selection);
    wm.addParameter(goldAPI_hours);
    wm.addParameter(goldAPI_minutes);
    wm.addParameter(weatherAPI_selection);
    wm.addParameter(weatherAPI_hours);
    wm.addParameter(weatherAPI_minutes);
    wm.addParameter(logo_selection);
    wm.addParameter(logo_hours);
    wm.addParameter(logo_minutes);
    wm.addParameter(Gold_API_key_inputfield);
    wm.addParameter(Gold_cert_thumbprint_inputfield);
    wm.addParameter(Weather_API_key_inputfield);
    
    //Serial.println("Setting Callback Function");
    wm.setSaveParamsCallback(std::bind(&WiFiManagerApp::saveParamCallback, this));
    //Serial.println("Callback function set");
}

void WiFiManagerApp::saveParamCallback(){
  //Serial.println("[CALLBACK] saveParamCallback fired");
  //Careful : checkbox returns Null if not selected and the value of above defined when selected.
  //Serial.print("Submitted checkbox = ");
  if(wm.server->hasArg("choice1")) {
    //Serial.println("GOLD API SELECTED");
    //programSettings.GoldActive = true;
  }
  else{
    //programSettings.GoldActive = false;
  }
  if(wm.server->hasArg("choice2")) {
    //Serial.println("WEATHER API SELECTED");
    //programSettings.WeatherActive = true;
  }
  else{
    //programSettings.WeatherActive = false;
  }
  if(wm.server->hasArg("choice3")) {
    //Serial.println("LOGO SELECTED");
    //programSettings.LogoActive = true;
  }
  else{
    //programSettings.LogoActive = false;
  }
  if(wm.server->hasArg("goldAPI_hours")) {
    //Serial.print("goldAPI_hours:");
    //programSettings.GoldHour = strtol(wm.server->arg("goldAPI_hours").c_str(),NULL,10)*60;
    //Serial.println(programSettings.GoldHour);
  }
  if(wm.server->hasArg("goldAPI_minutes")) {
    //Serial.print("goldAPI_minutes:");
    //programSettings.GoldHour += strtol(wm.server->arg("goldAPI_minutes").c_str(),NULL,10);
    //Serial.println(programSettings.GoldHour);
  }
  if(wm.server->hasArg("weatherAPI_hours")) {
    //Serial.print("weatherAPI_hours:");
    //programSettings.WeatherHour = strtol(wm.server->arg("weatherAPI_hours").c_str(),NULL,10)*60;
    //Serial.println(programSettings.WeatherHour);
  }
  if(wm.server->hasArg("weatherAPI_minutes")) {
    //Serial.print("weatherAPI_minutes:");
    //programSettings.WeatherHour += strtol(wm.server->arg("weatherAPI_minutes").c_str(),NULL,10);
    //Serial.println(programSettings.WeatherHour);
  }
  if(wm.server->hasArg("logo_hours")) {
    //Serial.print("logo_hours:");
    //programSettings.LogoHour = strtol(wm.server->arg("logo_hours").c_str(),NULL,10)*60;
    //Serial.println(programSettings.LogoHour);
  }
  if(wm.server->hasArg("logo_minutes")) {
    //Serial.print("logo_minutes:");
    //programSettings.LogoHour += strtol(wm.server->arg("logo_minutes").c_str(),NULL,10);
    //Serial.println(programSettings.LogoHour);
  }
  
  const char* c = Gold_API_key_inputfield->getValue();
  if ((c[0] != '\0')) {
    //Serial.print("Setting Gold API KEY to: ");
    //goldAppSettings.gold_api_key = strdup(c);
    //Serial.println(goldAppSettings.gold_api_key);
  }

  c = Gold_cert_thumbprint_inputfield->getValue();
  if ((c[0] != '\0')) {
    //Serial.print("found the following thumbprint: ");
    //Serial.println(c);
    //Conversion of string to fingerprint
    //Serial.println("Processing thumbprint");

    char hexstring[3] = {'0','0','\0'};
    uint8_t fingerprint[20];
    for (int i=0;i<20;i++){
      int position = i*2;
      hexstring[0]=c[position];
      hexstring[1]=c[position+1];
      hexstring[2] = '\0';
      fingerprint[i] = (uint8_t)strtol(hexstring, NULL, 16);
    }
    //memcpy(goldAppSettings.fingerprint, fingerprint, sizeof(uint8_t)*20);
    //Serial.print("Set GoldAPI fingerprint to: ");
    for (int i=0;i<20;i++){
      //Serial.print(goldAppSettings.fingerprint[i]);
    }
    //Serial.println();
  }
  
  //c = Weather_API_key_inputfield->getValue();
  if ((c[0] != '\0')) {
    //Serial.print("Setting Gold API KEY to: ");
    //weatherAppSettings.weather_api_key = strdup(c);
    //Serial.println(weatherAppSettings.weather_api_key);
  }


  //Serial.println("Backing up Program Settings To RTC RAM");
  //BackupStateToRTCRAM(programSettings);
  //Serial.println("Backing up Program Settings To Flash");
  //StoreSettings(programSettings);
  //Serial.println("Backing up Weather App Settings To Flash");
  //StoreSettings(weatherAppSettings);
  //Serial.println("Backing up Gold App Settings To Flash");
  //StoreSettings(goldAppSettings);
  //Serial.println("Backed up all settings, returning");
  return;
}

