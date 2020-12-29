

void loopOTA() {
  ArduinoOTA.handle();
  
}


void setupOTA() {

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname( "DevBoard" );
  ArduinoOTA.setHostname( g_prefs.hostname );

  // No authentication by default
  // ArduinoOTA.setPassword( g_prefs.ota_passwd );

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    DEBUG_PRINT("Start updating ");
    DEBUG_PRINT(type.c_str());
    DEBUG_PRINT("\n");
    fileSystem->end();
  });
  ArduinoOTA.onEnd([]() {
    DEBUG_PRINT("\nEnd\n");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINT("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_PRINT("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      DEBUG_PRINT("Auth Failed\n");
    } else if (error == OTA_BEGIN_ERROR) {
      DEBUG_PRINT("Begin Failed\n");
    } else if (error == OTA_CONNECT_ERROR) {
      DEBUG_PRINT("Connect Failed\n");
    } else if (error == OTA_RECEIVE_ERROR) {
      DEBUG_PRINT("Receive Failed\n");
    } else if (error == OTA_END_ERROR) {
      DEBUG_PRINT("End Failed\n");
    }
  });
  ArduinoOTA.begin();
}
