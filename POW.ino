#ifdef USE_POW
#include "POW.h"


// local variables
static unsigned _cumulatedEnergy; // delta of comulated energy 



void unblockingDelay(unsigned long mseconds) {
    unsigned long timeout = millis();
    while ((millis() - timeout) < mseconds) delay(1);
}


HLW8012 POW::hlw8012;

const int ledPin   = LED_PIN;
static const int relayPin  = 12;  // Sonoff 12
static const int buttonPin = 0;

// When using interrupts we have to call the library entry point
// whenever an interrupt is triggered
void ICACHE_RAM_ATTR POW::hlw8012_cf1_interrupt() {
    hlw8012.cf1_interrupt();
}
void ICACHE_RAM_ATTR POW::hlw8012_cf_interrupt() {
    hlw8012.cf_interrupt();
}


POW::POW()
  : m_activePwr(0), m_voltage(0), m_current(0), m_apparentPwr(0), m_averagePwr(0), m_pwrFactor(0), m_cumulatedEnergy(0) 
{

    m_ledState   = LED_OFF;
    m_relayState = RELAY_OFF;
    pinMode(ledPin, OUTPUT);
    pinMode(relayPin, OUTPUT);
    pinMode(buttonPin, INPUT);

    digitalWrite(relayPin, relayState );
    digitalWrite(ledPin,   LED_LEVEL(ledState) );

}

// Library expects an interrupt on both edges
void POW::setInterrupts() {
    attachInterrupt(HLW8012_CF1_PIN, POW::hlw8012_cf1_interrupt, CHANGE);
    attachInterrupt(HLW8012_CF_PIN,  POW::hlw8012_cf_interrupt, CHANGE);
}


void POW::setprefs( unsigned i_cumulatedEnergy, double i_pwrMultiplier, double i_currentMultiplier, double i_voltageMultiplier)
{
    DEBUG_PRINT("[HLW] power multiplier   : %f -> %f\n", pwrMultiplier(), i_pwrMultiplier);
    DEBUG_PRINT("[HLW] current multiplier : %f -> %f\n", currentMultiplier(), i_currentMultiplier) ;
    DEBUG_PRINT("[HLW] voltage multiplier : %f -> %f\n", voltageMultiplier(), i_voltageMultiplier);
    DEBUG_PRINT("\n");

    m_cumulatedEnergy = i_cumulatedEnergy;
    hlw8012.setPowerMultiplier( i_pwrMultiplier );
    hlw8012.setCurrentMultiplier( i_currentMultiplier );
    hlw8012.setVoltageMultiplier( i_voltageMultiplier );

}

void POW::calibrate(    double expectedVoltage, double expectedPwr, double expectedCurrent )
{

    DEBUG_PRINT("[HLW] Before calibration\n");
    DEBUG_PRINT("[HLW] current multiplier : %f\n", currentMultiplier());
    DEBUG_PRINT("[HLW] voltage multiplier : %f\n", voltageMultiplier());
    DEBUG_PRINT("[HLW] power multiplier   : %f\n", pwrMultiplier());
    hlw8012.resetMultipliers();
    if (expectedVoltage!=0 && expectedPwr != 0 && expectedCurrent !=0 ) {
        // Calibrate using a 60W bulb (pure resistive) on a 230V line
        hlw8012.expectedActivePower(expectedPwr); //double
        hlw8012.expectedVoltage(expectedVoltage);
        hlw8012.expectedCurrent(expectedCurrent ); // (expectedCurrent = double(expectedPwr) / expectedVoltage)) ;
    }
    // Show corrected factors
    DEBUG_PRINT("[HLW] current  : %f\n",(float)expectedCurrent);
    DEBUG_PRINT("[HLW] voltage  : %f\n",expectedVoltage);
    DEBUG_PRINT("[HLW] power    : %f\n\n",expectedPwr);
    DEBUG_PRINT("\n");

    saveCalibration();
    
    DEBUG_PRINT("[HLW] After calibration\n");
    DEBUG_PRINT("[HLW] current multiplier : %f\n", currentMultiplier());
    DEBUG_PRINT("[HLW] voltage multiplier : %f\n", voltageMultiplier());
    DEBUG_PRINT("[HLW] power multiplier   : %f\n", pwrMultiplier());
    unblockingDelay(1000);
}


