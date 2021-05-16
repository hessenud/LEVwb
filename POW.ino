
#include "POW.h"




void unblockingDelay(unsigned long mseconds) {
    unsigned long timeout = millis();
    while ((millis() - timeout) < mseconds) delay(1);
}


POW::POW( uSEMP* i_semp, AppCb_t i_appCb )
:m_applicationCB(i_appCb),m_sense(0), m_semp(i_semp), m_activePwr(0), m_voltage(0), m_current(0)
, m_apparentPwr(0), m_averagePwr(0), m_pwrFactor(0), m_cumulatedEnergy(0),_cumulatedEnergy(0)
, m_activeProfile(0), m_ad_state(AD_OFF), online(true)
{
    m_last_update=millis();
}  

void POW::setup() 
{
    loadCalibration(this);
    m_semp->startService();

    // Open the relay to switch on the load
    m_relayState = RELAY_OFF;
    m_semp->setEmState( EM_OFF );
    m_semp->acceptEMSignal( online );

    m_pwrIdx = 0;
    m_averagePwr = m_minPwr=m_maxPwr = 0;

    m_ledState   = LED_OFF;
    m_relayState = RELAY_OFF;
    pinMode(m_ledPin, OUTPUT);
    pinMode(m_relayPin, OUTPUT);
    pinMode(m_buttonPin, INPUT);

    digitalWrite(m_relayPin, m_relayLogic^relayState );
    digitalWrite(m_ledPin,   m_relayLogic^ledState );


    for(unsigned  n=0; n<DIM(m_powers); ++n)
    { 
        m_powers[n] = 0;
    }

    m_sumPwr = 0;

}


void POW::setprefs( unsigned i_cumulatedEnergy, double i_pwrMultiplier, double i_currentMultiplier, double i_voltageMultiplier)
{
    m_cumulatedEnergy = i_cumulatedEnergy;
    if ( m_sense ) {
        m_sense->setPowerMultiplier( i_pwrMultiplier );
        m_sense->setCurrentMultiplier( i_currentMultiplier );
        m_sense->setVoltageMultiplier( i_voltageMultiplier );
    }

}

void POW::calibrate(    double expectedVoltage, double expectedPwr, double expectedCurrent )
{

    DEBUG_PRINT("[HLW] Before calibration\n");
    DEBUG_PRINT("[HLW] current multiplier : %f\n", currentMultiplier());
    DEBUG_PRINT("[HLW] voltage multiplier : %f\n", voltageMultiplier());
    DEBUG_PRINT("[HLW] power multiplier   : %f\n", pwrMultiplier());
    // m_sense->resetMultipliers();    unblockingDelay(2*UPDATE_TIME);
    // if multipliers are reset, then at least wait for one Update timebefor reading a new value
    if (m_sense && (expectedVoltage!=0) && (expectedPwr != 0) && (expectedCurrent !=0) ) {
        // Calibrate using a 60W bulb (pure resistive) on a 230V line
        m_sense->expectedActivePower(expectedPwr); //double
        m_sense->expectedVoltage(expectedVoltage);
        m_sense->expectedCurrent(expectedCurrent ); // (expectedCurrent = double(expectedPwr) / expectedVoltage)) ;
    }
    // Show corrected factors
    DEBUG_PRINT("[HLW] current  : %f\n",(float)expectedCurrent);
    DEBUG_PRINT("[HLW] voltage  : %f\n",expectedVoltage);
    DEBUG_PRINT("[HLW] power    : %f\n\n",expectedPwr);
    DEBUG_PRINT("\n");

    saveCalibration(this);

    DEBUG_PRINT("[HLW] After calibration\n");
    DEBUG_PRINT("[HLW] current multiplier : %f\n", currentMultiplier());
    DEBUG_PRINT("[HLW] voltage multiplier : %f\n", voltageMultiplier());
    DEBUG_PRINT("[HLW] power multiplier   : %f\n", pwrMultiplier());
    unblockingDelay(1000);
}




unsigned POW::calcOnTime( unsigned requestedEnergy, unsigned avr_pwr)
{
    return (Wh2Ws(requestedEnergy) / (avr_pwr?avr_pwr:1)) // OnTime is in seconds => Wh2Ws
            +180; // + 3 Minutes to give SEMP EM a bit time for planning an control
}


