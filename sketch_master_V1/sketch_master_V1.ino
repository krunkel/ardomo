// #################################################################
// ###### MASTER  ##################################################
// #################################################################
/* Master:  4xPWM / 4x0-10V / 2 Ana Out/ 24 DigIn /  8 DigOut /   0KB Eeprom 96KB SRAM 512KB Flash
Ethernet
SD-Card
I2C bus 0: (wire) diverse sensoren: RTC / Lichtsensor
I2C bus 1: (wire1) = X2C = bus tussen eigen modules
X2C address defaults to X2C_DEFAULT and can be changed by master. Master can control outputs, program inputs, poll state/pending actions
X2C bus multimastermodel om eenvoud. Defacto master = '0x10'
RF24 
V03: uitlijnen X2C comm met RF24 comm
*/
// #include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <Ethernet.h>
#include <Scheduler.h>
#include <Wire.h>
//#include <SD.h>
#include <SdFat.h>
#include <SPI.h>
#include <utility/w5100.h>
#include <utility/socket.h>
#include <DS3231.h>     // RTC clock
#include <Time.h>  
#include <TimeLord.h>   // Sunset
#include <TM1637.h>     // UU:MM display
#include <nRF24L01.h>   // RF24 
#include <RF24.h>       // RF24
#include <BH1750.h>     // Lichtmeter

#define DO_DEBUG  // comment out to eliminate debugging
#define MAX_MODULE 16
#define MAX_TIMERS 64
#define MAX_OUTPUT 128
#define MAX_SENSOR 128
#define MAX_RANGE 0 // (0=1/1=2/2=4/3=8) 
#define MAX_VAL 1024 // (256+3x256xRanges)
#define MAX_MOODS 48
#define MAX_MOODS_EL 255
#define ACT_LENGHT 3
#define MAX_QUEUE 64
#define MAX_CMD_LEN 12   // AFZ/CC/LL/.8./CC
#define MAX_RCV_QUEUE 32  //(4, 8, 16) 
#define MAX_SND_QUEUE 64 //??? TODO
#define MAX_LOGIC 256
#define MAX_DELAY 32
#define MAX_ARRAY 512
#define RF24_CE 53 //  "Chip Enable" pin, activates the RX or TX role 
#define RF24_CSN 52 // SPI Chip select
#define SD_CSN 4    //SD Chipselect
//
const char MyModel[6] = "MASTR";
const char MyVersion[6] = "01.08";  // HW_SW versie  

// LOCAL PORTS
const byte LOC_PORT = 17;  // 1 extra voor nul-entry - toekomstig gebruik
const byte LOC_DIGIN = 21;  // 1 extra voor nul-entry - toekomstig gebruik
const byte AAN = 255;  // Used in Value for 'On'
const byte UIT = 0;    // Used in Value for 'Off'
// ================================================================================
// ===== INIT BUS 1:X2C 2:RF24 ====================================================
// ================================================================================
const byte BUS_MASTER[3] = {0,0x10,0xC1};
const byte BUS_DEFAULT[3] = {0,0x3E,0xDF}; 
byte BUS_Next[3] = {0,0,0}; 
byte MyAddress[3] = {0,0x10,0xC1};

RF24 radio(RF24_CE,RF24_CSN);
uint8_t RF_MASTER_FULL[4] = {0xCC,0xCC,0xCC,0xCC};
uint8_t RF_CLIENT_FULL[4] = {0xCC,0xCC,0xCC,0xCC};
// ===== INIT RF24 ==<END>=========================================================

byte MyModID = 1;  // 1 = Master

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,3);
IPAddress SenderIP;
// IPAddress gateway( 192,168,1,1 );
// IPAddress subnet( 255,255,255,0 );
/*  ==WEB==START=============================================
    Based on Web server sketch for IDE v1.0.5 and w5100/w5200
    Originally posted November 2013 by SurferTim
    Last modified 11 NOV 2014      
    ==WEB==================================================== */
