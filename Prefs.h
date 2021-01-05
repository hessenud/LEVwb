/*
 * Prefs.h

 */

#ifndef PREFS_H_
#define PREFS_H_

#define  N_PREFS 23
#define  PREFS_REVISION 2
class Prefs {
public:
    Prefs();
    unsigned    revision; 
    const char* hostname;
    const char* ota_passwd;
    unsigned    assumed_power;      ///< assumed consumption for time calculation
    const char* mqtt_broker;        ///< hostname of mqtt broker
    unsigned    mqtt_broker_port;   ///< port of mqtt broker
    const char* mqtt_user;          ///< username for mqtt broker
    const char* mqtt_password;      ///< password of mqtt user

    const char* device_name;        ///< Device name for SEMP identification
    const char* model_name;         ///< model name for SSDP id
    unsigned    serialNr;
    unsigned    updateTime;         ///< update time in milliseconds
    unsigned    modelVariant;       ///< model variant 1-Pow R1  2- Pow R2


    unsigned    devType;            ///< SEMP defined device Type   uSEMP::devTypeStr( typeId );
    unsigned    maxPwr;             ///< SEMp Maximum Power -> assumed Power
    bool        intr;                ///< SEMP interrupible
    unsigned    defCharge;           ///< default charge energy
    bool        use_oled;
    
    ChgProfile  chgProfile[N_CHRG_PROFILES] = {
            {true, 11 Hrs,15 Hrs, 3000, 6000,{'\0'},{'\0'}},
            {false, 0,0, 3000, 0,{'\0'},{'\0'}},
            {false, 0,3 Hrs, 4000, 6500,{'\0'},{'\0'}}
    };

    bool        loaded;

    static const int capacity =   1024; 
};



#define storePref( __prf )  cfg[ #__prf ]= g_prefs.__prf 
#define _loadPref( __prf )     g_prefs.__prf=((cfg[ #__prf ]))
#define _loadPrefStr( __prf )  replaceString(((char**)&g_prefs.__prf), cfg[ #__prf ])
#define loadPref( __prf, __def )     g_prefs.__prf=((cfg[ #__prf ]) | __def)
#define loadPrefStr( __prf, __def )  replaceString(((char**)&g_prefs.__prf),  cfg[ #__prf ] | __def)


#endif /* PREFS_H_ */
