#ifndef POW_H
#define POW_H


#include "uSEMP.h"
#include "HLW8012.h"


#ifdef DEV_BOARD
# define LED_PIN 2
# define LED_LEVEL(l) (l)
#else
// the real deal
# define LED_PIN 15
# define LED_LEVEL(l) (!(l))
#endif
#define RELAY_PIN                       12



// Check values every 2 seconds
#define UPDATE_TIME                     3000


// HLW8012 -- POW Rev1
#define _POW_REV 1

#if _POW_REV == 1 
#elif _POW_REV == 2 
#else
# error "not supported"
#endif

#define HLW8012_SEL_PIN     5
#define HLW8012_CF1_PIN     13
#define HLW8012_CF_PIN      14

// CSE7766 -- POW Rev2 
///@todo REV2 uses CSE7766
#define CSE7766_PIN             1

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
    PROFILE_STD=0,
    PROFILE_QCK,
    PROFILE_P2,
    //--------------
    N_CHRG_PROFILES
};
struct ChgProfile {
    bool     timeOfDay;
    unsigned est;   
    unsigned let;
    unsigned req;
    unsigned opt;

    char est_s[6+1];// storage for times as strings
    char let_s[6+1];// needed for WebGui
};




/**----- POW ------
 * POW class encapsulates the hardware- and control of the Sonoff POW
 */
class POW {
    static HLW8012 hlw8012; ///@todo 
    uSEMP* semp;

    unsigned m_activePwr;
    unsigned m_voltage;
    double   m_current;
    unsigned m_apparentPwr;
    unsigned m_averagePwr;
    double   m_pwrFactor;
    unsigned m_cumulatedEnergy;


    int m_ledState;
    int m_relayState;

    void calibrate(    double expectedVoltage, double expectedPwr, double expectedCurrent );

    static void ICACHE_RAM_ATTR hlw8012_cf1_interrupt();
    static void ICACHE_RAM_ATTR hlw8012_cf_interrupt();
    void setInterrupts();


    unsigned long sumPwr;
    unsigned minPwrIdx,maxPwrIdx;
    unsigned minPwr,_minPwr;
    unsigned maxPwr,_maxPwr;
    unsigned powers[DIM_PWRS];
    unsigned pwrIdx;

public:

    static const int ledPin   = LED_PIN;
    static const int relayPin  = 12;  // Sonoff 12
    static const int buttonPin = 0;

    const int& ledState=m_ledState;
    const int& relayState=m_relayState;

    const unsigned& activePwr = m_activePwr;
    const unsigned& voltage = m_voltage;
    const double&   current =   m_current;
    const unsigned&  apparentPwr = m_apparentPwr;

    const unsigned& averagePwr = m_averagePwr;

    const double&  pwrFactor = m_pwrFactor;
    double   pwrMultiplier(){ return hlw8012.getPowerMultiplier();};
    double   currentMultiplier(){ return hlw8012.getCurrentMultiplier();};
    double   voltageMultiplier(){ return hlw8012.getVoltageMultiplier();};
    const unsigned  cumulatedEnergy = m_cumulatedEnergy;

    POW();
    void setup(uSEMP* i_semp);

    unsigned calcOnTime( unsigned requestedEnergy, unsigned avr_pwr);
    void setprefs( unsigned i_cumulatedEnergy, double i_pwrMultiplier, double i_currentMultiplier, double i_voltageMultiplier);

    void handlePwrReq();
    void handleCalReq();
    void handleEnergyReq();

    void setPwr(bool i_state );
    void toggleRelay();
    void setLED(bool i_state );
    void toggleLED();

    void loop();
};

#endif
