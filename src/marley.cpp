#include <marley.h>
#include <basics.h>
#include <mqtt.h>

#include <ELECHOUSE_cc1101_SRC_DRV.h>
#include <RCSwitch.h>

// ==================================================================================================
// Protocoll information
// ==================================================================================================
// Used and testet with Marley MEnV 180mm
// length:          24bit 
// Protocol:        1
// Pulse length:    300ms 
//
// The messages can be split into address (0x8EA06) and data
// The address may change from Fan to Fan and can be changed in marley.h
// 
// Send commands from remote control:
// Button summer:   0x8EA061	(data:1)
// Button Minus:    0x8EA062	(data:2)
// Button Plus:		0x8EA064	(data:4)
// Button Power:	0x8EA068	(data:8)
// 
// received commands from Marley Fan:
// Status off:                  0x8EA060 (data:0)
// Status interval slow in:     0x8EA066 (data:6) 
// Status interval slow out:    0x8EA067 (data:7) 
// Status interval middle in:   0x8EA06A (data:A) 
// Status interval middle out:  0x8EA06B (data:B)
// Status interval fast in:     0x8EA06C (data:C)
// Status interval fast out:    0x8EA066 (data:D) 
// Status summer mode:          0x8EA06D (data:E)
//
// ==================================================================================================

#define DEBUG_ON                 // activate for debug information over Serial and MQTT

// ==================================================================================================
// variables and structs
// ==================================================================================================
e_txState TXstate;                  // states for communication statemachine
marley_s marley;                    // struct for marley information
marleyStatus_s marleyStatus;        // status of Marley Fan

RCSwitch mySwitch = RCSwitch();     // instance for communication
muTimer sendTimeOut = muTimer();    // Timer for timeout in TX handling
// ==================================================================================================


/**
 *  ******************************************************
 * @brief   Setup for Receive and Transmitt
 * @param   none
 * @return  none
 */
void marleySetup() {

    //CC1101 Settings:                
    ELECHOUSE_cc1101.Init();            // must be set to initialize the cc1101!
    ELECHOUSE_cc1101.setMHZ(433.92);    // set the frequency = 433.92)
    ELECHOUSE_cc1101.SetRx();           // set cc1101 to receive mode at start
    mySwitch.enableReceive(RX_PIN);     // set mySwitch to receive mode at start
    mySwitch.setPulseLength(300);       // set pulse length. 
}

/**
 *  ******************************************************
 * @brief   Main handling for Receive and Transmitt
 * @param   none
 * @return  none
 */
void marleyCyclic() {

    
    if (mySwitch.available()) {                                 // wait for incomming message
        marleyCheckMsg(mySwitch.getReceivedValue());            // check if message is valid and extract data into "marley.lastState"
        marleySendMQTT();                                       // send actual received status
        #ifdef DEBUG_ON
            marleyDebugInfo();                                  // send debug information
        #endif                                      
        mySwitch.resetAvailable();
    }

    switch (TXstate)                                            // handling of send commands
    {
    case COM_WAIT_CMD:      
        if (marley.command != CMD_NONE){                        // request for command power on
            marleySendData(marley.txData);
            TXstate = COM_CHECK_CMD;
            mqttPublish(addTopic("/message"), "command busy!", false);
        }
        break;

    case COM_CHECK_CMD:
        if (sendTimeOut.delayOn(true, TX_TIMEOUT)) {             // wait for status until timeout
            sendTimeOut.delayReset(); 
            if (marley.txRetries<TX_RETRIES) {                   // try again until max. retries reached
                marley.txRetries++;
                marleySendData(marley.txData);
            }
            else{
                TXstate = COM_SEND_ERROR;
            }
        }
        else {
            switch (marley.command)                             // check state depending on requestet command
            {
            case CMD_PWR_ON:
                if (marley.lastState!=CODE_STATE_PWR_OFF){
                    TXstate = COM_SEND_DONE;
                }
                break;
            
            case CMD_PWR_OFF:
                if (marley.lastState==CODE_STATE_PWR_OFF){
                    TXstate = COM_SEND_DONE;
                }
                break;            
            
            case CMD_SUMMER:
                if (marley.lastState==CODE_STATE_MODE_SUMMER){
                    TXstate = COM_SEND_DONE;
                }
                break; 

            case CMD_INTERVAL_SLOW:
                if (marley.lastState==CODE_STATE_INTERVAL_SLOW_IN || marley.lastState==CODE_STATE_INTERVAL_SLOW_OUT){
                    TXstate = COM_SEND_DONE;
                }
                break; 
            
            case CMD_INTERVAL_MIDDLE:
                if (marley.lastState==CODE_STATE_INTERVAL_MIDDLE_IN || marley.lastState==CODE_STATE_INTERVAL_MIDDLE_OUT){
                    TXstate = COM_SEND_DONE;
                }
                break; 
            
            case CMD_INTERVAL_FAST:
                if (marley.lastState==CODE_STATE_INTERVAL_FAST_IN || marley.lastState==CODE_STATE_INTERVAL_FAST_OUT){
                    TXstate = COM_SEND_DONE;
                }
                break; 
            
            default:
                break;
            }

        }
        break;

    case COM_SEND_DONE:
        sendTimeOut.delayReset();   // reset timeout
        marley.command = CMD_NONE;  // reset command
        marley.txRetries = 0;       // reset counter
        TXstate = COM_WAIT_CMD;     // back to wait
        mqttPublish(addTopic("/message"), "command done!", false);
        break;

    case COM_SEND_ERROR:
        sendTimeOut.delayReset();   // reset timeout
        marley.command = CMD_NONE;  // reset command
        marley.txRetries = 0;       // reset counter
        TXstate = COM_WAIT_CMD;     // back to wait
        marley.txErrorCnt++;        // increase error counter
        mqttPublish(addTopic("/message"), "command timeout!", false);
        break;

    default:
        break;
    }
      
}


