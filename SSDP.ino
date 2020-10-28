String schemaS;
void setupSSDP() 
{

  String myIP = WiFi.localIP().toString();
    ssdp_cfg ssdpcfg = {
      /* description URL  */  "description.xml",
      /* udn_uuid         */  udn_uuid,
      /* IP               */  myIP.c_str(),
      
      /* deviceName       */  DEVICE_NAME,
      /* modelName        */  "TPOW Simple " HOSTNAME "DevBoard",
      
      /* description      */  "Sonoff POW reloaded",
      /* modelNr          */  "0.1",
      /* modelURL         */  "http://www.marihessenauer.de/TPOW_V1",

  
      /* manufacturer     */  "marihessenauer.de",
      /* manufacturererURL*/  "http://www.marihessenauer.de/",
      /* presentationURL  */  "/pwr" // "index.html"
  
    };



    Serial.printf_P( PSTR("Starting SSDP...\n"));
    SSDP.setSchemaURL( ssdpcfg.descriptionURL);
    SSDP.setURL(ssdpcfg.descriptionURL);
    SSDP.setDeviceType("urn:schemas-simple-energy-management-protocol:device:Gateway:1");  
    SSDP.setHTTPPort(SEMP_PORT);

    // SSDP.setTTL(9);
   
    /** SSDP schema has to be recreated, as the SSDP Library Scheme can't be sufficiently expanded **/
    //-------  Schema overwrite BEGIN
    const char* ssdpScheme = g_semp->makeSsdpScheme( &ssdpcfg);
    DEBUG_PRINT(" SSDPC Scheme:\n%s\n", ssdpScheme );        

    semp_server.on(String("/") + ssdpcfg.descriptionURL, HTTP_GET, [ssdpScheme,ssdpcfg]() {
      DEBUG_PRINT("SSDP request /%s\n%s", ssdpcfg.descriptionURL, ssdpScheme ); 
      semp_server.send(200, "text/xml", ssdpScheme ); 
    });
    //-------  Schema overwrite END    
    SSDP.begin();

    Serial.printf_P(PSTR("Ready!\n"));
}
