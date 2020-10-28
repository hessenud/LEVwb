#ifdef MQTTBROKER
#include <ArduinoMqttClient.h>
 
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char* broker = MQTTBROKER;
int        port    = MQTTPORT;

const char* topic_led               = "home/" HOSTNAME "/led";
const char* topic_rel               = "home/" HOSTNAME "/relay";
const char* topic_time              = "home/" HOSTNAME "/time";
const char* topic_pwr               = "home/" HOSTNAME "/pwr";
const char* topic_volt              = "home/" HOSTNAME "/voltage";
const char* topic_curr              = "home/" HOSTNAME "/current";
const char* topic_energy_total      = "home/" HOSTNAME "/energy/total";
const char* topic_energy_requested  = "home/" HOSTNAME "/energy/requested";
const char* topic_energy_optional   = "home/" HOSTNAME "/energy/optional";
const char* topic_switch            = "home/" HOSTNAME "/switch";

const long interval = 1000;
unsigned long previousMillis = 0;

int count = 0;
bool mqtt_conn;

void setupMQTT()
{
  
  // You can provide a unique client ID, if not set the library uses Arduino-millis()
  // Each client must have a unique client ID
  // mqttClient.setId("clientId");

  // You can provide a username and password for authentication
  // mqttClient.setUsernamePassword("username", "password");

  DEBUG_PRINT("[MQTT] Attempting to connect to the MQTT broker: %s\n", broker );

  if (mqttClient.connect(broker, port)) {
    mqtt_conn = true;
    DEBUG_PRINT("[MQTT] You're connected to the MQTT broker!");
  } else {
    mqtt_conn = false;
    DEBUG_PRINT("[MQTT] connection failed! Error code = %d\n", mqttClient.connectError());
  } 
}

void sendTopic(const char* topic, float value)
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


void sendTopic(const char* topic, const char* value)
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

void loopMQTT() 
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
  
      sendTopic( topic_led,  ledState ? "On" :"Off" );
      sendTopic( topic_rel,  relayState  ? "On" :"Off" ) ;
      sendTopic( topic_time, TimeClk::getTimeString( getTime() ) );
      
      sendTopic( topic_pwr,    pow_averagePwr );
      sendTopic( topic_volt,   pow_voltage );
      sendTopic( topic_curr,   pow_current );
      
      PlanningData* plan = g_semp->getActivePlan();
      if (plan){
          unsigned requestedEnergy = g_semp->getActivePlan()->m_requestedEnergy;
          unsigned optionalEnergy  = g_semp->getActivePlan()->m_optionalEnergy;
        
          sendTopic( topic_energy_requested, requestedEnergy );
          sendTopic( topic_energy_optional, optionalEnergy );
      
      }
      sendTopic( topic_energy_total, pow_cumulatedEnergy );
      sendTopic( topic_switch,  relayState);
      count++;
    }
  } else {
     unsigned long currentMillis = millis();
    
    if (currentMillis - previousMillis >= 10000) {
      // save the last time a message was sent
      previousMillis = currentMillis;
      setupMQTT();
    }
 }
  _DEBUG_PRINT("[MQTT] LOOP en´it\n");  
}
#else
void setupMQTT(){}
void loopMQTT(){}
#endif
