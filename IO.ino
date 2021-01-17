
#define LED_TICKLEN 100  // ms Ticklen
//-------------------------------------
PushButton* key;
  //  
  
BlinkSignal    g_LED(LED_TICKLEN);
MorseCoder    g_Morse(LED_TICKLEN*2);


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

  {
    int keyEvt = key->getEvent();
    switch ( keyEvt ) {
      case PushButton::UP:
          DEBUG_PRINT("KEY UP RESET LEDpattern\n");
          g_LED.reset();
          break;
      case PushButton::DN:
          DEBUG_PRINT("KEY DN reset Morse\n");
          g_Morse.reset();
          g_LED.set(5,4,true);
          break;
      case PushButton::CLICK:
          g_pow->toggleRelay();// relayState = !relayState;
          DEBUG_PRINT(" Relay: %s\n", g_pow->relayState ? "ON" :"OFF");
          g_LED.reset();
          break;
      case PushButton::DN_LONG1:
          DEBUG_PRINT(" Reset all plans %s\n", g_pow->relayState ? "ON" :"OFF");
          g_semp->deleteAllPlans( );
          g_LED.reset();
          break;
      case PushButton::DN_LONG2:
          g_Morse.set(".....  -----");
            g_semp->deleteAllPlans( );
          g_LED.reset();
          {
            unsigned long _now = getTime();
  #ifdef DEV_BOARD
            g_semp->modifyPlan(0, _now, 100, 300,  _now ,  _now+200 );     
  #else
            g_semp->modifyPlan(0, _now, 3000,  6000,  _now ,  _now+8000 );
  #endif
          }
          
          g_LED.reset();
          break;
     case PushButton::DBLCLICK: 
          g_Morse.next("..  --", true );
          g_LED.reset();
          g_semp->setPwrState( true );
          DEBUG_PRINT(" DBLCLICK!!!: %s\n", g_pow->relayState ? "ON" :"OFF");
          break;
     case PushButton::TRIPLECLICK:
          g_Morse.next(".. ---", true );
          g_LED.reset();
           g_semp->setPwrState( false );
          DEBUG_PRINT(" TRIPLECLICK!!!: %s\n", g_pow->relayState ? "ON" :"OFF");
          break;
     case PushButton::QUADCLICK:
          g_Morse.next(".. ----", true );
          g_LED.reset();
          DEBUG_PRINT(" QUADCLICK!!!: %s\n", g_pow->relayState ? "ON" :"OFF");
          break;
    } 
  }
}

  
void loopIO()
{
  buttonControl(); // Manual ctrl of relay
  loopTimeClk();

//g_LED.tick();
//g_Morse.tick();
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
  g_LED.setIdle( 0x9 );
  
}
