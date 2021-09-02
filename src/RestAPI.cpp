////
///  RestAPI.cpp
//

#include <Arduino.h>
#include <AsyncTCP.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <SimpleTimer.h>
#include "mdns.h"

#include "LedDriver.h"

int yellowLevel = SLIDER_MAXVALUE /2;
int whiteLevel = SLIDER_MAXVALUE /2;
enum LedsPowerState powerState = On;

ArRequestHandlerFunction getLed(AsyncWebServerRequest *request);
ArJsonRequestHandlerFunction postLed(AsyncWebServerRequest *request, JsonVariant &root);
ArJsonRequestHandlerFunction postLedOffet(AsyncWebServerRequest *request, JsonVariant &root);
static void newLedValues(int yellowLevel, int whiteLevel, enum LedsPowerState powerState);
ArRequestHandlerFunction getAlarm(AsyncWebServerRequest *request);
ArJsonRequestHandlerFunction postAlarm(AsyncWebServerRequest *request, JsonVariant &root);
ArRequestHandlerFunction getParams(AsyncWebServerRequest *request);
ArJsonRequestHandlerFunction postParams(AsyncWebServerRequest *request, JsonVariant &root);
void restAPISetup(AsyncWebServer *server);

static AsyncWebServer *server = NULL;
void start_webserver(void)
{
    if (server == NULL) {
        server = new AsyncWebServer(REST_API_PORT);
        restAPISetup(server);
    }
}

void stop_webserver(void)
{
    if (server != NULL) {
        delete server;
        server = NULL;
    }
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404);
}

void restAPISetup(AsyncWebServer *server) {
    server->on("/api/led", HTTP_GET, &getLed);
    server->on("/api/alarm", HTTP_GET, &getAlarm);
    server->on("/api/params", HTTP_GET, &getParams);

    AsyncCallbackJsonWebHandler* handler;
    handler = new AsyncCallbackJsonWebHandler("/api/led", &postLed);
    handler->setMethod(HTTP_POST);
    server->addHandler(handler);

    handler = new AsyncCallbackJsonWebHandler("/api/ledoffset", &postLedOffet);
    handler->setMethod(HTTP_POST);
    server->addHandler(handler);

    handler = new AsyncCallbackJsonWebHandler("/api/alarm", &postAlarm);
    handler->setMethod(HTTP_POST);
    server->addHandler(handler);

    handler = new AsyncCallbackJsonWebHandler("/api/params", &postParams);
    handler->setMethod(HTTP_POST);
    server->addHandler(handler);

    server->onNotFound(notFound);
    server->begin();
}

ArRequestHandlerFunction getParams(AsyncWebServerRequest *request) {
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonVariant& root = response->getRoot();

    root["slider_min"] = SLIDER_MINVALUE;
    root["slider_max"] = SLIDER_MAXVALUE;
    root["location"] = locationName;
    response->setLength();
    request->send(response);
    return 0;
}

ArJsonRequestHandlerFunction postParams(AsyncWebServerRequest *request, JsonVariant &root) {
    String location = root["location"];
    location.toCharArray(locationName, sizeof(locationName));
    mdns_service_txt_item_set("_led", "_tcp", "location", locationName);

    request->send(204);
    return 0;
}

ArRequestHandlerFunction getLed(AsyncWebServerRequest *request) {
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonVariant& root = response->getRoot();
    
    const char* powerStateString = "on";
    switch (powerState)
    {
    case On:
        powerStateString = "on";
        break;
    case Off:
        powerStateString = "off";
        break;
    case Night:
        powerStateString = "night";
        break;
    }
    root["yellow_level"] = yellowLevel;
    root["white_level"] = whiteLevel;
    root["power_state"] = powerStateString;
    response->setLength();
    request->send(response);
    return 0;
}

ArJsonRequestHandlerFunction postLed(AsyncWebServerRequest *request, JsonVariant &root) {
    enum LedsPowerState powerState = On;

    String powerStateString = root["power_state"];
    if (powerStateString == "on") {
        powerState = On;
    } else if (powerStateString == "off") {
        powerState = Off;
    } else if (powerStateString == "night") {
        powerState = Night;
    } else {
        request->send(400);
    }

    int yellowLevel = root["yellow_level"];
    int whiteLevel = root["white_level"];
    newLedValues(yellowLevel, whiteLevel, powerState);

    request->send(204);
    return 0;
}