void POW::handleTimeReq()
{
    DEBUG_PRINT("\n\n\n\n\n\n\n\nhandleTimeReq\n\n\n\n\n\n" );

    unsigned earliestStart = 0;
    unsigned latestStop    = 0;

    unsigned minOnTime = 0 KWh;
    unsigned maxOnTime = 0 KWh;

    unsigned long _now = getTime();
    unsigned dayoffset = _now - (_now%(1 DAY));
    int    planHandle = -1;

    for( int n = 0; n < http_server.args(); ++n)
    {
        String p1Name = http_server.argName(n);
        String p1Val = http_server.arg(n);
        double value= atof( p1Val.c_str() );

        DEBUG_PRINT("p%dName: %s  val: %s\n",n, p1Name.c_str(), p1Val.c_str() );

        if (p1Name == String("min"))          { minOnTime = value;
        } else if (p1Name == String("max"))          { maxOnTime  = value;
        } else if (p1Name == String("start"))        { earliestStart   = value + _now;
        } else if (p1Name == String("end"))          { latestStop      = value + _now;
        } else if (p1Name == String("startTime"))    { earliestStart   = TimeClk::timeStr2Value(p1Val.c_str()) + dayoffset;
        } else if (p1Name == String("endTime"))      { latestStop      = TimeClk::timeStr2Value(p1Val.c_str()) + dayoffset;
        } else if (p1Name == String("fastEnd"))      { latestStop      = _now +  minOnTime;
        } else if (p1Name == String("plan"))         { planHandle      = atoi(p1Val.c_str()) ;
        } else {
        }
    }
    if ( earliestStart < _now ) {
        earliestStart += (1 DAY);
        latestStop += (1 DAY);
    }
    if ( latestStop < _now ) {
        latestStop += (1 DAY);
    }
    DEBUG_PRINT("POW now: %s EST: %s  LET: %s\n", String(TimeClk::getTimeString(_now)).c_str(), String(TimeClk::getTimeString(earliestStart)).c_str()
            , String(TimeClk::getTimeString(latestStop)).c_str());
    if (m_semp && planHandle <0 ) {
        planHandle = m_semp->requestTime(_now, minOnTime,  maxOnTime,  earliestStart, latestStop );
    } else {
        m_semp->modifyPlanTime(planHandle, _now, minOnTime,  maxOnTime,  earliestStart, latestStop );
    }
    DEBUG_PRINT("POW requested Energy on plan %d\n", planHandle);


    String resp = String("[HLW] Active Power (W)    : ") + String(activePwr) +
            String("\n[HLW] Voltage (V)         : ") + String(voltage) +
            String("\n[HLW] Current (A)         : ") + String(current) +
            String("\n[HLW] Energy") +
            String("\n[HLW] \ttotal     : ") + String(_cumulatedEnergy) +
            String("\n[HLW] \trequested : ") + String(minOnTime) +
            String("\n[HLW] \toptional  : ") + String(maxOnTime)  +
            String("\n[HLW] \tstart     : ") + String(TimeClk::getTimeString(earliestStart))   +
            String("\n[HLW] \tend       : ") + String(TimeClk::getTimeString(latestStop))      +
            +"\n";

    char buffer[resp.length() + 10*40];
    unsigned wp = sprintf(buffer,"\n%s\n",resp.c_str());
    m_semp->dumpPlans(&buffer[wp-1]);

    DEBUG_PRINT("%s\n", buffer);

    replyOKWithMsg(resp);

}

void POW::requestProfile() {

}



void POW::handleSimpleReq()
{
    for( int n = 0; n < http_server.args(); ++n)
    {
        String p1Name = http_server.argName(n);
        String p1Val = http_server.arg(n);
        double value= atof( p1Val.c_str() );

        DEBUG_PRINT("p%dName: %s  val: %s\n",n, p1Name.c_str(), p1Val.c_str() );

        if (p1Name == String("std")) {
            procAdRequest();
        } else  {
        }
    }

}

void POW::suspendAutodetect()
{
    // override AD state to suppress unintended overwrite
    m_ad_state = AD_EXTERNAL;
}

void POW::handleEnergyReq()
{
    DEBUG_PRINT("\n\n\n\n\n\n\n\nhandleEnergyReq\n\n\n\n\n\n" );

    unsigned earliestStart = 0;
    unsigned latestStop    = 0;

    unsigned requestedEnergy = 0 KWh;
    unsigned optionalEnergy  = 0 KWh;

    unsigned long _now = getTime();
    unsigned dayoffset = _now - (_now%(1 DAY));
    int    planHandle = -1;

    for( int n = 0; n < http_server.args(); ++n)
    {
        String p1Name = http_server.argName(n);
        String p1Val = http_server.arg(n);
        double value= atof( p1Val.c_str() );

        DEBUG_PRINT("p%dName: %s  val: %s\n",n, p1Name.c_str(), p1Val.c_str() );

        if (p1Name == String("requested"))           { requestedEnergy = value;
        } else if (p1Name == String("optional"))     { optionalEnergy  = value;
        } else if (p1Name == String("start"))        { earliestStart   = value + _now;
        } else if (p1Name == String("end"))          { latestStop      = value + _now;
        } else if (p1Name == String("startTime"))    { earliestStart   = TimeClk::timeStr2Value(p1Val.c_str()) + dayoffset;
        } else if (p1Name == String("endTime"))      { latestStop      = TimeClk::timeStr2Value(p1Val.c_str()) + dayoffset;
        } else if (p1Name == String("fastEnd"))      { latestStop      = _now +   calcOnTime(requestedEnergy, g_prefs.assumed_power );
        } else if (p1Name == String("plan"))         { planHandle      = atoi(p1Val.c_str()) ;
        } else {
        }
    }
    if ( earliestStart < _now ) {
        earliestStart += (1 DAY);
        latestStop += (1 DAY);
    }
    if ( latestStop < _now ) {
        latestStop += (1 DAY);
    }
    DEBUG_PRINT("POW now: %s EST: %s  LET: %s\n", String(TimeClk::getTimeString(_now)).c_str(), String(TimeClk::getTimeString(earliestStart)).c_str()
            , String(TimeClk::getTimeString(latestStop)).c_str());
    if (planHandle <0 ) {
        planHandle = m_semp->requestEnergy(_now, requestedEnergy,  optionalEnergy,  earliestStart, latestStop );
    } else {
        m_semp->modifyPlan(planHandle, _now, requestedEnergy,  optionalEnergy,  earliestStart, latestStop );
    }
    DEBUG_PRINT("POW requested Energy on plan %d\n", planHandle);

    // override AD state to suppress unintended overwrite
    suspendAutodetect();

    String resp = String("[HLW] Active Power (W)    : ") + String(activePwr) +
            String("\n[HLW] Voltage (V)         : ") + String(voltage) +
            String("\n[HLW] Current (A)         : ") + String(current) +
            String("\n[HLW] Energy") +
            String("\n[HLW] \ttotal     : ") + String(_cumulatedEnergy) +
            String("\n[HLW] \trequested : ") + String(requestedEnergy) +
            String("\n[HLW] \toptional  : ") + String(optionalEnergy)  +
            String("\n[HLW] \tstart     : ") + String(TimeClk::getTimeString(earliestStart))   +
            String("\n[HLW] \tend       : ") + String(TimeClk::getTimeString(latestStop))      +
            +"\n";

    char buffer[resp.length() + 10*40];
    unsigned wp = sprintf(buffer,"\n%s\n",resp.c_str());
    m_semp->dumpPlans(&buffer[wp-1]);
    _DEBUG_PRINT("response:\n%s\n",  buffer );

    DEBUG_PRINT("%s\n", buffer);
    handleStat();
    _DEBUG_PRINT(" requested Energy end\n" );

}


