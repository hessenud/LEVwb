
#include "POW.h"




void unblockingDelay(unsigned long mseconds) {
    unsigned long timeout = millis();
    while ((millis() - timeout) < mseconds) delay(1);
}



POW*  newPOW( unsigned i_variant, uSEMP* i_semp )
{
    switch ( i_variant ) {
    case 0:   return new POW_Sim( i_semp );             break;
    case 1:   return new POW_R1( i_semp );              break; 
    case 2:   return new POW_R2(  i_semp );             break;
    default:  return new POW_Sim( i_semp );               break;
    }

}

POW::POW( uSEMP* i_semp)
:m_sense(0), m_semp(i_semp), m_activePwr(0), m_voltage(0), m_current(0)
, m_apparentPwr(0), m_averagePwr(0), m_pwrFactor(0), m_cumulatedEnergy(0),_cumulatedEnergy(0)
, m_activeProfile(0), m_ad_state(AD_OFF)
{
    m_last_update=millis();
}  

void POW::setup() {


    loadCalibration(this);
    m_semp->startService();

    // Open the relay to switch on the load
    m_relayState = RELAY_OFF;
    m_semp->setPwrState( relayState );

    m_pwrIdx = 0;
    m_averagePwr = 0;
    m_minPwrIdx=m_maxPwrIdx = 0;


    {
        m_ledState   = LED_OFF;
        m_relayState = RELAY_OFF;
        pinMode(m_ledPin, OUTPUT);
        pinMode(m_relayPin, OUTPUT);
        pinMode(m_buttonPin, INPUT);

        digitalWrite(m_relayPin, m_relayLogic^relayState );
        digitalWrite(m_ledPin,   m_relayLogic^ledState );
    }



    for(unsigned  n=0; n<DIM(m_powers); ++n)
    { 
        m_powers[n] = 0;
    }

    m_sumPwr = 0;


    m_application = [this](){  DEBUG_PRINT(" app Delegate"); };

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
    return (Wh2Ws(requestedEnergy) / avr_pwr) // OnTime is in seconds => Wh2Ws
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
    if (planHandle <0 ) {
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

int  POW::findTimeFrame( bool i_timf, unsigned i_req  )
{
    unsigned ret = N_POW_PROFILES;

    // earliest start is NOW
    // find a timeframe, that's LET has enough time left to fulfill the energy need of this request
    // the timeframe should end before <i_let> if i_let is not 0
    // if no matching timeframe is found return is -1

    unsigned long _now = TimeClk::unixtime2daytime(getTime());
    unsigned long endTime  = _now + i_req;
    DEBUG_PRINT("find Frame for %u finish by %s (+%s) now:%s (%lus)\n", i_req,
            Tstr(TimeClk::getTimeString(endTime)), Tstr(TimeClk::getTimeString(i_req)),  Tstr(TimeClk::getTimeString(_now)), _now );

    int idx = i_timf ? PROFILE_TIMEFRAME : PROFILE_START;
    int endIdx   = i_timf ?  N_POW_PROFILES : PROFILE_TIMEFRAME;
    for(  /*startIdx*/ ; idx < endIdx; ++idx )
    {
        PowProfile& prf = g_prefs.powProfile[idx];
        DEBUG_PRINT("\t");  dump_profile( prf );
        if ( prf.armed ) {
            if ( prf.timeOfDay ) {
                _DEBUG_PRINT(" end: %s (%lu) vs end: %s(%lu) req:%s(%u) vs len: %lu \n"
                        , Tstr(TimeClk::getTimeString(prf.let)), prf.let
                        , Tstr(TimeClk::getTimeString(endTime)), endTime
                        , Tstr(TimeClk::getTimeString(prf.let - prf.est)),(prf.let - prf.est)
                        , i_req);
                if ( (prf.let >= endTime)  && ((prf.let - prf.est) >= i_req)) {
                    DEBUG_PRINT("\t\t possible candidate absolute daytime %d:\n", idx);
                    dump_profile( prf );
                      ret = idx;
                    break;
                }
            } else {
                DEBUG_PRINT(" relend: %s (%u) vs req:%s(%u) vs len: %s(%u) \n",  Tstr(TimeClk::getTimeString(prf.let)), prf.let
                        , Tstr(TimeClk::getTimeString(i_req)), i_req
                        , Tstr(TimeClk::getTimeString(prf.let - prf.est)),(prf.let - prf.est));
                if ( (prf.let - prf.est) >= i_req) {
                    DEBUG_PRINT("\n\t possible candidate relative Time %d:\n", idx);
                       dump_profile( prf );
                    ret = idx;
                    break;
                }
            }
        }
    }

    if (ret < N_POW_PROFILES) {
        DEBUG_PRINT("found Timeframe %d: ", ret);
        dump_profile( g_prefs.powProfile[ret] );
    }

    return ret >= N_POW_PROFILES ? -1 : int(ret);
}

void POW::prolongActivePlan()
{
  DEBUG_PRINT(" prolongActivePlan  by %u\n", g_prefs.ad_prolong_inc);
    
    m_semp->updateEnergy( getTime(),  0,  0, g_prefs.ad_prolong_inc );
}

void POW::procAdRequest()
{
    DEBUG_PRINT(" Autodetected  Energy Request\n");
    // a energy request might be pending at this point:
    // the EM Device was switched on while waiting on the EM and a job was programmed (e.g. washingmachine) and started
    // this is not the usual use-case, because normally a Energy request is EIHTER already planned OR created right now
    // so all plans are deleted and a new request generated.
    // this can be handled more intelligently... but after brewing over this, i decided it would not be worth the effort


    int idx = findTimeFrame( PowProfile::TIMF, Wh2Ws(g_prefs.defCharge)/g_prefs.assumed_power );
    if ( idx >=0 ) {
        DEBUG_PRINT(" matching timeframe: %d\n", idx);
        m_semp->deleteAllPlans();  m_semp->setPwrState( false );
        g_LED.set( 0x5f, 8, true);
        PowProfile& prf = g_prefs.powProfile[idx];
        unsigned long _now = getTime();
        dump_profile( prf );
        if ( prf.timeframe ) {
            DEBUG_PRINT(" modifyPlanTime(0, prf:%d\n", idx);
            unsigned dayoffset = _now - (_now%(1 DAY));
            unsigned required = Wh2Ws(g_prefs.defCharge)/g_prefs.assumed_power;
            m_semp->modifyPlanTime(0, _now, required,  prf.opt,  prf.est + dayoffset, prf.let + dayoffset );
        } else {
            DEBUG_PRINT(" modifyPlan(0, prf:%d\n", idx);
            unsigned required = g_prefs.defCharge;
            m_semp->modifyPlan(0, _now, required,  prf.opt,  prf.est + _now, prf.let + _now );
        }
    }
}

ad_event_t POW::autoDetect() {
    ad_event_t ret = AD_IDLE;

    /* detect energy need if request-threshld-power surpassed more than specified time
     *
     * detect end of energy need by power falling below lower threshold for more than specified time
     */


    static unsigned long _start;
    unsigned long        _now = getTime();
    if ( relayState &&
         g_prefs.autoDetect ) {

        DEBUG_PRINT(" autoDetect state: %d   PWR: %u   dt=%lu\n", m_ad_state, activePwr, (_now- _start) );
        switch ( m_ad_state ) {
        case AD_OFF:      if (  activePwr > g_prefs.ad_on_threshold )   {
            DEBUG_PRINT(" autoDetect state:AD_OFF  ABOVE THRESHOLD ==> TEST ON\n" );
            _start = _now; m_ad_state = AD_TEST_ON;
        }
        break;

        case AD_TEST_ON:  if ( activePwr > g_prefs.ad_on_threshold )   {
            DEBUG_PRINT(" autoDetect state:AD_TEST_ON  ABOVE THRESHOLD  %lu(%u)\n", (_now- _start), g_prefs.ad_on_time );
            if ((_now- _start) > g_prefs.ad_on_time)     {
                DEBUG_PRINT(" autoDetect state:AD_TEST_ON ==> AD_ON\n" );
                m_ad_state = AD_ON; _start = _now;
                ret = AD_REQUEST;
            }
        } else {
            DEBUG_PRINT(" autoDetect state:AD_TEST_ON  BELOW THRESHOLD  ==> AD_OFF\n" );
            m_ad_state = AD_OFF;    _start = _now;

        }
        break;

        case AD_ON:       if ( activePwr < g_prefs.ad_off_threshold )  {
            DEBUG_PRINT(" autoDetect state:AD_ON  BELOW THRESHOLD  ==> AD_TEST_OFF\n" );
            m_ad_state = AD_TEST_OFF;  _start = _now;
        }
        break;

        case AD_TEST_OFF: if ( activePwr < g_prefs.ad_off_threshold )  {
            DEBUG_PRINT(" autoDetect state:AD_TEST_OFF  BELOW THRESHOLD  %lu(%u)\n", (_now- _start), g_prefs.ad_on_time );

            if ((_now- _start)> g_prefs.ad_off_time)    {
                DEBUG_PRINT(" autoDetect state:AD_TEST_OFF ==> AD_OFF\n" );
                m_ad_state = AD_OFF;  _start = _now;
                ret = AD_END_REQUEST;
            }
        } else {
            DEBUG_PRINT(" autoDetect state:AD_TEST_OFF  ABOVE THRESHOLD  ==> AD_ON\n" );
            m_ad_state = AD_ON; _start = _now;
        }
        break;
        }
    }
    return ret;
}

void POW::loop() {
    m_sense->loop();  // Polled sensors need some cpu
    unsigned long now=millis();
    // This UPDATE_TIME should be at least twice the minimum time for the current or voltage
    // signals to stabilize. Experimentally that's about 1 second.
    long dt = ( now - m_last_update);
    if ( dt > UPDATE_TIME) {

        _DEBUG_PRINT("POW update\n");
        m_last_update = now;
        unsigned _time = getTime();
        unsigned requestedEnergy = 0;
        unsigned optionalEnergy  = 0;
        PlanningData* activePlan = m_semp->getActivePlan();
        if (activePlan){
            requestedEnergy = activePlan->m_requestedEnergy;
            optionalEnergy  = activePlan->m_optionalEnergy;
        }

        if ( m_sense ) {
            m_activePwr = relayState ? m_sense->getActivePower(): 0;
            m_voltage   = m_sense->getVoltage();
            m_current   = relayState ? m_sense->getCurrent(): 0;
            m_apparentPwr = relayState ? m_sense->getApparentPower():0;

            // manipulate sum directly
            m_sumPwr -= m_powers[m_pwrIdx];
            m_sumPwr += activePwr;
            // maintain mix/maxIdx to avoid searching the array
            if ( m_powers[m_pwrIdx] <= m_powers[m_minPwrIdx] ) m_minPwrIdx = m_pwrIdx;
            if ( m_powers[m_pwrIdx] >= m_powers[m_maxPwrIdx] ) m_maxPwrIdx = m_pwrIdx;
            m_powers[m_pwrIdx] = activePwr;
            m_averagePwr = m_sumPwr/DIM(m_powers);

            if ( ++m_pwrIdx >= DIM(m_powers) ) {
                m_pwrIdx = 0;;
                //DEBUG_PRINT("idx: %u/%u --average POWER: %u\n", pwrIdx, DIM_PWRS, averagePwr );
            } else {
                //DEBUG_PRINT("summing actualPower: %u\n", activePwr );
            }

            m_pwrFactor = (int) (100 *  m_sense->getPowerFactor());

#ifdef USE_POW_DBG
            DEBUG_PRINT("[HLW] Active Power (W)    : %u  avr: %u\n", activePwr, averagePwr );
            DEBUG_PRINT("[HLW] Voltage (V)         : %u\n", voltage       );
            DEBUG_PRINT("[HLW] Current (A)         : %f\n", current );
            DEBUG_PRINT("[HLW] Apparent Power (VA) : %u\n", apparentPwr );
            DEBUG_PRINT("[HLW] Power Factor (%)    : %f\n", pwrFactor     );
#endif // USE_POW_DBG

        }
#ifdef USE_POW_DBG
        DEBUG_PRINT("[HLW] _cumulatedEnergy    : %ul\n", _cumulatedEnergy     );  
        DEBUG_PRINT("[HLW] cumulatedEnergy    : %u\n", cumulatedEnergy     );
        DEBUG_PRINT("[HLW] requestedEnergy    : %u\n", requestedEnergy     );  
        DEBUG_PRINT("[HLW] optionalEnergy     : %u\n", optionalEnergy     );  
#endif

        // auto detect
        int autoDetectState = autoDetect();
        if ( autoDetectState  == AD_REQUEST ) {
            procAdRequest();
        } else if ( autoDetectState  == AD_END_REQUEST ) {
            DEBUG_PRINT(" Autodetected  End of Energy Request\n");
            m_semp->resetPlan(); 
            m_semp->setPwrState( false );
            g_LED.reset();
        } else {
            // prolong active Plan if job not completed
            if ( activePlan && (activePlan->end() - _time < g_prefs.ad_prolong_inc) ) 
              activePlan->updateEnergy(  _time, m_semp->pwrState(), 0, 0,  g_prefs.ad_prolong_inc);
        }

        if ( m_application )   m_application();



        ////////////////////////////////
        { 
            _cumulatedEnergy += activePwr*dt;  /// Wms  -> 1/(3600*1000) Wh
            while ( _cumulatedEnergy > Wh2Wms(1) ) {
              
                ++m_cumulatedEnergy;
                _cumulatedEnergy -= Wh2Wms(1);
                if ( requestedEnergy > 0) {
                    --requestedEnergy;
                    m_semp->updateEnergy( _time,  -1,  0 );
                }

                if ( optionalEnergy > 0)  {
                    --optionalEnergy;
                    m_semp->updateEnergy( _time, 0,  -1 );
                }
            }
        }
        /////////////////////////////////

        m_semp->updateTime( _time );
        m_semp->setPwr( averagePwr, m_powers[m_minPwrIdx], m_powers[m_maxPwrIdx]);
        _DEBUG_PRINT("POW update end\n");
    }
}



void POW::setPwr(bool i_state )
{
    if (relayState != i_state ) {
      digitalWrite(m_relayPin, m_relayLogic^(m_relayState = i_state) );
    }
}

void POW::setLED(bool i_state )
{
    if (ledState != i_state ) digitalWrite(m_ledPin, m_ledLogic ^(m_ledState = i_state) );
}
void POW::toggleRelay()
{
    setPwr( !relayState );
}
void POW::toggleLED()
{
    setLED( !ledState );
}

void dump_profile( PowProfile& i_prf )
{
#define value2timeStr( vs )   (snprintf_P( (vs##_s), sizeof(vs##_s), PSTR("%2lu:%02lu"), (((vs)  % 86400L) / 3600), (((vs)  % 3600) / 60) ), (vs##_s))

    DEBUG_PRINT("Profiles:");
    DEBUG_PRINT("%5s %3s %4s %3s %7u-%7u |  %7s/%7s  r:%u/o:%u\n",
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

POW_R1::POW_R1( uSEMP* i_semp): POW( i_semp) 
{
    // Sonoff POW Rev1
    DEBUG_PRINT(" POW instance Sonoff POW Rev1\n" );
    m_sense = new PwrSensHLW8012();
    m_ledPin      = LED_PIN_R1;
    m_relayPin    = RELAY_PIN;
    m_buttonPin   = 0;
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

POW_R2::POW_R2( uSEMP* i_semp) : POW( i_semp) 
{
    // Sonoff POW Rev2
    DEBUG_PRINT(" POW instance Sonoff POW Rev2\n" );
    m_sense = new PwrSensCSE7766();
    m_ledPin      = LED_PIN_R2;
    m_relayPin    = RELAY_PIN;
    m_buttonPin   = 0;
    m_ledLogic  = false;
    m_relayLogic = false;
    if(m_sense) ((PwrSensCSE7766*) m_sense)->begin( CSE7766_PIN, 4800 );
    setup();
}

POW_Sim::POW_Sim( uSEMP* i_semp) : POW( i_semp) 
{
    DEBUG_PRINT(" POW instance Simu\n" );
    m_sense = new PwrSensSim();
    m_ledPin    = LED_BUILTIN;
    m_relayPin  = 12;
    m_buttonPin = 0;

    m_ledLogic  = false;
    m_relayLogic = false;
    setup();
}



#define SIM_SENSE_TIME 1000
const unsigned PwrSensSim::hlw_sim[] = { 900, 600, 1250, 1750, 1500, 150, 850, 10, 500, 1000, 1250, 120, 750 ,80, 500, 1000, 250
        ,750 , 1000, 150, 850,  500, 1000, 250, 500, 1000, 250, 750, 300, 800, 1400, 1600, 900, 700   };
const unsigned PwrSensSim::hlw_vsim[] = {229, 230,231,235,232,228,227 };

unsigned int PwrSensSim::getVoltage(){ return  hlw_vsim[(m_sim_rd)%DIM(hlw_vsim)];}
unsigned int PwrSensSim::getActivePower(){ return  hlw_sim[(m_sim_rd)%DIM(hlw_sim)]; }
unsigned int PwrSensSim::getApparentPower(){ return   hlw_sim[(m_sim_rd)%DIM(hlw_sim)];}

void PwrSensSim::loop(){
    unsigned long now = millis();

    if ( now - m_last_update > SIM_SENSE_TIME )
    {
        ++m_sim_rd;
        m_last_update = now;
    }
}
