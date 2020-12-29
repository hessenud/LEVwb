
//------------------- WIFI


/**********************************************  WIFI Client  *********************************
 * wifi client
 */

//WiFi&OTA 

#define PASSWORD "S8nti8g0" //the password for OTA upgrade, can set it in any char you want

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}

/*********************************************************************
 * setup wifi
 */

#define USE_WIFIMANAGER
void setupWIFI() {
  // put your setup code here, to run once:
#ifdef USE_WIFIMANAGER
  
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  // wifiManager.resetSettings();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(180);
  
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect("LEVwbCfg","leviathan")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  } 
#else
 /* Connect WiFi */
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.printf("WiFi Failed!\n");
      return;
  }
#endif
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  String hostNameWifi = g_prefs.hostname;
  hostNameWifi.concat(".local");
  WiFi.hostname(hostNameWifi);
  if (MDNS.begin(g_prefs.hostname)) {
      Serial.print("* MDNS responder started. Hostname -> ");
      Serial.println( g_prefs.hostname) ;
  }
}
 


//---------------------------------