void POW::handleCalReq()
{
    double expectedVoltage = REF_VOLT;
    double expectedPwr     = REF_PWR;
    double expectedCurrent = REF_PWR /REF_VOLT;

    String resp = String("[HLW] Active Power (W)    : ") + String(activePwr) +
            String("\n[HLW] Voltage (V)         : ") + String(voltage) +
            String("\n[HLW] Current (A)         : ") + String(current) +
            String("\n[HLW] Apparent Power (VA) : ") + String(apparentPwr) +
            String("\n[HLW] Power Factor (%)    : ") + String(pwrFactor) +
            String("\n[HLW] New current multiplier : ") + String(currentMultiplier()) +
            String("\n[HLW] New voltage multiplier : ") + String(voltageMultiplier()) +
            String("\n[HLW] New power multiplier   : ") + String(pwrMultiplier()) + "\n";

    for( int n = 0; n < http_server.args(); ++n)
    {
        String p1Name = http_server.argName(n);
        String p1Val = http_server.arg(n);
        double value= atof( p1Val.c_str() );
        DEBUG_PRINT("p%dName: %s  val: %s\n",n, p1Name.c_str(), p1Val.c_str() );
        if (p1Name == String("power"))           { expectedPwr=  value; resp = "OK";
        } else if (p1Name == String("voltage"))  { expectedVoltage = value; resp = "OK";
        } else if (p1Name == String("current"))  { expectedCurrent = value; resp = "OK";
        } else {
        }
    }

    calibrate( expectedVoltage, expectedPwr, expectedCurrent );

    replyOKWithMsg( resp);
}


void POW::handleSimReq()
{
    String resp;
    for( int n = 0; n < http_server.args(); ++n)
    {
        String p1Name = http_server.argName(n);
        String p1Val = http_server.arg(n);
        //float value= atof( p1Val.c_str() );
        DEBUG_PRINT("p%dName: %s  val: %s\n",n, p1Name.c_str(), p1Val.c_str() );
        if (p1Name == String("pwron"))         {  resp = "pwron";  setSimPwr( true );
        } else if (p1Name == String("pwroff")) {  resp = "pwroff"; setSimPwr( false );
        } else if (p1Name == String("factor")) {  resp = "factor: " + p1Val;
        } else {
            replyNotFound( resp );
            return;
        }
    }
    DEBUG_PRINT("PWR req: %s\n",resp.c_str());

    replyOKWithMsg( resp + "OK!");
}

void POW::handlePwrReq()
{
    bool def = true;
    String resp = String("<?xml version=\"1.0\"?>\r\n"
            "<Stat  xmlns=\"http://www.sma.de/communication/schema/SEMP/v1\">\r\n"
            " <led> ") + String(!ledState) +
                    String("</led>\r\n  <switch> ") + String(relayState) +
                    String("</switch>\r\n <activePower> ") + String(activePwr) +
                    String("</activePower>\r\n  <averagePower> ") + String(averagePwr) +
                    String("</averagePower>\r\n  <voltage> ") + String(voltage) +
                    String("</voltage>\r\n  <current> ") + String(current) +
                    String("</current>\r\n  <apparentPower> ") + String(apparentPwr) +
                    String("</apparentPower>\r\n  <powerFactor> ") + String(pwrFactor) +
                    String("</powerFactor>\r\n"
                            "</Stat>\r\n"
                    );

    for( int n = 0; n < http_server.args(); ++n)
    {
        def = false;
        String p1Name = http_server.argName(n);
        String p1Val = http_server.arg(n);
        //float value= atof( p1Val.c_str() );
        DEBUG_PRINT("p%dName: %s  val: %s\n",n, p1Name.c_str(), p1Val.c_str() );
        if (p1Name == String("power"))           {      resp = String("power:") + String(activePwr); //constantly 15W Power
        } else if (p1Name == String("voltage"))  {      resp = String("volt:") + String(voltage); //constantly 230V Power
        } else if (p1Name == String("current"))  {      resp = String("current:") + String(current); //constantly 15W Power
        } else if (p1Name == String("apparent")) {      resp = String("apparent:") + String(apparentPwr); //constantly 15W Power
        } else if (p1Name == String("factor"))   {      resp = String("factor:") + String(pwrFactor); //
        } else {
            replyNotFound( resp );
            return;
        }
    }
    DEBUG_PRINT("PWR req: %s\n",resp.c_str());
    if (def)  replyOKWithXml(resp);
    else      replyOKWithMsg(resp);
}

