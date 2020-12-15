#ifndef _MQTT_H_
#define _MQTT_H_
#ifdef MQTTBROKER

#include <uHelper.h>
#include <ArduinoMqttClient.h>

class PowMqtt {
    POW* m_pow;
    const char* mkTopicPath(const char* hostname,  const char* topic );

    struct PowMqttCfg {
        const char* led;
        const char* relay;
        const char* _time;
        const char* pwr;
        const char* volt;
        const char* curr;
        const char* energy_total;
        const char* energy_requested;
        const char* energy_optional;
        const char* pushButton;
    } m_topics;

    WiFiClient m_wifiClient;
    MqttClient mqttClient;
    const unsigned interval = 1000;
    unsigned long previousMillis = 0;

    int count = 0;
    bool mqtt_conn;
    const char* m_broker;
    unsigned m_port;
    void reconnect();

public:
    PowMqtt();
    PowMqttCfg& config(){ return m_topics; }

    void sendTopic(const char* topic, float value);
    void sendTopic(const char* topic, const char* value);

    void setup(const char* i_hostname, const char* i_broker, unsigned i_port, POW* i_pow);

    void loop();
};

#else 

class PowMqtt {  
public:
    void setup(const char* , const char* , unsigned , POW* ){}

    void loop(){};
};
#endif

#endif
