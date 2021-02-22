#ifndef POW_H
#define POW_H

#include "uSEMP.h"
#include "HLW8012.h"
#include "CSE7766.h"


#define RELAY_PIN         12

#define LED_PIN_R1        15
#define LED_PIN_R2        13
#define LED_PIN_DEVBOARD  2


// Check values every 2 seconds
#define UPDATE_TIME                     3000

#define HLW8012_SEL_PIN     5
#define HLW8012_CF1_PIN     13
#define HLW8012_CF_PIN      14

// CSE7766 -- POW Rev2 
///@todo REV2 uses CSE7766
#define CSE7766_PIN             1
// Needs CSE7766 Energy sensor, via Serial RXD 4800 baud 8E1 (GPIO1), TXD (GPIO3)


// Set SEL_PIN to HIGH to sample current
// This is the case for Itead's Sonoff POW, where a
// the SEL_PIN drives a transistor that pulls down
// the SEL pin in the HLW8012 when closed
#define CURRENT_MODE                    HIGH

// These are the nominal values for the resistors in the circuit
#define CURRENT_RESISTOR                0.001
#define VOLTAGE_RESISTOR_UPSTREAM       ( 5 * 470000 ) // Real: 2280k
#define VOLTAGE_RESISTOR_DOWNSTREAM     ( 1000 ) // Real 1.009k

#define RELAY_ON    true
#define RELAY_OFF   false
#define LED_ON      true
#define LED_OFF     false

#define REF_PWR 500.0
#define REF_VOLT 230.0
#define DIM_PWRS ((60000+500)/UPDATE_TIME)

#define Wh2Wms( e ) ((e)*3600*1000)

enum {
    PROFILE_START=0,
    PROFILE_STD=PROFILE_START,
    PROFILE_QCK,
    PROFILE_P2,
    //--------------
    PROFILE_TIMEFRAME,
    PROFILE_TIMEFRAME_1 = PROFILE_TIMEFRAME,
    PROFILE_TIMEFRAME_2,
    PROFILE_TIMEFRAME_3,
    PROFILE_TIMEFRAME_MAX = PROFILE_TIMEFRAME_3,
    //--------------
    N_POW_PROFILES,
    N_TIMEFRAMES = PROFILE_TIMEFRAME_MAX - PROFILE_TIMEFRAME +1
};

struct PowProfile {
    static const bool TIMF = true;
    static const bool NRGY = false;
    static const bool ToD  = true;
    static const bool REL  = false;

    static const bool REPT  = true;
    static const bool ONCE  = false;
    static const bool ARMD  = true;
    static const bool IDLE  = false;

    bool     valid;     ///< thsi frame is valid for planning
    bool     timeframe; ///< true => time oriented plan false => energy oriented
    bool     timeOfDay; ///< true let and est are offset [s] form 00:00 as a Time of day otherwise its relative time [s] from now
    bool     armed;     ///< armed for planning generation
    bool     repeat;    ///< re-arm this frame after completion  

    unsigned est;   // relative time from now in [s] or TimeOfDay in [s] from 0:00
    unsigned let;   // -"-  
    unsigned req;   // 
    unsigned opt;   // 

    char est_s[6+1];// storage for times as strings
    char let_s[6+1];// needed for WebGui

    bool used() { return req !=0; }
    void clear() { req = opt = let = est = 0; armed = repeat = timeframe = timeOfDay = false;   *est_s = *let_s=0; }

    PowProfile() {
        clear();
    }

    PowProfile(   bool     i_tf, bool     i_tod,bool   i_armed, bool i_repeat,  unsigned i_est, unsigned i_let, unsigned i_req, unsigned i_opt )
    :timeframe(i_tf), timeOfDay(i_tod), armed(i_armed), repeat(i_repeat), est(i_est), let(i_let), req(i_req), opt(i_opt)
    {
        *est_s=0;
        *let_s=0;
    }
};

/**
 * PwrSensor provides an interface for Pwr Sensors.
 * It' more or less the same Interface as the HLW8012 class.
 */
class PwrSensor {

public:

    virtual ~PwrSensor(){}

