// #################################################################
// ###### MODRF24A #################################################
// #################################################################
/* Kenmerken Mini: 8 AnaIn 0 AnaOut 14 DigIn 6 PWM	 1KB Eeprom 2KB SRAM 32KB Flash












*/
#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include <nRF24L01.h>   // RF24 
#include <RF24.h>       // RF24
#include <printf.h> 
#define DO_DEBUG        // comment out > No debug info on serial
#define IR_CODE         // comment out > No IR code

const char MyModel[6] = "MR44A";    // M(ini)R(F24)4(In)4(Uit)A  
const char MyVersion[6] = "01.10";  // HW_SW versoe  
#define MAX_QUEUE 8
#define MAX_CMD_LEN 12 
#define MAX_RCV_QUEUE 8  //(4, 8, 16) 
#define MAX_SND_QUEUE 8  //(4, 8, 16)

// 
// Gnd = Bruin = 1 / 3.3V = Rood = 2
// 9   = CE = Oranje = 3  / 10 = CSN = Geel = 4
// 11  = MOSI = Blauw = 6  / 12 = MISO = Paars = 7  / 13 = SCK = Groen = 5  
#define RF24_CE 8  // WAS 9 //  "Chip Enable" pin, activates the RX or TX role 
#define RF24_CSN 7  // WAS 10 // SPI Chip select
RF24 radio(RF24_CE,RF24_CSN);

const byte LOC_PORT = 8;  // 1 extra voor nul-entry - toekomstig gebruik
const byte LOC_DIGIN = 3;  // 1 extra voor nul-entry - toekomstig gebruik
const byte LOC_ANAIN = 3;  // 1 extra voor nul-entry - toekomstig gebruik
const byte AAN = 255;  // Used in Value for 'On'
const byte UIT = 0;    // Used in Value for 'Off'

const byte BUS_MASTER  = 0xC1; 
const byte BUS_DEFAULT = 0xDF; 
byte RF_MASTER_FULL[4] = {0xCC,0xCC,0xCC,0xCC};
byte RF_CLIENT_FULL[4] = {0xCC,0xCC,0xCC,0xCC};

byte MyBUS = 0x02;      // RF24 bus  
byte MyAddress = 0x00;  
byte MyModID = 0; 
byte MyRange = 1;
byte MyProg = '-';
// =QUEUE=lokale queue=========================================
const byte Que_Mask = MAX_QUEUE-1;
byte QueNext = 0;
byte QueProc = 0;
byte QueType[MAX_QUEUE];
byte QueParm[MAX_QUEUE][6];
// =QUEUE =LOW PRIORITY =======================================
byte QueLNext = 0;
byte QueLProc = 0;
byte QueLType[MAX_QUEUE];
byte QueLParm[MAX_QUEUE][6];
//  ==COMMUNICATION===========================================
// SND BUFFER (inkomende commando's)
const byte MAX_SND_Mask = MAX_SND_QUEUE - 1; //???
volatile byte snd_Index = 0; // Volgende positie in command buffer
volatile byte snd_Procs = 0; // Volgend uit te voeren command
volatile byte snd_Status[MAX_SND_QUEUE];   // 1 = verzonden / 2 = resending / ??? Nodig
volatile byte snd_CC[MAX_SND_QUEUE];   // Positie van CC (0 > te berekenen) 
// volatile byte snd_Bus[MAX_SND_QUEUE];
volatile byte snd_Address[MAX_SND_QUEUE];
byte snd_Buf[MAX_SND_QUEUE][MAX_CMD_LEN];    //  Volatile????? 
volatile unsigned long snd_Time[MAX_SND_QUEUE];
volatile byte snd_Count[MAX_SND_QUEUE];   // ??????
const int Resend_TimeOut = 500;  // Wait for respons in ms
const int snd_MaxCount = 10;   
// RCV BUFFER (inkomende commando's)
const byte MAX_RCV_Mask = MAX_RCV_QUEUE - 1; //???
volatile byte rcv_Index = 0; // Volgende positie in command buffer
volatile byte rcv_Procs = 0; // Volgend uit te voeren command
// volatile byte rcv_Bus[MAX_SND_QUEUE];
byte rcv_Buf[MAX_RCV_QUEUE][MAX_CMD_LEN];    //  Volatile????? 
//  ==COMMUNICATION==END======================================
// =LOCAL PORTS================================================
int DigInp[LOC_DIGIN] = {0,A0,A1}; // 
int AnaInp[LOC_DIGIN] = {0,A6,A7}; // 
// int AnaVal[3] = {0,128,128};  //0 = Dummy, we use 1-2
int PrtPin[LOC_PORT] = {0,A2,A3,3,5,6,9,10};
byte PrtType[LOC_PORT] = {0,1,1,2,2,2,2,2};
byte LocPrt2Out[LOC_PORT] = {0,0,0,0,0,0,0,0}; 
bool OutUpd[LOC_PORT] = {0,0,0,0,0,0,0,0};
byte PrtVal[LOC_PORT];
byte PrtPrevVal[LOC_PORT];
//
int LongClick = 500; // after x millis a click is a long click
int LongIncr = 200;  // increment value per x millis
byte LocInp2Sens[LOC_DIGIN] = {0,0,0}; 
byte PortPrevStat[LOC_DIGIN];
byte PortState[LOC_DIGIN];
unsigned long PortStateStart[LOC_DIGIN];   // Millis met start state
unsigned long PortStart[LOC_DIGIN];   // Millis met start pulse
unsigned long PortLastIncr[LOC_DIGIN];   // Millis met start pulse
byte Action[LOC_DIGIN][6] = {{0,0,0,0,0,0},{0,3,1,0,3,140},{0,0,0,0,0,0}};
unsigned long NextReport;
//
#ifdef IR_CODE 
  #include "IRLremote.h"
  const int interruptIR = digitalPinToInterrupt(2);
  uint8_t IRProtocol = 0;
  uint16_t IRAddress = 0;
  uint32_t IRCommand = 0;
  byte IRLastPort = 0;
  byte IRLastAction = 0;
  unsigned long IRPortChange = 0;
