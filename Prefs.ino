#include "Prefs.h"

#define PREFERENCES_FILE  "config.json"

#define DEVICE_FILE       "_device.json"
#define PREFS_FILE        "_prefs.json"
#define CHG_PROFILE_FILE  "_charge.json"
#define TMR_PROFILE_FILE  "_timers.json"

int eeprom_wp = 0;
Prefs::Prefs()
{
    hostname=0; 
    ota_passwd=0; 
    assumed_power=0; 
    mqtt_broker=0; 
    mqtt_broker_port=0; 
    mqtt_user=0; 
    mqtt_password=0; 

    device_name=0; 
    model_name=0; 
    serialNr=0; 
    updateTime=0; 
    modelVariant=0; 


    devType=12; 
    maxPwr=0; 
    intr=false; 
    use_oled = false;
    defCharge=0; 

    //    for(unsigned n=0; n < N_POW_PROFILES; ++n ) {
    //      g_prefs.powProfile[n] = PowProfile( PowProfile::NRGY,  PowProfile::ToD,  PowProfile::IDLE,  PowProfile::ONCE,  0,0, 0, 0);
    //    };
}


void loadCalibration(POW* i_pow)
{
    if( i_pow ) {
        OPEN_EEPROM();
        SEEK_SET_EEPROM( 128 );

        /*calibration*/
        double pwrMultiplier;
        double currentMultiplier;
        double voltageMultiplier;
        unsigned cumulatedEnergy;

        GET_EEPROM( pwrMultiplier );
        GET_EEPROM( currentMultiplier );
        GET_EEPROM( voltageMultiplier );
        GET_EEPROM( cumulatedEnergy );

        char ok[2+1];
        GET_EEPROM(ok);
        CLOSE_EEPROM();

        if (strncmp(ok, "OK",2) == 0 ) {
            //i_pow->cumulatedEnergy = cumulatedEnergy;
            i_pow->setprefs(cumulatedEnergy, pwrMultiplier, currentMultiplier, voltageMultiplier);
        } else {
            DEBUG_PRINT("OK is: %s\n no prefs recovered from EEPROM", ok );
        }
    } else {
        DEBUG_PRINT("no POW yet can't load calibration!\n");
    }
}

/** Store calibration to EEPROM */
void saveCalibration(POW* i_pow)
{
    if( i_pow ) {
        OPEN_EEPROM();

        SEEK_SET_EEPROM( 128 );
        /*Parameters*/
        PUT_EEPROM( i_pow->pwrMultiplier());
        PUT_EEPROM( i_pow->currentMultiplier());
        PUT_EEPROM( i_pow->voltageMultiplier());
        PUT_EEPROM( i_pow->cumulatedEnergy );

        const char ok[3] = "OK";
        PUT_EEPROM(ok);
        COMMIT_EEPROM();
        CLOSE_EEPROM();
    }  else {
        DEBUG_PRINT("no POW yet can'T save!\n");
    }
}


void saveDevice()
{

    DynamicJsonDocument cfg(Prefs::capacity);


    //--- device description
    storePref(  ota_passwd );
    storePref(  hostname );

    storePref(  device_name);
    storePref(  model_name);
    storePref(  serialNr  );
    storePref(  modelVariant);
    storePref(  maxPwr ); 
    storePref(  use_oled ); 


    // Serialize JSON to file
    File file = fileSystem->open(DEVICE_FILE, "w+");
    if (serializeJsonPretty(cfg, file) == 0) {
        Serial.println( F("Failed to write to file") );
    }

    file.close();
}

void loadDevice()
{

    DynamicJsonDocument cfg(Prefs::capacity);

    File file = LittleFS.open(DEVICE_FILE,"r");
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(cfg, file);
    if ((error != DeserializationError::Ok )) { 
        saveDevice();
    } else {
        loadPrefStr(  ota_passwd, "wadsdapassvoid");
        loadPrefStr(  hostname, HOSTNAME); 
        loadPrefStr(  device_name, DEVICE_NAME); 
        loadPrefStr(  model_name, HOSTNAME " Dev"  );
        loadPref(     serialNr, 9999  ); 
        loadPref(     updateTime, 1000  ); 
        loadPref(     modelVariant,  0 ); 

        loadPref(     maxPwr, MAX_CONSUMPTION );
        _loadPref(     use_oled ); 

    }

    file.close();
}

