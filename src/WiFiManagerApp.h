#ifndef WIFIMANAGER_APP_H
#define WIFIMANAGER_APP_H

#include <Arduino.h>
#include <WiFiManager.h>
#include "datatypes.h"
#include "LFSOperations.h"
#include "RTCRAM.h"

class WiFiManagerApp{

        private:
        WiFiManager wm;
        const char* checkboxGoldAPI_str =
        "<p>Please select e-paper mode and refresh time (h:m) </p>"
        "<input style='width: auto; margin: 0 10px 10px 10px;' type='checkbox' id='choice1' name='choice1' value='GoldAPI'>"
        "<label for='choice1'>Gold API</label>";
        const char* checkboxWeatherAPI_str =
        "<input style='width: auto; margin: 0 10px 10px 10px;' type='checkbox' id='choice2' name='choice2' value='WeatherAPI'>"
        "<label for='choice2'>Weather API</label>";
        const char* checkboxLogo_str =
        "<input style='width: auto; margin: 0 10px 10px 10px;' type='checkbox' id='choice3' name='choice3' value='Logo'>"
        "<label for='choice3'>Logo</label>";
        const char* goldAPI_hours_str = 
        "<input placeholder='8' style='width: 3em; margin: 0 10px 10px 10px;' size='2' maxlength='2' type='number' id='goldAPI_hours' name='goldAPI_hours' min='0' max='23'>:";
        const char* goldAPI_minutes_str = 
        "<input placeholder='30' style='width: 3em; margin: 0 10px 10px 10px;' size='2' maxlength='2' type='number' id='goldAPI_minutes' name='goldAPI_minutes' min='0' max='59'><br>";
        const char* weatherAPI_hours_str = 
        "<input placeholder='12' style='width: 3em; margin: 0 10px 10px 10px;' size='2' maxlength='2' type='number' id='weatherAPI_hours' name='weatherAPI_hours' min='0' max='23'>:";
        const char* weatherAPI_minutes_str = 
        "<input placeholder='0' style='width: 3em; margin: 0 10px 10px 10px;' size='2' maxlength='2' type='number' id='weatherAPI_minutes' name='weatherAPI_minutes' min='0' max='59'><br>";
        const char* logo_hours_str = 
        "<input placeholder='16' style='width: 3em; margin: 0 10px 10px 10px;' size='2' maxlength='2' type='number' id='logo_hours' name='logo_hours' min='0' max='23'>:";
        const char* logo_minutes_str = 
        "<input placeholder='30' style='width: 3em; margin: 0 10px 10px 10px;' size='2' maxlength='2' type='number' id='logo_minutes' name='logo_minutes' min='0' max='59'><br>";
        
        WiFiManagerParameter* goldAPI_selection;
        WiFiManagerParameter* weatherAPI_selection;
        WiFiManagerParameter* logo_selection;
        WiFiManagerParameter* goldAPI_hours;
        WiFiManagerParameter* goldAPI_minutes;
        WiFiManagerParameter* weatherAPI_hours;
        WiFiManagerParameter* weatherAPI_minutes;
        WiFiManagerParameter* logo_hours;
        WiFiManagerParameter* logo_minutes;
        WiFiManagerParameter* Gold_API_key_inputfield;
        WiFiManagerParameter* Gold_cert_thumbprint_inputfield;
        WiFiManagerParameter* Weather_API_key_inputfield;

        public:
        WiFiManagerApp();
        bool Run();

        private:
        void AddParameters();
        void saveParamCallback();
        void LoadAppSettings();
};
#endif