    virtual double getCurrent()=0;
    virtual unsigned int getVoltage()=0;
    virtual unsigned int getActivePower()=0;
    virtual unsigned int getApparentPower()=0;
    virtual double getPowerFactor()=0;
    virtual unsigned int getReactivePower()=0;

    virtual void setResistors(double current, double voltage_upstream, double voltage_downstream)=0;

    virtual void expectedCurrent(double current)=0;
    virtual void expectedVoltage(unsigned int voltage)=0;
    virtual void expectedActivePower(unsigned int power)=0;

    virtual double getCurrentMultiplier()=0;
    virtual double getVoltageMultiplier()=0;
    virtual double getPowerMultiplier()=0;

    virtual void setCurrentMultiplier(double current_multiplier)=0;
    virtual void setVoltageMultiplier(double voltage_multiplier)=0;
    virtual void setPowerMultiplier(double power_multiplier)=0;
    virtual void resetMultipliers()=0;

    virtual void loop() {};
};

class PwrSensHLW8012: public PwrSensor {
    HLW8012 m_hlw;

public:

    void ICACHE_RAM_ATTR cf1_interrupt(){ m_hlw.cf1_interrupt(); }
    void ICACHE_RAM_ATTR cf_interrupt(){ m_hlw.cf_interrupt();  }

    virtual ~PwrSensHLW8012(){}
    HLW8012* getHLW8012() { return &m_hlw; }

    void begin(
            unsigned char cf_pin,
            unsigned char cf1_pin,
            unsigned char sel_pin,
            unsigned char currentWhen = HIGH,
            bool use_interrupts = true,
            unsigned long pulse_timeout = PULSE_TIMEOUT) { 
              (void)pulse_timeout;
        // Initialize HLW8012
        // void begin(unsigned char cf_pin, unsigned char cf1_pin, unsigned char sel_pin, unsigned char currentWhen = HIGH, bool use_interrupts = false, unsigned long pulse_timeout = PULSE_TIMEOUT);
        // * cf_pin, cf1_pin and sel_pin are GPIOs to the HLW8012 IC
        // * currentWhen is the value in sel_pin to select current sampling
        // * set pulse_timeout to 500ms for a fast response but losing precision (that's ~24W precision :( )
        m_hlw.begin(cf_pin, cf1_pin, sel_pin, currentWhen, use_interrupts); };

    double getCurrent(){ return  m_hlw.getCurrent();}
    unsigned int getVoltage(){ return  m_hlw.getVoltage();}
    unsigned int getActivePower(){ return  m_hlw.getActivePower();}
    unsigned int getApparentPower(){ return  m_hlw.getApparentPower();}
    double getPowerFactor(){ return  m_hlw.getPowerFactor();}
    unsigned int getReactivePower(){ return  m_hlw.getReactivePower();}

    void setResistors(double current, double voltage_upstream, double voltage_downstream){  m_hlw.setResistors( current,  voltage_upstream,  voltage_downstream);}

    void expectedCurrent(double current){   m_hlw.expectedCurrent(current);}
    void expectedVoltage(unsigned int voltage){   m_hlw.expectedVoltage(voltage);}
    void expectedActivePower(unsigned int power){   m_hlw.expectedActivePower(power);}

    double getCurrentMultiplier() { return m_hlw.getCurrentMultiplier(); };
    double getVoltageMultiplier() { return m_hlw.getVoltageMultiplier(); };
    double getPowerMultiplier() { return m_hlw.getPowerMultiplier(); };

    void setCurrentMultiplier(double current_multiplier) { m_hlw.setCurrentMultiplier(current_multiplier); };
    void setVoltageMultiplier(double voltage_multiplier) { m_hlw.setVoltageMultiplier(voltage_multiplier); };
    void setPowerMultiplier(double power_multiplier) { m_hlw.setPowerMultiplier(power_multiplier); };
    void resetMultipliers() { m_hlw.resetMultipliers(); }

};


class PwrSensCSE7766: public PwrSensor {
    CSE7766 m_cse;

public:

    virtual ~PwrSensCSE7766(){}

