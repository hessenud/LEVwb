#ifdef USE_POW

// Check values every 2 seconds
#define UPDATE_TIME                     3000
#define SEL_PIN                         5
#define CF1_PIN                         13
#define CF_PIN                          14

// Set SEL_PIN to HIGH to sample current
// This is the case for Itead's Sonoff POW, where a
// the SEL_PIN drives a transistor that pulls down
// the SEL pin in the HLW8012 when closed
#define CURRENT_MODE                    HIGH

// These are the nominal values for the resistors in the circuit
#define CURRENT_RESISTOR                0.001
#define VOLTAGE_RESISTOR_UPSTREAM       ( 5 * 470000 ) // Real: 2280k
#define VOLTAGE_RESISTOR_DOWNSTREAM     ( 1000 ) // Real 1.009k


// local variables
static unsigned _cumulatedEnergy; // delta of comulated energy 



void unblockingDelay(unsigned long mseconds) {
    unsigned long timeout = millis();
    while ((millis() - timeout) < mseconds) delay(1);
}


#define REF_PWR 500.0
#define REF_VOLT 230.0
double expectedVoltage = REF_VOLT;
double expectedPwr     = REF_PWR;
double expectedCurrent = REF_PWR /REF_VOLT;

#define DIM_PWRS ((60000+500)/UPDATE_TIME)

unsigned long sumPwr;
unsigned minPwrIdx,maxPwrIdx;
unsigned minPwr,_minPwr;
unsigned maxPwr,_maxPwr;
unsigned powers[DIM_PWRS];
unsigned pwrIdx;  



// When using interrupts we have to call the library entry point
// whenever an interrupt is triggered
void ICACHE_RAM_ATTR hlw8012_cf1_interrupt() {
  hlw8012.cf1_interrupt();
}
void ICACHE_RAM_ATTR hlw8012_cf_interrupt() {
  hlw8012.cf_interrupt();
}

// Library expects an interrupt on both edges
void setInterrupts() {
    attachInterrupt(CF1_PIN, hlw8012_cf1_interrupt, CHANGE);
    attachInterrupt(CF_PIN, hlw8012_cf_interrupt, CHANGE);
}


void setPOWprefs(double i_pwrMultiplier, double i_currentMultiplier, double i_voltageMultiplier)
{
   DEBUG_PRINT("[HLW] power multiplier   : %f -> %f\n", pow_pwrMultiplier, i_pwrMultiplier);
   DEBUG_PRINT("[HLW] current multiplier : %f -> %f\n", pow_currentMultiplier, i_currentMultiplier) ;
   DEBUG_PRINT("[HLW] voltage multiplier : %f -> %f\n", pow_voltageMultiplier, i_voltageMultiplier);
   DEBUG_PRINT("\n");
   
   hlw8012.setPowerMultiplier( pow_pwrMultiplier = i_pwrMultiplier);
   hlw8012.setCurrentMultiplier(pow_currentMultiplier = i_currentMultiplier);
   hlw8012.setVoltageMultiplier( pow_voltageMultiplier = i_voltageMultiplier);

}

void calibrate() 
{
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
    DEBUG_PRINT("[HLW] current multiplier : %f\n", pow_currentMultiplier);
    DEBUG_PRINT("[HLW] voltage multiplier : %f\n", pow_voltageMultiplier);
    DEBUG_PRINT("[HLW] power multiplier   : %f\n", pow_pwrMultiplier);
    DEBUG_PRINT("\n");
    
    
    savePrefs();
    pow_pwrMultiplier     = hlw8012.getPowerMultiplier();
    pow_currentMultiplier = hlw8012.getCurrentMultiplier();
    pow_voltageMultiplier = hlw8012.getVoltageMultiplier();
    
    DEBUG_PRINT("[HLW] After calibration\n");
    DEBUG_PRINT("[HLW] current multiplier : %f\n", pow_currentMultiplier);
    DEBUG_PRINT("[HLW] voltage multiplier : %f\n", pow_voltageMultiplier);
    DEBUG_PRINT("[HLW] power multiplier   : %f\n", pow_pwrMultiplier);
    unblockingDelay(1000);
}

