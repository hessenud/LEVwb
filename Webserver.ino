
////////////////////////////////
// Utils to return HTTP codes, and determine content-type

static const char TEXT_PLAIN[] PROGMEM = "text/plain";
static const char FS_INIT_ERROR[] PROGMEM = "FS INIT ERROR";
static const char FILE_NOT_FOUND[] PROGMEM = "FileNotFound";
static const char TEXT_XML[] PROGMEM = "text/xml";
static const char TEXT_JSON[] PROGMEM = "application/json";

void replyOK() {
    http_server.send(200, FPSTR(TEXT_PLAIN), "");
}

void replyOKWithMsg(String msg) {
    http_server.send(200, FPSTR(TEXT_PLAIN), msg);
}

void replyOKWithXml(String msg) {
    http_server.send(200, FPSTR(TEXT_XML), msg);
}
void replyOKWithJSON(String msg) {
    http_server.send(200, FPSTR(TEXT_JSON), msg);
}

void replyNotFound(String msg) {
    http_server.send(404, FPSTR(TEXT_PLAIN), msg);
}

void replyBadRequest(String msg) {
    DEBUG_PRINT(msg.c_str());
    http_server.send(400, FPSTR(TEXT_PLAIN), msg + "\r\n");
}

void replyServerError(String msg) {
    DEBUG_PRINT(msg.c_str());
    http_server.send(500, FPSTR(TEXT_PLAIN), msg + "\r\n");
}



String unsupportedFiles = String();

File uploadFile;

void setupWebSrv()
{
    if (WiFi.waitForConnectResult() == WL_CONNECTED) {

        DEBUG_PRINT("Starting HTTP...\n");

        http_server.on("/",     HTTP_GET, []() { handleFileRead("/main.html"); });

        http_server.on("/on",   HTTP_GET, []() { g_pow->setRelay(true);  handleStat(); });
        http_server.on("/off",  HTTP_GET, []() { g_pow->setRelay(false); handleStat(); });

        http_server.on("/pwr", HTTP_GET,        mkDelegate( g_pow, handlePwrReq) ); 
        http_server.on("/sim", HTTP_GET,        mkDelegate( g_pow, handleSimReq) );
        http_server.on("/calibrate", HTTP_GET,  mkDelegate( g_pow, handleCalReq) ); 
        http_server.on("/energy", HTTP_GET,     mkDelegate( g_pow, handleEnergyReq));
        http_server.on("/simplerq", HTTP_GET,     mkDelegate( g_pow, handleSimpleReq));
        http_server.on("/timer", HTTP_GET,      mkDelegate( g_pow, handleTimeReq));

        http_server.on("/stat",       HTTP_GET, handleStat );
        http_server.on("/ctl",        HTTP_GET, handleCtl );
        http_server.on("/setProfile", HTTP_GET, handleSetProfile );

        http_server.on("/getProfiles",HTTP_GET, handleGetProfiles );
        http_server.on("/setConfig",  HTTP_GET, handleSetConfig );
        http_server.on("/setTimer",   HTTP_GET, handleSetTimer );

        http_server.on("/reqDaily",  HTTP_GET, []() { dailyChores (false); replyOKWithMsg("Plans set");} );

        http_server.on("/restart",    HTTP_GET, []() { 
            fileSystem->end();
            ESP.reset();}
        );

        http_server.on("/reload",     HTTP_GET, []() { loadPrefs(); replyOKWithMsg("pref reloaded");  });
        http_server.on("/save",       HTTP_GET, []() { savePrefs(); replyOKWithMsg("prefs saved!");   });


        setupFSbrowser();

        http_server.begin();
        socket_server.begin();
    }
}



