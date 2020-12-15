#ifdef MQTTBROKER
#include "MQTT.h"



const char* PowMqtt::mkTopicPath(const char* i_hostname,  const char* i_topic )
{

#define ROOT_PATH "home"

#define MAXLEN 64
    char* buf = 0;
    unsigned len = 0;
    len += strnlen( ROOT_PATH, MAXLEN )  +1; // plus '/' seperator
    len += strnlen( i_hostname, MAXLEN ) +1; // plus '/' seperator
    len += strnlen( i_topic, MAXLEN )    +1; // plus '\n' terminator

    buf = (char*) malloc( len );
    snprintf(buf, len,"%s/%s/%s", ROOT_PATH, i_hostname, i_topic);

    return buf;
}


PowMqtt::PowMqtt(  )
: mqttClient(m_wifiClient)
{  
}

void PowMqtt::reconnect()
{
    // You can provide a unique client ID, if not set the library uses Arduino-millis()
    // Each client must have a unique client ID
    // mqttClient.setId("clientId");

    // You can provide a username and password for authentication
    // mqttClient.setUsernamePassword("username", "password");

    DEBUG_PRINT("[MQTT] Attempting to connect to the MQTT broker: %s\n", m_broker );

    if (mqttClient.connect(m_broker, m_port)) {
        mqtt_conn = true;
        DEBUG_PRINT("[MQTT] You're connected to the MQTT broker!");
    } else {
        mqtt_conn = false;
        DEBUG_PRINT("[MQTT] connection failed! Error code = %d\n", mqttClient.connectError());
    }
}
void PowMqtt::setup(const char* i_hostname, const char* i_broker, unsigned i_port,POW* i_pow)
{
    m_pow = i_pow;

    m_topics.led               = mkTopicPath(i_hostname, "led");
    m_topics.relay             = mkTopicPath(i_hostname, "relay");
    m_topics._time             = mkTopicPath(i_hostname, "time");
    m_topics.pwr               = mkTopicPath(i_hostname, "pwr");
    m_topics.volt              = mkTopicPath(i_hostname, "voltage");
    m_topics.curr              = mkTopicPath(i_hostname, "current");
    m_topics.energy_total      = mkTopicPath(i_hostname, "energy/total");
    m_topics.energy_requested  = mkTopicPath(i_hostname, "energy/requested");
    m_topics.energy_optional   = mkTopicPath(i_hostname, "energy/optional");
    m_topics.pushButton        = mkTopicPath(i_hostname, "button");

    m_broker  = i_broker;
    m_port    = i_port;
    reconnect();
}

void PowMqtt::sendTopic(const char* topic, float value)
{
    _DEBUG_PRINT("[MQTT] --- send topic %s message: %f\n", topic, value);
    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(topic);
    mqttClient.print(topic);
    mqttClient.print(":");
    mqttClient.print(value);
    mqttClient.endMessage();
    _DEBUG_PRINT("[MQTT] -- done\n");
}


void PowMqtt::sendTopic(const char* topic, const char* value)
{
    _DEBUG_PRINT("[MQTT] -- send topic %s message: %s\n", topic, value);
    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(topic);
    mqttClient.print(topic);
    mqttClient.print(":");
    mqttClient.print(value);
    mqttClient.endMessage();
    _DEBUG_PRINT("[MQTT] -- done\n");
}

void PowMqtt::loop()
{
    _DEBUG_PRINT("[MQTT] LOOP enter\n");

    if ( mqtt_conn ) {
        // call poll() regularly to allow the library to send MQTT keep alives which
        // avoids being disconnected by the broker
        mqtt_conn = mqttClient.connected();
        if ( ! mqtt_conn ) {
            DEBUG_PRINT("[MQTT] lost connection\n");
            return;
        }

        // avoid having delays in loop, we'll use the strategy from BlinkWithoutDelay
        // see: File -> Examples -> 02.Digital -> BlinkWithoutDelay for more info
        unsigned long currentMillis = millis();

        if (currentMillis - previousMillis >= interval) {
            mqttClient.poll();
            // save the last time a message was sent
            previousMillis = currentMillis;

            sendTopic( config().led,    m_pow->ledState ? "On" : "Off" );
            sendTopic( config().relay,  m_pow->relayState  ? "On" : "Off" ) ;
            sendTopic( config()._time, TimeClk::getTimeString( getTime() ) );

            sendTopic( config().pwr,    m_pow->averagePwr );
            sendTopic( config().volt,   m_pow->voltage );
            sendTopic( config().curr,   m_pow->current );

            PlanningData* plan = g_semp->getActivePlan();
            if (plan) {
                unsigned requestedEnergy = g_semp->getActivePlan()->m_requestedEnergy;
                unsigned optionalEnergy  = g_semp->getActivePlan()->m_optionalEnergy;

                sendTopic( config().energy_requested, requestedEnergy );
                sendTopic( config().energy_optional,  optionalEnergy );

            }
            sendTopic( config().energy_total, m_pow->cumulatedEnergy );
            sendTopic( config().pushButton,   m_pow->relayState      );
            count++;
        }
    } else {
        unsigned long currentMillis = millis();

        if (currentMillis - previousMillis >= 10000) {
            // save the last time a message was sent
            previousMillis = currentMillis;
            reconnect();
        }
    }
    _DEBUG_PRINT("[MQTT] LOOP en´it\n");
}
#endif
