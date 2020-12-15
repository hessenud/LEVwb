//-------------------------------------
PushButton key(g_pow.buttonPin, 30, 3000, 8000, 300);

void buttonControl()
{
  int keyEvt = key.getEvent();
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
        g_pow.toggleRelay();// relayState = !relayState;
        DEBUG_PRINT(" Relay: %s\n", g_pow.relayState ? "ON" :"OFF");
        g_LED.reset();
        break;
    case PushButton::DN_LONG2:
        DEBUG_PRINT(" Reset all plans %s\n", g_pow.relayState ? "ON" :"OFF");
        g_semp->deleteAllPlans( );
        g_LED.reset();
        break;
            case PushButton::DN_LONG1:
        DEBUG_PRINT(" Request 3KWh: %s\n", g_pow.relayState ? "ON" :"OFF");
        g_Morse.set(".....    - - -");
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
       // relayState = LOW;
        g_Morse.next(". - . - . - . - . -", true );
        g_LED.reset();
        DEBUG_PRINT(" Doubleclick!!!: %s\n", g_pow.relayState ? "ON" :"OFF");
        break;
  } 
}
