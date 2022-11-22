#pragma once
#include "Credentials.h"

#define HOSTNAME "ESP_Marley"
#define MQTT_TOPIC "esp_marley"

#define WIFI_RECONNECT      5000    // Delay between wifi reconnection tries
#define MQTT_RECONNECT      5000    // Delay between mqtt reconnection tries

//#define MAINTIMER_CYCLE     600000  // Cycle Time for MainTimer 600.000 = 10 minutes
#define MAINTIMER_CYCLE     10000   // Cycle Time for MainTimer 10.000 = 10 seconds