# ESP_Marley_MQTT
Control your Marley Fresh air heat exchanger MEnV with ESP8266 and MQTT  
This project was tested with Marley MEnV 180mm (1. Edition)

---

## Hardware Requirements

### ESP-Module
Any kind of ESP with SPI interface and at least 2 GPIO to select TX and RX.  
I have used a Wemos D1 mini based on ESP8266

### 433Mhz Module
For the 433Mhz communication with the Marley MEnV, I recomment the CC1101 Module, that is able to send and receive within one module.

### Wiring

Wemos D1 | CC1101
---------|-------
D1       | GD02
D2       | GD00
D5       | SCLK
D6       | SO
D7       | SI
D8       | CSN


---

## MQTT Communication

### You can control the MEnV with the following commands:

```
Topic: esp_marley/cmd/power  
Payload:  0=OFF / 1=OFF

Topic: esp_marley/cmd/summer  
Payload:  1=ON

Topic: esp_marley/cmd/interval  
Payload:  1=SLOW / 2=MIDDLE / 3=FAST
```

### As Status you will get informations as JSON Object:

```
Topic: esp_marley/info =  {  
    "mode":"OFF/SUMMER/INTERVAL",  
    "speed":"NONE/SLOW/MIDDLE/FAST",  
    "direction":"NONE/IN/OUT"  
}

Topic: esp_marley/wifi = {  
    "status":"online",  
    "rssi":"-50",  
    "signal":"90",  
    "ip":"192.168.1.1",  
    "date-time":"01.01.2022 - 10:20:30"  
}

Topic: esp_marley/received = {  
    "data":"0x8EA06E",  
    "delay":"300",  
    "bit":"24",  
    "protocol":"1",  
    "rssi":"-40"  
}  
```
