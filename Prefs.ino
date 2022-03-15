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
    assumed_power=MAX_CONSUMPTION; 
    mqtt_broker=0; 
    mqtt_broker_port=0; 
    mqtt_user=0; 
    mqtt_password=0; 

    device_name=0; 
    model_name=0; 
    serialNr=0; 
    updateTime=0; 
    modelVariant=0; 
    absTimestamp=false;
    optionalEnergy=false;
    timezone = 0;

    devType=12; 
    maxPwr=0; 
    intr=false; 
    use_oled = false;
    defCharge=0; 

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



#include "POW.h"

void POW::loadPrefs(DynamicJsonDocument&cfg)
{
    //--- device description
}

void POW::savePrefs(DynamicJsonDocument&cfg)
{
    //--- device description
}

void saveDevice()
{
    DynamicJsonDocument cfg(Prefs::capacity());
    
    //--- device description
    storePref(  ota_passwd,   g_prefs );
    storePref(  hostname,     g_prefs );
    storePref(  device_name,  g_prefs );
    storePref(  model_name,   g_prefs );
    storePref(  serialNr,     g_prefs );
    storePref(  modelVariant, g_prefs );
    storePref(  maxPwr,       g_prefs );
    storePref(  use_oled,     g_prefs );
    storePref(  timezone,     g_prefs );
    storePref(  absTimestamp, g_prefs );

    // Serialize JSON to file
    File file = fileSystem->open(DEVICE_FILE, "w+");    
    size_t len = serializeJsonPretty(cfg, file);

    if (!file || (len == 0 )) {
      DEBUG_PRINT("failed to write to DEVICE_FILE\n");
      if(myLog)  myLog->log( "failed to write to DEVICE_FILE", true );
    }


   DEBUG_PRINT( " close DEVICE_FILE\n" );
   file.close();
#ifdef MAY_USE_SERIAL
    serializeJsonPretty(cfg, Serial);
#endif
   DEBUG_PRINT( " writteen to DEVICE_FILE\n" );
}

bool loadDevice()
{
    DynamicJsonDocument cfg(Prefs::capacity());
    File file = LittleFS.open(DEVICE_FILE,"r");
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(cfg, file);
    file.close();
    if ((error != DeserializationError::Ok )) {     
      
        DEBUG_PRINT("Device DeserializationError: %d\n", error);

        
        setDef(  ota_passwd, "wadsdapassvoid", g_prefs);
        setDef(  hostname, HOSTNAME, g_prefs);
        setDef(  device_name, DEVICE_NAME, g_prefs);
        setDef(  model_name, HOSTNAME, g_prefs );
        setDef(  serialNr, 9999, g_prefs  );
        setDef(  updateTime, 1000, g_prefs  );
        setDef(  modelVariant,  0, g_prefs );

        setDef(  maxPwr, MAX_CONSUMPTION, g_prefs );
        setDef(  use_oled, false, g_prefs );
        setDef(  timezone, 200, g_prefs );
        setDef(  absTimestamp, false, g_prefs );
        return true;
    } else {
        loadPrefStr(  ota_passwd, "wadsdapassvoid", g_prefs);
        loadPrefStr(  hostname, HOSTNAME, g_prefs);
        loadPrefStr(  device_name, DEVICE_NAME, g_prefs);
        loadPrefStr(  model_name, HOSTNAME, g_prefs );
        loadPref(     serialNr, 9999, g_prefs  );
        loadPref(     updateTime, 1000, g_prefs  );
        loadPref(     modelVariant,  0, g_prefs );

        loadPref(     maxPwr, MAX_CONSUMPTION, g_prefs );
        _loadPref(     use_oled, g_prefs );
        loadPref(  timezone, 200, g_prefs );
        _loadPref(  absTimestamp, g_prefs );
    }
    
    if (g_prefs.maxPwr == 0)  g_prefs.maxPwr    = MAX_CONSUMPTION;
    if (g_prefs.hostname == 0)  g_prefs.hostname    = HOSTNAME;
    return false;
}

void saveParams()
{
    DynamicJsonDocument cfg(Prefs::capacity());
    

    if (myLog)  myLog->log( " write to PREFS_FILE", true );
    //--- device setting
    storePref( assumed_power, g_prefs);
    storePref( mqtt_broker, g_prefs);
    storePref( mqtt_broker_port, g_prefs);
    storePref( mqtt_user, g_prefs);
    storePref( mqtt_password, g_prefs);
    storePref( updateTime, g_prefs);

    storePref( devType, g_prefs );
    storePref( intr, g_prefs   );
    storePref( optionalEnergy, g_prefs   );
    storePref( defCharge, g_prefs );

    storePref( autoDetect, g_prefs );
    storePref( ad_on_threshold , g_prefs);
    storePref( ad_on_time, g_prefs );
    storePref( ad_off_threshold, g_prefs );
    storePref( ad_off_time, g_prefs );
    storePref( ad_prolong_inc, g_prefs );

    // Serialize JSON to file
    File file = fileSystem->open(PREFS_FILE, "w+");    
    size_t len = serializeJsonPretty(cfg, file);
    if (!file || (len == 0 )) {
      if (myLog) myLog->log( "Failed to write to PREFS_FILE", true );
    }
    file.close();
#ifdef MAY_USE_SERIAL
    serializeJsonPretty(cfg, Serial);
#endif
}

