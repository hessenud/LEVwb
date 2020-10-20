



#define LED_TICKLEN 100  // ms Ticklen

  
BlinkSignal    g_LED(LED_TICKLEN);
MorseCoder    g_Morse(LED_TICKLEN*2);

  
void loopIO()
{
  static int _lastLed;
  static int _lastRelay;
  
  buttonControl(); // Manual ctrl of relay
  loopTimeClk();

  
  if ( g_Morse.is_active() ) ledState = !g_Morse.tick();
  else  ledState = !g_LED.tick();
  if (_lastLed != ledState ) digitalWrite(ledPin, (_lastLed = ledState ) );
  if (_lastRelay != relayState ) {
    digitalWrite(relayPin, (_lastRelay = relayState) );
  }
}


void setupIO()
{
    // initialize digital pin LED_BUILTIN as an output.
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  pinMode(buttonPin, INPUT);
  digitalWrite(relayPin, relayState );
  digitalWrite(ledPin,   ledState );
  
  g_LED.setIdle( 0x101 );
  
}