PowProfile  POW::findTimeFrame( bool i_timf, unsigned i_req  )
{
    const unsigned extraStartTime = 15; // 15s extra start time, if a timeframe has already begun
    // earliest start is NOW
    // find a timeframe, that's LET has enough time left to fulfill the energy need of this request
    // the timeframe should end before <i_let> if i_let is not 0
    // if no matching timeframe is found return empty inactive Timeframe

    unsigned long _now = TimeClk::unixtime2daytime(getTime());
    unsigned long _dayoffset = (_now - (_now%(1 DAY)));

    DEBUG_PRINT("find Frame for %u finish by %s (+%s) now:%s (%lus)\n", i_req,
            Tstr(TimeClk::getTimeString(_now + i_req)), Tstr(TimeClk::getTimeString(i_req)),  Tstr(TimeClk::getTimeString(_now)), _now );

    int _idx = i_timf ? PROFILE_TIMEFRAME : PROFILE_START;
    int _endIdx   = i_timf ?  N_POW_PROFILES : PROFILE_TIMEFRAME;
    /** @todo: find the earliest timeframe
     *  if no frame for today is found, try another run:
     *  for timeOfDay Frames add 1 DAY to EST and LET and try to find a frame the next day
     */

    long dT = 0; ///<  Offset for est and let to find a frame the next day(s)

    bool found = false;
    struct {
        unsigned prfIdx;
        long     startTime;   // modified startTime
        long     endTime;     // modified endTime
    } candidate;
    candidate.prfIdx = 0xdead;
    candidate.startTime = 0;
    candidate.endTime=0;
    do {
        for( int idx = _idx, endIdx = _endIdx; idx < endIdx; ++idx )
        {
            PowProfile& prf = g_prefs.powProfile[idx];
            DEBUG_PRINT("\t");
            dump_profile( prf );
            if ( prf.valid ) {
                _DEBUG_PRINT(" end: %s (%lu) vs end: %s(%lu) req:%s(%u) vs len: %lu \n"
                        , Tstr(TimeClk::getTimeString(prf.let)), prf.let
                        , Tstr(TimeClk::getTimeString(endTime)), endTime
                        , Tstr(TimeClk::getTimeString(prf.let - prf.est)),(prf.let - prf.est)
                        , i_req);

                unsigned long startTime = prf.est + ( prf.timeOfDay ? (_dayoffset + dT)  : _now); // relative timeframes are always in the future
                unsigned long endTime   =  prf.let + ( prf.timeOfDay ? (_dayoffset + dT)  : _now);
                if (startTime < _now) startTime = _now+extraStartTime; //  if timeframe has already startet, give EM some extra time for start and try
                long tfLength = (endTime - startTime);
                if ( tfLength >= long(i_req) ) {
                    DEBUG_PRINT("\t\t possible candidate absolute daytime %d:  i_req:%ds tfLength: %ds\n"
                            , idx,i_req, tfLength);
                    dump_profile( prf );
                    if (!found || (startTime < candidate.startTime) )
                    {
                        candidate.prfIdx =  idx;
                        candidate.startTime = startTime;
                        candidate.endTime = endTime;
                        DEBUG_PRINT("%s candidate: idx:%u :> %s (%u %s) -> %s (%u) \n\t", found ? "better" : "a", idx
                                , Tstr(TimeClk::getTimeString(candidate.startTime)), candidate.startTime, dT>0?"+1d":""
                                , Tstr(TimeClk::getTimeString(candidate.endTime)), candidate.endTime );
                        found = true;
                    }    
                }
            }
        }
        dT += 1 Day; // try again tomorrow
        if ( !found )  DEBUG_PRINT(" no frame found try again tomorrow\n");
    } while (  !found && (dT <= 1 Day) );

    // PowProfile( bool i_tf, bool     i_tod,bool   i_armed, bool i_repeat,  unsigned i_est, unsigned i_let, unsigned i_req, unsigned i_opt )

    return found ? PowProfile(  i_timf 
            ,g_prefs.powProfile[candidate.prfIdx].timeOfDay
            ,true, false
            ,candidate.startTime, candidate.endTime
            ,i_req, 0 )
            : PowProfile() ;
}

void POW::prolongPlan( PlanningData* i_plan )
{
    DEBUG_PRINT(" prolongPlan  by %u\n", g_prefs.ad_prolong_inc);
    myLog->reset( );
    myLog->log(String("prolong Timeframe by") + g_prefs.ad_prolong_inc + "s", true );
    i_plan->updateEnergy(  getTime(), relayState, 0, 0,  g_prefs.ad_prolong_inc);
}

