#ifndef LFSOPERATIONS_H_
#define LFSOPERATIONS_H_

#include <LittleFS.h>
#include "datatypes.h"

void initFS();
void StoreSettings(WiFiSecrets* settings);
WiFiSecrets RecoverWiFiSecrets();
void deleteAllFiles();

#endif