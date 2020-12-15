#include "Prefs.h"

#define PREFERENCES_FILE  "config.json"


int eeprom_wp = 0;

void loadCalibration()
{
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
        //g_pow.cumulatedEnergy = cumulatedEnergy;
        g_pow.setprefs(cumulatedEnergy, pwrMultiplier, currentMultiplier, voltageMultiplier);

    } else {
        Serial.printf("OK is: %s\n no prefs recovered from EEPROM", ok );
    }
}

/** Store calibration to EEPROM */
void saveCalibration()
{
    OPEN_EEPROM();

    SEEK_SET_EEPROM( 128 );
    /*Parameters*/
    PUT_EEPROM( g_pow.pwrMultiplier());
    PUT_EEPROM( g_pow.currentMultiplier());
    PUT_EEPROM( g_pow.voltageMultiplier());
    PUT_EEPROM( g_pow.cumulatedEnergy );

    const char ok[3] = "OK";
    PUT_EEPROM(ok);
    COMMIT_EEPROM();
    CLOSE_EEPROM();
}


const char* storeString( String istr )
{
    unsigned len = istr.length()+1;
    char* buf = (char*) malloc(len);
    strncpy(buf, istr.c_str(), len);
    return const_cast<const char*>(buf);
}

void loadPrefs()
{
    loadCalibration();

    ////////////////////////////////////////////
    // Device COnfiguration

    StaticJsonDocument<Prefs::capacity> cfg;
    File file = LittleFS.open(PREFERENCES_FILE,"r");
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(cfg, file);
    if (error || (cfg["hostname"] == "null")) {
        Serial.println(F("Failed to read file, using default configuration"));
        g_prefs.hostname      = HOSTNAME;
        g_prefs.assumed_power = MAX_CONSUMPTION;     ///< assumed power for calculations

        g_prefs.mqtt_broker   = MQTTBROKER;
        g_prefs.mqtt_broker_port = MQTTPORT;
        g_prefs.mqtt_user     = "tpow";
        g_prefs.mqtt_password = "tpowpw";

        g_prefs.device_name   = DEVICE_NAME;
        g_prefs.model_name    = "TPOW Simple " HOSTNAME "DevBoard" ;

        g_prefs.updateTime    = 1000;
        g_prefs.modelVariant   = 1;

        g_prefs.devType   = 0;
        g_prefs.maxPwr    = MAX_CONSUMPTION;
        g_prefs.intr      = true;
        g_prefs.defCharge = 0;

    } else {
        Serial.printf(" use stored Prefs!!!\n");
 
#define loadPref( __prf )     g_prefs.__prf=(cfg[ #__prf ])
#define loadPrefStr( __prf )  g_prefs.__prf=storeString(cfg[ #__prf ])

        loadPref(hostname); //g_prefs.hostname      =  storeString( cfg["hostname"] );
        loadPref(assumed_power   ); // = cfg["assumed_power"];
        loadPrefStr(mqtt_broker  ); // = storeString( cfg["mqtt_broker"] );
        loadPref(mqtt_broker_port); // = cfg["mqtt_broker_port"];
        loadPrefStr(mqtt_user    ); // = storeString( cfg["mqtt_user"] );
        loadPrefStr(mqtt_password); // = storeString( cfg["mqtt_password"]);

        loadPrefStr(device_name); //   = storeString( cfg["device_name"] );
        loadPrefStr(model_name ); //   = storeString( cfg["model_name"]);

        loadPref(updateTime  ); //  = cfg["updateTime"];
        loadPref(modelVariant); //  = cfg["modelVariant"];

        loadPref(devType); //   = cfg["devType"];
        loadPref(maxPwr ); //   = cfg["maxPwr"];
        loadPref(intr   ); //   = cfg["intr"];
        loadPref(defCharge); // = cfg["defCharge"];
       
    }
    file.close();

    g_prefs.loaded = true;
}
#define N_PREFS 12


void savePrefs() {
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.

    StaticJsonDocument<Prefs::capacity> cfg;
    // Set the values in the document
#define storePref( __prf )  cfg[ #__prf ]= g_prefs.__prf

    
    storePref( hostname ); // cfg["hostname"] = g_prefs.hostname;
    storePref(assumed_power); 
    storePref(mqtt_broker); 
    storePref(mqtt_broker_port);
    storePref(mqtt_user); 
    storePref(mqtt_password);

    storePref(device_name);
    storePref(model_name);

    storePref(updateTime);
    storePref(modelVariant);


    // Serialize JSON to file
    File file = fileSystem->open(PREFERENCES_FILE, "w+");
    if (serializeJsonPretty(cfg, file) == 0) {
        Serial.println(F("Failed to write to file"));
    }

    file.close();
}
