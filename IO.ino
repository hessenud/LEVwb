#define LED_TICKLEN 100  // ms Ticklen

  
BlinkSignal    g_LED(LED_TICKLEN);
MorseCoder    g_Morse(LED_TICKLEN*2);

  
void loopIO()
{
  buttonControl(); // Manual ctrl of relay
  loopTimeClk();

//g_LED.tick();
//g_Morse.tick();
  {
    if ( g_Morse.is_active() ) {
      g_pow.setLED( !g_Morse.tick() );  
    } else { 
       g_pow.setLED(  !g_LED.tick() );
    } 
  }
}


void setupIO()
{
  g_LED.setIdle( 0x9 );
  
}