void POW::setup(uSEMP* i_semp) {
    // Open the relay to switch on the load
    semp = i_semp;
    semp->startService();

    m_relayState = RELAY_OFF;
    semp->setPwrState( relayState );
    pwrIdx = 0;
    m_averagePwr = 0;
    _minPwr=minPwr=_maxPwr=maxPwr=minPwrIdx=maxPwrIdx = 0;
    // Initialize HLW8012
    // void begin(unsigned char cf_pin, unsigned char cf1_pin, unsigned char sel_pin, unsigned char currentWhen = HIGH, bool use_interrupts = false, unsigned long pulse_timeout = PULSE_TIMEOUT);
    // * cf_pin, cf1_pin and sel_pin are GPIOs to the HLW8012 IC
    // * currentWhen is the value in sel_pin to select current sampling
    // * set use_interrupts to false, we will have to call handle() in the main loop to do the sampling
    // * set pulse_timeout to 500ms for a fast response but losing precision (that's ~24W precision :( )
#ifdef USE_POW_INT    
    hlw8012.begin(HLW8012_CF_PIN, HLW8012_CF1_PIN, HLW8012_SEL_PIN, CURRENT_MODE, true); 
#else
    hlw8012.begin(HLW8012_CF_PIN, HLW8012_CF1_PIN, HLW8012_SEL_PIN, CURRENT_MODE, false, 500000);
#endif
    // These values are used to calculate current, voltage and power factors as per datasheet formula
    // These are the nominal values for the Sonoff POW resistors:
    // * The CURRENT_RESISTOR is the 1milliOhm copper-manganese resistor in series with the main line
    // * The VOLTAGE_RESISTOR_UPSTREAM are the 5 470kOhm resistors in the voltage divider that feeds the V2P pin in the HLW8012
    // * The VOLTAGE_RESISTOR_DOWNSTREAM is the 1kOhm resistor in the voltage divider that feeds the V2P pin in the HLW8012
    hlw8012.setResistors(CURRENT_RESISTOR, VOLTAGE_RESISTOR_UPSTREAM, VOLTAGE_RESISTOR_DOWNSTREAM);

    for(unsigned  n=0; n<DIM(powers); ++n)
    { 
        powers[n] = hlw8012.getActivePower();
    }
    
#ifdef USE_POW_INT 
    setInterrupts();
#endif
    loadCalibration();

    DEBUG_PRINT("\n[HLW] Default current multiplier : %f\n", currentMultiplier());
    DEBUG_PRINT("\n[HLW] Default voltage multiplier : %f\n", voltageMultiplier());
    DEBUG_PRINT("\n[HLW] Default power multiplier   : %f\n", pwrMultiplier());
    DEBUG_PRINT("\n");

    
}