void POW::procAdRequest()
{
    DEBUG_PRINT(" Autodetected  Energy Request\n");
    // a energy request might be pending at this point:
    // the EM Device was switched on while waiting on the EM and a job was programmed (e.g. washingmachine) and started
    // this is not the usual use-case, because normally a Energy request is EIHTER already planned OR created right now
    // so all plans are deleted and a new request generated.
    // this can be handled more intelligently... but after brewing over this, i decided it would not be worth the effort


    PowProfile powPrf = findTimeFrame( PowProfile::TIMF, Wh2Ws(g_prefs.defCharge)/( g_prefs.assumed_power ? g_prefs.assumed_power :1));
    if ( powPrf.valid ) {
        DEBUG_PRINT(" matching timeframe:" ); dump_profile( powPrf );
        m_semp->deleteAllPlans(); 
        setRelay( false );
        m_ad_state = AD_ON;
        if(m_applicationCB) m_applicationCB( APP_REQ, 0);
        unsigned long _now = getTime();
        if ( powPrf.timeframe ) {
            DEBUG_PRINT(" modifyPlanTime(0,...)\n" );
            unsigned dayoffset = _now - (_now%(1 DAY));
            unsigned required = Wh2Ws(g_prefs.defCharge)/( g_prefs.assumed_power ? g_prefs.assumed_power :1);
            //if ( powPrf.est + dayoffset < _now ) dayoffset  += 1 Day; 
            //if ( powPrf.let + dayoffset < _now ) powPrf.let += 1 Day;
            m_semp->modifyPlanTime(0, _now, required,  powPrf.opt,  powPrf.est + dayoffset, powPrf.let + dayoffset );
        } else {
            DEBUG_PRINT(" modifyPlan(0, prf:%d\n", powPrf.est);
            unsigned required = g_prefs.defCharge;
            m_semp->modifyPlan(0, _now, required,  powPrf.opt,  powPrf.est + _now, powPrf.let + _now );
        }
    } else {
        m_ad_state = AD_OFF;
    }
}

ad_event_t POW::autoDetect() {
    ad_event_t ret = AD_IDLE;

    /* if AD_IDLE
     *      detect energy need if request-threshld-power surpassed more than specified time
     * if AD_ON
     *      detect end of energy need by power falling below lower threshold for more than specified time
     *
     * ON detection is based on activePwr to be as quick as possible.
     *    Think of washing machines.. you don't want to have the clothes lying wet inside the drum for hours,
     *    so it's better to detect the request before there is too much water in the drum.
     * OFF detection based on activePwr will prevent the request from beeing ended if a machin does something like
     *    tumbling the clothes in intervals to prevent knittering. The request will be prolonged until
     *    you take the clothes out and or turn the machine off. This is intended
     */


    unsigned long        _now = getTime();

    if ( g_prefs.autoDetect ) {
        DEBUG_PRINT(" autoDetect state: %d   PWR: %u   dt=%lu\n", m_ad_state, activePwr, (_now- m_ad_start) );
        switch ( m_ad_state ) {
        case AD_OFF:
            if (  activePwr > g_prefs.ad_on_threshold )   {
                DEBUG_PRINT(" autoDetect state:AD_OFF  ABOVE THRESHOLD ==> TEST ON\n" );
                m_ad_start = _now; m_ad_state = AD_TEST_ON;
            }
            break;

        case AD_TEST_ON:
            if ( activePwr > g_prefs.ad_on_threshold )   {
                DEBUG_PRINT(" autoDetect state:AD_TEST_ON  ABOVE THRESHOLD  %lu(%u)\n", (_now- m_ad_start), g_prefs.ad_on_time );
                if ((_now- m_ad_start) > g_prefs.ad_on_time)     {
                    DEBUG_PRINT(" autoDetect state:AD_TEST_ON ==> AD_ON\n" );
                    m_ad_state = AD_ON; m_ad_start = _now;
                    ret = AD_REQUEST;
                }
            } else {
                DEBUG_PRINT(" autoDetect state:AD_TEST_ON  BELOW THRESHOLD  ==> AD_OFF\n" );
                m_ad_state = AD_OFF;    m_ad_start = _now;
            }
            break;

        case AD_ON:
            ret = AD_RQ_ACTIVE;
            if ( relayState ) {
                if ( activePwr < g_prefs.ad_off_threshold )  {
                    DEBUG_PRINT(" autoDetect state:AD_ON  BELOW THRESHOLD  ==> AD_TEST_OFF\n" );
                    m_ad_state = AD_TEST_OFF;  m_ad_start = _now;
                }
            }
            break;

        case AD_TEST_OFF:
            ret = AD_RQ_ACTIVE;
            if ( relayState ) {
                if ( activePwr < g_prefs.ad_off_threshold )  {
                    // begin to test for OFF if average is below threshold
                    DEBUG_PRINT(" autoDetect state:AD_TEST_OFF  BELOW THRESHOLD  %lu(%u)\n", (_now- m_ad_start), g_prefs.ad_on_time );

                    if ((_now- m_ad_start)> g_prefs.ad_off_time)    {
                        DEBUG_PRINT(" autoDetect state:AD_TEST_OFF ==> AD_OFF\n" );
                        m_ad_state = AD_OFF;  m_ad_start = _now;
                        ret = AD_END_REQUEST;
                    }
                } else {
                    DEBUG_PRINT(" autoDetect state:AD_TEST_OFF  ABOVE THRESHOLD  ==> AD_ON\n" );
                    m_ad_state = AD_ON; m_ad_start = _now;
                }
            }
            break;
        case AD_EXTERNAL:
            // external request do nothing
            ret = AD_IDLE;
            break;
        }
    }

    return ret;
}