// MAX_SOCK_NUM = 4 in Ethernet.h
EthernetServer webserver(88);

const char legalChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890/.-_~";
unsigned long connectTime[MAX_SOCK_NUM];
//  ==WEB==END================================================
//  ==COMMUNICATION===========================================
// SND BUFFER (inkomende commando's)
const byte MAX_SND_Mask = MAX_SND_QUEUE - 1; 
volatile byte snd_Index = 0; // Volgende positie in send buffer
volatile byte snd_Procs = 0; // Volgende te zenden buffer
volatile byte snd_Status[MAX_SND_QUEUE];   // 1 = verzonden / 2 = resending / ??? Nodig
volatile byte snd_CC[MAX_SND_QUEUE];   // Positie van CC (0 > te berekenen) 
volatile byte snd_Bus[MAX_SND_QUEUE];
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
volatile byte rcv_Bus[MAX_SND_QUEUE];
byte rcv_Buf[MAX_RCV_QUEUE][MAX_CMD_LEN];    //  Volatile????? 
//  ==COMMUNICATION==END======================================
//  ==BUS+MODULE==============================================
// In from SD MODULE.CSV 1;1;127;8;0;8,2    (int ID;byte BUS;int Address;NumDO;NumAO;NumDI;NumAI
// #define MAX_MODULE 16  
byte num_mod;
byte mod_ID[MAX_MODULE];
byte mod_BUS[MAX_MODULE];
int  mod_Address[MAX_MODULE];
byte mod_NumOUT[MAX_MODULE];
byte mod_NumIN[MAX_MODULE];
byte mod_Range[MAX_MODULE];
char mod_Model[MAX_MODULE][6];
char mod_Version[MAX_MODULE][6];
bool mod_Ok[MAX_MODULE];
byte mod_LastCont[MAX_MODULE][2];
// byte mod2Pol = 1;
//  ==TIMERS==================================================
byte num_tim;
byte tim_ID[MAX_TIMERS];
int  tim_Year[MAX_TIMERS];
byte tim_Month[MAX_TIMERS];
byte tim_Day[MAX_TIMERS];
byte tim_Hour[MAX_TIMERS];
byte tim_Min[MAX_TIMERS];
byte tim_Sec[MAX_TIMERS];
byte tim_Act1[MAX_TIMERS];
byte tim_Act2[MAX_TIMERS];
byte tim_Act3[MAX_TIMERS];
bool tim_Today[MAX_TIMERS];
bool tim_Done[MAX_TIMERS];
long tim_SecToday[MAX_TIMERS];   // Seconden vanaf 00h00 - indien Sunrise/Sunset timer 
long tim_Sunrise; // seconden na middernacht
long tim_Sunset; // Seconden na middernacht
byte tim_Reeks[6];
byte tim_Sun[6];
byte TimerDay = 0;
byte TimerTen = 7;
byte TimerHour = 32;
//  ==MOODS===================================================
byte num_moods;
byte moo_Lenght[MAX_MOODS];
short moo_Pointer[MAX_MOODS];
byte moo_ElRange[MAX_MOODS_EL];
byte moo_ElPort[MAX_MOODS_EL];
byte moo_ElValue[MAX_MOODS_EL];
short NextFreeMooElItem = 1;
//  ==OUTPUTS=================================================
byte num_output;
byte output_ID[MAX_OUTPUT];
byte output_ToMaster[MAX_OUTPUT];
byte output_TYP[MAX_OUTPUT];
byte output_MOD[MAX_OUTPUT];
byte output_PRT[MAX_OUTPUT];
short output_Logic[MAX_OUTPUT];
byte output_GRAF[MAX_OUTPUT];
byte output_X[MAX_OUTPUT];
byte output_Y[MAX_OUTPUT];
byte output_Z[MAX_OUTPUT];
//  ==SENSORS=================================================
byte num_sensor;
byte sensor_ID[MAX_SENSOR];
byte sensor_ToMaster[MAX_SENSOR];
byte sensor_TYP[MAX_SENSOR];
byte sensor_MOD[MAX_SENSOR];
byte sensor_PRT[MAX_SENSOR];
byte sensor_GRAF[MAX_SENSOR];
byte sensor_X[MAX_SENSOR];
byte sensor_Y[MAX_SENSOR];
byte sensor_Z[MAX_SENSOR];
short sensor_Logic[MAX_SENSOR];
byte sensor_Action[MAX_SENSOR][6];
// =======================================================
const byte Que_Mask = MAX_QUEUE-1;
byte QueNext = 0;
byte QueProc = 0;
byte QueType[MAX_QUEUE];
byte QueIndex[MAX_QUEUE];
byte QueParm[MAX_QUEUE][6];
// =QUEUE =LOW PRIORITY =======================================
byte QueLNext = 0;
byte QueLProc = 0;
byte QueLType[MAX_QUEUE];
byte QueLRepeat[MAX_QUEUE];
byte QueLParm[MAX_QUEUE][6];
// =LOGIC======================================================
short num_log = 0;
short NextFreeArr = 1;
short NextFreeArrItem = 1;
byte log_pType[MAX_LOGIC];
short log_pInp[MAX_LOGIC];
short log_pOut[MAX_LOGIC];
short log_Val1[MAX_LOGIC];
short log_Val2[MAX_LOGIC];
byte log_Act1Range[MAX_LOGIC];
byte log_Act1Port[MAX_LOGIC];
byte log_Act1Act[MAX_LOGIC];
byte log_Act2Range[MAX_LOGIC];
byte log_Act2Port[MAX_LOGIC];
byte log_Act2Act[MAX_LOGIC];
byte arr_len[MAX_ARRAY];
short arr_ptr[MAX_ARRAY];
short array[MAX_ARRAY*4]; //?
short log2Pol = 1;
// =DELAY======================================================
bool delay_act[MAX_DELAY];
byte delay_logic[MAX_DELAY];
//unsigned long delay_start[MAX_DELAY];
unsigned long delay_go[MAX_DELAY];
byte delay_ActRange[MAX_DELAY];
byte delay_ActPort[MAX_DELAY];
byte delay_ActAct[MAX_DELAY];
// ============================================================
// TODO Value reeksen
// Value[0][x]=Values intern gebruik
// Value[1/2][x]=Ranges output
// Value[3/4][x]=Ranges input
// Value[5][x]=Logic

