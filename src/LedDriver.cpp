////
/// LedDriver - main file.
//

#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>
#include "esp_system.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include <SimpleTimer.h>
#include "mdns.h"

#include "LedDriver.h"
#include "Credientals.h"

int rebootCounter = 0;
char locationName[100];
SimpleTimer timer;

static void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
static void initialise_mdns(void);

void setup()
{
  // Debug console
  Serial.begin(115200);

  ledSetup();
  restorePrefs();

  WiFi.mode(WIFI_STA);
  WiFi.onEvent(WiFiEvent);
  WiFi.begin(ssid, password);
  
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);
  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(ModuleHostname);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  initialize_alaram();
  timer.setInterval(PrefsSaveTimeout, savePrefs);
}

void loop()
{
  timer.run();
  ArduinoOTA.handle();
}

static void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
  switch (event)
  {
  case SYSTEM_EVENT_STA_DISCONNECTED:
    if (info.disconnected.reason == 6) {
      Serial.println("NOT_AUTHED reconnect");
      WiFi.reconnect();
    }
    /* Stop the web server */
    stop_webserver();
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
      Serial.printf("Got IP: '%s'\n", 
              ip4addr_ntoa(&info.got_ip.ip_info.ip));

      /* Start the web server */
      start_webserver();
      initialise_mdns();
      break;
  default:
    break;
  }
}

static void initialise_mdns(void)
{
    // mdns_init();
    // mdns_hostname_set(CONFIG_EXAMPLE_MDNS_HOST_NAME);
    // mdns_instance_name_set(MDNS_INSTANCE);

    mdns_txt_item_t serviceTxtData[] = {
        {"type", "Yellow-White-Night"},
        {"version", "1"},
        {"location", locationName},
    };

    ESP_ERROR_CHECK(mdns_service_add("LedDriverRest", "_led", "_tcp", REST_API_PORT, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}