/**
 *  ******************************************************
 * @brief   prepare command for Transmitt handling
 * @param   cmd:  requested command
 * @return  none
 */
void marleyCmd(e_cmdType cmd){
   switch (cmd)
   {
   case CMD_NONE:
    // nop
    break;
   // requested command: Power ON
   case CMD_PWR_ON:
    if (marley.lastState==CODE_STATE_PWR_OFF){
        marley.command = CMD_PWR_ON;
        marley.txData = (MARLEY_ADR << 4) + CODE_BTN_PWR;
    }
    break;
   // requested command: Power OFF
   case CMD_PWR_OFF:
    if (marley.lastState!=CODE_STATE_PWR_OFF){
        marley.command = CMD_PWR_OFF;
        marley.txData = (MARLEY_ADR << 4) + CODE_BTN_PWR;
    }
    break;
   // requested command: Mode SUMMER
   case CMD_SUMMER:
    if (marley.lastState==CODE_STATE_PWR_OFF){
        mqttPublish(addTopic("/message"), "command not possible!", false);
    }
    else if (marley.lastState!=CODE_STATE_MODE_SUMMER){
        marley.command = CMD_SUMMER;
        marley.txData = (MARLEY_ADR << 4) + CODE_BTN_SUMMER;
    }
    break;
   // requested command: Mode INTERVAL - SLOW
   case CMD_INTERVAL_SLOW:
    if (marley.lastState==CODE_STATE_PWR_OFF){
        mqttPublish(addTopic("/message"), "command not possible!", false);
    }
    else if (marley.lastState==CODE_STATE_INTERVAL_MIDDLE_IN || marley.lastState==CODE_STATE_INTERVAL_MIDDLE_OUT || 
             marley.lastState==CODE_STATE_INTERVAL_FAST_IN || marley.lastState==CODE_STATE_INTERVAL_FAST_OUT || 
             marley.lastState==CODE_STATE_MODE_SUMMER) {

        marley.command = CMD_INTERVAL_SLOW;
        marley.txData = (MARLEY_ADR << 4) + CODE_BTN_MINUS;
    }
    break;
   // requested command: Mode INTERVAL - MIDDLE
   case CMD_INTERVAL_MIDDLE:
    if (marley.lastState==CODE_STATE_PWR_OFF){
        mqttPublish(addTopic("/message"), "command not possible!", false);
    }
    if (marley.lastState==CODE_STATE_MODE_SUMMER){
        marley.command = CMD_INTERVAL_SLOW;
        marley.txData = (MARLEY_ADR << 4) + CODE_BTN_PLUS;
    }
    else if (marley.lastState==CODE_STATE_INTERVAL_SLOW_IN || marley.lastState==CODE_STATE_INTERVAL_SLOW_OUT) {
        marley.command = CMD_INTERVAL_MIDDLE;
        marley.txData = (MARLEY_ADR << 4) + CODE_BTN_PLUS;
    }
    else if (marley.lastState==CODE_STATE_INTERVAL_FAST_IN || marley.lastState==CODE_STATE_INTERVAL_FAST_OUT) {
        marley.command = CMD_INTERVAL_MIDDLE;
        marley.txData = (MARLEY_ADR << 4) + CODE_BTN_MINUS;
    }   
    break;
   // requested command: Mode INTERVAL - FAST
   case CMD_INTERVAL_FAST:
    if (marley.lastState==CODE_STATE_PWR_OFF){
        mqttPublish(addTopic("/message"), "command not possible!", false);
    }
    else if (marley.lastState==CODE_STATE_INTERVAL_MIDDLE_IN || marley.lastState==CODE_STATE_INTERVAL_MIDDLE_OUT || 
             marley.lastState==CODE_STATE_INTERVAL_SLOW_IN || marley.lastState==CODE_STATE_INTERVAL_SLOW_OUT || 
             marley.lastState==CODE_STATE_MODE_SUMMER) {

        marley.command = CMD_INTERVAL_FAST;
        marley.txData = (MARLEY_ADR << 4) + CODE_BTN_PLUS;
    }
    break;
   }
}


