#ifdef USE_OLED
void setupOLED()
{
  if( g_prefs.use_oled ) 
  {
    //Serial.println("SETUP OLDET allocating SSD1306\n");

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address  for 128x64
        //Serial.println("SSD1306 allocation failed");
        for(;;); // Don't proceed, loop forever
    }

    //display.setRotation(2);
    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    display.display();
  }
}

void displayTime(unsigned long theTime )
{
    const char* timestr = TimeClk::getTimeString( theTime );
    // print the hour, minute and second:
    display.print( timestr ); // print the second


    //----------------------------
    // print the hour, minute and second:
    //Serial.print("The local time is ");       
    //Serial.println(timestr); 
}



void draw( const char* i_text) {
  if( g_prefs.use_oled )
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);        // Draw white text
    display.setCursor(0,0);             // Start at top-left corner
    display.println( i_text );
    //
    //  display.setCursor(0,48);             // Start at top-left corner
    //  display.setTextSize(1);  display.print( "* " );
    //
    //  display.setTextSize(2);             //  2x pixel scale
    //  display.print(  TimeClk::getTimeString( getTime() ) );
    //
    //  display.setTextSize(1);  display.print( " *" );
    display.display();
  }
}
#else
void setupOLED(){}
void draw( const char* ){}

#endif