#endif

// #################################################################
// ###### SETUP ####################################################
// #################################################################
void setup() {
  #ifdef DO_DEBUG 
    Serial.begin(115200);
    printf_begin();
  #endif
  randomSeed(analogRead(A7));
  NextReport = millis() + random(2000,12000);
  // ===== SETUP LOCAL PORTS ===================================
  for (byte port = 1; port < LOC_PORT ; port++)   {
    pinMode(PrtPin[port],OUTPUT);
    if (PrtType[port] == 1) digitalWrite(PrtPin[port],LOW);
    else if (PrtType[port] == 2) {
      analogWrite(PrtPin[port],0);
      PrtPrevVal[port]=128;
    }
    PrtVal[port] = 0;
  }
  for (byte port = 1; port < LOC_DIGIN ; port++)   {
     pinMode(DigInp[port],INPUT_PULLUP);
     // # if (digitalRead(DigInp[port]) == HIGH)  modValue[port | B10000000] = AAN;
     // # else modValue[port | B10000000] = UIT; 
     // PortPrevStat[port] = modValue[port | B10000000];
     // PortState[port] = modValue[port | B10000000];
  }
  delay(100);
  for (byte port = 1; port < LOC_DIGIN ; port++)   {
    //pinMode(DigInp[port],INPUT_PULLUP);
    //modValue[port | B10000000] = AAN;
    PortPrevStat[port] = AAN;
    PortState[port] = AAN;
  }
  // TODO ANAIN
  #ifdef DO_DEBUG
    printf("Poort setup ok...\n\r");
  #endif
// ===== END SETUP LOCAL PORTS ===============================
  // Lees Address uit Eeprom (Std 119) als 0 dan 119
  if (EEPROM.read(1) == 'A')  MyAddress = EEPROM.read(2);
  if ((MyAddress <= BUS_MASTER) || (MyAddress > BUS_DEFAULT)) MyAddress = BUS_DEFAULT;   
  #ifdef DO_DEBUG 
    printf("MyAddress ex EEPROM = %i.\n\r ",MyAddress);
  #endif
  if ((EEPROM.read(3) == 'P') && (EEPROM.read(4) == 'G')) { // Program Inputs
    for (byte port = 0; port < LOC_DIGIN ; port++) {
      for (int x = 0; x < 6 ; x++) Action[port][x] = EEPROM.read(128+port*6+x);
    }
  } else {
    EEPROM.write(3,'P');
    EEPROM.write(4,'G');
    for (byte port = 0; port < LOC_DIGIN ; port++) {
      for (int x = 0; x < 6 ; x++) EEPROM.write((128+port*6+x),Action[port][x]);
    }
  }
// ===== INIT BUS  (RF/X2X/..)==en meld module ===============
  InitBUS();
// Poort 10???
  pinMode(10, OUTPUT);
  analogWrite(10,0);
  #ifdef IR_CODE 
    IRLbegin<IR_ALL>(interruptIR);
  #endif
}

