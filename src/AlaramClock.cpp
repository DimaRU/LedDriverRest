////
///  AlaramClock.cpp
//

#include <Arduino.h>
#include "esp_system.h"
#include "esp_err.h"
#include "lwip/apps/sntp.h"
#include <SimpleTimer.h>

#include "LedDriver.h"
#include "HardwareDefs.h"

static void checkAlarm();

enum AlarmState {
  waiting,
  alarming,
  cancelled
};

int alarmYellowLevel = SLIDER_MAXVALUE/4;
int alramWhiteLevel = SLIDER_MAXVALUE/4;
int riseTime = 15;
int alarmHour = 7;
int alarmMinute = 0;
bool alarmEnabled = false;
long gmtOffsetSec = 10800;

time_t startTime;
time_t endTime;
enum AlarmState alarmState = waiting;


void initialize_sntp(void);

void cancelAlarm() {
  if (alarmState == alarming) {
    alarmState = cancelled;
  }
}

static void alarm() {
  // Calculate start/end time;
  startTime = time(NULL);
  endTime = startTime + riseTime * 60;
  printf("Alarm interval %ld - %ld, riseTime %d\n", startTime, endTime, riseTime);

  powerState = On;
  nightLedOn(false);
}

static void adjustBrightness(time_t now) {
  int timeOffset = now - startTime;
  yellowLevel = timeOffset * alarmYellowLevel / (riseTime * 60);
  whiteLevel = timeOffset * alramWhiteLevel / (riseTime * 60);
  ledsRestore();
}

static void checkAlarm() {
  time_t now;
  struct tm timeinfo;
  enum AlarmState beginState = alarmState;
  
  now = time(NULL);
  localtime_r(&now, &timeinfo);

  switch(alarmState) {
    case waiting:
      if (!alarmEnabled) return;
      if (timeinfo.tm_hour == alarmHour &&
          timeinfo.tm_min == alarmMinute) {
          // Bingo!
          alarmState = alarming;
          alarm();
      }
      break;
    case alarming:
      if (now > endTime) {
        alarmState = waiting;
      } else {
        adjustBrightness(now);
      }
      break;
    case cancelled:
      if (timeinfo.tm_hour == alarmHour &&
      timeinfo.tm_min == alarmMinute) break;
      alarmState = waiting;
      break;
  }
  if (beginState != alarmState) {
    printf("Time %ld, %d:%d, state %d\n", now, timeinfo.tm_hour, timeinfo.tm_min, alarmState);
  }
}

void initialize_alaram(void) {
  initialize_sntp();
  timer.setInterval(1000L, checkAlarm);
}


void initialize_sntp(void)
{
  configTime(gmtOffsetSec, 0, "pool.ntp.org", NULL, NULL);
}