void POW::loop() 
{
    //======================
    //== timekeeping
    unsigned long entry_ts=millis();
    long dt = ( entry_ts - m_last_update);

    //======================
    //   Update Sensors-

    m_sense->loop();  // Polled sensors need some cpu

    //======================
    //==   Update POW
    // The UPDATE_TIME should be at least twice the minimum time for the current or voltage
    // signals to stabilize. Experimentally that's about 1 second.
    if ( dt > UPDATE_TIME) {
        _DEBUG_PRINT("POW update\n");
        m_last_update = entry_ts;
        unsigned _now = getTime();     // world time
        unsigned requestedEnergy = 0;
        unsigned optionalEnergy  = 0;
        PlanningData* activePlan = m_semp->getActivePlan();
        if (activePlan){
            requestedEnergy = activePlan->m_requestedEnergy;
            optionalEnergy  = activePlan->m_optionalEnergy;
        }


        //=====================
        //==   Update measurement
        if ( m_sense ) {
            m_activePwr = relayState ? m_sense->getActivePower(): 0;
            m_voltage   = m_sense->getVoltage();
            m_current   = relayState ? m_sense->getCurrent(): 0;
            m_apparentPwr = relayState ? m_sense->getApparentPower():0;  

            m_powers[m_pwrIdx] = activePwr;

            if ( ++m_pwrIdx >= DIM(m_powers) ) m_pwrIdx = 0;
            {  // min/max average
                unsigned minPwr = unsigned(-1);
                unsigned maxPwr = 0;
                unsigned sumPwr = 0;
                for ( unsigned i=0; i< DIM(m_powers); ++i ) {
                    unsigned pwr = m_powers[i];
                    sumPwr += pwr;
                    if ( pwr > maxPwr) maxPwr = pwr;
                    if ( pwr < minPwr) minPwr = pwr;
                }

                m_averagePwr = sumPwr/DIM(m_powers);
                m_minPwr = minPwr;
                m_maxPwr = maxPwr;
            }
            m_pwrFactor = (int) (100 *  m_sense->getPowerFactor());
        }
#ifdef USE_POW_DBG
        DEBUG_PRINT("[HLW] Active Power (W)    : %u  avr: %u\n", activePwr, averagePwr );
        DEBUG_PRINT("[HLW] Voltage (V)         : %u\n", voltage       );
        DEBUG_PRINT("[HLW] Current (A)         : %f\n", current );
        DEBUG_PRINT("[HLW] Apparent Power (VA) : %u\n", apparentPwr );
        DEBUG_PRINT("[HLW] Power Factor (%)    : %f\n", pwrFactor     );        DEBUG_PRINT("[HLW] _cumulatedEnergy    : %ul\n", _cumulatedEnergy     );
        DEBUG_PRINT("[HLW] cumulatedEnergy    : %u\n", cumulatedEnergy     );
        DEBUG_PRINT("[HLW] requestedEnergy    : %u\n", requestedEnergy     );  
        DEBUG_PRINT("[HLW] optionalEnergy     : %u\n", optionalEnergy     );  
#endif

        //=====================
        //== autodetect
        ad_event_t autoDetectState = autoDetect();
        /*  *
         * if no activePlan -> autodetect a new request   => autoDetectState  == AD_IDLE or AD_REQUEST
         *
         *
         * if activePlan -> its either  requested    => autoDetectState  == AD_IDLE
         *                      or      autodetected => autoDetectState  == AD_RQ_ACTIVE
         *
         * on AD_REQUEST     => procAdRequest  => AD_RQ_ACTIVE
         * on AD_IDLE        => nothing happens
         *                      on AD_REQUEST  -> procAdRequest  => AD_RQ_ACTIVE
         *
         * on AD_RQ_ACTIVE   => assert activePlan
         *                      if prolongInterval set -> if rest-len of activePlan < prolongInterval -> prolong activePlan.let
         * on AD_END_REQUEST => reset the autodetector
         */
        const char* ads2txt[] = {
                "AD_IDLE","AD_REQUEST","AD_RQ_ACTIVE","AD_END_REQUEST"
        };
        DEBUG_PRINT("AD state: %s(%d)\n", ads2txt[autoDetectState], autoDetectState );
        switch( autoDetectState) {
        case  AD_REQUEST: procAdRequest(); 
            break;
        case  AD_END_REQUEST:
            DEBUG_PRINT(" Autodetected  End of Energy Request\n");
            myLog->log("Autodetected  End of Energy Request\n");
            m_semp->resetPlan(); 
            endOfPlan(true);
            break;
        case  AD_RQ_ACTIVE:
            // prolong active Plan if job not completed
            if ( activePlan && ((activePlan->end() - _now) < g_prefs.ad_off_time) ) {
               prolongPlan(activePlan);
            }
            break;
        case  AD_IDLE:
            ;
        }

        //============================
        //== update energy and plans (energy)
        { 
            _cumulatedEnergy += activePwr*dt;  /// Wms  -> 1/(3600*1000) Wh
            while ( _cumulatedEnergy > Wh2Wms(1) ) {

                ++m_cumulatedEnergy;
                _cumulatedEnergy -= Wh2Wms(1);
                if ( requestedEnergy > 0) {
                    --requestedEnergy;
                    m_semp->updateEnergy( _now,  -1,  0 );
                }

                if ( optionalEnergy > 0)  {
                    --optionalEnergy;
                    m_semp->updateEnergy( _now, 0,  -1 );
                }
            }
        }
        //============================
        //== update plans (Timeframes)
        m_semp->updateTime( _now, relayState );
        m_semp->setPwr( averagePwr, m_minPwr, m_maxPwr);
        _DEBUG_PRINT("POW update end\n");
        if(m_applicationCB) m_applicationCB( APP_IDLE, 0);
    }
}