void saveParams()
{

    DynamicJsonDocument cfg(Prefs::capacity);

    //--- device setting
    storePref(  assumed_power); 
    storePref(  mqtt_broker); 
    storePref(  mqtt_broker_port);
    storePref(  mqtt_user); 
    storePref(  mqtt_password);
    storePref(  updateTime);

    storePref(  devType ); 
    storePref(  intr   ); 
    storePref(  defCharge );

    storePref( autoDetect );         
    storePref( ad_on_threshold );
    storePref( ad_on_time );
    storePref( ad_off_threshold );
    storePref( ad_off_time );
    storePref( ad_prolong_inc );

    // Serialize JSON to file
    File file = fileSystem->open(PREFS_FILE, "w+");
    if (serializeJsonPretty(cfg, file) == 0) {
        Serial.println( F("Failed to write to file") );
    }

    file.close();
}

void loadParams()
{

    DynamicJsonDocument cfg(Prefs::capacity);

    File file = LittleFS.open(PREFS_FILE,"r");
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(cfg, file);
    if ((error != DeserializationError::Ok )) { 
        saveParams();
    } else {
        loadPref(     assumed_power, MAX_CONSUMPTION ); 
        loadPrefStr(  mqtt_broker, ""  );
        loadPref(     mqtt_broker_port, 0);
        loadPrefStr(  mqtt_user, ""    ); 
        loadPrefStr(  mqtt_password, ""); 
        
        loadPref(     devType,  uSEMP::Other); 
        _loadPref(     intr   );
        loadPref(     defCharge, 2000 ); 
        _loadPref(     autoDetect);         
        loadPref(  ad_on_threshold, 0);
        loadPref( ad_on_time, 0);
        loadPref( ad_off_threshold, 0);
        loadPref(  ad_off_time, 0);
        loadPref( ad_prolong_inc, 0 );

        if (g_prefs.maxPwr == 0)  g_prefs.maxPwr    = MAX_CONSUMPTION;

    }

    file.close();
}

void saveChgPrf()
{

    DynamicJsonDocument cfg(Prefs::capacity);

    // profiles
    for (unsigned n = 0; n < N_POW_PROFILES; ++n) {
        storeProfileXMember( powProfile, n, valid );
        storeProfileXMember( powProfile, n, timeOfDay );
        storeProfileXMember( powProfile, n, timeframe );
        storeProfileXMember( powProfile, n, armed );
        storeProfileXMember( powProfile, n, repeat );
        storeProfileXMember( powProfile, n, est );   
        storeProfileXMember( powProfile, n, let) ;
        storeProfileXMember( powProfile, n, req );
        storeProfileXMember( powProfile, n, opt );
    }

    // Serialize JSON to file
    File file = fileSystem->open(CHG_PROFILE_FILE, "w+");
    if (serializeJsonPretty(cfg, file) == 0) {
        Serial.println( F("Failed to write to file") );
    }

    file.close();
}

void extractChgPrf(DynamicJsonDocument& cfg)
{
    for (unsigned n = 0; n < N_POW_PROFILES; ++n) {
      
        loadProfileXMember(powProfile, n, valid);
        loadProfileXMember(powProfile, n, timeOfDay);
        loadProfileXMember(powProfile, n, timeframe);
        loadProfileXMember(powProfile, n, armed);
        loadProfileXMember(powProfile, n, repeat);
        loadProfileXMember(powProfile, n, est);
        loadProfileXMember(powProfile, n, let);
        loadProfileXMember(powProfile, n, req);
        loadProfileXMember(powProfile, n, opt);
    }
}

void loadChgPrf()
{

    DynamicJsonDocument cfg(Prefs::capacity);

    File file = LittleFS.open(CHG_PROFILE_FILE,"r");
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(cfg, file);
    if ((error != DeserializationError::Ok )) { 
        saveChgPrf();
    } else {

        extractChgPrf(cfg);

    }

    file.close();
}


void saveTimers()
{

    DynamicJsonDocument cfg(Prefs::capacity);
    for (unsigned n = 0; n < N_TMR_PROFILES; ++n) {
        storeProfileXMember(tmrProfile, n, valid);
        storeProfileXMember( tmrProfile, n, sw_time );
        storeProfileXMember( tmrProfile, n, interval );
        storeProfileXMember( tmrProfile, n, mo );
        storeProfileXMember( tmrProfile, n, tu );
        storeProfileXMember( tmrProfile, n, we );
        storeProfileXMember( tmrProfile, n, th );
        storeProfileXMember( tmrProfile, n, fr );
        storeProfileXMember( tmrProfile, n, sa );
        storeProfileXMember( tmrProfile, n, su );
        storeProfileXMember( tmrProfile, n, everyday );
        storeProfileXMember( tmrProfile, n, repeat );
        storeProfileXMember( tmrProfile, n, armed );
        storeProfileXMember( tmrProfile, n, switchmode ); ///< true = on false = off
    }
    // Serialize JSON to file
    File file = fileSystem->open(TMR_PROFILE_FILE, "w+");
    if (serializeJsonPretty(cfg, file) == 0) {
        Serial.println( F("Failed to write to file") );
    }

    file.close();
}