bool loadParams()
{
    DynamicJsonDocument cfg(Prefs::capacity());     

    File file = LittleFS.open(PREFS_FILE,"r");
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(cfg, file);
    
    file.close();
    if ((error != DeserializationError::Ok )) { 
      
        DEBUG_PRINT("Params DeserializationError: %d\n", error);
        
        setDef(  assumed_power, MAX_CONSUMPTION, g_prefs );
        setDef(  mqtt_broker, "", g_prefs  );
        setDef(  mqtt_broker_port, 0, g_prefs);
        setDef(  mqtt_user, "", g_prefs    );
        setDef(  mqtt_password, "", g_prefs);
        
        setDef(  devType,  uSEMP::Other, g_prefs);
        setDef(  intr  ,false, g_prefs );
        setDef( optionalEnergy, false,g_prefs );
        setDef(     defCharge, 2000, g_prefs );
        setDef(     autoDetect, false, g_prefs);
        setDef(  ad_on_threshold, 0, g_prefs);
        setDef( ad_on_time, 0, g_prefs);
        setDef( ad_off_threshold, 0, g_prefs);
        setDef(  ad_off_time, 0, g_prefs);
        setDef( ad_prolong_inc, 0, g_prefs );
        return true;
    } else {
        loadPref(     assumed_power, MAX_CONSUMPTION, g_prefs );
        loadPrefStr(  mqtt_broker, "", g_prefs  );
        loadPref(     mqtt_broker_port, 0, g_prefs);
        loadPrefStr(  mqtt_user, "", g_prefs    );
        loadPrefStr(  mqtt_password, "", g_prefs);
        
        loadPref(     devType,  uSEMP::Other, g_prefs);
        _loadPref(     intr  , g_prefs );
        _loadPref( optionalEnergy, g_prefs );
        loadPref(     defCharge, 2000, g_prefs );
        _loadPref(     autoDetect, g_prefs);
        loadPref(  ad_on_threshold, 0, g_prefs);
        loadPref( ad_on_time, 0, g_prefs);
        loadPref( ad_off_threshold, 0, g_prefs);
        loadPref(  ad_off_time, 0, g_prefs);
        loadPref( ad_prolong_inc, 0, g_prefs );
    }
    if (g_prefs.maxPwr == 0)  g_prefs.maxPwr    = MAX_CONSUMPTION;
    if (g_prefs.assumed_power == 0)  g_prefs.assumed_power = MAX_CONSUMPTION;
    return false;
}

void saveChgPrf()
{
    DynamicJsonDocument cfg(Prefs::capacity());     

    if (myLog) myLog->log( " write to CHG_PROFILE_FILE",true );
    // profiles
    for (unsigned n = 0; n < N_POW_PROFILES; ++n) {
        storeProfileXMember( powProfile, n, g_prefs, valid );
        storeProfileXMember( powProfile, n, g_prefs, timeOfDay );
        storeProfileXMember( powProfile, n, g_prefs, timeframe );
        storeProfileXMember( powProfile, n, g_prefs, armed );
        storeProfileXMember( powProfile, n, g_prefs, repeat );
        storeProfileXMember( powProfile, n, g_prefs, est );
        storeProfileXMember( powProfile, n, g_prefs, let) ;
        storeProfileXMember( powProfile, n, g_prefs, req );
        storeProfileXMember( powProfile, n, g_prefs, opt );
    }

    // Serialize JSON to file
    File file = fileSystem->open(CHG_PROFILE_FILE, "w+");
    size_t len = serializeJsonPretty(cfg, file);
    if (!file || (len == 0 )) {
        if (myLog) myLog->log( "failed to write to CHG_PROFILE_FILE", true );
    }
    file.close();
#ifdef MAY_USE_SERIAL
    serializeJsonPretty(cfg, Serial);
#endif
}

void extractChgPrf(DynamicJsonDocument& cfg)
{
    for (unsigned n = 0; n < N_POW_PROFILES; ++n) {
      
        loadProfileXMember(powProfile, n, g_prefs, valid);
        loadProfileXMember(powProfile, n, g_prefs, timeOfDay);
        loadProfileXMember(powProfile, n, g_prefs, timeframe);
        loadProfileXMember(powProfile, n, g_prefs, armed);
        loadProfileXMember(powProfile, n, g_prefs, repeat);
        loadProfileXMember(powProfile, n, g_prefs, est);
        loadProfileXMember(powProfile, n, g_prefs, let);
        loadProfileXMember(powProfile, n, g_prefs, req);
        loadProfileXMember(powProfile, n, g_prefs, opt);
    }
}

