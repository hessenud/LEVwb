#include "Prefs.h"

#define PREFERENCES_FILE  "config.json"


int eeprom_wp = 0;

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
       
    StaticJsonDocument<Prefs::capacity> cfg;
    //StaticJsonDocument<512> cfg;
    
    File file = LittleFS.open(PREFERENCES_FILE,"r");
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(cfg, file);
    if ((error != DeserializationError::Ok )) { // || (cfg["revision"] < 1  )) {
        g_prefs.revision = cfg["revision"];
      
        DEBUG_PRINT("Failed to read file, using default configuration  REV:%u error: %s\n",  g_prefs.revision,  error.c_str()   );
        g_prefs.revision = PREFS_REVISION;
        g_prefs.hostname = HOSTNAME;
        g_prefs.assumed_power = MAX_CONSUMPTION;     ///< assumed power for calculations

        g_prefs.mqtt_broker   = "raspi";
        g_prefs.mqtt_broker_port = 1883;
        g_prefs.mqtt_user     = "tpow";
        g_prefs.mqtt_password = "tpowpw";

        g_prefs.device_name   = DEVICE_NAME;
        g_prefs.model_name    = HOSTNAME " DevBoard" ;

        g_prefs.updateTime    = 1000;
        g_prefs.modelVariant   = 1;

        g_prefs.devType   = 0;
        g_prefs.maxPwr    = MAX_CONSUMPTION;
        g_prefs.intr      = true;
        g_prefs.defCharge = 1000;
        
        file.close();
        savePrefs();
    } else {
        DEBUG_PRINT(" use stored Prefs!!!\n");
        loadPref( revision );
        loadPrefStr(  ota_passwd );
        loadPrefStr(  hostname); //g_prefs.hostname      =  storeString( cfg["hostname"] );
        loadPref(     assumed_power   ); // = cfg["assumed_power"];
        loadPrefStr(  mqtt_broker  ); // = storeString( cfg["mqtt_broker"] );
        loadPref(     mqtt_broker_port); // = cfg["mqtt_broker_port"];
        loadPrefStr(  mqtt_user    ); // = storeString( cfg["mqtt_user"] );
        loadPrefStr(  mqtt_password); // = storeString( cfg["mqtt_password"]);

        loadPrefStr(  device_name); //   = storeString( cfg["device_name"] );
        loadPrefStr(  model_name ); //   = storeString( cfg["model_name"]);

        loadPref(     updateTime  ); //  = cfg["updateTime"];
        loadPref(     modelVariant); //  = cfg["modelVariant"];

        loadPref(     devType); //   = cfg["devType"];
        loadPref(     maxPwr ); //   = cfg["maxPwr"];
        loadPref(     intr   ); //   = cfg["intr"];
        loadPref(     defCharge); // = cfg["defCharge"];
        if (g_prefs.maxPwr == 0)  g_prefs.maxPwr    = MAX_CONSUMPTION;
        file.close();
    }
  

    g_prefs.loaded = true;
}


void savePrefs() {
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.

    StaticJsonDocument<Prefs::capacity> cfg;
    // Set the values in the document

    storePref(  revision );
    storePref(  ota_passwd );
    storePref(  hostname ); // cfg["hostname"] = g_prefs.hostname;
    storePref(  assumed_power); 
    storePref(  mqtt_broker); 
    storePref(  mqtt_broker_port);
    storePref(  mqtt_user); 
    storePref(  mqtt_password);

    storePref(  device_name);
    storePref(  model_name);

    storePref(  updateTime);
    storePref(  modelVariant);

    storePref(  devType); //   = cfg["devType"];
    storePref(  maxPwr ); //   = cfg["maxPwr"];
    storePref(  intr   ); //   = cfg["intr"];
    storePref(  defCharge); // = cfg["defCharge"];
    
    // Serialize JSON to file
    File file = fileSystem->open(PREFERENCES_FILE, "w+");
    if (serializeJsonPretty(cfg, file) == 0) {
        Serial.println(F("Failed to write to file"));
    }

    file.close();
}
