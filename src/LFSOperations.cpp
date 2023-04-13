#include "LFSOperations.h"
static bool FSReady=false;

void initFS(){
  if(!FSReady){
    //Serial.print(F("Inizializing FS..."));  
    if (LittleFS.begin()){
      //Serial.println(F("done."));
    }else{
      //Serial.println(F("fail."));
    }
    FSReady = true;
  }  
}

void StoreSettings(WiFiSecrets* settings){
  initFS();
  File secrets = LittleFS.open(F("/WiFiSecrets.txt"), "w");
  //Serial.println("Saving the following info to Flash:");
  //Serial.println(settings[0].SSID);
  //Serial.println(settings[0].Pass);
  String toSave = settings[0].SSID + '\0' + settings[0].Pass + '\0';
  //Serial.println(toSave);
  //Serial.print("Size: ");
  //Serial.println(toSave.length());
  for(uint i=0; i < toSave.length(); i++){
    //Serial.println(toSave[i]);
    secrets.write(toSave[i]);
  }
  secrets.close();
}

WiFiSecrets RecoverWiFiSecrets(){
  initFS();
  WiFiSecrets secrets;
  //Serial.println("Attempting recovery of Wifi Settings");

  File Appinfo = LittleFS.open(F("/WiFiSecrets.txt"), "r");
  char data[50];
  for (uint i =0; i<50; i++){
    data[i] = Appinfo.read();
    //Serial.println(data[i]);
    if(data[i] == '\0') break;
  }
  //Serial.println(data);
  secrets.SSID = data;

  for (uint i =0; i<50; i++){
    data[i] = Appinfo.read();
    //Serial.println(data[i]);
    if(data[i] == '\0') break;
  }
  //Serial.println(data);
  secrets.Pass = data;
  
  Appinfo.close();
  //Serial.println("Secrets recovered : ");
  //Serial.println(secrets.SSID);
  //Serial.println(secrets.Pass);

  return secrets;
}

void deleteAllFiles(){
  initFS();
  LittleFS.format();
}

