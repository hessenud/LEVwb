
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
void setupWIFI()
{
  WiFi.begin();
  //
  byte count = 0;
  while(WiFi.status() != WL_CONNECTED && count < 50)
  {
    count ++;
    delay(500);
    Serial.print(".");
    digitalWrite(ledPin, (ledState = !ledState));   // toggle the LED on (HIGH is the voltage level)
  
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("Connecting...OK."); 
     digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  } else {
    Serial.println( "Connecting...Failed");
    digitalWrite(ledPin, LOW);   // turn the LED on (HIGH is the voltage level)
    Serial.printf("Connection Failed! setting WIFI parameters ssid: %s  pw: %s...\n", ssid, password);
 
   
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
#if USE_STATIC_IP
    WiFi.config(staticIP, gateway, subnet);
#endif
    count = 0;
    while(WiFi.status() != WL_CONNECTED && count < 50)
    {
      count ++;
      delay(500);
      Serial.print(":");
      digitalWrite(ledPin, (ledState = !ledState));   // toggle the LED on (HIGH is the voltage level)
    }
    if(WiFi.status() == WL_CONNECTED) {
      Serial.println("Connecting 2...OK."); 
       digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
    } else {
      Serial.println("Connection 2 Failed! Rebooting...\n");
      //savePrefs();
      delay(5000);
      ESP.restart(); 
    }     // Register host name in WiFi and mDNS
  }
  
  String hostNameWifi = HOSTNAME;
  hostNameWifi.concat(".local");
  WiFi.hostname(hostNameWifi);
  if (MDNS.begin(HOSTNAME)) {
      Serial.print("* MDNS responder started. Hostname -> ");
      Serial.println( HOSTNAME) ;
  }
}



//---------------------------------