byte Value[MAX_VAL];
// ===== DECLARATION LOCAL PORTS ==============================
byte MyRange = 1;  // TODO 
int DigInp[LOC_DIGIN] = {0,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43};  //0 = Dummy, we use 1-8int PrtPin[LOC_PORT] = {0,A2,A3,3,5,6,9,10};
int LocInp2Sens[LOC_DIGIN] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  //0 = Dummy, we use 1-8int PrtPin[LOC_PORT] = {0,A2,A3,3,5,6,9,10};
// Out
int PrtPin[LOC_PORT] = {0,45,44,47,46,49,48,51,50,3,5,6,7,8,9,11,12};
byte PrtType[LOC_PORT] = {0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2};
byte LocPrt2Out[LOC_PORT] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
bool OutUpd[LOC_PORT] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
byte DebugLed[5]{0,A2,A3,A4,A5};
byte PrtVal[LOC_PORT];
byte PrtPrevVal[LOC_PORT];
//int AnaOut[LOC_ANAOUT] = {0};  //0 = Dummy, we use 1-2
//byte LocAna2Out[LOC_DIGOUT] = {0};
//byte AnaOutV[LOC_DIGOUT];
int LongClick = 500; // after x millis a click is a long click
int LongIncr = 250;  // increment value per x millis
byte PortPrevStat[LOC_DIGIN]; // Boolean ??
byte PortState[LOC_DIGIN];
unsigned long PortStateStart[LOC_DIGIN];   // Millis met start state
unsigned long PortStart[LOC_DIGIN];   // Millis met start pulse
unsigned long PortLastIncr[LOC_DIGIN];   // Millis met start pulse
byte Action[LOC_DIGIN][6] = {{0,0,0,0,0,0},
                             {0,1,1,0,0,0},{0,1,3,0,0,0},{0,3,1,0,0,0},{0,4,1,0,0,0},{0,5,1,0,0,0},{0,6,1,0,0,0},{0,7,1,0,0,0},{0,8,1,0,0,0},
                             {0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},
                             {0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0}};
