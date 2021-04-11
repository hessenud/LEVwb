/*
 * Prefs.h

 */

#ifndef PREFS_H_
#define PREFS_H_

#define  N_PREFS 23
#define  PREFS_REVISION 2


/**
 * Global preferences are kept global. This is the simplest way for an application as small as this. 
 * Cleaner code should store the preferences locally in the corresponding objects. Like max power in POW...
 * 
 * They will move there eventually when i know which direction this software will go... right now it's
 * merily a playground, so i apply playground rules wherever i like/ deem appropriate
 */
class Prefs {
public:

    Prefs();
    const char* hostname;
    const char* ota_passwd;
    unsigned    assumed_power;      ///< assumed consumption for time calculation
    const char* mqtt_broker;        ///< hostname of mqtt broker
    unsigned    mqtt_broker_port;   ///< port of mqtt broker
    const char* mqtt_user;          ///< username for mqtt broker
    const char* mqtt_password;      ///< password of mqtt user

    const char* device_name;        ///< Device name for SEMP identification
    const char* model_name;         ///< model name for SSDP id
    unsigned    serialNr;           ///< yeah well... a real SN should be stored in a place not as easyly erased as flash-- 
    unsigned    updateTime;         ///< update time in milliseconds -- not used yet - if made dynamic the power averaging array has to be adapted
    unsigned    modelVariant;       ///< model variant 1-Pow R1  2- Pow R2

    bool        use_oled;           ///< use OLED driver (Devboard)

    unsigned    devType;            ///< SEMP defined device Type   uSEMP::devTypeStr( typeId );
    unsigned    maxPwr;             ///< SEMp Maximum Power -> assumed Power
    bool        intr;               ///< SEMP interrupible
    unsigned    defCharge;          ///< default charge energy // reqested ON time
    bool        autoDetect;         ///< autodetect energy need
    unsigned    ad_on_threshold;    ///< active power in [W] threshold for detection of energy need/start of a job
    unsigned    ad_on_time;         ///< if on threshold is surpassed for xxx[s] a new energy request ist detected
    unsigned    ad_off_threshold;   ///< lower active power in [W] threshold for detection of END or active job/energy need
    unsigned    ad_off_time;        ///< if active power stays below lower threshold for xxx[s] the running job is considered finished
    unsigned    ad_prolong_inc;     ///< increment in[s] to prolong a running request to let device finish the job
    unsigned    timezone;           ///< tz in 100/th hour

    PowProfile  powProfile[N_POW_PROFILES];
    
#define N_TMR_PROFILES 2
    uTimerCfg tmrProfile[N_TMR_PROFILES];

    bool        loaded;

    static const int capacity =   4096; 
};



#define storePref( __prf, __src )  cfg[ #__prf ]= __src.__prf
#define _loadPref( __prf, __src )     __src.__prf=((cfg[ #__prf ]))
#define _loadPrefStr( __prf, __src )  replaceString(((char**)&__src.__prf), cfg[ #__prf ])
#define loadPref( __prf, __def, __src )     __src.__prf=((cfg[ #__prf ]) | __def)
#define loadPrefStr( __prf, __def, __src )  replaceString(((char**)&__src.__prf),  cfg[ #__prf ] | __def)

#define loadProfileXMember(__s, __idx, __src, __member )    __src.__s[__idx].__member = cfg[ #__s ][__idx][ #__member ]
#define storeProfileXMember(__s, __idx, __src, __member )    cfg[ #__s ][__idx][ #__member ] = __src.__s[__idx].__member
    

#endif /* PREFS_H_ */
