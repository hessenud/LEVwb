//-----------------------------------------------------------------
//--- Configuration -----------------------------------------------
//-----------------------------------------------------------------

#define WITH_OLED
#define str(s) #s
#define xstr(xs) str(xs)

#define DEBUG_SUPPORT
#define _MAY_USE_SERIAL

#ifdef DEBUG_SUPPORT
# define USE_LIB_WEBSOCKET true 
# include "RemoteDebug.h"        //https://github.com/JoaoLopesF/RemoteDebug
RemoteDebug Debug;
# ifdef MAY_USE_SERIAL
#   define DEBUG_PRINT(...) Debug.printf(__VA_ARGS__);Serial.printf(__VA_ARGS__)
# else
#   define DEBUG_PRINT(...) Debug.printf(__VA_ARGS__);
# endif
#endif

#ifndef DEBUG_PRINT
# ifdef MAY_USE_SERIAL
#   define Debug Serial
#   define DEBUG_PRINT(...)  Serial.printf(__VA_ARGS__)
# else
#   define Debug 0
#   define DEBUG_PRINT(...)  
# endif
#endif


#define DEV_IDX 2
#define DEV_BASENAME "DevBoard"
#define DEV_EXT " CHG"
#define HOSTNAME "DEVPOW_2" DEV_NR 
#define MQTTBROKER  "raspi"
#define MQTTPORT    1883

/****************************************************************/
#include <EEPROM.h>

#include <uSEMP.h>
#include <ArduinoOTA.h>
#include <uHelper.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

#include <WebSocketsServer.h>
//local modules include
#include "POW.h"
#include "MQTT.h"

#include "Prefs.h"

//-------------------------------------------------------------------
//-- uLog -----------------------------------------------------------
//-------------------------------------------------------------------

class uLog {
    static const unsigned cBuffSize = 0x1000;
  FS* m_fs;
  const char* m_logFn;
  unsigned long (*m_getTime)();

  char m_logBuffer[cBuffSize +1];
  unsigned m_wp;
  unsigned m_rp;


  public:
  uLog(FS* i_fs, const char* i_logFn, unsigned long (*i_getTime)()):m_fs(i_fs),m_logFn(i_logFn), m_getTime(i_getTime), m_wp(0), m_rp(0) {
  }

  unsigned used() {
      return  (m_wp > m_rp) ?  (m_wp-m_rp) : ((m_wp == m_rp) ? 0 : (cBuffSize+1+m_wp-m_rp));
  }

  unsigned avail() {
      return cBuffSize - used();
  }
  void flush() { m_rp = m_wp; }
  
  unsigned write(const char* txt, unsigned i_len);

  void log(String txt, bool i_sync=false) {
    log ( txt.c_str(), i_sync );
  }

  void _log(const char* txt);

  void log(const char* txt, bool i_sync=false);
  void sync();

  String get(bool i_flush=false);
};


unsigned uLog::write(const char* txt, unsigned i_len) {
    unsigned nBytes = i_len;
    unsigned bytesWritten = 0;
    unsigned available = avail();
    int      missing = i_len - available;
    if ( missing > 0 ) {
      m_rp += missing;
      if ( m_rp >= cBuffSize) m_rp -= cBuffSize;
      DEBUG_PRINT("***** Buffer exhausted!!! overwrite %d******\n" , missing );
    }
    while ((nBytes > 0) && ((available = avail()) > 0) ){
        unsigned nMaxBytes = m_wp < m_rp ? m_rp-m_wp-1 : cBuffSize - m_wp; // Terminating NULL is appended to cBuffSize
        // todo use memcpy and copy w/o printing
        #define min(a,b) ((a) <(b) ? (a):(b))
        unsigned wrBytes = min( nMaxBytes , nBytes );
        memcpy((void*) &m_logBuffer[m_wp], txt, wrBytes );
        m_wp += wrBytes;
        bytesWritten += wrBytes;
        m_logBuffer[m_wp]= 0;
        txt += wrBytes;
        nBytes -= wrBytes;
        if ( m_wp >= cBuffSize ) {
          m_wp = 0;
        }
    }
    return bytesWritten;
}

void uLog::_log(const char* txt) {
  
    DEBUG_PRINT("%s\n", txt);
    write( txt, strlen(txt) );
}

