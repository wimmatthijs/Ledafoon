#include "FSOperations.h"


void StoreSettings(WiFiSecrets* settings){
  File secrets = SD.open("/WiFiSecrets.txt", "w");
  String toSave = settings[0].SSID + '\0' + settings[0].Pass + '\0';
  for(uint i=0; i < toSave.length(); i++){
    secrets.write(toSave[i]);
  }
  secrets.close();
}

WiFiSecrets RecoverWiFiSecrets(){
  WiFiSecrets secrets;
  File secretsFile = SD.open("/WiFiSecrets.txt", "r");
  char data[50];
  for (uint i =0; i<50; i++){
    data[i] = secretsFile.read();
    if(data[i] == '\0') break;
  }
  secrets.SSID = data;
  for (uint i =0; i<50; i++){
    data[i] = secretsFile.read();
    if(data[i] == '\0') break;
  }
  secrets.Pass = data;
  secretsFile.close();
  return secrets;
}

