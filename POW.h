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

    void begin( unsigned char i_rx_pin, unsigned i_baudrate ) { 
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
    unsigned  m_minPwrIdx;
    unsigned  m_maxPwrIdx;
    unsigned  m_powers[DIM_PWRS];
    unsigned  m_pwrIdx;
    unsigned long m_last_update;

    int m_ledPin;
    int m_relayPin;  
    int m_buttonPin;

public:

    attr_reader( int, buttonPin );  
    attr_reader( int, ledState );  
    attr_reader( int, relayState );  

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

    void handlePwrReq();
    void handleCalReq();
    void handleEnergyReq();
    void handleTimeReq();

    void setPwr(bool i_state );
    void toggleRelay();
    void setLED(bool i_state );
    void toggleLED();

    void loop();
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
