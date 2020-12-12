
///---------------
void requestDailyPlan(bool)
{
    unsigned long _now = getTime();
    g_semp->modifyPlan(0, _now, g_chgProfile[PROFILE_STD].req,  g_chgProfile[PROFILE_STD].opt
          ,TimeClk::daytime2unixtime( g_chgProfile[PROFILE_STD].est, _now) 
          ,TimeClk::daytime2unixtime( g_chgProfile[PROFILE_STD].let, _now ) );
}
///---------------


///---------------

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

uTimer* g_Timerlist;
TimeClk  g_Clk;

 
void setupTimeClk(int i_timeZone)
{
  g_Clk.begin( i_timeZone ); 
  
  g_Timerlist = new uTimer( TimeClk::daytime2unixtime(8 Hrs, getTime()) /*0800h*/, 1 DAY, true, 0, requestDailyPlan );
 
}

unsigned long getTime()
{
  return g_Clk.read();
}


void loopTimeClk()
{
  static unsigned long _last;
  unsigned long _time = getTime(); 
 // Timer
  if ( _last != _time ) {
    _last = _time;
    for( uTimer* wp = g_Timerlist; wp; wp = wp->next() ) {
      wp->check(_time);
    }
  }
}