    void begin( unsigned char i_rx_pin, unsigned ) { 
        m_cse.setRX( i_rx_pin );
        m_cse.begin();
    };
    double getCurrent(){ return  m_cse.getCurrent();}
    unsigned int getVoltage(){ return  m_cse.getVoltage();}
    unsigned int getActivePower(){ return  m_cse.getActivePower();}
    unsigned int getApparentPower(){ return  m_cse.getApparentPower();}
    double getPowerFactor(){ return  m_cse.getPowerFactor();}
    unsigned int getReactivePower(){ return  m_cse.getReactivePower();}

    void setResistors(double , double , double ){ }

    void expectedCurrent(double current){   m_cse.expectedCurrent(current);}
    void expectedVoltage(unsigned int voltage){   m_cse.expectedVoltage(voltage);}
    void expectedActivePower(unsigned int power){   m_cse.expectedPower(power);}

    double getCurrentMultiplier() { return m_cse.getCurrentRatio(); };
    double getVoltageMultiplier() { return m_cse.getVoltageRatio(); };
    double getPowerMultiplier() { return m_cse.getPowerRatio(); };

    void setCurrentMultiplier(double current_multiplier) { m_cse.setCurrentRatio(current_multiplier); };
    void setVoltageMultiplier(double voltage_multiplier) { m_cse.setVoltageRatio(voltage_multiplier); };
    void setPowerMultiplier(double power_multiplier) { m_cse.setPowerRatio(power_multiplier); };
    void resetMultipliers() { m_cse.resetRatios(); }

    void loop() { m_cse.handle();}
};



class PwrSensSim: public PwrSensor {

    unsigned long m_last_update;
    unsigned m_sim_rd;
    static const unsigned hlw_sim[];
    static const unsigned hlw_vsim[];

    void ICACHE_RAM_ATTR _isr_A(){ }
    void ICACHE_RAM_ATTR _isr_B(){ }


public:

    virtual ~PwrSensSim(){}

    void begin( ) {
        m_sim_rd = 0;
    };

    unsigned int getVoltage();
    unsigned int getActivePower();
    unsigned int getApparentPower();

    double getCurrent(){ return  getActivePower()/getVoltage();}
    double getPowerFactor(){ return  1.0; }
    unsigned int getReactivePower(){ return  getPowerFactor() * getActivePower();}

    void setResistors(double , double , double ){ }

    void expectedCurrent(double ){  }
    void expectedVoltage(unsigned int ){ }
    void expectedActivePower(unsigned int ){}

    double getCurrentMultiplier() { return 1.0; };
    double getVoltageMultiplier() { return 1.0;};
    double getPowerMultiplier()   { return 1.0;};

    void setCurrentMultiplier(double ) {  };
    void setVoltageMultiplier(double ) {  };
    void setPowerMultiplier(double ) {  };
    void resetMultipliers() { }

    void loop();

};


/**----- POW ------
 * POW class encapsulates the hardware- and control of the Sonoff POW
 * @todo  thsi will morph into a base class and instances of POW will be 
 *        0 - SImulation      sim
 *        1 - Sonoff POW      HLW8012
 *        2 - Sonoff POW R2   CES7766
 */

typedef enum  {
    AD_REQUEST = 1,
    AD_IDLE    = 0,
    AD_END_REQUEST = -1
} ad_event_t;


class POW {
protected:
    PwrSensor* m_sense; 
    uSEMP*    m_semp;
    unsigned  m_activePwr;
    unsigned  m_voltage;
    double    m_current;
    unsigned  m_apparentPwr;
    unsigned  m_averagePwr;
    double    m_pwrFactor;
    unsigned  m_cumulatedEnergy;
    unsigned _cumulatedEnergy; // delta of comulated energy 



    int       m_ledState;
    bool      m_ledLogic;
    int       m_relayState;
    bool      m_relayLogic;

    void calibrate(    double expectedVoltage, double expectedPwr, double expectedCurrent );

    unsigned long m_sumPwr;
    unsigned  m_minPwr;
    unsigned  m_maxPwr;
    unsigned  m_powers[DIM_PWRS];
    unsigned  m_pwrIdx;
    unsigned long m_last_update;

