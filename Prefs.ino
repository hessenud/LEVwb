#include "Prefs.h"

#define PREFERENCES_FILE  "config.json"


int eeprom_wp = 0;
Prefs::Prefs()
{
    revision=0; 
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


void loadPrefs()
{
    ////////////////////////////////////////////
    // Device COnfiguration
    DEBUG_PRINT("JsonDeserializationBuffer capacity:%u\n", Prefs::capacity  );
       
    DynamicJsonDocument cfg(Prefs::capacity);
    
    File file = LittleFS.open(PREFERENCES_FILE,"r");
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(cfg, file);
    if ((error != DeserializationError::Ok )) { // || (cfg["revision"] < 1  )) {
        g_prefs.revision = cfg["revision"];
      
        DEBUG_PRINT("Failed to read file, using default configuration  REV:%u error: %s\n",  g_prefs.revision,  error.c_str()   );
        g_prefs.revision = PREFS_REVISION;
        g_prefs.hostname = HOSTNAME;
        g_prefs.assumed_power = MAX_CONSUMPTION;     ///< assumed power for calculations

        g_prefs.mqtt_broker   = "";
        g_prefs.mqtt_broker_port = 0;
        g_prefs.mqtt_user     = "";
        g_prefs.mqtt_password = "";

        g_prefs.device_name   = DEVICE_NAME;
        g_prefs.model_name    = HOSTNAME " DevBoard" ;

        g_prefs.updateTime    = 1000;
        g_prefs.modelVariant   = 1;
        g_prefs.serialNr  = 1;
        g_prefs.devType   = 0;
        g_prefs.maxPwr    = MAX_CONSUMPTION;
        g_prefs.intr      = true;
        g_prefs.defCharge = 1000;
        g_prefs.use_oled  = false;
        file.close();
        savePrefs();
    } else {
        DEBUG_PRINT(" use stored Prefs!!!\n");
        loadPref( revision, 0 );
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
        loadPref(     maxPwr, MAX_CONSUMPTION );
        loadPref(     intr, true   );
        loadPref(     defCharge, 2000 ); 

        if (g_prefs.maxPwr == 0)  g_prefs.maxPwr    = MAX_CONSUMPTION;

    
        for (unsigned n = 0; n < N_CHRG_PROFILES; ++n) {
    #define loadProfileMember( __member )    g_prefs.chgProfile[n].__member = cfg["profiles"][n][ #__member ] 
            loadProfileMember(  timeOfDay );
            loadProfileMember( est );   
            loadProfileMember( let) ;
            loadProfileMember( req );
            loadProfileMember( opt );
        }

        
        file.close();
    }
  

    g_prefs.loaded = true;
}


void savePrefs() {
    // Allocate a temporary JsonDocument
    DynamicJsonDocument cfg(Prefs::capacity);
    // Set the values in the document

    storePref(  revision );
    storePref(  ota_passwd );
    storePref(  hostname );
    storePref(  assumed_power); 
    storePref(  mqtt_broker); 
    storePref(  mqtt_broker_port);
    storePref(  mqtt_user); 
    storePref(  mqtt_password);

    storePref(  device_name);
    storePref(  model_name);
    storePref(  serialNr  ); 

    storePref(  updateTime);
    storePref(  modelVariant);

    storePref(  devType); 
    storePref(  maxPwr ); 
    storePref(  intr   ); 
    storePref(  defCharge);
    storePref(  use_oled); 
      
    // profiles
    
    for (unsigned n = 0; n < N_CHRG_PROFILES; ++n) {
#define storeProfileMember( __member )    cfg["profiles"][n][ #__member ] = g_prefs.chgProfile[n].__member 

        storeProfileMember(  timeOfDay );
        storeProfileMember( est );   
        storeProfileMember( let) ;
        storeProfileMember( req );
        storeProfileMember( opt );
    }

    
    // Serialize JSON to file
    File file = fileSystem->open(PREFERENCES_FILE, "w+");
    if (serializeJsonPretty(cfg, file) == 0) {
        Serial.println(F("Failed to write to file"));
    }

    file.close();
}
