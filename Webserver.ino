void setupWebSrv()
{
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {

    Serial.printf("Starting HTTP...\n");

     http_server.on("/", HTTP_GET, handlePwrReq);
    
    http_server.on("/index.html", HTTP_GET, []() {
      http_server.send(200, "text/plain", "Hello User, this is POW!");
    });

    http_server.on(
      "/on", HTTP_GET, []() {
      http_server.send(200, "text/plain", "ON!");
      relayState = HIGH;
    });
    http_server.on("/off", HTTP_GET, []() {
      http_server.send(200, "text/plain", "OFF!");
      relayState = LOW;

    });
    
    http_server.on("/reload", HTTP_GET, []() {
      loadPrefs();
      http_server.send(200, "text/plain", "pref reloaded");
    });
    
    http_server.on("/save", HTTP_GET, []() {
      savePrefs();
      http_server.send(200, "text/plain", "prefs saved!");
    });
    
    http_server.on("/ctl", HTTP_GET, handleCtl );
    http_server.on("/cfg", HTTP_GET, handleCfg );
    
    http_server.onNotFound ( handleNotFound );
    http_server.begin();

  }
}



void loopWebSrv()
{
  http_server.handleClient();
}

void showRequest(unsigned i_retCode, String i_msg) {
    String message = i_msg + "\n---------------------\n";
  message += "URI: ";
  message += http_server.uri();
  message += "\nMethod: ";
  message += ( http_server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += http_server.args();
  message += "\n";

  for ( int i = 0; i < http_server.args(); i++ ) {
    message += " " + http_server.argName ( i ) + ": " + http_server.arg ( i ) + "\n";
  }

  DEBUG_PRINT("Web%d: %s\n", i_retCode, message.c_str());
}


void handleNotFound() 
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += http_server.uri();
  message += "\nMethod: ";
  message += ( http_server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += http_server.args();
  message += "\n";

  for ( int i = 0; i < http_server.args(); i++ ) {
    message += " " + http_server.argName ( i ) + ": " + http_server.arg ( i ) + "\n";
  }

  DEBUG_PRINT("Web404: %s\n", message.c_str());
  
  http_server.send ( 404, "text/plain", message );
}

void handleCfg()
{
  for ( int n = 0; n < http_server.args(); ++n) {
    String p1Name = http_server.argName(n);
    String p1Val = http_server.arg(n);
   // float value = atof( p1Val.c_str() );
    DEBUG_PRINT("p%dName: %s  val: %s\n", n, p1Name.c_str(), p1Val.c_str() );
    if (p1Name == String("name")) {
      String theName = p1Val;
      http_server.send(200, "text/plain", (String("Name:") + String(theName)) );
      return;
    } else {

    }
  }
  http_server.send(404, "text/plain", "ERR");
}

void handleCtl()
{
  for ( int n = 0; n < http_server.args(); ++n) {
    String p1Name = http_server.argName(n);
    String p1Val = http_server.arg(n);
    //float value = atof( p1Val.c_str() );
    DEBUG_PRINT("CTL p%dName: %s  val: %s\n", n, p1Name.c_str(), p1Val.c_str() );
    if (p1Name == String("on")) {
      relayState = HIGH;
    } else if (p1Name == String("off")) {
      relayState = LOW;
    } else {
      http_server.send(404, "text/plain", "ERR");
      return;
    }
  }
  http_server.send(200, "text/plain", relayState ? "on" : "off" + String(" -> OK"));
}
