#include <mqtt.h>
#include <basics.h>
#include <marley.h>

// ======================================================
// declaration
// ======================================================
WiFiClient espClient;
PubSubClient mqtt_client(espClient);


/**
 *  ******************************************************
 * @brief   helper function to add Topic to String
 * @param   suffix  Topic to add
 * @return  none
 */
char newTopic[256];
const char * addTopic(const char *suffix){
  strcpy(newTopic, MQTT_TOPIC);
  strcat(newTopic, suffix);
  return newTopic;
}


/**
 *  ******************************************************
 * @brief   MQTT Callback function
 * @param   topic     MQTT Topic
 * @param   paload    MQTT Payload
 * @param   length    payload length
 * @return  none
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String payloadString = String((char*)payload);
  int payloadINT = payloadString.toInt();

  Serial.print("topic: ");
  Serial.println(topic);

  // restart ESP
  if (strcmp (topic, addTopic("/cmd/restart")) == 0){
    mqtt_client.publish(addTopic("/message"), "restart requested!");
    delay(1000);
    ESP.restart();
  }
  // Marley command Power On/Off
  if (strcmp (topic, addTopic("/cmd/power")) == 0){
    if(payloadINT==0){
      marleyCmd(CMD_PWR_OFF);
    }
    else if (payloadINT==1){
      marleyCmd(CMD_PWR_ON);
    }
    else{
      mqtt_client.publish(addTopic("/message"), "invalid command!");
    }
  }
  // Marley command Summer Mode On
  if (strcmp (topic, addTopic("/cmd/summer")) == 0){
    if(payloadINT==1){
      marleyCmd(CMD_SUMMER);
    }
    else{
      mqtt_client.publish(addTopic("/message"), "invalid command!");
    }
  }
  // Marley command interval (1=slow, 2=middle, 3=fast)
  if (strcmp (topic, addTopic("/cmd/interval")) == 0){
    if(payloadINT==1){
      marleyCmd(CMD_INTERVAL_SLOW);
    }
    else if (payloadINT==2){
      marleyCmd(CMD_INTERVAL_MIDDLE);
    }
    else if (payloadINT==3){
      marleyCmd(CMD_INTERVAL_FAST);
    }
    else{
      mqtt_client.publish(addTopic("/message"), "invalid command!");
    }
  }
}


/**
 *  ******************************************************
 * @brief   Check MQTT connection
 * @param   none
 * @return  none
 */
void mqttCyclic(){
    mqtt_client.loop();
    
    const char* willTopic = addTopic("/status");
    const char* willMsg = "offline";
    int mqtt_retry = 0;
    bool res;
    
    if (!mqtt_client.connected() && (WiFi.status() == WL_CONNECTED)) {
        while (!mqtt_client.connected() && mqtt_retry < 5 && WiFi.status() == WL_CONNECTED) {
            mqtt_retry++;
            Serial.println("MQTT not connected, reconnect...");
            res = mqtt_client.connect(HOSTNAME, MQTT_USER, MQTT_PW, willTopic, 0, 1, willMsg);
            if (!res) {
                Serial.print("failed, rc=");
                Serial.print(mqtt_client.state());
                Serial.println(", retrying");
                delay(MQTT_RECONNECT);
            } else {
                Serial.println("MQTT connected");
                // Once connected, publish wifi status
                sendWiFiInfo();
                // ... and resubscribe
                mqtt_client.subscribe(addTopic("/cmd/#"));
            }          
        }
        if(mqtt_retry >= 5){
          Serial.print("MQTT connection not possible, rebooting...");
          // store data before restart
          storeData();
          ESP.restart();
        }
    } 
}

/**
 *  ******************************************************
 * @brief   MQTT Setup function
 * @param   none
 * @return  none
 */
void mqttSetup(){
  mqtt_client.setServer(MQTT_SERVER, 1883);
  mqtt_client.setCallback(mqttCallback);
}


/**
 *  ******************************************************
 * @brief   MQTT Publish function for external use
 * @param   sendtopic MQTT Topic
 * @param   paload    MQTT Payload
 * @param   retained  retained option
 * @return  none
 */
void mqttPublish(const char* sendtopic, const char* payload, boolean retained){
  mqtt_client.publish(sendtopic, payload);
}