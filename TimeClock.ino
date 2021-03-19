

///---------------
uTimerList g_Timerlist;

uTimer  g_Timers[N_TMR_PROFILES];
TimeClk  g_Clk;

///---------------


void  setTimer( const uTimerCfg& ) // i_cfg )
{
  
}

void genTimer( const uTimerCfg& i_cfg )
{
    uTimer* tmr = new uTimer( i_cfg, [](bool s) { g_pow->setRelay( s ); g_LED.set( 0x5f, 8, true);} );
    g_Timerlist.append( tmr );
}

///---------------
void dailyChores( bool )
{
    unsigned long _now = getTime();
    
    DEBUG_PRINT(" daily chores: \n");

    
  /* this is merily a test function
   * daily timeframes need good planning and coordination with autodetect/ manual request etc...
   * 
   * leave it this way until a use case crystalizes itself while usgn and experimenting....
   */
    if(g_semp) {
      for (unsigned n=PROFILE_TIMEFRAME; n < N_POW_PROFILES; ++n) {    
        PowProfile& prf =  g_prefs.powProfile[n];
        DEBUG_PRINT("requesting profile %u - > plan: %u ", n, (1+n-PROFILE_TIMEFRAME));         dump_profile( prf );  
        if ( prf.armed ) {
          // if armed and repeat => this frame shoudld be scheduled daily
          if ( prf.timeframe ) {
              g_semp->modifyPlanTime((1+n-PROFILE_TIMEFRAME), _now, prf.req,  prf.opt
                  ,TimeClk::daytime2unixtime( prf.est, _now)
                  ,TimeClk::daytime2unixtime( prf.let, _now ) );
          } else {
              g_semp->modifyPlan((1+n-PROFILE_TIMEFRAME), _now, prf.req,  prf.opt
                    , prf.est + _now
                    , prf.let +  _now );
          }
          if( !prf.repeat ) {
            prf.armed = false;
          }
        }
      }
   }
   
   DEBUG_PRINT("Profiles:\n------------------------------------------\n");    
   for (unsigned n=0; n < N_POW_PROFILES; ++n) {   
    dump_profile( g_prefs.powProfile[n] );
   }
}
///---------------

void setTimers() 
{
   unsigned long _now = getTime();
      
   for(  unsigned n= 0; n< N_TMR_PROFILES; ++n){
            
                DEBUG_PRINT(" Timer(%u) in %s\n", n, TimeClk::getTimeString(g_prefs.tmrProfile[n].sw_time) );
#if 0
                g_prefs.tmrProfile[n].sw_time = TimeClk::daytime2unixtime(g_prefs.tmrProfile[n].sw_time, _now);
                if( g_prefs.tmrProfile[n].sw_time <= _now ) g_prefs.tmrProfile[n].sw_time += 1 Day;
#else
                g_prefs.tmrProfile[n].sw_time += _now;
#endif
                 
                DEBUG_PRINT(" Timer(%u) in %s\n", n, TimeClk::getTimeString( g_prefs.tmrProfile[n].sw_time) );
                g_Timers[n].set(g_prefs.tmrProfile[n], [n](bool s) {
                    DEBUG_PRINT(" Timer(%u) set Pwr %s\n", n, s ? "ON": "OFF" );
                    g_pow->setRelay( s );
                    if ( s ) g_LED.set( 0x5f, 8, true);
                    else g_LED.reset();
                  }
                );
   }
}

void setupTimeClk(int i_timeZone)
{
    ///@todo: there are some good libraries for ntp/timezone etc out there...  eventually use them
    g_Clk.begin( i_timeZone, "fritz.box" );
    g_Timerlist.append( new uTimer( TimeClk::daytime2unixtime(1 Hrs, getTime()) /*0100h*/, 1 DAY, true, true, dailyChores ));

}

unsigned long getTime()
{
    return g_Clk.read();
}


void loopTimeClk()
{
    static unsigned long _last;
    unsigned long _time = getTime();
    static bool started;
    if ( !started ) {
      started = true;
      setTimers();
    }
    // Timer
    if ( _last != _time ) {
        _last = _time;
        for(  uTimer* wp = g_Timerlist.head(); wp; wp = wp->next() ) {
            wp->check(_time);
        }
        for(  unsigned n= 0; n< N_TMR_PROFILES; ++n){
            g_Timers[n].check(_time);
        }
    }
}