void loopWebSrv()
{
    http_server.handleClient();
    socket_server.loop();
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


///@todo
void handleCfg()
{
    for ( int n = 0; n < http_server.args(); ++n) {
        String p1Name = http_server.argName(n);
        String p1Val = http_server.arg(n);
        // float value = atof( p1Val.c_str() );
        DEBUG_PRINT("p%dName: %s  val: %s\n", n, p1Name.c_str(), p1Val.c_str() );
        if (p1Name == String("name")) {
            String theName = p1Val;
            replyOKWithMsg(  (String("Name:") + String(theName)) );
            return;
        } else {

        }
    }
    replyNotFound( "wrong params");
}



void handleAux()
{
    String resp = String("[HLW] Active Power (W)    : ") + String(g_pow->activePwr) +
            String("\n[HLW] Voltage (V)         : ") + String(g_pow->voltage) +
            String("\n[HLW] Current (A)         : ") + String(g_pow->current) +
            "\n\n";

    char buffer[resp.length() + 10*40];
    unsigned wp = sprintf(buffer,"\n%s\n",resp.c_str());
    g_semp->dumpPlans(&buffer[wp-1]);

    DEBUG_PRINT("%s\n", buffer);
    replyOKWithMsg(  buffer);
}


void handleCtl() {
    for ( int n = 0; n < http_server.args(); ++n) {
        String pName = http_server.argName(n);
        String pVal = http_server.arg(n);
        //float value = atof( p1Val.c_str() );
        DEBUG_PRINT("CTL p%dName: %s  val: %s\n", n, pName.c_str(), pVal.c_str() );
        if (pName == String("on")) {
            g_pow->setRelay(true);
        } else if (pName == String("off")) {
            g_pow->setRelay(false);
        } else if (pName == String("togOnline")) {
          if ( g_pow->online ) {
              g_pow->online = false;
          } else {
            g_pow->online = true;
          }
          g_semp->acceptEMSignal( g_pow->online );
          g_semp->setEmState( g_pow->relayState );

        } else if (pName == String("chrgProfile")) {
            if (pVal == "std"){
                requestProfile( 0 );
            } else if (pVal == "qck"){
                // quickcharge: Profile QCK, forcibly on and OFFLINE, don't let SHM control POW
                requestProfile( 1 );
                g_pow->setRelay(true);
                g_semp->acceptEMSignal( (g_pow->online = false) );
            } else if (pVal == "del"){
                g_semp->resetPlan(-1); // reset active Plan
                g_pow->endOfPlan();
            } else if (pVal == "delAll"){
                g_semp->deleteAllPlans();
                g_pow->endOfPlan();
            } else {
                requestProfile( atoi(pVal.c_str())  );
            }
        } else if (pName == String("stat")) {
        } else {
            replyNotFound(  "ERR");
            return;
        }
    }
    handleStat();
    pushStat();
}

#define value2timeStr( vs )   (snprintf_P( (vs##_s), sizeof(vs##_s), PSTR("%2lu:%02lu"), (((vs)  % 86400L) / 3600), (((vs)  % 3600) / 60) ), (vs##_s))

void handleGetProfiles()
{
    const int capacity = JSON_ARRAY_SIZE(N_POW_PROFILES+1) + (N_POW_PROFILES+1)*JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> doc;
    for (unsigned n=0; n < N_POW_PROFILES; ++n) {
        doc[n]["timeOfDay"] = g_prefs.powProfile[n].timeOfDay; 
        doc[n]["armed"]     = g_prefs.powProfile[n].armed;
        doc[n]["repeat"]    = g_prefs.powProfile[n].repeat;

        doc[n]["est"]       =  value2timeStr( g_prefs.powProfile[n].est );
        doc[n]["let"]       =  value2timeStr( g_prefs.powProfile[n].let );
        doc[n]["req"]       = g_prefs.powProfile[n].req;
        doc[n]["opt"]       = g_prefs.powProfile[n].opt;
        
        DEBUG_PRINT("EST(%u): %u -> %s\n", n, g_prefs.powProfile[n].est, g_prefs.powProfile[n].est_s);
        DEBUG_PRINT("LET(%u): %u -> %s\n", n, g_prefs.powProfile[n].let, g_prefs.powProfile[n].let_s);
    }

    String resp;
    if (serializeJsonPretty(doc, resp) == 0) {
        DEBUG_PRINT(PSTR("Failed to serialize"));
    }
    
    DEBUG_PRINT(PSTR("Profiles(%u): \n%s\n"),N_POW_PROFILES, resp.c_str());
    replyOKWithMsg(  resp);
}

void handleGetTimers()
{
    const int capacity = JSON_ARRAY_SIZE(N_POW_PROFILES+1) + (N_TMR_PROFILES+1)*JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> doc;
    for (unsigned n=0; n < N_POW_PROFILES; ++n) {
        doc[n]["timeOfDay"] = g_prefs.powProfile[n].timeOfDay;
        doc[n]["armed"]     = g_prefs.powProfile[n].armed;
        doc[n]["repeat"]    = g_prefs.powProfile[n].repeat;

        doc[n]["est"]       =  value2timeStr( g_prefs.powProfile[n].est );
        doc[n]["let"]       =  value2timeStr( g_prefs.powProfile[n].let );
        doc[n]["req"]       = g_prefs.powProfile[n].req;
        doc[n]["opt"]       = g_prefs.powProfile[n].opt;

        DEBUG_PRINT("EST(%u): %u -> %s\n", n, g_prefs.powProfile[n].est, g_prefs.powProfile[n].est_s);
        DEBUG_PRINT("LET(%u): %u -> %s\n", n, g_prefs.powProfile[n].let, g_prefs.powProfile[n].let_s);
    }

    String resp;
    if (serializeJsonPretty(doc, resp) == 0) {
        DEBUG_PRINT(PSTR("Failed to serialize"));
    }

    DEBUG_PRINT(PSTR("Profiles(%u): \n%s\n"),N_POW_PROFILES, resp.c_str());
    replyOKWithMsg(  resp);
}

void handleSetConfig()
{
     DEBUG_PRINT("handleSetConfig:\n");
     
  for ( int n = 0; n < http_server.args(); ++n) {
        String pName = http_server.argName(n);
        String pVal = http_server.arg(n);
        //float value = atof( p1Val.c_str() );
        DEBUG_PRINT("p%02d Name: %s  val: %s\n", n, pName.c_str(), pVal.c_str() );


#define setPrefStr( __prf, __v )  replaceString( ((char**)&(g_prefs.__prf) ), __v.c_str() )
#define setPref( __prf, __v )  g_prefs.__prf = __v
#define pSWITCH(__key, __val )  { String _k = __key; String _v = __val; if ( false ) {
#define pCASE_str( __prf )  } else if (pName == String( #__prf ) ){  setPrefStr( __prf, pVal )
#define pCASE_unsigned( __prf )  } else if (pName == String( #__prf ) ){ setPref( __prf, atoi(pVal.c_str()) )
#define pCASE_bool( __prf )  } else if (pName == String( #__prf ) ){  setPref( __prf, (pVal == "true") )
#define pDEFAULT } else {
#define pEND }}

        pSWITCH( pName, pVal ) 
          pCASE_str( hostname );
          pCASE_str( mqtt_broker ); 
          pCASE_unsigned( mqtt_broker_port ); 
          pCASE_str( mqtt_user );
          pCASE_str( ota_passwd ); 
          pCASE_str( device_name ); 
          pCASE_str( model_name ); 
          pCASE_unsigned( serialNr ); 
          pCASE_unsigned( modelVariant ); 
          pCASE_unsigned( devType ); 
          pCASE_unsigned( maxPwr );
          pCASE_unsigned( updateTime ); 
          pCASE_unsigned( defCharge ); 
          pCASE_bool( intr ); 
          pCASE_bool( use_oled ); 
        pDEFAULT
          replyNotFound(  "ERR");
            return; 
        pEND
    }
    savePrefs();
    
    replyOK();
    
}

void handleSetTimer()
{
    long sw_time= 0;
    bool armed = false;
    bool mode = false;
    bool daily = false;
    long interval = 0;

    int tmr = -1;
    for( int n = 0; n < http_server.args(); ++n)
    {
        String pName = http_server.argName(n);
        String pVal = http_server.arg(n);
        int value= atoi( pVal.c_str() );

        DEBUG_PRINT("p%dName: %s  val: %s\n",n, pName.c_str(), pVal.c_str() );
        if (pName == String("timer"))           { tmr = value&N_TMR_PROFILES;
                } else if (pName == String("sw_time"))      { sw_time  = TimeClk::timeStr2Value(pVal.c_str());
                } else if (pName == String("interval"))     { interval   = value;
                } else if (pName == String("armed"))        { armed   = pVal;
                } else if (pName == String("switchmode"))   { mode   = pVal;
                } else if (pName == String("everyday"))     { daily   = pVal;
                } else {
                }
    }

    if ( tmr>=0)    g_prefs.tmrProfile[tmr] = uTimerCfg(sw_time, interval, mode, armed, daily );


    replyOKWithJSON(  "{}");
}

void handleSetProfile()
{
    unsigned earliestStart = 0;
    unsigned latestStop    = 0;

    unsigned requestedEnergy = 0 KWh;
    unsigned optionalEnergy  = 0 KWh;
    bool     timeOfDay       = false;
    bool     timeframe       = false;

    int prof_idx = -1;
    for( int n = 0; n < http_server.args(); ++n)
    {
        String pName = http_server.argName(n);
        String pVal = http_server.arg(n);
        int value= atoi( pVal.c_str() );

        DEBUG_PRINT("p%dName: %s  val: %s\n",n, pName.c_str(), pVal.c_str() );

        if (pName == String("requested"))           { requestedEnergy = value;
        } else if (pName == String("optional"))     { optionalEnergy  = value;
        } else if (pName == String("startTime"))    { earliestStart   = TimeClk::timeStr2Value(pVal.c_str());
        } else if (pName == String("endTime"))      { latestStop      = TimeClk::timeStr2Value(pVal.c_str());
        } else if (pName == String("profile"))      { prof_idx        = value %N_POW_PROFILES;
        } else if (pName == String("ToD"))          { timeOfDay       = true;
        } else if (pName == String("timeframe"))    { timeframe       = true;
        } else {
        }
    }

    if ( prof_idx>=0)    g_prefs.powProfile[prof_idx % N_POW_PROFILES] = PowProfile(
                                      timeframe,  timeOfDay, PowProfile::ARMD, PowProfile::ONCE, 
                                      earliestStart, latestStop, requestedEnergy, optionalEnergy );

    for(unsigned n=0;n < N_POW_PROFILES;++n) {
               dump_profile( g_prefs.powProfile[n] );
    }

    savePrefs();
    replyOKWithJSON(  "{}");
}

void requestProfile(unsigned i_profile)
{
    PowProfile prof = g_prefs.powProfile[i_profile % N_POW_PROFILES];
    unsigned long _now = getTime();
    //unsigned daytime = _now%(1 DAY);
    unsigned long est = prof.est;
    unsigned long let = prof.let;

    if (prof.timeOfDay) {
        DEBUG_PRINT("TimeOfDayPlan: now: %lu est: %lu let: %lu req:%u opt:%u\n", _now, est, let, prof.req, prof.opt);
        est = TimeClk::daytime2unixtime( est, _now);
        let = TimeClk::daytime2unixtime( let, _now);
    } else {
        // relatve time
        DEBUG_PRINT("RelativePlan: now: %lu est: %lu let: %lu req:%u opt:%u\n", _now, est, let, prof.req, prof.opt);
        if( let == 0){      //approximate end time =
            est = 0;
            unsigned ereq = (prof.opt>0 ? prof.opt : prof.req);
            let = g_pow->calcOnTime( ereq, g_prefs.assumed_power);
            DEBUG_PRINT("calc LET form req:%u at %uW)=> LET:%lu\n",  ereq, g_prefs.assumed_power, let );
        }
        let += _now;
        est += _now;
    }
    if ( est < _now ) {
        DEBUG_PRINT(" est < now => add one day\n");
        est += (1 DAY);
        let += (1 DAY);
    }
    if ( let < _now ) {
        DEBUG_PRINT(" let < now => add one day\n");
        let += (1 DAY);
    }
    DEBUG_PRINT("plan: now: %lu est: %lu let: %lu req:%u opt:%u\n", _now, est, let, prof.req, prof.opt);
    g_semp->modifyPlan(0, _now, prof.req, prof.opt, est, let );
}

void mkStat( String& resp) {
    PlanningData* activePlan = g_semp->getActivePlan(); 
    StaticJsonDocument<512> stat;
    stat["device_name"] = g_prefs.device_name;

    // Set the values in the document
    stat["voltage"]     = g_pow->voltage;
    stat["pwr"]         = g_pow->activePwr;
    stat["current"]     = g_pow->current;
    stat["switchState"] = g_pow->relayState ? "ON" : (activePlan ? "pending" :"OFF") ;
    stat["ledState"]    = g_pow->ledState ? "ON" : "OFF";
    stat["avrPwr"]      = g_pow->averagePwr;
    stat["appPwr"]      = g_pow->apparentPwr;
    stat["pwrFactor"]   = g_pow->pwrFactor;

    
    stat["online"]      =  g_pow->online ? "online" : "offline";
    unsigned  req_chg = 0;
    long let =  0;
    if ( activePlan ) {
      if (activePlan->is_timebased() ){
        req_chg =  activePlan->m_minOnTime;
        stat["reqUnit"]   =" s";
      } else {
        req_chg = activePlan->m_requestedEnergy;
        stat["reqUnit"]   =" Wh";
      }

      let =  activePlan->m_latestEnd;
    }
    stat["rchg"]  = req_chg;
    stat["latestEnd"] = g_Clk.getTimeStringS( let );
   
    if (serializeJson(stat, resp) == 0) {
        DEBUG_PRINT(PSTR("Failed to serialize"));
    }
}

void handleStat() {
    //   String res = "{\"voltage":"pow_a","pwr":"350", "rchg":"345" })";
    String resp;
    mkStat( resp);
    replyOKWithJSON(  resp);
}




//-------------
/*
  FSBrowser - A web-based FileSystem Browser for ESP8266 filesystems

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WebServer library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  See readme.md for more information.
 */


////////////////////////////////
// Request handlers

/*
   Return the FS type, status and size info
 */
void handleStatus() {
    DEBUG_PRINT("handleStatus");
    FSInfo fs_info;
    String json;
    json.reserve(128);

    json = "{\"type\":\"";
    json += fsName;
    json += "\", \"isOk\":";
    if (fsOK) {
        fileSystem->info(fs_info);
        json += F("\"true\", \"totalBytes\":\"");
        json += fs_info.totalBytes;
        json += F("\", \"usedBytes\":\"");
        json += fs_info.usedBytes;
        json += "\"";
    } else {
        json += "\"false\"";
    }
    json += F(",\"unsupportedFiles\":\"");
    json += unsupportedFiles;
    json += "\"}";

    replyOKWithJSON(  json);
}


/*
   Return the list of files in the directory specified by the "dir" query string parameter.
   Also demonstrates the use of chuncked responses.
 */
void handleFileList() {
    if (!fsOK) {
        return replyServerError(FPSTR(FS_INIT_ERROR));
    }

    if (!http_server.hasArg("dir")) {
        return replyBadRequest(F("DIR ARG MISSING"));
    }

    String path = http_server.arg("dir");
    if (path != "/" && !fileSystem->exists(path)) {
        return replyBadRequest("BAD PATH");
    }

    DEBUG_PRINT("handleFileList: %s\n", path.c_str());
    Dir dir = fileSystem->openDir(path);
    path.clear();

    // use HTTP/1.1 Chunked response to avoid building a huge temporary string
    if (!http_server.chunkedResponseModeStart(200, "text/json")) {
        http_server.send(505, F("text/html"), F("HTTP1.1 required"));
        return;
    }

    // use the same string for every line
    String output;
    output.reserve(64);
    while (dir.next()) {
        if (output.length()) {
            // send string from previous iteration
            // as an HTTP chunk
            http_server.sendContent(output);
            output = ',';
        } else {
            output = '[';
        }

        output += "{\"type\":\"";
        if (dir.isDirectory()) {
            output += "dir";
        } else {
            output += F("file\",\"size\":\"");
            output += dir.fileSize();
        }

        output += F("\",\"name\":\"");
        // Always return names without leading "/"
        if (dir.fileName()[0] == '/') {
            output += &(dir.fileName()[1]);
        } else {
            output += dir.fileName();
        }

        output += "\"}";
    }

    // send last string
    output += "]";
    http_server.sendContent(output);
    http_server.chunkedResponseFinalize();
}


/*
   Read the given file from the filesystem and stream it back to the client
 */
bool handleFileRead(String path) {
    DEBUG_PRINT("handleFileRead: %s\n", path.c_str());
    if (!fsOK) {
        replyServerError(FPSTR(FS_INIT_ERROR));
        return true;
    }

    if (path.endsWith("/")) {
        path += "index.htm";
    }

    String contentType;
    if (http_server.hasArg("download")) {
        contentType = F("application/octet-stream");
    } else {
        contentType = mime::getContentType(path);
    }

    if (!fileSystem->exists(path)) {
        // File not found, try gzip version
        path = path + ".gz";
    }
    if (fileSystem->exists(path)) {
        File file = fileSystem->open(path, "r");
        if (http_server.streamFile(file, contentType) != file.size()) {
            DEBUG_PRINT("Sent less data than expected!\n");
        }
        file.close();
        return true;
    }

    return false;
}


/*
   As some FS (e.g. LittleFS) delete the parent folder when the last child has been removed,
   return the path of the closest parent still existing
 */
String lastExistingParent(String path) {
    while (!path.isEmpty() && !fileSystem->exists(path)) {
        if (path.lastIndexOf('/') > 0) {
            path = path.substring(0, path.lastIndexOf('/'));
        } else {
            path = String();  // No slash => the top folder does not exist
        }
    }
    DEBUG_PRINT("Last existing parent: %s\n", path.c_str());
    return path;
}

/*
   Handle the creation/rename of a new file
   Operation      | req.responseText
   ---------------+--------------------------------------------------------------
   Create file    | parent of created file
   Create folder  | parent of created folder
   Rename file    | parent of source file
   Move file      | parent of source file, or remaining ancestor
   Rename folder  | parent of source folder
   Move folder    | parent of source folder, or remaining ancestor
 */
void handleFileCreate() {
    if (!fsOK) {
        return replyServerError(FPSTR(FS_INIT_ERROR));
    }

    String path = http_server.arg("path");
    if (path.isEmpty()) {
        return replyBadRequest(F("PATH ARG MISSING"));
    }

    if (path == "/") {
        return replyBadRequest("BAD PATH");
    }
    if (fileSystem->exists(path)) {
        return replyBadRequest(F("PATH FILE EXISTS"));
    }

    String src = http_server.arg("src");
    if (src.isEmpty()) {
        // No source specified: creation
        DEBUG_PRINT("handleFileCreate: %s\n", path.c_str());
        if (path.endsWith("/")) {
            // Create a folder
            path.remove(path.length() - 1);
            if (!fileSystem->mkdir(path)) {
                return replyServerError(F("MKDIR FAILED"));
            }
        } else {
            // Create a file
            File file = fileSystem->open(path, "w");
            if (file) {
                file.write((const char *)0);
                file.close();
            } else {
                return replyServerError(F("CREATE FAILED"));
            }
        }
        if (path.lastIndexOf('/') > -1) {
            path = path.substring(0, path.lastIndexOf('/'));
        }
        replyOKWithMsg(path);
    } else {
        // Source specified: rename
        if (src == "/") {
            return replyBadRequest("BAD SRC");
        }
        if (!fileSystem->exists(src)) {
            return replyBadRequest(F("SRC FILE NOT FOUND"));
        }

        DEBUG_PRINT("handleFileCreate: %s from %s\n", path.c_str() , src.c_str());

        if (path.endsWith("/")) {
            path.remove(path.length() - 1);
        }
        if (src.endsWith("/")) {
            src.remove(src.length() - 1);
        }
        if (!fileSystem->rename(src, path)) {
            return replyServerError(F("RENAME FAILED"));
        }
        replyOKWithMsg(lastExistingParent(src));
    }
}


/*
   Delete the file or folder designed by the given path.
   If it's a file, delete it.
   If it's a folder, delete all nested contents first then the folder itself

   IMPORTANT NOTE: using recursion is generally not recommended on embedded devices and can lead to crashes (stack overflow errors).
   This use is just for demonstration purpose, and FSBrowser might crash in case of deeply nested filesystems.
   Please don't do this on a production system.
 */
void deleteRecursive(String path) {
    File file = fileSystem->open(path, "r");
    bool isDir = file.isDirectory();
    file.close();

    // If it's a plain file, delete it
    if (!isDir) {
        fileSystem->remove(path);
        return;
    }

    // Otherwise delete its contents first
    Dir dir = fileSystem->openDir(path);

    while (dir.next()) {
        deleteRecursive(path + '/' + dir.fileName());
    }

    // Then delete the folder itself
    fileSystem->rmdir(path);
}


/*
   Handle a file deletion request
   Operation      | req.responseText
   ---------------+--------------------------------------------------------------
   Delete file    | parent of deleted file, or remaining ancestor
   Delete folder  | parent of deleted folder, or remaining ancestor
 */
void handleFileDelete() {
    if (!fsOK) {
        return replyServerError(FPSTR(FS_INIT_ERROR));
    }

    String path = http_server.arg(0);
    if (path.isEmpty() || path == "/") {
        return replyBadRequest("BAD PATH");
    }

    DEBUG_PRINT("handleFileDelete: %s\n", path.c_str());
    if (!fileSystem->exists(path)) {
        return replyNotFound(FPSTR(FILE_NOT_FOUND));
    }
    deleteRecursive(path);

    replyOKWithMsg(lastExistingParent(path));
}

/*
   Handle a file upload request
 */
void handleFileUpload() {
    if (!fsOK) {
        return replyServerError(FPSTR(FS_INIT_ERROR));
    }
    if (http_server.uri() != "/edit") {
        return;
    }
    HTTPUpload& upload = http_server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        // Make sure paths always start with "/"
        if (!filename.startsWith("/")) {
            filename = "/" + filename;
        }
        DEBUG_PRINT("handleFileUpload Name: %s\n", filename.c_str());
        uploadFile = fileSystem->open(filename, "w");
        if (!uploadFile) {
            return replyServerError(F("CREATE FAILED"));
        }
        DEBUG_PRINT("Upload: START, filename:  %s\n", filename.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            size_t bytesWritten = uploadFile.write(upload.buf, upload.currentSize);
            if (bytesWritten != upload.currentSize) {
                return replyServerError(F("WRITE FAILED"));
            }
        }
        DEBUG_PRINT("Upload: WRITE, Bytes: %d\n", upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
        }
        DEBUG_PRINT("Upload: END, Size: %d‘\n", upload.totalSize);
    }
}


