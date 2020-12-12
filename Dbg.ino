////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

#define PREFERENCES_FILE  "config.json"

bool prefs_loaded;
int eeprom_wp = 0;

void loadPrefs() {
  prefs_loaded = false;
  OPEN_EEPROM();
  SEEK_SET_EEPROM( 128 );

  /*Parameters*/
  GET_EEPROM( pow_prefs_pwrMultiplier ); 
  GET_EEPROM( pow_prefs_currentMultiplier ); 
  GET_EEPROM( pow_prefs_voltageMultiplier ); 
  unsigned pow_prefs_cumulatedEnergy; GET_EEPROM( pow_prefs_cumulatedEnergy );
  
  char ok[2+1];
  GET_EEPROM(ok);
  CLOSE_EEPROM();
    
  if (strncmp(ok, "OK",2) == 0 ) {
    prefs_loaded = true; 
    pow_cumulatedEnergy = pow_prefs_cumulatedEnergy;
    setPOWprefs(pow_prefs_pwrMultiplier, pow_prefs_currentMultiplier, pow_prefs_voltageMultiplier);

  } else {
    Serial.printf("OK is: %s\n no prefs recovered from EEPROM", ok );
  }


  StaticJsonDocument<512> cfg;
   File file = LittleFS.open(PREFERENCES_FILE,"r");
 // Deserialize the JSON document
  DeserializationError error = deserializeJson(cfg, file);
   if (error)
    Serial.println(F("Failed to read file, using default configuration"));


  file.close();
}


void dumpConfig(String& out)
{
  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> cfg;
  // Set the values in the document
  cfg["plans"] = "dayly plan";
  cfg["devType"] = "EVCharger";
  cfg["maxPwr"] = "2000";
  cfg["intr"] = true;
  cfg["defCharge"] = "3000";

  // Serialize JSON to file
  if (serializeJson(cfg, Serial) == 0) {
    Serial.println(F("Failed to write to Serial"));
  }

  if (serializeJson(cfg, out) == 0) {
    Serial.println(F("Failed to write to Serial"));
  }

  
}
/** Store WLAN credentials to EEPROM */
void savePrefs() 
{
  OPEN_EEPROM();
  
  SEEK_SET_EEPROM( 128 ); 
  /*Parameters*/
  PUT_EEPROM( pow_pwrMultiplier     = hlw8012.getPowerMultiplier());
  PUT_EEPROM( pow_currentMultiplier = hlw8012.getCurrentMultiplier()); 
  PUT_EEPROM( pow_voltageMultiplier = hlw8012.getVoltageMultiplier()); 
  PUT_EEPROM ( pow_cumulatedEnergy );
  
  const char ok[3] = "OK";
  PUT_EEPROM(ok);
  COMMIT_EEPROM();
  CLOSE_EEPROM();

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> cfg;
  // Set the values in the document
  cfg["plans"] = "dayly plan";

  // Serialize JSON to file
  File file = fileSystem->open(PREFERENCES_FILE, "w+");
  if (serializeJson(cfg, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }
 
  file.close();
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