void POW::setRelay(bool i_state )
{
    if ( relayState != i_state ) {
        digitalWrite( relayPin, m_relayLogic^(m_relayState = i_state) );
        m_semp->setEmState( relayState ); // update/acknowledge state
    }
}


/**
 * EM signals to pow the state of SEMP device
 * OFFLINE = no control by EM
 * 
 */
void POW::rxEmState(EM_state_t i_em_state, unsigned /*i_recommendedPwr*/ )
{
    if ( online ) {
        if(i_em_state != EM_OFFLINE) {
          myLog->log((String("POW::rxEmState ") + (i_em_state==EM_ON ? "ON" : (i_em_state == EM_OFF ? "OFF" : "OFFLINE")) ));
          if ( i_em_state == EM_ON ) {
            setRelay(true);
          } else {
            if ( !g_prefs.intr ) setRelay(false);
          }
        }
    }
}

void  POW::endOfPlan(bool i_force  )
{
  // some self -defense against unwanted termination by EM  
    if ( (online && ( m_ad_state == AD_OFF)) || i_force ) {
        DEBUG_PRINT("End OF Plan!!\n");
        online = true; // if plan is forcedly terminated then go online again
        setRelay( false ); ///< this will reflect EM state EM_OFF to semp object
        if(m_applicationCB) m_applicationCB( APP_EOR, 0);
        resetAutoDetectionState();
        m_semp->acceptEMSignal( (online = true) );
    }
}

void POW::setLED(bool i_state )
{
    if ( ledState != i_state ) digitalWrite(m_ledPin, m_ledLogic ^(m_ledState = i_state) );
}

void POW::toggleRelay()
{
    setRelay( !relayState );
}

void POW::toggleLED()
{
    setLED( !ledState );
}

void dump_profile( PowProfile& i_prf )
{
#define value2timeStr( vs )   (snprintf_P( (vs##_s), sizeof(vs##_s), PSTR("%1u.%02lu:%02lu"), (vs/86400L),(((vs)  % 86400L) / 3600), (((vs)  % 3600) / 60) ), (vs##_s))

    DEBUG_PRINT("Profile:");
    DEBUG_PRINT("%s %5s %3s %4s %3s %7u-%7u |  %7s/%7s  r:%u/o:%u\n", i_prf.valid ? "[*]" : "[ ]",
            i_prf.timeframe ? "TMR" : "NRGY",
                    i_prf.timeOfDay ? "ToD" :"rel",
                            i_prf.armed ? "ARMD" :"IDLE",
                                    i_prf.repeat ? "REPT" :"ONCE",
                                            i_prf.est, i_prf.let,
                                            value2timeStr( i_prf.est ), value2timeStr( i_prf.let ),
                                            i_prf.req,
                                            i_prf.opt
    );
}

void POW::dump() 
{
    DEBUG_PRINT("Profiles:");
}



//-------- 
HLW8012* g_HLW;
void ICACHE_RAM_ATTR hlw_cf1_interrupt(){ if(g_HLW) g_HLW->cf1_interrupt(); }
void ICACHE_RAM_ATTR hlw_cf_interrupt(){ if(g_HLW) g_HLW->cf_interrupt();  }

void POW_R1::setInterrupts() {
    g_HLW = static_cast<PwrSensHLW8012*>(m_sense)->getHLW8012();
    attachInterrupt(HLW8012_CF1_PIN, hlw_cf1_interrupt, CHANGE   );
    attachInterrupt(HLW8012_CF_PIN,  hlw_cf_interrupt,  CHANGE   );
}

