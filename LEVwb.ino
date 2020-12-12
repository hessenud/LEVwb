#define DEV_BOARD     /////////<<<<<<< DEVELOPER Board Selection

/****************************************************************/
#include <EEPROM.h>

#include <uSEMP.h>
#include <ArduinoOTA.h>
#include <uHelper.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#ifdef USE_ASYNC
//# include <WiFiManager.h>
#else
# include <WiFiManager.h>
#endif


#ifdef INCLUDE_FALLBACK_INDEX_HTM
#include "extras/index_htm.h"
#endif

const char* fsName = "LittleFS";
FS* fileSystem = &LittleFS;
LittleFSConfig fileSystemConfig = LittleFSConfig();
unsigned long getTime();  // forward reference for definitions following in "later" TABs


//-----------------------------------------------------------------
//--- Configuration -----------------------------------------------
//-----------------------------------------------------------------

#define USE_SSDP
#define USE_POW
#define _USE_POW_DBG
#define USE_POW_INT

#define str(s) #s
#define xstr(xs) str(xs)

#define _DEBUG_SUPPORT         
#ifdef DEBUG_SUPPORT
# include "RemoteDebug.h"        //https://github.com/JoaoLopesF/RemoteDebug
  RemoteDebug Debug;
# define DEBUG_PRINT(...) Debug.printf(__VA_ARGS__)
#else
   #include <WebSocketsServer.h>
#endif

#ifndef DEBUG_PRINT
    #define DEBUG_PRINT(...) Serial.printf(__VA_ARGS__)
#endif
#define _DEBUG_PRINT(...)


#ifdef DEV_BOARD
# define DEV_IDX 2
# define LED_PIN 2
# define HLW_SIM
# define USE_OLED
# define DEV_BASENAME "DevBoard"
# define DEV_EXT " CHG"
# define HOSTNAME "DEVPOW_" DEV_NR 
# define LED_LEVEL(l) (l)
# define _MQTTBROKER  "raspi"
# define _MQTTPORT    1883
#else
// the real deal
# define LED_PIN 15
# define DEV_IDX 1
# define DEV_EXT
# define DEV_BASENAME "TwizyPOW"
# define HOSTNAME "TPOW_" DEV_NR 
# define _MQTTBROKER  "raspi"
# define _MQTTPORT    1883
# define LED_LEVEL(l) (!(l))
#endif

#define TIMEZONE (+1*100)   /*MESZ / CEST +2 */  /*MEZ / CET +1*/

#define DEV_NR xstr(DEV_IDX)
#define DEVICE_SERIAL_NR    1


# define DEVICE_NAME DEV_BASENAME DEV_NR DEV_EXT

#define VENDOR   "HesTec"
 
// Device 00 Params
#define MAX_CONSUMPTION  2300
#define SERIAL_BAUDRATE       115200

/*--------helper macros -----------*/
#define DIM(a) (sizeof(a)/sizeof(a[0]))



#define _SCL    // HLW CF
#define _PWM1   // HLW CF1
#define _IO5    // SEL
#define _PWM0   12 // RELAIS

// HLW8012 -- POW Rev1

#ifndef HLW8012_SUPPORT
#define HLW8012_SUPPORT     1
#endif
#define HLW8012_SEL_PIN     5
#define HLW8012_CF1_PIN     13
#define HLW8012_CF_PIN      14

// CSE7766 -- POW Rev2
#define CSE7766_SUPPORT         1
#define CSE7766_PIN             1

// GPIOs
#define RELAY_PIN                       12


//-----------------------------------------------------------------
//--- Globals -----------------------------------------------------
//-----------------------------------------------------------------

const int ledPin   = LED_PIN;
const int relayPin  = 12;  // Sonoff 12
const int buttonPin = 0;

#define RELAY_ON    LOW
#define RELAY_OFF   HIGH
#define LED_ON      LOW
#define LED_OFF     HIGH


int ledState   = LED_OFF;
int relayState = RELAY_OFF;
//----- POW ------
unsigned pow_activePwr;
unsigned pow_voltage;
double   pow_current;

unsigned pow_apparentPwr;
unsigned pow_averagePwr;

double   pow_prefs_pwrMultiplier;    
double   pow_prefs_currentMultiplier;
double   pow_prefs_voltageMultiplier;

double   pow_pwrFactor;
double   pow_pwrMultiplier;
double   pow_currentMultiplier;
double   pow_voltageMultiplier;
unsigned pow_cumulatedEnergy;

//----- PLAN  -----


