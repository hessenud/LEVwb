
class CTimer; // forward
class CTimer {
  unsigned long m_swTime;     ///< switch time in minutes
  unsigned long m_interval;     ///< switch time in minutes
  bool          m_switchmode; ///< true = on false = off
  int*          m_channel;    ///< switch number 
  void          (*m_channel_cb)(bool i_switchmode); ///<  call a delegate if any
  bool          m_hot;        //< time != swTime; hot<-true | when swTime is reached timer is executed and hot <- false
  
  CTimer*       m_next;  
public:
  CTimer(unsigned long i_swTime, unsigned long i_interval=0, bool i_switchmode=true, int* i_channel =0, void (*i_channel_cb)(bool)=0):
   m_swTime(i_swTime), m_interval(i_interval), m_switchmode(i_switchmode),m_channel(i_channel),m_channel_cb(i_channel_cb),m_hot(true){}  
  
  unsigned long getSwTime() { return m_swTime; }
  bool          hot() { return m_hot; }
  CTimer*       next(){ return m_next; }
  void          trigger();  
  void          append(CTimer* i_tmt);
  void          append_to(CTimer* i_list);
  bool          check(unsigned long _time);

  /** re-arm the timer by adding an interval to last trigger time
   *  the interval is applied until next trigger lies in the future,
   *  the standard interval is 0s = no repetition   // for repetition 1DAY == 24*3600s
   *  
   *  @param i_interval  if nonzero advance trigger by i_interval seconds else use m_interval 
   */
  void rearm(unsigned long i_interval=1 DAY) {
    m_hot = true; 
    if ( i_interval ) {
      m_interval = i_interval;
    }
    m_swTime +=  m_interval;
  }

  /**
   * set a CTimer
   * @param i_swTime
   * @param i_interval    repetition time 0 => sigle shot
   * @param i_switchmode  ON=true OFF=false
   * @param i_channel     e.g. PIN number 
   * @param i_channel_cb  callback function on trigger
   */
  void set(unsigned long i_swTime, unsigned long i_interval=0, bool i_switchmode=true, int* i_channel =0, void (*i_channel_cb)(bool)=0)
  {
    m_swTime = i_swTime;
    m_interval = i_interval;
    m_switchmode = i_switchmode;
    m_channel = i_channel;
    m_channel_cb = i_channel_cb;
    m_hot = true;  
  }


  ///  disarm timer  
  void disarm() { m_hot = false; }

  /// @return true if timer is repetitive
  bool is_repetitive() { return (m_swTime!=0) && (m_interval!=0); }
  
  /// @return true if timer is repetitive
  bool is_active() { return m_swTime!=0; }
  
};


void CTimer::append_to(CTimer* i_list)
{
  CTimer* wp= i_list;
  if (wp){
    while(  wp->next()) wp = wp->next();
    wp->m_next = this;
  }
}

void CTimer::append(CTimer* i_tmr)
{
  CTimer* wp= this;
  while(  wp->m_next)  wp = wp->next();
  wp->m_next = i_tmr;
}


void CTimer::trigger()
{
  if (m_channel) *m_channel = m_switchmode;
  if (m_channel_cb) m_channel_cb( m_switchmode );
  m_hot = false;
}

bool CTimer::check(unsigned long _time) 
{
  if(m_swTime >= _time) 
  {
    if ( m_hot  ) // hot an time reached
    {
      trigger(); 
      return true;
    } 
  } else {
    if (m_interval) rearm( 0 );  
  }
  return false;
}

///---------------
void requestDailyPlan(bool)
{
    unsigned long _now = getTime();
    g_semp->modifyPlan(0, _now, 1000,  6000,   daytime2today( 8 Hrs, _now) ,  daytime2today( 15 Hrs, _now ) );
}
///---------------

CTimer* g_Timerlist;
TimeClk  g_Clk;

 
void setupTimeClk(int i_timeZone)
{
  g_Clk.begin( i_timeZone ); 
  g_Timerlist = new CTimer( 8 * 3600 /*0800h*/, 1 DAY, true, 0, requestDailyPlan );

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
    for( CTimer* wp = g_Timerlist; wp; wp = wp->next() ) {
      wp->check(_time);
    }
  }
}
