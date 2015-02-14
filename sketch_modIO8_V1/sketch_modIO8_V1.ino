// #################################################################
// ###### MODIO8A ##################################################
// #################################################################
/* kenmerken Mini: 8 AnaIn 0 AnaOut 14 DigIn 6 PWM	 1KB Eeprom 2KB SRAM 32KB Flash
modIO8 Arduino Mini with 8 inputs, 8 outputs (relais), 2 PWM, I2C bus for comunication with master
I2C bus 0: (wire) = X2C = bus tussen eigen modules
X2C address defaults to 119 and can be changed by master. Master can control outputs, program inputs, poll state/pending actions
X2C bus multimastermodel om eenvoud. Defacto master = '0x10'
input: 8 buttons (pull down) Action configurable by master. Default = ON relay 
output: 8 relays, 2 PWM
Action : 6 bytes per input. Byte 0-2 = single click, byte3-5 = long click
   byte 0/3 = Range (0 = Local Port/ 1 - ... OutputRange / 128 = Local Function / 129 = Mood Master... 
   byte 1/4 = Local Port/Output - Local Port: 0x0... = Digital / 0x1... = Analog
   byte 2/5 = action: 0 = none / 1 = toggle / 2 = off / 3 = on / 4 - 127 = fix val / 128 - 191 = delta + / 192 - 192 = delta - 
myInt = (int)myLong;
myByte = (byte)MyInt 
*/
#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include <printf.h>        
#define DO_DEBUG           // comment out to eliminate debugging

const char MyModel[6] = "MX88A";    // M(ini)X(2C)8(In)8(Uit)A  
const char MyVersion[6] = "01.09";  // HW_SW versie  
#define MAX_QUEUE 8
#define MAX_CMD_LEN 12 
#define MAX_RCV_QUEUE 8  //(4, 8, 16) 
#define MAX_SND_QUEUE 8  //(4, 8, 16)

const byte LOC_PORT = 9;  // 1 extra voor nul-entry - toekomstig gebruik
const byte LOC_DIGIN = 9;  // 1 extra voor nul-entry - toekomstig gebruik
const byte LOC_ANAIN = 0;  // 1 extra voor nul-entry - toekomstig gebruik
const byte AAN = 255;  // Used in Value for 'On'
const byte UIT = 0;    // Used in Value for 'Off'

const byte BUS_MASTER = 0x10;
const byte BUS_DEFAULT = 0x3E;  // Mogelijk tot 45 modules

byte MyBUS = 0x01;      // RF24 bus  
byte MyAddress = 0x00;  // 0x10 = Master
byte MyModID = 2;  // Wordt door master opgegeven 
byte MyRange = 1;  // Wordt door master opgegeven 
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
int DigInp[LOC_DIGIN] = {0,A3,A2,A1,A0,13,12,11,10};// ,A7,A6};  //0 = Dummy, we use 1-8  A7/A6 Analog ONLY
// int AnaOut[LOC_ANAOUT] = {0,10,11};  //0 = Dummy, we use 1-2
// int AnaVal[3] = {0,128,128};  //0 = Dummy, we use 1-2
int PrtPin[LOC_PORT] = {0,2,3,4,5,6,7,8,9};
byte PrtType[LOC_PORT] = {0,1,1,1,1,1,1,1,1};
byte LocPrt2Out[LOC_PORT] = {0,0,0,0,0,0,0,0,0}; 
bool OutUpd[LOC_PORT] = {0,0,0,0,0,0,0,0};
byte PrtVal[LOC_PORT];
byte PrtPrevVal[LOC_PORT];
//
int LongClick = 500; // after x millis a click is a long click
int LongIncr = 500;  // increment value per x millis
byte LocInp2Sens[LOC_DIGIN] = {0,0,0,0,0,0,0,0,0}; 
byte PortPrevStat[LOC_DIGIN];
byte PortState[LOC_DIGIN];
unsigned long PortStateStart[LOC_DIGIN];   // Millis met start state
unsigned long PortStart[LOC_DIGIN];   // Millis met start pulse
unsigned long PortLastIncr[LOC_DIGIN];   // Millis met start pulse
byte Action[LOC_DIGIN][6] = {{0,0,0,0,0,0},{0,1,1,0,0,0},{0,2,1,0,0,0},{0,3,1,0,0,0},{0,4,1,0,0,0},{0,5,1,0,0,0},{0,6,1,0,0,0},{0,7,1,0,0,0},{0,8,1,0,0,0}};
unsigned long NextReport;

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
  if (EEPROM.read(1) == 'A')  MyAddress = EEPROM.read(2);  // Enkel write als commando Master
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
// ===== INIT BUS  (RF/X2X/..)=+ meld module==================
  InitBUS();
}

// #################################################################
// ###### Main Loop ################################################
// #################################################################
void loop()
{
  ReadMyPins();
  yield();
  // ReceiveBUS(); Interrupt
  // yield();
  ProcessCommand();
  yield();
  ProcessQueue();
  yield();
  SendBusQueue();
  yield();
}