ArJsonRequestHandlerFunction postLedOffet(AsyncWebServerRequest *request, JsonVariant &root) {
    enum LedsPowerState powerState = On;

    String powerStateString = root["power_state"];
    if (powerStateString == "on") {
        powerState = On;
    } else if (powerStateString == "off") {
        powerState = Off;
    } else if (powerStateString == "night") {
        powerState = Night;
    } else {
        request->send(400);
    }

    int yellowOffset = root["yellow_offset"];
    int whiteOffset = root["white_offset"];
    int newYellowLevel = yellowLevel + yellowOffset;
    int newWhiteLevel = whiteLevel + whiteOffset;

    if (newYellowLevel < 0) {
        newYellowLevel = 0;
    }
    if (newYellowLevel > SLIDER_MAXVALUE) {
        newYellowLevel = SLIDER_MAXVALUE;
    }
    if (newWhiteLevel < 0) {
        newWhiteLevel = 0;
    }
    if (newWhiteLevel > SLIDER_MAXVALUE) {
        newWhiteLevel = SLIDER_MAXVALUE;
    }

    newLedValues(newYellowLevel, newWhiteLevel, powerState);

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonVariant& responceRoot = response->getRoot();
    
    const char* powerStatePtr;
    switch (powerState)
    {
    case On:
        powerStatePtr = "on";
        break;
    case Off:
        powerStatePtr = "off";
        break;
    case Night:
        powerStatePtr = "night";
        break;
    }
    responceRoot["yellow_level"] = yellowLevel;
    responceRoot["white_level"] = whiteLevel;
    responceRoot["power_state"] = powerStatePtr;
    response->setLength();
    request->send(response);

//    request->send(204);
    return 0;
}

void ledsOff() {
  setBrightness(0, YellowChannel);
  setBrightness(0, WhiteChannel);
}

void ledsRestore() {
  setBrightness(yellowLevel, YellowChannel);
  setBrightness(whiteLevel, WhiteChannel);
}

static void newLedValues(int yellowLevel, int whiteLevel, enum LedsPowerState powerState) {
    Serial.printf("Set: %d %d %d\n", (int)powerState, yellowLevel, whiteLevel);

    if (powerState != ::powerState) {
        ::powerState = powerState;
        switch (powerState) {
        case On:
            nightLedOn(false);
            ledsRestore();
            break;
        case Off:
            nightLedOn(false);
            ledsOff();
            cancelAlarm();
            break;
        case Night:
            nightLedOn(true);
            ledsOff();
            cancelAlarm();
            break;
        }
    }

    if (yellowLevel != ::yellowLevel) {
        ::yellowLevel = yellowLevel;
        if (powerState == On) {
            setBrightness(yellowLevel, YellowChannel);
            cancelAlarm();
        }
    }

    if (whiteLevel != ::whiteLevel) {
        ::whiteLevel = whiteLevel;
        if (powerState == On) {
            setBrightness(whiteLevel, WhiteChannel);
            cancelAlarm();
        }
    }
}

ArRequestHandlerFunction getAlarm(AsyncWebServerRequest *request) {
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonVariant& root = response->getRoot();

    root["yellow_level"] = alarmYellowLevel;
    root["white_level"] = alramWhiteLevel;
    root["rise_time"] = riseTime;
    root["hour"] = alarmHour;
    root["minute"] = alarmMinute;
    root["gmt_offset_sec"] = gmtOffsetSec;
    root["enabled"] = alarmEnabled;

    response->setLength();
    request->send(response);
    return 0;
}

ArJsonRequestHandlerFunction postAlarm(AsyncWebServerRequest *request, JsonVariant &root) {
    alarmYellowLevel = root["yellow_level"];
    alramWhiteLevel = root["white_level"];
    riseTime = root["rise_time"];
    alarmHour = root["hour"];
    alarmMinute = root["minute"];

    int offset = root["gmt_offset_sec"];
    if (offset != gmtOffsetSec) {
        gmtOffsetSec = offset;
        configTime(gmtOffsetSec, 0, "pool.ntp.org", NULL, NULL);
    }

    alarmEnabled = root["enabled"];
    if (!alarmEnabled) {
        cancelAlarm();
    }

    request->send(204);
    return 0;
}