    int m_ledPin;
    int m_relayPin;  
    int m_buttonPin;

    int m_activeProfile; ///< index of active Profile


    /**
     * @brief find Timeframe, generate a Energy request and switch off (if appropriate)
     *
     */
    void procAdRequest();


    /**
     * @brief find a matching timeframe for a request
     * @param  i_nrgy -- true if energy profile is searched
     * @param  i_req  -- the energy amount/minOnTime of the request 
     * @return the profile containing a matching timeframe
     */
    int findTimeFrame( bool i_nrgy,unsigned i_req  );

    /**
     * prolong an active PlanningRequest if a device needs more energy to complete the running job.
     * like washing machines.
     * A non-interruptible machine with autodetect feature should prolong an active timeframe until operation
     * completed. ProlongActivePlan() is only used if a request is already active/started but was not sufficient
     * because the job took longer than expected/planned. This happens for example with washing machines that adapt
     * to the needs of the specific job at hand. Or a dryer that needs more time because humidity is too high...
     *
     */
    void prolongActivePlan();

    typedef enum {
        AD_OFF,
        AD_TEST_ON,
        AD_ON,
        AD_TEST_OFF
    } ad_state_t;

    ad_state_t m_ad_state;

public:

    /**
     * @brief detect start / end of energy flow for atomated generation of energy requests to EM
     * ®return AD_REQUEST(1) = energy needed, AD_END_REQUEST(-1) = end of Request , AD_IDLE(0) = no decission
     */
    ad_event_t autoDetect(); 

    
public:
    
    uDelegate  m_application;

    attr_reader( int, ledState );
    attr_reader( int, relayState );
    
    attr_reader( int, buttonPin );  
    attr_reader( int, relayPin );
    attr_reader( int, ledPin );

    attr_reader( unsigned, activePwr );
    attr_reader( unsigned, voltage );
    attr_reader( double,   current );
    attr_reader( unsigned, apparentPwr );

    attr_reader( unsigned, averagePwr );

    attr_reader( double, pwrFactor );
    attr_reader( unsigned, cumulatedEnergy );

    double   pwrMultiplier(){ return m_sense ? m_sense->getPowerMultiplier() : 1;};
    double   currentMultiplier(){ return m_sense ? m_sense->getCurrentMultiplier(): 1;};
    double   voltageMultiplier(){ return m_sense ? m_sense->getVoltageMultiplier(): 1;};

    POW( uSEMP* i_semp );
    void setup();

    unsigned calcOnTime( unsigned requestedEnergy, unsigned avr_pwr);
    void setprefs( unsigned i_cumulatedEnergy, double i_pwrMultiplier, double i_currentMultiplier, double i_voltageMultiplier);

    void resetAutoDetectionState() { m_ad_state = AD_OFF; }
    void handlePwrReq();
    void handleCalReq();
    void handleEnergyReq();
    void handleTimeReq();
    void requestProfile();

   /**
     * true means the device is under control of the EM
     * false => the device runs w/o EM control. E.g. on forced operation even when EM suggests OFF
     *          of when using a pure Timer, or alternative decision
     */
    bool online;
    
    /**
     * @param i_state   bool true -> relay/pwr on
     */
    void setPwr(bool i_state );

    /**
     * signal EnergyManager State to POW 
     * @param i_em_state    ON/OFF/OFFLINE
     * @param i_recommendedPwr  recommended power
     *
     */
    void rxEmState(EM_state_t i_em_state, unsigned i_recommendedPwr=0 );

    /**
     * signal active plan is exhausted
     */
    void endOfPlan(  );
    
    void toggleRelay();
    void setLED(bool i_state );
    void toggleLED();

    void loop();


    void dump();
};

class POW_R1: public POW {
    void setInterrupts();

public:
    POW_R1( uSEMP* i_semp);
};


class POW_0: public POW {
public:
    POW_0( uSEMP* i_semp);
};


class POW_R2: public POW {
public:
    POW_R2( uSEMP* i_semp);
};


class POW_Sim: public POW {
public:
    POW_Sim( uSEMP* i_semp);
};


POW*  newPOW( unsigned i_variant, uSEMP* i_semp );

#endif