/*
   The "Not Found" handler catches all URI not explicitely declared in code
   First try to find and return the requested file from the filesystem,
   and if it fails, return a 404 page with debug information
 */
void handleNotFound() {
    if (!fsOK) {
        return replyServerError(FPSTR(FS_INIT_ERROR));
    }

    String uri = ESP8266WebServer::urlDecode(http_server.uri()); // required to read paths with blanks

    if (handleFileRead(uri)) {
        return;
    }

    // Dump debug data
    String message;
    message.reserve(100);
    message = F("Error: File not found\n\nURI: ");
    message += uri;
    message += ("\nMethod: ");
    message += (http_server.method() == HTTP_GET) ? "GET" : "POST";
    message += ("\nArguments: ");
    message += http_server.args();
    message += '\n';
    for (uint8_t i = 0; i < http_server.args(); i++) {
        message += (" NAME:");
        message += http_server.argName(i);
        message += ("\n VALUE:");
        message += http_server.arg(i);
        message += '\n';
    }
    message += "path=";
    message += http_server.arg("path");
    message += '\n';
    DEBUG_PRINT(message.c_str());

    return replyNotFound(message);
}

/*
   This specific handler returns the index.htm (or a gzipped version) from the /edit folder.
   If the file is not present but the flag INCLUDE_FALLBACK_INDEX_HTM has been set, falls back to the version
   embedded in the program code.
   Otherwise, fails with a 404 page with debug information
 */
