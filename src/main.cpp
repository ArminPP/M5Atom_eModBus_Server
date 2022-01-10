
/*
Test eModBus Client based on the RTUserver file
Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to ModbusClient

Armin Pressler 2022

The server (M5Atom) handles a request from the client at holdReg adress 300d @ 8 registers

IMPORTANT:
  - If one server on the bus is down and still electrically connected
    the whole bus is affected
  - If one sever disturbes the bus all the communication is affected


SERVER ISSUES:
  - Atom-Base RS485 has no 120 Ohm burden resistor! (R4 n/c ??)
  - M5Stack W5500 burden resistor unknown?!
  - XY-MD02 needs >5 VDC USB from M5Stack delivers only ~ 4.6V
  - USB2RS485 Adapter no external VCC! -> kills/resets USB from Notebook!


*/

#include <Arduino.h>
#include <M5Atom.h>

// Modbus server include
#include "ModbusServerRTU.h"
#include "Logging.h"

#define SERVER_ID 27

uint16_t data  =  0;

// Create a ModbusRTU server instance listening on Serial2 with 2000ms timeout
ModbusServerRTU MBserver(Serial2, 2000);

// FC03: worker do serve Modbus function code 0x03 (READ_HOLD_REGISTER)
ModbusMessage FC03(ModbusMessage request)
{
  uint16_t address;       // requested register address
  uint16_t words;         // requested number of registers
  ModbusMessage response; // response message to be sent back

  // get request values
  request.get(2, address); // 2
  request.get(4, words);   // 4

  // Serial.printf("------------------------ adress: %i  words:%i\n", address, words);
  // LOG_I("------------------------ adress: %i  words:%i\n", address, words);

  // Address and words valid? We assume 8 registers here for demo
  if (address && words && (address + words)) // <= SERVER_NUM_VALUES
  {
    // Looks okay. Set up message with serverID, FC and length of data
    // Serial.printf("------------------------ serverID: %i  FC:%i length:%i \n", request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    // Fill response with requested data
    for (uint16_t i = address; i < address + words; ++i)
    {
      response.add(data++);
      // Serial.printf("------------------------  response.add%i\n", i);
    }
  }
  else
  {
    // No, either address or words are outside the limits. Set up error response.
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
  }
  return response;
}

// Setup() - initialization happens here
void setup()
{
  M5.begin();

  // Init Serial monitor
  Serial.begin(115200);
  Serial.println("\nPress some serial key or M5 Button to start program"); // DEBUG
  while (Serial.available() == 0)
  {
    M5.update();
    if (M5.Btn.wasPressed())
    { // if M5 Button was pressed, then also start...
      break;
    }
  }
  Serial.println("OK"); // DEBUG

  // Init Serial2 connected to the RTU Modbus
  // (Fill in your data here!)
  Serial2.begin(9600, SERIAL_8N1, GPIO_NUM_22, GPIO_NUM_19); // Modbus connection

  // Register served function code worker for server 1, FC 0x03
  MBserver.registerWorker(SERVER_ID, READ_HOLD_REGISTER, &FC03); // ID 0x1B??

  // Start ModbusRTU background task
  MBserver.start();
}

// loop() - nothing done here today!
void loop()
{
  delay(10000);
}