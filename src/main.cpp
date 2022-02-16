
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



MODBUS Basics:
**************

https://ipc2u.de/artikel/wissenswertes/modbus-rtu-einfach-gemacht-mit-detaillierten-beschreibungen-und-beispielen/

REGISTERNUMMER	REGISTERADRESSE HEX	    TYP	                  NAME	                   TYP
1-9999	        0000 to 270E	      lesen-schreiben	  Discrete Output Coils	            DO
10001-19999	    0000 to 270E	      lesen	            Discrete Input Contacts	          DI
30001-39999	    0000 to 270E	      lesen	            Analog Input Registers	          AI
40001-49999	    0000 to 270E	      lesen-schreiben	  Analog Output Holding Registers	  AO

FUNKTIONSKODE	                FUNKTION   	                                                    WERTTYP	  ZUGRIFFSTYP
01 (0x01)	      Liest DO	                  Read Discrete Output Coil	                        Diskret	    Lesen
02 (0x02)	      Liest DI	                  Read Discrete Input Contact	                      Diskret	    Lesen
03 (0x03)	      Liest AO	                  Read Analog Output Holding Register	              16 Bit	    Lesen
04 (0x04)	      Liest AI	                  Read Analog Input Register	                      16 Bit	    Lesen
05 (0x05)	      Schreibt ein DO	            Setzen einer Discrete Output Coil	                Diskret	    Schreiben
06 (0x06)	      Schreibt ein AO	            Setzen eines Analog Output Holding Registers	    16 Bit	    Schreiben
15 (0x0F)	      Aufzeichnung mehrerer DOs	  Setzen mehrerer Discrete Output Coil	            Diskret	    Schreiben
16 (0x10)	      Aufzeichnung mehrerer AOs	  Setzen mehrerer Analog Output Holding Registers	  16 Bit	    Schreiben

####################################################################
#   The maximum packet sizes are:                                  #
#        Read registers (function codes 03 & 04)   = 125 registers #
#        Write registers (function code 16)        = 123 registers #
#        Read Booleans (function codes 01 & 02)    = 2000 bits     #
#        Write Booleans (function code 15)         = 1968 bits     #
####################################################################

Modbus Client == Master
Modbus Server == Slave
*/

#include <Arduino.h>
#include <M5Atom.h>

// Modbus server include
#include "ModbusServerRTU.h"
#include "Logging.h"

#define SERVER_ID 26

uint16_t data = 0;

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

  Serial.printf("------------------------ adress: %i  words:%i\n", address, words);
  // LOG_I("------------------------ adress: %i  words:%i\n", address, words);

  // Address and words valid? We assume 8 registers here for demo
  if (address && words && (address + words)) // <= SERVER_NUM_VALUES
  {
    // Looks okay. Set up message with serverID, FC and length of data
    Serial.printf("------------------------ serverID: %i  FC:%i length:%i \n", request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    // Fill response with requested data
    for (uint16_t i = address; i < address + words; ++i)
    {
      response.add(data++);
      Serial.printf("------------------------  response.add %i\n", data);
    M5.dis.drawpix(0, 0, 0xff3300); // RED
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
  M5.begin(true, true, true);

  // Init Serial monitor
  Serial.begin(115200);
  Serial.printf("\nModBus serverID: %3i", SERVER_ID);
  // Serial.println("\nPress some serial key or M5 Button to start program"); // DEBUG
  // while (Serial.available() == 0)
  // {
  //   M5.update();
  //   if (M5.Btn.wasPressed())
  //   { // if M5 Button was pressed, then also start...
  //     break;
  //   }
  // }
  // Serial.println("OK"); // DEBUG

  M5.dis.drawpix(0, 0, 0x00ff00); // green

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

  // static bool blink = false;
  // static unsigned long LoopPM = 0;
  // unsigned long LoopCM = millis();
  // if (LoopCM - LoopPM >= (1000 * 1))
  // {
  //   if ((blink = !blink))
  //   {
  //     M5.dis.drawpix(0, 0, 0x0000cc); // blue
  //   }
  //   else
  //   {
  //     M5.dis.drawpix(0, 0, 0x00ff00); // green
  //   }
  // }
  // // --------  Loop end ----------------------------------------------------------------
  // LoopPM = LoopCM;

  for (uint16_t i = 0; i < 20; ++i) // DELAY of 10s !
  {
    M5.dis.drawpix(0, 0, 0x0000cc); // blue
    delay(250);
    M5.dis.drawpix(0, 0, 0x00ff00); // green
    delay(250);
  }
  Serial.printf("Delay 10s == ModBus serverID: %3i\n", SERVER_ID);
}