void setupPOW() {
    // Open the relay to switch on the load
    relayState = LOW;
    
    g_semp->startService();
    g_semp->setPwrState( relayState );
    pwrIdx = 0;
    pow_averagePwr = 0;
    _minPwr=minPwr=_maxPwr=maxPwr=minPwrIdx=maxPwrIdx = 0;
    // Initialize HLW8012
    // void begin(unsigned char cf_pin, unsigned char cf1_pin, unsigned char sel_pin, unsigned char currentWhen = HIGH, bool use_interrupts = false, unsigned long pulse_timeout = PULSE_TIMEOUT);
    // * cf_pin, cf1_pin and sel_pin are GPIOs to the HLW8012 IC
    // * currentWhen is the value in sel_pin to select current sampling
    // * set use_interrupts to false, we will have to call handle() in the main loop to do the sampling
    // * set pulse_timeout to 500ms for a fast response but losing precision (that's ~24W precision :( )
#ifdef USE_POW_INT    
    hlw8012.begin(CF_PIN, CF1_PIN, SEL_PIN, CURRENT_MODE, true); 
#else
    hlw8012.begin(CF_PIN, CF1_PIN, SEL_PIN, CURRENT_MODE, false, 500000);
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
    // Show default (as per datasheet) multipliers
    pow_pwrMultiplier     = hlw8012.getPowerMultiplier();
    pow_currentMultiplier = hlw8012.getCurrentMultiplier();
    pow_voltageMultiplier = hlw8012.getVoltageMultiplier();
//
    DEBUG_PRINT("\n[HLW] Default current multiplier : %f\n", pow_currentMultiplier);
    DEBUG_PRINT("\n[HLW] Default voltage multiplier : %f\n", pow_voltageMultiplier);
    DEBUG_PRINT("\n[HLW] Default power multiplier   : %f\n", pow_pwrMultiplier);
    DEBUG_PRINT("\n");
 
    http_server.on("/pwr", HTTP_GET, handlePwrReq );
    http_server.on("/calibrate", HTTP_GET, handleCalReq );
    http_server.on("/energy", HTTP_GET, handleEnergyReq );

#ifdef USE_POW_INT 
    setInterrupts();
#endif
    // calibrate();

    _cumulatedEnergy = 0;
    setPOWprefs(pow_prefs_pwrMultiplier, pow_prefs_currentMultiplier, pow_prefs_voltageMultiplier);  
}

unsigned timeStr2Value(  const char* i_ts)
{
  /* timestr    hh:mm  h:mm  hh:mm:ss
   * split
   */
  unsigned timeval = 0;
  unsigned n=0;
  char* p = ( char*)i_ts;
  char* str;
  while ((str = strtok_r(p, ":", &p)))
  {
    int val = atoi(str);
    switch(n)
    {
      case 0: timeval = val *3600;  break;
      case 1: timeval += val *60;   break;
      case 2: timeval += val;       break;
      default:
      Serial.printf(" Holy shit something went bonkers\n");
    }
    ++n;
  }
  
  return timeval;  
}

void handleEnergyReq()
{
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
    } else if (p1Name == String("startTime"))    { earliestStart   = timeStr2Value(p1Val.c_str()) + dayoffset;
    } else if (p1Name == String("endTime"))      { latestStop      = timeStr2Value(p1Val.c_str()) + dayoffset;
    } else if (p1Name == String("plan"))         { planHandle      = atoi(p1Val.c_str()) ;
    } else {
    }
  }
  if ( earliestStart < _now ) {
    earliestStart += (1 DAY);
    latestStop += (1 DAY);
  }
  Serial.printf("POW now: %s EST: %s  LET: %s\n", String(TimeClk::getTimeString(_now)).c_str(), String(TimeClk::getTimeString(earliestStart)).c_str()
  , String(TimeClk::getTimeString(latestStop)).c_str());
  if (planHandle <0 ) {
    planHandle = g_semp->requestEnergy(_now, requestedEnergy,  optionalEnergy,  earliestStart, latestStop );
  } else {
    g_semp->modifyPlan(planHandle, _now, requestedEnergy,  optionalEnergy,  earliestStart, latestStop );
  }
  Serial.printf("POW requested Energy on plan %d\n", planHandle);

    
  String resp = String("[HLW] Active Power (W)    : ") + String(pow_activePwr) +
        String("\n[HLW] Voltage (V)         : ") + String(pow_voltage) +
        String("\n[HLW] Current (A)         : ") + String(pow_current) +
        String("\n[HLW] Energy") +
        String("\n[HLW] \ttotal     : ") + String(_cumulatedEnergy) +
        String("\n[HLW] \trequested : ") + String(requestedEnergy) + 
        String("\n[HLW] \toptional  : ") + String(optionalEnergy)  +
        String("\n[HLW] \tstart     : ") + String(TimeClk::getTimeString(earliestStart))   +
        String("\n[HLW] \tend       : ") + String(TimeClk::getTimeString(latestStop))      +
        +"\n"; 

  char buffer[resp.length() + 10*40];
  unsigned wp = sprintf(buffer,"\n%s\n",resp.c_str());
  g_semp->dumpPlans(&buffer[wp-1]);       
  Serial.printf("response:\n%s\n",  buffer );
    
  DEBUG_PRINT("%s\n", buffer);
  http_server.send(200, "text/plain", buffer);
  Serial.printf(" requested Energy end\n" );
    
}