// ===== END DECLARATION LOCAL PORTS =========================
const char TIMEZONE = 1; 
//const int TIMEZONE = -8; //PST
const float LATITUDE = 51.173439, LONGITUDE = 5.132745; // Locatie
const bool DST = true;  //Daylight Saving Time
const bool h12 = false;
DS3231 RTClock;
#define TM1637_CLK 22 //pins definitions for TM1637 Display   
#define TM1637_DIO 23
TM1637 tm1637(TM1637_CLK,TM1637_DIO);
int8_t TimeDisp[] = {0x01,0x02,0x03,0x04};
TimeLord myLord;
byte sunTime[]  = {0, 0, 0, 1, 1, 15}; // 1 jan 2015
unsigned long LastTime;
unsigned long LastLogic = 0;   
bool TimeFlash = false; // flash :
BH1750 lightMeter;
char sLine[120];
char lLine[80];
SdFat MySD;
Sd2Card MyCard;
SdFile MySDfile;
// SdFile MyLogfile;
// ==== Logfile =====
// char logFile[20]; // /log/YYYYMMDD.LOG
byte numFailed = 0;
char logFailed[60];
bool logOk = false;

// ################################################################################
// ###### SETUP ###################################################################
// ################################################################################
void setup() {
  logFailed[0] = 0;
  #ifdef DO_DEBUG
    Serial.begin(115200);
    Serial.println(F("Start"));
  #endif
  delay(50);
  //SPI.begin(4);
  if (!MySD.begin(SD_CSN, SPI_HALF_SPEED)) {
    #ifdef DO_DEBUG
      Serial.println("initialization failed!");
    #endif 
  }
  else {
    MyCard.init(SPI_HALF_SPEED, SD_CSN);
    LoadProg();  
    #ifdef DO_DEBUG
      Serial.println(F("LoadProg ready"));
    #endif  
  }
  Wire.begin();  // voor speciale I2C modules
  Wire.setClock(100000UL);      //1412 was 10000
  InitX2C();
  #ifdef DO_DEBUG
    Serial.println(F("I2C en X2C ready"));
    Serial.println(F("Internet starting"));
  #endif  
// ================================================================================
// ===== SETUP RF24 ===============================================================
// ================================================================================
  //printf_begin();
  //SPI.begin(52);
  InitRF();
  delay(10);
// ===== SETUP RF24 =<END>=========================================================
// ===== SETUP WEB  & Get NTP=================================
  //SPI.begin(10);
  Ethernet.begin(mac, ip);  // , gateway, gateway, subnet
  delay(200);
  RTClock.setClockMode(false);   // 24u
  DoTijdSync(false);
  // TIMELORD
  myLord.TimeZone(TIMEZONE * 60);
  myLord.Position(LATITUDE, LONGITUDE);
  //myLord.DstRules(3,2,11,1,60); // DST Rules for USA
  // In de Europese Unie loopt de zomertijd van de laatste zondag van maart tot de laatste zondag van oktober.
  // myLord.DstRules(dstMonStart, dstWeekStart, dstMonEnd, dstWeekEnd, 60);
  myLord.DstRules(3,4,10,4,60);
  webserver.begin();
  unsigned long thisTime = millis();
  for(int i=0;i<MAX_SOCK_NUM;i++) connectTime[i] = thisTime;
  // =======================================================
  // START WEBSERVER IN AFZONDERLIJKE THREAD
  // Scheduler.startLoop(webserver);
  // =======================================================
  #ifdef DO_DEBUG
    Serial.print("webserver is at ");
    Serial.println(Ethernet.localIP());
    Serial.println(F("Internet ready"));
  #endif  
  // ===== END SETUP WEB =======================================
  // ===== SETUP LOCAL PORTS ===================================
  for (byte port = 1; port < LOC_PORT ; port++)  {
    pinMode(PrtPin[port],OUTPUT);
    if (PrtType[port] == 1) digitalWrite(PrtPin[port],LOW);
    else if (PrtType[port] == 2) analogWrite(PrtPin[port],0);
    PrtVal[port] = 0;
    PrtPrevVal[port] = 128;
    // short x = ((MyRange<<8) + port);
    // Serial.println(x);
    Value[(MyRange<<8) + port] = UIT; //?????TODO
  }
  for (byte port = 1; port < 5 ; port++)  {
    pinMode(DebugLed[port],OUTPUT);
    digitalWrite(DebugLed[port],HIGH);
  }
  for (byte port = 1; port < LOC_DIGIN ; port++)   {
     pinMode(DigInp[port],INPUT_PULLUP); 
  }
  delay(10);
  for (byte port = 1; port < LOC_DIGIN ; port++)   {
     // modValue[port | B10000000] = AAN;
     PortPrevStat[port] = AAN;
     PortState[port] = AAN;
  }
  #ifdef DO_DEBUG
    Serial.println(F("Poort setup ok..."));
  #endif
  // ===== END SETUP LOCAL PORTS ===============================
  tm1637.set();
  tm1637.init();
  DispTime();
  lightMeter.begin();
  // Scheduler.start(loop2);
  // LOG =====
  sprintf(lLine,"=============================================================================="); lLog(lLine);
  sprintf(lLine,"Programma #Mod:%i, #Out:%i, #Sens%i, #Logic:%i ,#Mood:%i.\0", num_mod, num_output, num_sensor, num_log, num_moods); lLog(lLine);
  // 
}

