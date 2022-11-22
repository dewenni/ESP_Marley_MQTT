#include <basics.h>
#include <mqtt.h>
#include <marley.h>


// =====================================================
// Timer
// =====================================================
muTimer mainTimer = muTimer();
muTimer sendTimer = muTimer();

/**
 *  ******************************************************
 * @brief   function to store data before update or restart
 * @param   none
 * @return  none
 */
void storeData(){
  // add function or code that has to be stored
}

/**
 *  ******************************************************
 * @brief   Main Setup function
 * @param   none
 * @return  none
 */
void setup()
{
  // basic setup function (WiFi, OTA)
  basic_setup();

  // MQTT
  mqttSetup();

  //Enable serial port
  Serial.begin(115200);
  while(!Serial) {} // Wait

  // user function
  marleySetup();
}

/**
 *  ******************************************************
 * @brief   Main Loop
 * @param   none
 * @return  none
 */
void loop()
{
  // WiFi + MQTT
  check_wifi();
  mqttCyclic();

  // OTA Update
  ArduinoOTA.handle();

if (mainTimer.cycleTrigger(MAINTIMER_CYCLE))
  {
    sendWiFiInfo();
  }

  // user function loop
  marleyCyclic();


}

