#pragma once
// ======================================================
// DEFINES
// ======================================================

/* Configuration of NTP */
#define MY_NTP_SERVER "de.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/3 " 

// ======================================================
// INCLUDES
// ======================================================

// include intern
#include <config.h>
#include <mqtt.h>

// include extern
#include <Arduino.h>
#include <SPI.h>

#ifdef ARDUINO_ARCH_ESP32
    #include <WiFi.h>
    #else
    // ESP8266
    #include <ESP8266WiFi.h>
#endif

#include <ArduinoOTA.h> 
#include <ArduinoJSON.h>
#include <muTimer.h>
           
// ======================================================
// Prototypes
// ======================================================
void ntpSetup();
void buildDateTime();
void setupOTA();
void setup_wifi();
void check_wifi();
void basic_setup();
void sendWiFiInfo();
void storeData();