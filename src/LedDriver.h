////
/// LedDriver.h - common defs for pproject 
//

#define SLIDER_MAXVALUE 1023
#define SLIDER_MINVALUE 0

#define YellowChannel 0
#define WhiteChannel 1

#define PrefsMagic 0x55aa22b3L
#define PrefsFilePath "/preferences.bin"
#define PrefsSaveTimeout 10000

enum LedsPowerState {
  On, Off, Night
};

#define REST_API_PORT 80

extern SimpleTimer timer;
extern int yellowLevel;
extern int whiteLevel;
extern enum LedsPowerState powerState;
extern int rebootCounter;

extern int alarmYellowLevel;
extern int alramWhiteLevel;
extern int riseTime;
extern int alarmHour;
extern int alarmMinute;
extern bool alarmEnabled;
extern long gmtOffsetSec;
extern char locationName[100];


void ledsOff();
void ledsRestore();

void savePrefs();
void restorePrefs();

void ledSetup();
void setBrightness(int bright, int channel);
void nightLedOn(bool on);

void cancelAlarm();

void initialize_alaram();

void start_webserver(void);
void stop_webserver(void);
