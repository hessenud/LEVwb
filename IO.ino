
#define LED_TICKLEN 100  // ms Ticklen
//-------------------------------------
PushButton* key;
  //  
  
BlinkSignal    g_LED(LED_TICKLEN);
MorseCoder     g_Morse(LED_TICKLEN*2);


void handleAppEvt( AppEvt_t i_evt, void* i_par)
{
  switch(i_evt)
  {
    case APP_EOR:
        //End of request or plan 
        DEBUG_PRINT("handleAppEvt:  END OF REQUEST %d par:%p\n", i_evt, i_par);
        if(myLog) myLog->log("handleAppEvt:  END OF REQUEST");
        g_LED.reset();
        g_pow->m_online= true; // default is: resume control by EM if active request ends
        break;
    case APP_REQ: // new request active
        g_LED.set( 0x5ff, 12, true);
        g_pow->m_online= true; // default is: resume control by EM if a new request is planned
        DEBUG_PRINT("handleAppEvt: NEW REQUEST gone ACTIVE%d par:%p\n", i_evt, i_par);
        if(myLog) myLog->log("handleAppEvt:  NEW REQUEST gone ACTIVE");
        break;
    case APP_IDLE:
        break;
    default:
       DEBUG_PRINT("handleAppEvt: UNKNOWN %d par:%p\n", i_evt, i_par);
    ;
  }
}


/**
 * @todo  local control  activate some pre defined/configured patterns like
 *      - request a certain amount of energy
 *      - forced start
 *      - cancel an active request
 *      - increase amount of energy for a request
 *      - ....
 *      
 *      CLICK:>  toggle switch ignoring EM Control
 *      DblCLick:> initiate a standard request
 *      TripleClick:> Initiate Quick/Immediate Request
 *      QuadClick:> 
 *      
 *      DN_LONG1:>  reset all active Plans
 */
void buttonControl()
{
    int keyEvt = key->getEvent();
    switch ( keyEvt ) {
      case PushButton::UP:;
          g_LED.reset();
          break;
      case PushButton::DN:
          g_Morse.reset();
          g_LED.set(1,2,true);
          break;
      case PushButton::CLICK:
          g_pow->toggleRelay();// relayState = !relayState;
          DEBUG_PRINT(" Relay: %s\n", g_pow->relayState ? "ON" :"OFF");
          if(myLog) myLog->log(String("PushButton::CLICK Relay: ") + ( g_pow->relayState ? "ON\n" :"OFF\n" ));
          pushStat();
          break;
      case PushButton::TRIPLECLICK:
          if(myLog) myLog->log("PushButton::TRIPLECLICK: ");
          break;
      case PushButton::DN_LONG1:
          DEBUG_PRINT(" Reset all plans %s\n", g_pow->relayState  ? "ON" :"OFF");
          g_semp->deleteAllPlans( );
          g_pow->resetAutoDetectionState();
          g_LED.reset();
          g_Morse.reset();
          g_pow->m_online = true;
          if(myLog) myLog->log(String(" Reset all plans: ") +  (g_pow->relayState ? "ON\n" :"OFF\n"));
          break;
     case PushButton::DN_LONG2:
          g_Morse.set("factory default settings...");
          if(myLog) myLog->log("factory default settings...\n");
          break;
     case PushButton::DBLCLICK:
          // make a manual AD request and run forcibly
           if(myLog) myLog->log("PushButton::DBLCLICK: ");
     
          g_pow->forceAdRequest();
          g_Morse.next("ar ", true );
          g_LED.reset();
          g_pow->setRelay( true );
          g_pow->m_online = false;

          DEBUG_PRINT(" DBLCLICK!!!: %s\n", g_pow->relayState ? "ON" :"OFF");
          break;
     case PushButton::QUADCLICK:
          if(myLog) myLog->log("PushButton::QUADCLICK: ");
     
          g_Morse.next("..  --  ", true );
          g_LED.reset();
          static bool s_simOn = true;
          g_pow->setSimPwr( (s_simOn ^=true) );
          if(myLog) myLog->log( s_simOn ? "SIM ON\n" :"SIM OFF\n");
          break;
     
  } 
}

  
void loopIO()
{
  buttonControl(); // Manual ctrl of relay
  loopTimeClk();

  if ( g_pow ) {
    if ( g_Morse.is_active() ) {
      g_pow->setLED( !g_Morse.tick() );  
    } else { 
       g_pow->setLED(  !g_LED.tick() );
    } 
  }
}


void setupIO()
{
  key = new PushButton(g_pow ? g_pow->buttonPin : 0, 30, 3000, 8000, 300);
  DEBUG_PRINT("setupIO g_pow: %p k:%p\n", g_pow, key);
  g_LED.setIdle( 0x5 );
}
