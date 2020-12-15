#define _DEV_BOARD     /////////<<<<<<< DEVELOPER Board Selection  set befor inlcudes!!

//-----------------------------------------------------------------
//--- Configuration -----------------------------------------------
//-----------------------------------------------------------------

#define USE_SSDP
#define USE_POW
#define _USE_POW_DBG
#define USE_POW_INT

#define str(s) #s
#define xstr(xs) str(xs)

#define __DEBUG_SUPPORT         
#ifdef DEBUG_SUPPORT

#include <WebSocketsServer.h>
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
# define HLW_SIM
# define USE_OLED
# define DEV_BASENAME "DevBoard"
# define DEV_EXT " CHG"
# define HOSTNAME "DEVPOW_" DEV_NR 
# define LED_LEVEL(l) (l)
# define MQTTBROKER  "raspi"
# define MQTTPORT    1883
#else
// the real deal
# define DEV_IDX 1
# define DEV_EXT
# define DEV_BASENAME "TwizyPOW"
# define HOSTNAME "TPOW_" DEV_NR 
# define MQTTBROKER  "raspi"
# define MQTTPORT    1883
#endif
/****************************************************************/
#include <EEPROM.h>

#include <uSEMP.h>
#include <ArduinoOTA.h>
#include <uHelper.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#ifdef USE_ASYNC
///@todo completely adopt ASNYC Webserver - this also makes more memory efficiency possible 
//# include <WiFiManager.h>
#else
# include <WiFiManager.h>
#endif


//local modules include
#include "POW.h"
#include "MQTT.h"

#include "Prefs.h"

const char* fsName = "LittleFS";
FS* fileSystem = &LittleFS;
LittleFSConfig fileSystemConfig = LittleFSConfig();
unsigned long getTime();  // forward reference for definitions following in "later" TABs



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


// GPIOs

//-----------------------------------------------------------------
//--- Globals -----------------------------------------------------
//-----------------------------------------------------------------


Prefs g_prefs;
//----- POW ------
POW g_pow;

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


uSEMP* g_semp;
bool fsOK;
PowMqtt g_mqtt;




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


////////////////////////////////////////////////////////





void setup() {
    Serial.begin(SERIAL_BAUDRATE );

    snprintf_P( ChipID, sizeof(ChipID), PSTR("%08x"), ESP.getChipId() );
    snprintf_P( udn_uuid, sizeof(udn_uuid), PSTR("f1d67bee-2a4e-d608-ffff-affe%08x"), ESP.getChipId() );
    snprintf_P( DeviceID , sizeof(DeviceID), PSTR("F-05161968-0000%08x-00"), ESP.getChipId() );
    snprintf_P( DeviceSerial , sizeof(DeviceSerial), PSTR("%04d"), DEVICE_SERIAL_NR );
    Serial.printf_P(PSTR("ChipID: %s\n"), ChipID);


    g_semp = new uSEMP( udn_uuid, DeviceID, DeviceName, DeviceSerial, "EVCharger", Vendor, MAX_CONSUMPTION, getTime
            , ([](bool s) { g_pow.setPwr(s); })
            , &semp_server, SEMP_PORT );
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
    g_pow.setup(g_semp);
    setupTimeClk( TIMEZONE );
    setupIO();
    setupOTA();
    setupWebSrv();
    setupFSbrowser();
#ifdef MQTTBROKER
    g_mqtt.setup(HOSTNAME, MQTTBROKER, MQTTPORT, &g_pow );
#endif
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
        DEBUG_PRINT("LED: %s  Relay: %s\n", state2txt(!g_pow.ledState), state2txt( !g_pow.relayState) );
        DEBUG_PRINT("OLED:\n%s\n", buffer );

        ltime = now;

        // socket_server.send
        String st;
        mkStat( st );
        Serial.printf("STATUS: msg len:%u\n---------------------------\n%s\n--------------------------\n", st.length(), st.c_str() );
        socket_server.broadcastTXT(st.c_str(), st.length());
    }

    g_pow.loop();
    g_mqtt.loop();
    g_semp->loop();
}


///-------------------------------------------------------
/// Debugging --------------------------------------------
///-------------------------------------------------------

void loopDebug()
{
#ifdef DEBUG_SUPPORT
  Debug.handle(); // Remote debug over telnet 
#endif
}

void setupDebug()
{
#ifdef DEBUG_SUPPORT
    Serial.println("* telnetd ");
    MDNS.addService("telnet", "tcp", 23);
    // Initialize the telnet server of RemoteDebug

    Debug.begin(HOSTNAME); // Initiaze the telnet server
    Debug.setResetCmdEnabled(true); // Enable the reset command

    Serial.println("* Arduino RemoteDebug Library");

    Serial.println("*");
    Serial.print("* WiFI connected. IP address: ");
    Serial.println(WiFi.localIP());
    delay(500);
#endif
}