POW_R1::POW_R1( uSEMP* i_semp, AppCb_t i_appCb ): POW( i_semp, i_appCb ) 
{
    // Sonoff POW Rev1
    DEBUG_PRINT(" POW instance Sonoff POW Rev1\n" );
    m_sense = new PwrSensHLW8012();
    m_ledPin      = LED_PIN_R1;
    m_relayPin    = RELAY_PIN_R1;
    m_buttonPin   = BUTTON_PIN_R1;
    m_ledLogic  = true; // inverted
    m_relayLogic = false;
    if(m_sense) {
        ((PwrSensHLW8012*) m_sense)->begin(HLW8012_CF_PIN, HLW8012_CF1_PIN, HLW8012_SEL_PIN, CURRENT_MODE, true); 

        DEBUG_PRINT(" set resistors\n" );

        // HLW8012 specific
        // These values are used to calculate current, voltage and power factors as per datasheet formula
        // These are the nominal values for the Sonoff POW resistors:
        // * The CURRENT_RESISTOR is the 1milliOhm copper-manganese resistor in series with the main line
        // * The VOLTAGE_RESISTOR_UPSTREAM are the 5 470kOhm resistors in the voltage divider that feeds the V2P pin in the HLW8012
        // * The VOLTAGE_RESISTOR_DOWNSTREAM is the 1kOhm resistor in the voltage divider that feeds the V2P pin in the HLW8012
        m_sense->setResistors(CURRENT_RESISTOR, VOLTAGE_RESISTOR_UPSTREAM, VOLTAGE_RESISTOR_DOWNSTREAM);
    }

    setup();

    setInterrupts();
} 

void POW_R3::setInterrupts() {
    g_HLW = static_cast<PwrSensHLW8012*>(m_sense)->getHLW8012();
    attachInterrupt(HLW8012_CF1_PIN, hlw_cf1_interrupt, CHANGE   );
    attachInterrupt(HLW8012_CF_PIN,  hlw_cf_interrupt,  CHANGE   );
}

POW_R3::POW_R3( uSEMP* i_semp, AppCb_t i_appCb ): POW( i_semp, i_appCb ) 
{
    DEBUG_PRINT(" POW instance Gosund SP1\n" );
    m_sense = new PwrSensHLW8012();
    m_ledPin      = LED_PIN_R3;
    m_relayPin    = RELAY_PIN_R3;
    m_buttonPin   = BUTTON_PIN_R3;
    m_ledLogic  = true; // inverted
    m_relayLogic = false;
    if(m_sense) {
        ((PwrSensHLW8012*) m_sense)->begin(HLW8012_CF_PIN_R3, HLW8012_CF1_PIN_R3, HLW8012_SEL_PIN_R3, CURRENT_MODE, true); 

        DEBUG_PRINT(" set resistors\n" );

        // HLW8012 specific
        // These values are used to calculate current, voltage and power factors as per datasheet formula
        // These are the nominal values for the Sonoff POW resistors:
        // * The CURRENT_RESISTOR is the 1milliOhm copper-manganese resistor in series with the main line
        // * The VOLTAGE_RESISTOR_UPSTREAM are the 5 470kOhm resistors in the voltage divider that feeds the V2P pin in the HLW8012
        // * The VOLTAGE_RESISTOR_DOWNSTREAM is the 1kOhm resistor in the voltage divider that feeds the V2P pin in the HLW8012
        m_sense->setResistors(CURRENT_RESISTOR, VOLTAGE_RESISTOR_UPSTREAM_R3, VOLTAGE_RESISTOR_DOWNSTREAM);
    }

    setup();

    setInterrupts();
} 


POW_R2::POW_R2( uSEMP* i_semp, AppCb_t i_appCb ) : POW( i_semp, i_appCb ) 
{
    // Sonoff POW Rev2
    DEBUG_PRINT(" POW instance Sonoff POW Rev2\n" );
    m_sense = new PwrSensCSE7766();
    m_ledPin      = LED_PIN_R2;
    m_relayPin    = RELAY_PIN_R2;
    m_buttonPin   = BUTTON_PIN_R2;
    m_ledLogic  = false;
    m_relayLogic = false;
    if(m_sense) ((PwrSensCSE7766*) m_sense)->begin( CSE7766_PIN, 4800 );
    setup();
}

POW_Sim::POW_Sim( uSEMP* i_semp, AppCb_t i_appCb ) : POW( i_semp, i_appCb ) 
{
    DEBUG_PRINT(" POW instance Simu\n" );
    m_sense = new PwrSensSim();
    m_ledPin    = LED_BUILTIN;
    m_relayPin  = RELAY_PIN_R0;
    m_buttonPin = BUTTON_PIN_R0;

    m_ledLogic  = false;
    m_relayLogic = false;
    setup();
    ((PwrSensSim*)m_sense)->begin();
}



#define SIM_SENSE_TIME 1000
const unsigned PwrSensSim::hlw_sim[] = { 900, 600, 1250, 1750, 1500, 150, 850, 10, 500, 1000, 1250, 120, 750 ,80, 500, 1000, 250
        ,750 , 1000, 150, 850,  500, 1000, 250, 500, 1000, 250, 750, 300, 800, 1400, 1600, 900, 700   };
const unsigned PwrSensSim::hlw_vsim[] = {229, 230,231,235,232,228,227 };

unsigned int PwrSensSim::getVoltage(){ return  hlw_vsim[(m_sim_rd)%DIM(hlw_vsim)];}
unsigned int PwrSensSim::getActivePower(){ return  m_on ? hlw_sim[(m_sim_rd)%DIM(hlw_sim)] : 0; }
unsigned int PwrSensSim::getApparentPower(){ return  m_on ? hlw_sim[(m_sim_rd)%DIM(hlw_sim)] : 0; }

void PwrSensSim::loop(){
    unsigned long now = millis();

    if ( now - m_last_update > SIM_SENSE_TIME )
    {
        ++m_sim_rd;
        m_last_update = now;
    }
}
