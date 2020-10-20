


//-------------------------------------
PushButton key(buttonPin, 1500, 5000, 350);

void buttonControl()
{
  int keyEvt = key.getEvent();
  switch ( keyEvt ) {
    case PushButton::PRESSED:
        relayState = !relayState;
        Serial.printf(" Relay: %s\n", relayState ? "ON" :"OFF");
        
        g_LED.set(0x11,8);
        {
          static unsigned cnt;
          static char buf[10];
          sprintf(buf, "%1d ", (++cnt)%10);
          g_Morse.queue( buf );   
        }
        break;
    case PushButton::DN_LONG2:
        Serial.printf(" RESET Settings!: %s\n", relayState ? "ON" :"OFF");
        g_LED.set(0x2222ff,32); 
        
        g_Morse.queue("?      ", true );
        break;
   case PushButton::DBLCLICK:
       // relayState = LOW;
        g_LED.set(0x3,4,true);
        g_Morse.queue(". - . - . - . - . -", true );
        Serial.printf(" Doubleclick!!!: %s\n", relayState ? "ON" :"OFF");
        break;
  } 
}
