#ifndef FSOPERATIONS_H_
#define FSOPERATIONS_H_

#include <SD.h>
#include "datatypes.h"

void StoreSettings(WiFiSecrets* settings);
WiFiSecrets RecoverWiFiSecrets();

#endif