bool loadChgPrf()
{ 
    DynamicJsonDocument cfg(Prefs::capacity());     
    File file = LittleFS.open(CHG_PROFILE_FILE,"r");
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(cfg, file);
    file.close();
    if ((error != DeserializationError::Ok )) { 
      
        DEBUG_PRINT("ChgPrfDeserializationError: %d\n", error);
        return true;
    } else {
        extractChgPrf(cfg);
    }
    return false;
}


void saveTimers()
{
    if(myLog)  myLog->log( "write to file TMR_PROFILE_FILE", true);
   
    DynamicJsonDocument cfg(Prefs::capacity());     
    for (unsigned n = 0; n < N_TMR_PROFILES; ++n) {
        storeProfileXMember(tmrProfile, n, g_prefs, valid);
        storeProfileXMember( tmrProfile, n, g_prefs, sw_time );
        storeProfileXMember( tmrProfile, n, g_prefs, interval );
        storeProfileXMember( tmrProfile, n, g_prefs, mo );
        storeProfileXMember( tmrProfile, n, g_prefs, tu );
        storeProfileXMember( tmrProfile, n, g_prefs, we );
        storeProfileXMember( tmrProfile, n, g_prefs, th );
        storeProfileXMember( tmrProfile, n, g_prefs, fr );
        storeProfileXMember( tmrProfile, n, g_prefs, sa );
        storeProfileXMember( tmrProfile, n, g_prefs, su );
        storeProfileXMember( tmrProfile, n, g_prefs, everyday );
        storeProfileXMember( tmrProfile, n, g_prefs, repeat );
        storeProfileXMember( tmrProfile, n, g_prefs, armed );
        storeProfileXMember( tmrProfile, n, g_prefs, switchmode ); ///< true = on false = off
    }
    // Serialize JSON to file
    File file = fileSystem->open(TMR_PROFILE_FILE, "w+");
    size_t len = serializeJsonPretty(cfg, file );
    if (!file || (len == 0 )) {
       if (myLog)  myLog->log( "Failed to write to file TMR_PROFILE_FILE", true);
    }
    file.close();
#ifdef MAY_USE_SERIAL
    serializeJsonPretty(cfg, Serial);
#endif
}

void extractTimers(DynamicJsonDocument& cfg)
{
    for (unsigned n = 0; n < N_TMR_PROFILES; ++n) {
        loadProfileXMember(tmrProfile, n, g_prefs, valid);
        loadProfileXMember(tmrProfile, n, g_prefs, sw_time);
        loadProfileXMember(tmrProfile, n, g_prefs, interval);
        loadProfileXMember(tmrProfile, n, g_prefs, mo);
        loadProfileXMember(tmrProfile, n, g_prefs, tu);
        loadProfileXMember(tmrProfile, n, g_prefs, we);
        loadProfileXMember(tmrProfile, n, g_prefs, th);
        loadProfileXMember(tmrProfile, n, g_prefs, fr);
        loadProfileXMember(tmrProfile, n, g_prefs, sa);
        loadProfileXMember(tmrProfile, n, g_prefs, su);
        loadProfileXMember(tmrProfile, n, g_prefs, everyday);
        loadProfileXMember(tmrProfile, n, g_prefs, repeat);
        loadProfileXMember(tmrProfile, n, g_prefs, armed);
        loadProfileXMember(tmrProfile, n, g_prefs, switchmode); ///< true = on false = off
        setTimers();
    }
}

bool loadTimers()
{
    DynamicJsonDocument cfg(Prefs::capacity());     

    File file = LittleFS.open(TMR_PROFILE_FILE,"r");
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(cfg, file);
    file.close();
    
    if ((error != DeserializationError::Ok )) { 
        DEBUG_PRINT("Timer DeserializationError: %d\n", error);
        return true;
    } else {
        extractTimers(cfg);
    }
    return false;
}




void savePrefs() {
  
    DEBUG_PRINT("***********savePrefs\n");
    DEBUG_PRINT("JsonSerializationBuffer capacity:%u  max avail:%d \n", Prefs::capacity, ESP.getMaxFreeBlockSize() - 512  );
    
    saveDevice();
    saveParams();
    saveChgPrf();
    saveTimers();

    fileSystem->end();
    fileSystem->begin();
}


void loadPrefs()
{
    bool need_save = false; 
    ////////////////////////////////////////////
    // Device COnfiguration
    DEBUG_PRINT("JsonDeserializationBuffer capacity:%u  max avail:%d \n", Prefs::capacity, ESP.getMaxFreeBlockSize() - 512  );
    need_save |= loadDevice();
    need_save |= loadParams();
    need_save |= loadChgPrf();
    need_save |= loadTimers();
    
    g_prefs.loaded = true;
    if ( need_save ) {
      DEBUG_PRINT("need to save Preferences!\n");
      if(myLog) myLog->log("need to save Preferences!\n",true);
      savePrefs();
    }
    
}