/**
 *  ******************************************************
 * @brief   check message and extract data
 * @param   message: complete message
 * @return  none
 */
void marleyCheckMsg(unsigned long txData){
    if ( (txData & 0xFFFFF0) >> 4 == MARLEY_ADR ) {
        int code = txData & 0xF;                                                                                        // extract the last 4bit as data 
        if (code == CODE_BTN_PWR || code == CODE_BTN_MINUS || code == CODE_BTN_PLUS || code == CODE_BTN_SUMMER) {       // check if code = button code
            marley.lastButton = code;
        }
        else {                                                                                                          // code should be status
            marley.lastState = code;    
        } 
    } 
}

/**
 *  ******************************************************
 * @brief   send data to cc1101
 * @param   txData: complete message to send
 * @return  none
 */
void marleySendData(unsigned long txData){
    mySwitch.disableReceive();                      // Receiver off
    ELECHOUSE_cc1101.SetTx();                       // set Transmit on
    mySwitch.enableTransmit(TX_PIN);                // Transmit on
    mySwitch.setRepeatTransmit(3);                  // set repeat transmitt
    mySwitch.send(txData, 24);                      // send data to sender module
    delay(500);                                     // wait until switch back to receive
    mySwitch.disableTransmit();                     // set Transmit off    
    ELECHOUSE_cc1101.SetRx();                       // set Receive on
    mySwitch.enableReceive(RX_PIN);                 // Receiver on
    delay(500);                                     // wait after mode change
}

/**
 *  ******************************************************
 * @brief   send actual status over MQTT
 * @param   none
 * @return  none
 */
