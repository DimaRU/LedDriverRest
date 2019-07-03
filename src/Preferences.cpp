////
///  Preferences.c
//

#include "FS.h"
#include "SPIFFS.h"
#include "esp_system.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "apps/sntp/sntp.h"
#include <SimpleTimer.h>

#include "LedDriver.h"

struct Preferences {
    uint32_t magic;
    int rebootCounter;
    int yellowLevel;
    int whiteLevel;
    enum LedsPowerState powerState;
    int alarmYellowLevel;
    int alramWhiteLevel;
    int riseTime;
    int alarmHour;
    int alarmMinute;
    bool alarmEnabled;
    long gmtOffsetSec;
};

static struct Preferences currentPrefs;
static struct Preferences savedPrefs;

static void writePrefs() {
    File file = SPIFFS.open(PrefsFilePath, FILE_WRITE);
    if (!file) {
        Serial.println("- failed to open preferences file for writing");
        return;
    }
    
    if ( file.write((uint8_t *)&currentPrefs, sizeof(currentPrefs)) ) {
        Serial.println("Preferences written");
    } else {
        Serial.println("Preferences write failed");
    }
    file.close();
}

void savePrefs() {
    currentPrefs.rebootCounter = rebootCounter;
    currentPrefs.whiteLevel = whiteLevel;
    currentPrefs.yellowLevel = yellowLevel;
    currentPrefs.powerState = powerState;

    currentPrefs.alarmYellowLevel = alarmYellowLevel;
    currentPrefs.alramWhiteLevel = alramWhiteLevel;
    currentPrefs.riseTime = riseTime;
    currentPrefs.alarmHour = alarmHour;
    currentPrefs.alarmMinute = alarmMinute;
    currentPrefs.alarmEnabled = alarmEnabled;
    currentPrefs.gmtOffsetSec = gmtOffsetSec;

    if (memcmp(&currentPrefs, &savedPrefs, sizeof(currentPrefs)) == 0) return;
    writePrefs();
    memcpy(&savedPrefs, &currentPrefs, sizeof(currentPrefs));
    savedPrefs = currentPrefs;
}


void restorePrefs() {

    currentPrefs.magic = PrefsMagic;

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    Serial.println("Reading preferences");

    File file = SPIFFS.open(PrefsFilePath);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open preferences file for reading");
        return;
    }

    if ( !file.read((uint8_t *)&savedPrefs, sizeof(savedPrefs)) ) {
        Serial.println("- read failed");
    }

    if (savedPrefs.magic == PrefsMagic) {
        rebootCounter = savedPrefs.rebootCounter + 1;
        powerState = savedPrefs.powerState;
        yellowLevel = savedPrefs.yellowLevel;
        whiteLevel = savedPrefs.whiteLevel;

        alarmYellowLevel = savedPrefs.alarmYellowLevel;
        alramWhiteLevel = savedPrefs.alramWhiteLevel;
        riseTime = savedPrefs.riseTime;
        alarmHour = savedPrefs.alarmHour;
        alarmMinute = savedPrefs.alarmMinute;
        alarmEnabled = savedPrefs.alarmEnabled;
        gmtOffsetSec = savedPrefs.gmtOffsetSec;
    } else {
        Serial.println("Preferences set init values");
        rebootCounter = 0;
        yellowLevel = SLIDER_MAXVALUE /2;
        whiteLevel = SLIDER_MAXVALUE /2;
        powerState = On;

        alarmYellowLevel = SLIDER_MAXVALUE/4;
        alramWhiteLevel = SLIDER_MAXVALUE/4;
        riseTime = 15;
        alarmHour = 7;
        alarmMinute = 0;
        alarmEnabled = false;
        gmtOffsetSec = 10800L;
    }
    nightLedOn(false);
    switch(powerState) {
        case On:
            ledsRestore();
            break;
        case Off:
            ledsOff();
            break;
        case Night:
            nightLedOn(true);
            break;
    }
}