void handleCalReq()
{
   pow_pwrMultiplier     = hlw8012.getPowerMultiplier();
   pow_currentMultiplier = hlw8012.getCurrentMultiplier();
   pow_voltageMultiplier = hlw8012.getVoltageMultiplier();
   
   hlw8012.setPowerMultiplier(pow_pwrMultiplier);
   hlw8012.setCurrentMultiplier(pow_currentMultiplier);
   hlw8012.setVoltageMultiplier(pow_voltageMultiplier);
 
   String resp = String("[HLW] Active Power (W)    : ") + String(pow_activePwr) +
        String("\n[HLW] Voltage (V)         : ") + String(pow_voltage) +
        String("\n[HLW] Current (A)         : ") + String(pow_current) +
        String("\n[HLW] Apparent Power (VA) : ") + String(pow_apparentPwr) +
        String("\n[HLW] Power Factor (%)    : ") + String(pow_pwrFactor) + 
        String("\n[HLW] New current multiplier : ") + String(pow_currentMultiplier) +
        String("\n[HLW] New voltage multiplier : ") + String(pow_voltageMultiplier) +
        String("\n[HLW] New power multiplier   : ") + String(pow_pwrMultiplier) + "\n"; 
        
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

  calibrate();
  
  http_server.send(200, "text/plain", resp);
  
  
}

void handlePwrReq()
{
  bool def = true;
   String resp = String("<?xml version=\"1.0\"?>\r\n"
        "<Stat  xmlns=\"http://www.sma.de/communication/schema/SEMP/v1\">\r\n"
        " <led> ") + String(!ledState) + 
        String("</led>\r\n  <switch> ") + String(relayState) + 
        String("</switch>\r\n <activePower> ") + String(pow_activePwr) +
        String("</activePower>\r\n  <averagePower> ") + String(pow_averagePwr) +
        String("</averagePower>\r\n  <voltage> ") + String(pow_voltage) +
        String("</voltage>\r\n  <current> ") + String(pow_current) +
        String("</current>\r\n  <apparentPower> ") + String(pow_apparentPwr) +
        String("</apparentPower>\r\n  <powerFactor> ") + String(pow_pwrFactor) +
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
    if (p1Name == String("power"))           {      resp = String("power:") + String(pow_activePwr); //constantly 15W Power
    } else if (p1Name == String("voltage"))  {      resp = String("volt:") + String(pow_voltage); //constantly 230V Power
    } else if (p1Name == String("current"))  {      resp = String("current:") + String(pow_current); //constantly 15W Power
    } else if (p1Name == String("apparent")) {      resp = String("apparent:") + String(pow_apparentPwr); //constantly 15W Power
    } else if (p1Name == String("factor"))   {      resp = String("factor:") + String(pow_pwrFactor); //
    } else {
      http_server.send(404, "text/plain", resp);
      return;
    }
  }
  DEBUG_PRINT("PWR req: %s\n",resp.c_str());
  if (def)  http_server.send(200, "text/xml", resp);
  else      http_server.send(200, "text/plain", resp);
}




#define Wh2Wms( e ) ((e)*3600*1000)