void handleGetEdit() {
    if (handleFileRead("/edit/index.htm")) {
        return;
    }

#ifdef INCLUDE_FALLBACK_INDEX_HTM
    http_server.sendHeader(F("Content-Encoding"), "gzip");
    http_server.send(200, "text/html", index_htm_gz, index_htm_gz_len);
#else
    replyNotFound(FPSTR(FILE_NOT_FOUND));
#endif

}

void setupFSbrowser(void) {


    ////////////////////////////////
    // WEB SERVER FS Part init
    // format directory
    http_server.on("/fmtfs", HTTP_GET, [](){ DEBUG_PRINT("formatting FS\n"); fileSystem->format();   replyOKWithMsg("Format Complete");});

    // Filesystem status
    http_server.on("/status", HTTP_GET, handleStatus);

    // List directory
    http_server.on("/list", HTTP_GET, handleFileList);

    // Load editor
    http_server.on("/edit", HTTP_GET, handleGetEdit);

    // Create file
    http_server.on("/edit",  HTTP_PUT, handleFileCreate);

    // Delete file
    http_server.on("/edit",  HTTP_DELETE, handleFileDelete);

    // Upload file
    // - first callback is called after the request has ended with all parsed arguments
    // - second callback handles file upload at that location
    http_server.on("/edit",  HTTP_POST, replyOK, handleFileUpload);

    // Default handler for all URIs not defined above
    // Use it to read files from filesystem
    http_server.onNotFound(handleNotFound);

}