void uLog::log(const char* txt, bool i_sync) {
  
    DEBUG_PRINT("%s\n", txt);
    
    unsigned long  _now = m_getTime();
    write("\n",1);
    

    const char* tms  = TimeClk::getDateString( _now ); 
    write( tms, strlen(tms) );
    write(" ",1);
    tms = TimeClk::getTimeString( _now );   write( tms, strlen(tms) );
    write(":> ",3);

    write( txt, strlen(txt) );
    if ( i_sync ) sync();
}

String uLog::get(bool i_flush) {
  String ret;
  unsigned nBytes = m_wp - m_rp;
  unsigned rp = m_rp;
  if ( nBytes ) {
      while (  m_wp != rp ) {
           if ( m_wp > rp ) {
               ret += &m_logBuffer[rp];
               rp = m_wp;
           } else {
               // first slice to end of Ringbuff
              // ret += "::::>";
               m_logBuffer[cBuffSize] = 0;
               ret += &m_logBuffer[rp];
               rp = 0;
               //ret += "<::::>";
           }
      }
  }
  if ( i_flush ) m_rp = rp;
  return ret;
}


void uLog::sync() {
   unsigned nBytes = m_wp - m_rp;

   if ( nBytes ) {
       File file = m_fs->open( m_logFn, "a+");
       while (  m_wp != m_rp ) {
            if ( m_wp > m_rp ) {
                nBytes = m_wp - m_rp;
            } else {
                // first slice to end of Ringbuffer
                nBytes = cBuffSize - m_rp;
            }
            m_rp += file.write( &m_logBuffer[m_rp], nBytes );
            if ( m_rp >= cBuffSize ) m_rp = 0;
       }
       file.close();
   }
}
/////------------------------------------------------------



const char* fsName = "LittleFS";
FS* fileSystem = &LittleFS;
LittleFSConfig fileSystemConfig = LittleFSConfig();
bool fsOK;
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

POW* g_pow;
//----- PLAN  -----


//----- DEVICE -----
char ChipID[8+1];
char udn_uuid[36+1]; 
char DeviceID[26+1];
char DeviceSerial[4+1];
char Vendor[] = VENDOR;


#define SEMP_PORT 9980
WebServer_T http_server(80);
WebServer_T semp_server(SEMP_PORT);
WebSocketsServer socket_server(81);

uSEMP*  g_semp;
PowMqtt g_mqtt;
bool    g_gsi;      // true - control by GSI
bool    g_lastRelay;
static unsigned long ltime=0;


#ifdef WITH_OLED
///
// OLED Display support
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif




const char* state2txt( int state)
{
    return state== HIGH ? "ON" : "OFF";
}


void pushStat()
{
  DEBUG_PRINT("pushStat\n");
    
    // socket_server.send
    String st;
    mkStat( st );
    DEBUG_PRINT("STATUS: msg len:%u\n---------------------------\n%s\n--------------------------\n", st.length(), st.c_str());
    socket_server.broadcastTXT(st.c_str(), st.length());
}

uLog* myLog;