unsigned POW::calcOnTime( unsigned requestedEnergy, unsigned avr_pwr)
{
    return (Wh2Ws(requestedEnergy) / avr_pwr) // OnTime is in seconds => Wh2Ws
            +180; // + 3 Minutes to give SEMP EM a bit time for planning an control
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
    Serial.printf("POW now: %s EST: %s  LET: %s\n", String(TimeClk::getTimeString(_now)).c_str(), String(TimeClk::getTimeString(earliestStart)).c_str()
            , String(TimeClk::getTimeString(latestStop)).c_str());
    if (planHandle <0 ) {
        planHandle = semp->requestEnergy(_now, requestedEnergy,  optionalEnergy,  earliestStart, latestStop );
    } else {
        semp->modifyPlan(planHandle, _now, requestedEnergy,  optionalEnergy,  earliestStart, latestStop );
    }
    Serial.printf("POW requested Energy on plan %d\n", planHandle);


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
    semp->dumpPlans(&buffer[wp-1]);
    Serial.printf("response:\n%s\n",  buffer );

    DEBUG_PRINT("%s\n", buffer);
    handleStat();
    Serial.printf(" requested Energy end\n" );

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

void POW::loop() {
    static unsigned long last;
    unsigned long now=millis();
    // This UPDATE_TIME should be at least twice the minimum time for the current or voltage
    // signals to stabilize. Experimentally that's about 1 second.
    long dt = ( now - last);
    if ((now - last) > UPDATE_TIME) {
        last = now;
        unsigned _time = getTime();
        unsigned requestedEnergy = 0;
        unsigned optionalEnergy  = 0;
        PlanningData* plan = semp->getActivePlan();
        if (plan){
            requestedEnergy = plan->m_requestedEnergy;
            optionalEnergy  = plan->m_optionalEnergy;
        }

#ifdef HLW_SIM
        static unsigned sim_rd;
        static const unsigned hlw_sim[] = { 900, 600, 1250, 1750, 1500, 150, 850, 10, 500, 1000, 1250, 120, 750 ,80, 500, 1000, 250
                ,750 , 1000, 150, 850,  500, 1000, 250, 500, 1000, 250, 750, 300, 800, 1400, 1600, 900, 700   };
        static unsigned hlw_vsim[] = {229, 230,231,235,232,228,227 };


        m_apparentPwr = m_activePwr =  relayState ? hlw_sim[(sim_rd)%DIM(hlw_sim)] : 0;
        m_voltage =  hlw_vsim[(sim_rd)%DIM(hlw_vsim)];
        m_current = (double)activePwr / (double)voltage;
        ++sim_rd;
        Serial.printf("Simulated POW read(%u): %u  avr:%u  u:%u i:%f\n",sim_rd, activePwr, averagePwr, voltage, current);
#else
        m_activePwr = hlw8012.getActivePower();
        m_voltage   = hlw8012.getVoltage();
        m_current   = hlw8012.getCurrent();
        m_apparentPwr = hlw8012.getApparentPower();
#endif

        // get min and max of last values
        _minPwr = _maxPwr = activePwr;
#if 1
        // manipulate sum directly
        sumPwr -= powers[pwrIdx];
        sumPwr += activePwr;
        // maintain mix/maxIdx to avoid searching the array
        if ( powers[pwrIdx] <= powers[minPwrIdx] ) minPwrIdx = pwrIdx;
        if ( powers[pwrIdx] >= powers[maxPwrIdx] ) maxPwrIdx = pwrIdx;
        powers[pwrIdx] = activePwr;
        m_averagePwr = sumPwr/DIM(powers);
        // commit the min/max values
        minPwr = powers[minPwrIdx];
        maxPwr = powers[maxPwrIdx] ; 
#else

        unsigned long sum = 0;
        powers[pwrIdx] = activePwr;
        for(unsigned  n=0; n<DIM(powers); ++n)
        {
            if ( powers[n] < _minPwr ) _minPwr = (unsigned) powers[n];
            if ( powers[n] > _maxPwr ) _maxPwr = (unsigned) powers[n];
            sum += powers[n];
            averagePwr = sum/DIM(powers);
        }
        // commit the min/max values
        minPwr = (unsigned) _minPwr;
        maxPwr = (unsigned) _maxPwr;  
#endif
        if ( ++pwrIdx >= DIM(powers) ) {
            pwrIdx = 0;;
            //Serial.printf("idx: %u/%u --average POWER: %u\n", pwrIdx, DIM_PWRS, averagePwr );
        } else {
            //Serial.printf("summing actualPower: %u\n", activePwr );
        }

        m_pwrFactor = (int) (100 * hlw8012.getPowerFactor());

#ifdef USE_POW_DBG
        DEBUG_PRINT("[HLW] Active Power (W)    : %u\n", activePwr     );
        DEBUG_PRINT("[HLW] Voltage (V)         : %u\n", voltage       );
        DEBUG_PRINT("[HLW] Current (A)         : %f\n", current );
        DEBUG_PRINT("[HLW] Apparent Power (VA) : %u\n", apparentPwr );
        DEBUG_PRINT("[HLW] Power Factor (%)    : %f\n", pwrFactor     );  
#endif // USE_POW_DBG
        _cumulatedEnergy += activePwr*dt;  /// Wms  -> 1/(3600*1000) Wh
        while ( _cumulatedEnergy > Wh2Wms(1) ) {

            ++m_cumulatedEnergy;
            _cumulatedEnergy -= Wh2Wms(1);
            if ( requestedEnergy > 0) {
                --requestedEnergy;
                semp->updateEnergy( _time,  -1,  0 );
            }
            //          else {
            //            // switch OFF
            //            semp->updateEnergy( 0,  0 );
            //            relayState = LOW;
            //            semp->setPwrState( relayState );
            //          }
            if ( optionalEnergy > 0)  {
                --optionalEnergy;
                semp->updateEnergy( _time, 0,  -1 );
            }

        }
        DEBUG_PRINT("[HLW] _cumulatedEnergy    : %ul\n", _cumulatedEnergy     );  
        DEBUG_PRINT("[HLW] cumulatedEnergy    : %u\n", cumulatedEnergy     );
        DEBUG_PRINT("[HLW] requestedEnergy    : %u\n", requestedEnergy     );  
        DEBUG_PRINT("[HLW] optionalEnergy     : %u\n", optionalEnergy     );  
        semp->updateEnergy( _time, 0,  0 );
        semp->setPwr( averagePwr, minPwr, maxPwr);

#ifndef USE_POW_INT
        // When not using interrupts we have to manually switch to current or voltage monitor
        // This means that every time we get into the conditional we only update one of them
        // while the other will return the cached value.
        hlw8012.toggleMode();
#endif
    }
}

void POW::setPwr(bool i_state )
{
    if (relayState != i_state ) digitalWrite(relayPin, m_relayState = i_state );
}

void POW::setLED(bool i_state )
{
    if (ledState != i_state ) digitalWrite(ledPin, LED_LEVEL(m_ledState = i_state) );
}
void POW::toggleRelay()
{
    setPwr( !relayState );
}
void POW::toggleLED()
{
    setLED( !ledState );
}

#else
void setupPOW() {

}
void loopPOW() {}
#endif
