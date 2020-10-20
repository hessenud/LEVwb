
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////



bool prefs_loaded;
int eeprom_wp = 0;
char ssid2[64] = {0};
char password2[64] = {0};
  

void loadPrefs() {
  prefs_loaded = false;
  OPEN_EEPROM();
  GET_EEPROM( ssid2);
  GET_EEPROM( password2 );

  /*Parameters*/
  GET_EEPROM( pow_prefs_pwrMultiplier ); 
  GET_EEPROM( pow_prefs_currentMultiplier ); 
  GET_EEPROM( pow_prefs_voltageMultiplier ); 
  unsigned pow_prefs_cumulatedEnergy; GET_EEPROM( pow_prefs_cumulatedEnergy );
  
  char ok[2+1];
  GET_EEPROM(ok);
  CLOSE_EEPROM();
    
  if (strncmp(ok, "OK",2) == 0 ) {
    Serial.printf("Recovered credentials: %s\n%s\n", ssid2, password2);
  
    ssid            = ssid2;
    password        = password2;
    pow_cumulatedEnergy = pow_prefs_cumulatedEnergy;
  
    prefs_loaded = true;
    setPOWprefs(pow_prefs_pwrMultiplier, pow_prefs_currentMultiplier, pow_prefs_voltageMultiplier);

  } else {
    Serial.printf("OK is: %s\n no prefs recovered", ok );
  }
}

/** Store WLAN credentials to EEPROM */
void savePrefs() 
{
  OPEN_EEPROM();
  strncpy(ssid2, ssid, sizeof(ssid2)-1);
  strncpy(password2, password, sizeof(password2)-1);
  
  PUT_EEPROM( ssid2 );
  PUT_EEPROM( password2 );
 
  /*Parameters*/
  PUT_EEPROM( pow_pwrMultiplier     = hlw8012.getPowerMultiplier());
  PUT_EEPROM( pow_currentMultiplier = hlw8012.getCurrentMultiplier()); 
  PUT_EEPROM( pow_voltageMultiplier = hlw8012.getVoltageMultiplier()); 
  PUT_EEPROM ( pow_cumulatedEnergy );
  
  const char ok[3] = "OK";
  PUT_EEPROM(ok);
  COMMIT_EEPROM();
  CLOSE_EEPROM();

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