// #################################################################
// ###### Main Loop ################################################
// #################################################################
void loop() {
  ReadMyPins();
  yield();
  ReceiveBUS();
  yield();
  ProcessCommand();
  yield();
  ReceiveBUS();
  yield();
  ProcessQueue();
  yield();
  ReceiveBUS();
  yield();
  SendBusQueue();
  yield();
  ReceiveBUS();
  yield();
  #ifdef IR_CODE 
    ProcessIR();
  #endif
}

#ifdef IR_CODE 
void IREvent(uint8_t protocol, uint16_t address, uint32_t command) {
  // called when directly received an interrupt CHANGE.
  // do not use Serial inside, it can crash your program!

  // update the values to the newest valid input
  IRProtocol = protocol;
  IRAddress = address;
  IRCommand = command;
}

void ProcessIR() {
   // 1 = 29 / 2 = 12 / 3 = 32 / 4 = 13 / 5 = 10 / 6 = 24 / 8 = 9 /
   // Keyes Remote: IR Protocol: 3 Address: 0xFF00    
   //    1=0xE916  2=0xE619  3=0xF20D  4=0xF30C  5=0xE718  6=0xA15E  7=0xF708  8=0xE31C  9=0xA55A  0=0xAD52
   //    Up=0xB946 Down=0xEA15 Left=0xBB44 Right=0xBC43 Repeat: Address: 0x0000 Command: 0xFFFF
   // Disable interrupts IR
  uint8_t oldSREG = SREG;
  cli();
  if (IRProtocol) {
    byte IRAction = 0;
    // print as much as you want in this function
    // see source to terminate what number is for each protocol
    #ifdef DO_DEBUG
      printf("IR Protocol: %i Address: 0x%04X Command: 0x%04X.\n\r",IRProtocol,IRAddress,IRCommand);
    #endif
    switch (IRProtocol) {
      case 3: 
        switch (IRAddress) {
          case 0x02D2:  // GAME
          case 0xFF00:  // KEYES
            switch (IRCommand) {
              // KEYES   1=0xE916  2=0xE619  3=0xF20D  4=0xF30C  5=0xE718  6=0xA15E  7=0xF708  8=0xE31C  9=0xA55A  0=0xAD52
              case 0x2AD5: case 0xE916: IRLastPort = 29; IRPortChange = millis(); break; // 1
              case 0x29D6: case 0xE619: IRLastPort = 12; IRPortChange = millis(); break; // 2
              case 0x28D7: case 0xF20D: IRLastPort = 29; IRPortChange = millis(); break; // 3
              case 0x26D9: case 0xE718: IRLastPort = 10; IRPortChange = millis(); break; // 5
              case 0x23DC: case 0xE31C: IRLastPort = 9; IRPortChange = millis(); break; // 8
              // KEYES Up=0xB946 Down=0xEA15 Left=0xBB44 Right=0xBC43 Repeat: Address: 0x0000 Command: 0xFFFF
              case 0xBB44: IRAction = 2; break; // Links
              case 0xBC43: IRAction = 3; break; // Rechts
              case 0xB946: IRAction = 200; break; // Up
              case 0xEA15: IRAction = 136; break; // Down
            }
            break;
          case 0x6DD2: 
            switch (IRCommand) {
              case 0x7B84: IRAction = 2; break; // Links
              case 0x7A85: IRAction = 3; break; // Rechts
              case 0x7D82: IRAction = 200; break; // Up
              case 0x7C83: IRAction = 136; break; // Down
            }
            break;
          case 0x0000: 
            switch (IRCommand) {
              case 0xFFFF: IRAction = IRLastAction; // Repeat
            }
            break;
        }
        break;
      case 6: 
        switch (IRAddress) {
          case 0x01:  // Vol up/Down
            switch (IRCommand) {
              case 0x12: IRAction = 200; break;  // Up
              case 0x13: IRAction = 136; break; // Down
            }
            break;
        }
        break;
    }
    IRProtocol = 0;
    if (IRAction  && ((millis() - IRPortChange) < 8000)) {
      IRPortChange = millis();
      DoAction(1,IRLastPort,IRAction);
      IRLastAction = IRAction;
    }
  }
  SREG = oldSREG;
}
#endif