//----- DEVICE -----
char ChipID[8+1];
char udn_uuid[36+1]; 
char DeviceID[26+1];
char DeviceName[] = DEVICE_NAME;
char DeviceSerial[4+1];
char Vendor[] = VENDOR;


#define SEMP_PORT 9980
WebServer_T http_server(80);
WebServer_T semp_server(SEMP_PORT);
WebSocketsServer socket_server(81);

#ifdef USE_POW
# include "HLW8012.h"
  HLW8012 hlw8012;
#endif
 
uSEMP* g_semp;
bool fsOK;
 
struct ChgProfile {
  bool     timeOfDay;
  unsigned est;
  unsigned let;
  unsigned req;
  unsigned opt;
  
  char est_s[6+1];
  char let_s[6+1];
};

enum {
  PROFILE_STD=0,
  PROFILE_QCK,
  PROFILE_P2,
  //--------------
  N_CHRG_PROFILES
};
ChgProfile g_chgProfile[N_CHRG_PROFILES] = {
  {true, 11 Hrs,15 Hrs, 3000, 6000,0,0},
  {false, 0,0, 3000, 0,0,0},
  {false, 0,3 Hrs, 4000, 6500,0,0}
};

unsigned assumed_power = MAX_CONSUMPTION;     ///< assumed power for calculations


#ifdef USE_OLED
///
// OLED Display support
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif




void setup() {
  Serial.begin(SERIAL_BAUDRATE);
 
  snprintf_P( ChipID, sizeof(ChipID), PSTR("%08x"), ESP.getChipId() );
  snprintf_P( udn_uuid, sizeof(udn_uuid), PSTR("f1d67bee-2a4e-d608-ffff-affe%08x"), ESP.getChipId() );
  snprintf_P( DeviceID , sizeof(DeviceID), PSTR("F-05161968-0000%08x-00"), ESP.getChipId() );
  snprintf_P( DeviceSerial , sizeof(DeviceSerial), PSTR("%04d"), DEVICE_SERIAL_NR );
  Serial.printf_P(PSTR("ChipID: %s\n"), ChipID);
  

  g_semp = new uSEMP( udn_uuid, DeviceID, DeviceName, DeviceSerial, "EVCharger", Vendor, MAX_CONSUMPTION, getTime, setPwr, &semp_server, SEMP_PORT );
  Serial.printf_P(PSTR("uuid  : %s\n"), g_semp->udn_uuid());
  Serial.printf_P(PSTR("DevID : %s\n"), g_semp->deviceID());
  
  
  ////////////////////////////////
  // FILESYSTEM INIT
  DEBUG_PRINT("Initializing filesystem...\n");
  fileSystemConfig.setAutoFormat(false);
  fileSystem->setConfig(fileSystemConfig);
  fsOK = fileSystem->begin();
  DEBUG_PRINT(fsOK ? ("Filesystem initialized.\n") : ("Filesystem init failed!\n"));

  loadPrefs();
    
  setupOLED();
  setupWIFI();
  setupPOW(); 
  setupTimeClk( TIMEZONE );
  setupIO();
  setupOTA(); 
  setupWebSrv();
  setupFSbrowser();
  setupMQTT();

  setupDebug();
  setupSSDP();
  
  requestDailyPlan(false);
}


static unsigned long ltime=0;

const char* state2txt( int state)
{
  return state== HIGH ? "HIGH" : "LOW";
}



void loop() 
{
  
  static unsigned long lastLoop;
  unsigned long thisLoop = millis();
  if (thisLoop - lastLoop > 50 ) {
    DEBUG_PRINT("Long cycle %lu!!\n", thisLoop - lastLoop);
  }
  lastLoop = thisLoop;
  
  loopIO();
  loopOTA();
  loopWebSrv();
  loopDebug();
  unsigned long now = getTime();
  char buffer[10*40];
  char* wp = &buffer[0];
  
  g_semp->dumpPlans( wp ) ;
  draw( buffer );
  if ( now - ltime > 5  ) {  
    DEBUG_PRINT("LED: %s  Relay: %s\n", state2txt(!ledState), state2txt( !relayState) );
    DEBUG_PRINT("OLED:\n%s\n", buffer );
 
    ltime = now;

    // socket_server.send
    String st;
    mkStat( st );
    Serial.printf("STATUS: msg len:%u\n---------------------------\n%s\n--------------------------\n", st.length(), st.c_str() );
    socket_server.broadcastTXT(st.c_str(), st.length());
  }
  
  loopPOW();
  loopMQTT();
  g_semp->loop();
}