// #################################################################
// ###### Main loop ################################################
// #################################################################
void loop() {
  //REP 1
  checkWebServer();
  ReadMyPins();
  ReceiveRF();
  ProcessCommand();
  ProcessQueue();
  yield();
  //REP 2
  checkWebServer();
  ReadMyPins();
  ReceiveRF();
  ProcessCommand();
  SendBusQueue(); 
  yield();
  //REP 3
  checkWebServer();
  ReadMyPins();
  ReceiveRF();
  ProcessCommand();
  ProcessTimers();
  yield();
  //REP 4
  checkWebServer();
  ReadMyPins();
  ReceiveRF();
  ProcessCommand();
  DispTime();
  checkSockStatus();  // Om timeouts te genereren ? 
  //  uint16_t lux = lightMeter.readLightLevel();
  //  printf("Light: %i lx.\r\n", lux);
  yield();
}

// #################################################################
// ###### Main loop2 ###############################################
// #################################################################
// void loop2() {
//   ReadMyPins();  //  Mogelijk 2 de keer in loop???
//   yield();
//  ProcessTimers();  // 
//  yield();
// }

// #################################################################
// ###### Webserver ## afzonderlijke thread ########################
// #################################################################
// void webserver() {
//  checkWebServer();
//  checkSockStatus();  // Om timeouts te genereren ? 
//  yield();
// }
// #######################
// ###### DebugOnOff #####
// #######################
   void DebugOn(byte dLed) {
     digitalWrite(DebugLed[dLed],LOW);
   }
   void DebugOff(byte dLed) {
     digitalWrite(DebugLed[dLed],HIGH);
   }