void marleySendMQTT(){

    // send actual status information
    DynamicJsonDocument statusJSON(1024);
    // check MODE
    if (marley.lastState==CODE_STATE_PWR_OFF){
        statusJSON["mode"] = "OFF";
        marleyStatus.mode = 0;
        statusJSON["speed"] = "NONE";
        marleyStatus.speed = 0;
        statusJSON["direction"] = "NONE";
        marleyStatus.direction = 0;
    }
    else if(marley.lastState==CODE_STATE_MODE_SUMMER){
        statusJSON["mode"] = "SUMMER";
        marleyStatus.mode = 2;
        statusJSON["speed"] = "SLOW";
        marleyStatus.speed = 1;
        statusJSON["direction"] = "OUT";
        marleyStatus.direction = 2;
    } 
    else if (marley.lastState==CODE_STATE_INTERVAL_FAST_IN || marley.lastState==CODE_STATE_INTERVAL_FAST_OUT ||
            marley.lastState==CODE_STATE_INTERVAL_MIDDLE_IN || marley.lastState==CODE_STATE_INTERVAL_MIDDLE_OUT ||
            marley.lastState==CODE_STATE_INTERVAL_SLOW_IN || marley.lastState==CODE_STATE_INTERVAL_SLOW_OUT){
        statusJSON["mode"] = "INTERVAL";
        marleyStatus.mode = 1;
    } 
    // check DIRECTION
    if (marley.lastState==CODE_STATE_INTERVAL_FAST_IN || marley.lastState==CODE_STATE_INTERVAL_MIDDLE_IN || marley.lastState==CODE_STATE_INTERVAL_SLOW_IN){
        statusJSON["direction"] = "IN";
        marleyStatus.direction = 1;
    }
    else if (marley.lastState==CODE_STATE_INTERVAL_FAST_OUT || marley.lastState==CODE_STATE_INTERVAL_MIDDLE_OUT || marley.lastState==CODE_STATE_INTERVAL_SLOW_OUT){
        statusJSON["direction"] = "OUT";
        marleyStatus.direction = 2;
    } 
    // check SPEED
    if (marley.lastState==CODE_STATE_INTERVAL_FAST_IN || marley.lastState==CODE_STATE_INTERVAL_FAST_OUT){
        statusJSON["speed"] = "FAST";
        marleyStatus.speed = 3;
    }
    else if (marley.lastState==CODE_STATE_INTERVAL_MIDDLE_IN || marley.lastState==CODE_STATE_INTERVAL_MIDDLE_OUT){
        statusJSON["speed"] = "MIDDLE";
        marleyStatus.speed = 2;
    } 
    else if (marley.lastState==CODE_STATE_INTERVAL_SLOW_IN || marley.lastState==CODE_STATE_INTERVAL_SLOW_OUT){
        statusJSON["speed"] = "SLOW";
        marleyStatus.speed = 1;
    } 
    // check remote control buttons
    if (marley.lastButton==CODE_BTN_PWR){
        statusJSON["button"] = "POWER";
        marleyStatus.button = 1;
    }
    else if (marley.lastButton==CODE_BTN_SUMMER){
        statusJSON["button"] = "SUMMER";
        marleyStatus.button = 2;
    }
    else if (marley.lastButton==CODE_BTN_PLUS){
        statusJSON["button"] = "PLUS";
        marleyStatus.button =3;
    }
    else if (marley.lastButton==CODE_BTN_MINUS){
        statusJSON["button"] = "MINUS";
        marleyStatus.button = 4;
    }
    // send status as JSON Object with text
    String stringJSON;
    serializeJson(statusJSON, stringJSON);
    mqttPublish(addTopic("/info"),String(stringJSON).c_str(), false);    

    // send status as separate integer status 
    mqttPublish(addTopic("/mode"),String(marleyStatus.mode).c_str(), false);
    mqttPublish(addTopic("/direction"),String(marleyStatus.direction).c_str(), false);
    mqttPublish(addTopic("/speed"),String(marleyStatus.speed).c_str(), false); 
    mqttPublish(addTopic("/button"),String(marleyStatus.button).c_str(), false); 
}

/**
 *  ******************************************************
 * @brief   send actual debug information
 * @param   none
 * @return  none
 */
void marleyDebugInfo(){        
    if (ELECHOUSE_cc1101.getCC1101()){       // Check the CC1101 Spi connection.
        // send received information over MQTT
        DynamicJsonDocument recvJSON(1024);
        char temp[30]={'\0'};
        sprintf(temp, "%#x", (unsigned int)(mySwitch.getReceivedValue()));
        recvJSON["data"] = temp;
        recvJSON["delay"] = mySwitch.getReceivedDelay();
        recvJSON["bit"] = mySwitch.getReceivedBitlength();
        recvJSON["protocol"] = mySwitch.getReceivedProtocol();
        recvJSON["rssi"] = ELECHOUSE_cc1101.getRssi();
        String stringDebugJSON;
        serializeJson(recvJSON, stringDebugJSON);
        mqttPublish(addTopic("/received"),String(stringDebugJSON).c_str(), false); 
    }
    else {
        mqttPublish(addTopic("/received"),"CC1101 connection Error", false);
    }

    
    // output received information for debug
    if (ELECHOUSE_cc1101.getCC1101()){       // Check the CC1101 Spi connection.
        Serial.print("data: ");
        Serial.print( mySwitch.getReceivedValue() );
        Serial.print(" / ");
        Serial.print("Delay: ");
        Serial.print( mySwitch.getReceivedDelay() );
        Serial.print(" / ");
        Serial.print( mySwitch.getReceivedBitlength() );
        Serial.print("bit: ");
        Serial.print("Protocol: ");
        Serial.print( mySwitch.getReceivedProtocol() );
        Serial.print(" / ");
        Serial.print("rssi: ");
        Serial.println( ELECHOUSE_cc1101.getRssi() );
    }
    else {
        Serial.println("CC1101 connection Error");
    }
    
}