void loopPOW() {
   
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
      PlanningData* plan = g_semp->getActivePlan();
      if (plan){
        requestedEnergy = plan->m_requestedEnergy;
        optionalEnergy  = plan->m_optionalEnergy;
      }
        
#ifdef HLW_SIM
        static unsigned sim_rd;
        static const unsigned hlw_sim[] = { 900, 600, 1250, 1750, 1500, 150, 850, 10, 500, 1000, 1250, 120, 750 ,80, 500, 1000, 250
           ,750 , 1000, 150, 850,  500, 1000, 250, 500, 1000, 250, 750, 300, 800, 1400, 1600, 900, 700   };
        static unsigned hlw_vsim[] = {229, 230,231,235,232,228,227 };
           

        pow_apparentPwr = pow_activePwr =  g_semp->pwrState() ? hlw_sim[(sim_rd)%DIM(hlw_sim)] : 0;
        pow_voltage =  hlw_vsim[(sim_rd)%DIM(hlw_vsim)];
        pow_current = (double)pow_activePwr / (double)pow_voltage;
        ++sim_rd;
        Serial.printf("Simulated POW read(%u): %u  avr:%u  u:%u i:%f\n",sim_rd, pow_activePwr, pow_averagePwr, pow_voltage, pow_current);
#else
        pow_activePwr = hlw8012.getActivePower();
        pow_voltage   = hlw8012.getVoltage();
        pow_current   = hlw8012.getCurrent();
        pow_apparentPwr = hlw8012.getApparentPower();
#endif
      
        // get min and max of last values
        _minPwr = _maxPwr = pow_activePwr;
#if 1
        // manipulate sum directly
        sumPwr -= powers[pwrIdx];
        sumPwr += pow_activePwr;
        // maintain mix/maxIdx to avoid searching the array
        if ( powers[pwrIdx] <= powers[minPwrIdx] ) minPwrIdx = pwrIdx;
        if ( powers[pwrIdx] >= powers[maxPwrIdx] ) maxPwrIdx = pwrIdx;
        powers[pwrIdx] = pow_activePwr;
        pow_averagePwr = sumPwr/DIM(powers);
        // commit the min/max values
        minPwr = powers[minPwrIdx];
        maxPwr = powers[maxPwrIdx] ; 
#else

        unsigned long sum = 0;
        powers[pwrIdx] = pow_activePwr;
        for(unsigned  n=0; n<DIM(powers); ++n)
        {
          if ( powers[n] < _minPwr ) _minPwr = (unsigned) powers[n];
          if ( powers[n] > _maxPwr ) _maxPwr = (unsigned) powers[n];
          sum += powers[n];
          pow_averagePwr = sum/DIM(powers);
        }
        // commit the min/max values
        minPwr = (unsigned) _minPwr;
        maxPwr = (unsigned) _maxPwr;  
#endif
        if ( ++pwrIdx >= DIM(powers) ) {
          pwrIdx = 0;;  
          //Serial.printf("idx: %u/%u --average POWER: %u\n", pwrIdx, DIM_PWRS, pow_averagePwr );
        } else {
          //Serial.printf("summing actualPower: %u\n", activePwr );
        }
        
        pow_pwrFactor = (int) (100 * hlw8012.getPowerFactor());

#ifdef USE_POW_DBG
        DEBUG_PRINT("[HLW] Active Power (W)    : %u\n", activePwr     );
        DEBUG_PRINT("[HLW] Voltage (V)         : %u\n", voltage       );
        DEBUG_PRINT("[HLW] Current (A)         : %f\n", current );
        DEBUG_PRINT("[HLW] Apparent Power (VA) : %u\n", apparentPwr );
        DEBUG_PRINT("[HLW] Power Factor (%)    : %f\n", pwrFactor     );  
#endif // USE_POW_DBG
        _cumulatedEnergy += pow_activePwr*dt;  /// Wms  -> 1/(3600*1000) Wh
        while ( _cumulatedEnergy > Wh2Wms(1) ) {
         
          ++pow_cumulatedEnergy;
          _cumulatedEnergy -= Wh2Wms(1);
          if ( requestedEnergy > 0) {
            --requestedEnergy;
              g_semp->updateEnergy( _time,  -1,  0 );  
          } 
//          else {
//            // switch OFF
//            g_semp->updateEnergy( 0,  0 );
//            relayState = LOW;
//            g_semp->setPwrState( relayState );
//          }
          if ( optionalEnergy > 0)  {
            --optionalEnergy;
            g_semp->updateEnergy( _time, 0,  -1 );
          }
      
        }
        DEBUG_PRINT("[HLW] _cumulatedEnergy    : %ul\n", _cumulatedEnergy     );  
        DEBUG_PRINT("[HLW] cumulatedEnergy    : %u\n", pow_cumulatedEnergy     );  
        DEBUG_PRINT("[HLW] requestedEnergy    : %u\n", requestedEnergy     );  
        DEBUG_PRINT("[HLW] optionalEnergy     : %u\n", optionalEnergy     );  
        g_semp->updateEnergy( _time, 0,  0 );
        g_semp->setPwr( pow_averagePwr, minPwr, maxPwr);
        
#ifndef USE_POW_INT
        // When not using interrupts we have to manually switch to current or voltage monitor
        // This means that every time we get into the conditional we only update one of them
        // while the other will return the cached value.
        hlw8012.toggleMode();
#endif
    }
}

void setPwr(bool i_state )
{
  relayState = i_state;
}


#else
void setupPOW() {

}
void loopPOW() {}
#endif
