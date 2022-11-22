#pragma once

// ======================================================
// includes
// ======================================================
#include <config.h>
#include <Arduino.h>


// ======================================================
// variables and structs
// ======================================================
typedef enum {
  COM_WAIT_CMD,                     // Step: wait for command
  COM_CHECK_CMD,                    // Step: check new status
  COM_SEND_DONE,                    // Step: command send successful
  COM_SEND_ERROR                    // Step: comand send timeout                                     
} e_txState;

typedef enum {
  CMD_NONE,                         // command type: NONE
  CMD_PWR_ON,                       // command type: PowerON                               
  CMD_PWR_OFF,                      // command type: PowerOFF                                
  CMD_SUMMER,                       // command type: Mode: Summer
  CMD_INTERVAL_SLOW,                // command type: Mode: Interval slow
  CMD_INTERVAL_MIDDLE,              // command type: Mode: Interval middle
  CMD_INTERVAL_FAST,                // command type: Mode: Interval fast                                     
} e_cmdType;

typedef struct {                                               
  int           cmdCode;            // command data 
  int           txData;             // complete TX message    
  e_cmdType     command;            // requested command          
  int           lastState;          // last received state
  int           lastButton;         // last received button
  int           txRetries;          // number of TX retries
  unsigned long txErrorCnt;         // error count for failed send commands
} marley_s;

typedef struct {                                               
  int           mode;               // Mode: 0=OFF / 1=INTERVAL / 2=SUMMER
  int           speed;              // Speed: 0=OFF / 1=SLOW / 2=MIDDLE / 3=FAST    
  int           direction;          // Direction: 0=OFF / 1=IN / 2=OUT
  int           button;             // Button: 1=POWER / 2=SUMMER / 3=PLUS / 4=MINUS
} marleyStatus_s;


// ======================================================
// Defines
// ======================================================
#define TX_PIN D2                                 // PIN that is used to transmitt data
#define RX_PIN D1                                 // PIN that is used to receive data

#define MARLEY_ADR 0x8EA06                        // This address may change from Fan to Fan?!

#define TX_TIMEOUT 3000                           // wait 3 seconds before retry
#define TX_RETRIES 3                              // try 3 times before abort

#define CODE_BTN_SUMMER    0x1                    // Code to send for Button Summer
#define CODE_BTN_MINUS     0x2                    // Code to send for Button Minus
#define CODE_BTN_PLUS      0x4                    // Code to send for Button Plus
#define CODE_BTN_PWR       0x8                    // Code to send for Button Power

#define CODE_STATE_PWR_OFF                 0x0    // Status Code for Power is off
#define CODE_STATE_INTERVAL_SLOW_IN        0x6    // Status Code for Mode: INTERVAL - Speed: SLOW - Direction: IN
#define CODE_STATE_INTERVAL_SLOW_OUT       0x7    // Status Code for Mode: INTERVAL - Speed: SLOW - Direction: OUT
#define CODE_STATE_INTERVAL_MIDDLE_IN      0xA    // Status Code for Mode: INTERVAL - Speed: MIDDLE - Direction: IN
#define CODE_STATE_INTERVAL_MIDDLE_OUT     0xB    // Status Code for Mode: INTERVAL - Speed: MIDDLE - Direction: OUT
#define CODE_STATE_INTERVAL_FAST_IN        0xC    // Status Code for Mode: INTERVAL - Speed: FAST - Direction: IN
#define CODE_STATE_INTERVAL_FAST_OUT       0xD    // Status Code for Mode: INTERVAL - Speed: FAST - Direction: OUT
#define CODE_STATE_MODE_SUMMER             0xE    // Status Code for Mode: SUMMER


// ======================================================
// Prototypes
// ======================================================
void marleySetup();
void marleyCyclic();
void marleyCmd(e_cmdType cmd);
void marleyCheckMsg(unsigned long txData);
void marleySendData(unsigned long txData);
void marleySendMQTT();
void marleyDebugInfo();