void setup() {
    Serial.begin(115200);

    ////////////////////////////////
    // FILESYSTEM INIT


    DEBUG_PRINT("Initializing filesystem...\n");
    fileSystemConfig.setAutoFormat(false);
    fileSystem->setConfig(fileSystemConfig);
    fsOK = fileSystem->begin();
    
    
    g_gsi = false;
    DEBUG_PRINT(fsOK ? ("Filesystem initialized.\n") : ("Filesystem init failed!\n"));
    // boot counter
    myLog = new uLog( fileSystem, "__log.txt", getTime );
    if(myLog) myLog->log( "booted", true );
   
    loadPrefs();
    setupWIFI();
    updateWIFI();
    setupDebug();
    setupOTA();
    
    setupTimeClk( g_prefs.timezone );
 

    ///////////////////////////////
    // SEMP init
    snprintf_P( ChipID, sizeof(ChipID), PSTR("%08x"), ESP.getChipId() );
    snprintf_P( udn_uuid, sizeof(udn_uuid), PSTR("05161968-2a4e-d608-ffff-affe%08x"), ESP.getChipId() );
    snprintf_P( DeviceID , sizeof(DeviceID), PSTR("F-15161968-%04u%08x-00"),g_prefs.serialNr, ESP.getChipId() );
    snprintf_P( DeviceSerial , sizeof(DeviceSerial), PSTR("%04d"), g_prefs.serialNr );
    //Serial.printf_P(PSTR("ChipID: %s\n"), ChipID);


    g_semp = new uSEMP( udn_uuid, DeviceID, g_prefs.device_name, DeviceSerial, uSEMP::devTypeStr(g_prefs.devType), Vendor
            , g_prefs.maxPwr
            , g_prefs.intr, g_prefs.optionalEnergy ,g_prefs.absTimestamp
            , &semp_server, SEMP_PORT );
    g_pow = newPOW( g_prefs.modelVariant, g_semp );

    g_semp->setCallbacks( getTime
            ,([]( EM_state_t ems) {  g_pow->rxEmState(ems);  })
            ,([]( ) {  g_pow->endOfPlan();  }));

    
    ///////////////////////////////
    // 

        //Serial.printf("set OLED\n");
    setupOLED();
    
        //Serial.printf("set IO\n");
    setupIO();
    
        //Serial.printf("set WEB\n");
    setupWebSrv();


        //Serial.printf("set MQTT\n");
    if ( g_prefs.mqtt_broker_port )
        g_mqtt.setup(g_prefs.hostname,   g_prefs.mqtt_broker, g_prefs.mqtt_broker_port, g_pow );


  
        //Serial.printf("set SSDP\n");
    setupSSDP();
    
 

        //Serial.printf("after set OTA\n");
    // requestDailyPlan(false);
    
        //Serial.printf("set PushStat\n");
    pushStat();

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
    unsigned long _now = getTime();
    char buffer[20*40];
    char* wp = &buffer[0];

    wp += g_semp->dumpPlans( wp ) ;
    for ( unsigned n=0; n< N_TMR_PROFILES; ++n) {
      extern uTimer  g_Timers[N_TMR_PROFILES];
      wp += uTimer::dump( wp, g_Timers[n] );
    
    }
    wp += sprintf_P(wp, PSTR("-------------------\n"));
    const char* days[] = {"Mo","Tu","We","Th","Fr","Sa","So" };
    int day_of_week = ( (_now/(1 DAY)) + 3) % 7; 
    wp += sprintf_P(wp, PSTR("* %s %s *"), days[day_of_week],TimeClk::getTimeString( _now ) );
    draw( buffer );  
    if ( ( _now - ltime > 5  ) 
       || ( g_lastRelay != g_pow->relayState   )
       ) {
      ltime = _now;
      g_lastRelay = g_pow->relayState;
      pushStat();
      if (g_pow)  DEBUG_PRINT("LED: %s  Relay: %s EMstate: %s\n", state2txt(g_pow->ledState ), state2txt( g_pow->relayState), g_pow->m_online ? "ONLINE" : "OFFLINE"  );
    
      DEBUG_PRINT("%s\n", buffer );
//      if(myLog) myLog->_log( buffer );
//      if(myLog) myLog->_log( "available: ");
//      if(myLog) myLog->_log( (String(myLog->avail()) + "\n").c_str());
//      if(myLog) myLog->_log( "\n" );
      // DEBUG_PRINT( "%s\n", myLog->get().c_str() );
    }

    if (g_pow) g_pow->loop();
    g_mqtt.loop();
    g_semp->loop();
}


///-------------------------------------------------------
/// Debugging --------------------------------------------
///-------------------------------------------------------

void setupDebug()
{
#ifdef DEBUG_SUPPORT
    Serial.println("* telnetd ");
    MDNS.addService("telnet", "tcp", 23);
    // Initialize the telnet server of RemoteDebug

    Debug.begin(g_prefs.hostname); // Initiaze the telnet server
    Debug.setResetCmdEnabled(true); // Enable the reset command

    Serial.println("* Arduino RemoteDebug Library");

    Serial.println("*");
    Serial.print("* WiFI connected. IP address: ");
    Serial.println(WiFi.localIP());
    delay(500);
#endif
}

void loopDebug()
{
#ifdef DEBUG_SUPPORT
    Debug.handle(); // Remote debug over telnet
#endif
}




POW*  newPOW( unsigned i_variant, uSEMP* i_semp )
{
    switch ( i_variant ) {
    case 0:   return new POW_Sim( i_semp, handleAppEvt );             break;
    case 1:   return new POW_R1(  i_semp, handleAppEvt );             break; 
    case 2:   return new POW_R2(  i_semp, handleAppEvt );             break;
    case 3:   return new POW_R3(  i_semp, handleAppEvt );             break;
    default:  return new POW_Sim( i_semp, handleAppEvt );             break;
    }

}
