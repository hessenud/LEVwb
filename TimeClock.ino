
///---------------
void requestDailyPlan(bool)
{
    unsigned long _now = getTime();
    g_semp->modifyPlan(0, _now, 2000 /*2kWh*/,  6000/*6kWh*/,   TimeClk::daytime2unixtime( 8 Hrs, _now) ,  TimeClk::daytime2unixtime( 15 Hrs, _now ) );
}
///---------------

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