void extractTimers(DynamicJsonDocument& cfg)
{
    for (unsigned n = 0; n < N_TMR_PROFILES; ++n) {
        loadProfileXMember(tmrProfile, n, valid);
        loadProfileXMember(tmrProfile, n, sw_time);
        loadProfileXMember(tmrProfile, n, interval);
        loadProfileXMember(tmrProfile, n, mo);
        loadProfileXMember(tmrProfile, n, tu);
        loadProfileXMember(tmrProfile, n, we);
        loadProfileXMember(tmrProfile, n, th);
        loadProfileXMember(tmrProfile, n, fr);
        loadProfileXMember(tmrProfile, n, sa);
        loadProfileXMember(tmrProfile, n, su);
        loadProfileXMember(tmrProfile, n, everyday);
        loadProfileXMember(tmrProfile, n, repeat);
        loadProfileXMember(tmrProfile, n, armed);
        loadProfileXMember(tmrProfile, n, switchmode); ///< true = on false = off
        setTimers();
    }
}

void loadTimers()
{
    DynamicJsonDocument cfg(Prefs::capacity);

    File file = LittleFS.open(TMR_PROFILE_FILE,"r");
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(cfg, file);
    if ((error != DeserializationError::Ok )) { 
        saveTimers();
    } else {
        extractTimers(cfg);
    }

    file.close();
}



void loadPrefs()
{
    ////////////////////////////////////////////
    // Device COnfiguration
    DEBUG_PRINT("JsonDeserializationBuffer capacity:%u\n", Prefs::capacity  );

    DynamicJsonDocument cfg(Prefs::capacity);
    if (  LittleFS.exists( DEVICE_FILE ) ) {
        loadDevice();
        loadTimers();
        loadParams();
        loadChgPrf();
    } else {
        // Convert old version
        File file = LittleFS.open(PREFERENCES_FILE,"r");
        // Deserialize the JSON document
        DeserializationError error = deserializeJson(cfg, file);
        if ((error != DeserializationError::Ok )) { 
            DEBUG_PRINT("Failed to read file, using default configuration  error: %s\n",   error.c_str()   );
            g_prefs.hostname = HOSTNAME;
            g_prefs.device_name   = DEVICE_NAME;
            g_prefs.model_name    = HOSTNAME " DevBoard" ;
            g_prefs.modelVariant   = 1;
            g_prefs.serialNr  = 1;

            g_prefs.use_oled  = false;
            g_prefs.mqtt_broker   = "";
            g_prefs.mqtt_broker_port = 0;
            g_prefs.mqtt_user     = "";
            g_prefs.mqtt_password = "";

            g_prefs.updateTime    = 1000;
            g_prefs.devType   = 0;
            g_prefs.maxPwr    = MAX_CONSUMPTION;

            g_prefs.assumed_power = MAX_CONSUMPTION;     ///< assumed power for calculations
            g_prefs.intr      = true;
            g_prefs.defCharge = 1000;

            file.close();
            savePrefs();
        } else {
            DEBUG_PRINT(" use stored Prefs!!!\n");
            loadPrefStr(  ota_passwd, "wadsdapassvoid");
            loadPrefStr(  hostname, HOSTNAME);
            loadPref(     assumed_power, MAX_CONSUMPTION );
            loadPrefStr(  mqtt_broker, ""  );
            loadPref(     mqtt_broker_port, 0);
            loadPrefStr(  mqtt_user, ""    );
            loadPrefStr(  mqtt_password, "");

            loadPrefStr(  device_name, DEVICE_NAME);
            loadPrefStr(  model_name, HOSTNAME " Dev"  );
            loadPref(     serialNr, 9999  );

            loadPref(     updateTime, 1000  );
            loadPref(     modelVariant,  0 );

            loadPref(     devType,  uSEMP::Other);
            _loadPref(     use_oled );

            loadPref(     maxPwr, MAX_CONSUMPTION );
            _loadPref(     intr   );
            loadPref(     defCharge, 2000 );
            _loadPref(     autoDetect);
            loadPref(  ad_on_threshold, 0);
            loadPref( ad_on_time, 0);
            loadPref( ad_off_threshold, 0);
            loadPref(  ad_off_time, 0);
            loadPref( ad_prolong_inc, 0 );

            if (g_prefs.maxPwr == 0)  g_prefs.maxPwr    = MAX_CONSUMPTION;

            extractTimers(cfg);

            extractChgPrf(cfg);

    saveDevice();
    saveParams();
    saveChgPrf();
    saveTimers();
    
            file.close();
        }

    } 

    g_prefs.loaded = true;
}

void savePrefs() {
    saveDevice();
    saveParams();
    saveChgPrf();
    saveTimers();
}
