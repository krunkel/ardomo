// ################################################################################
// ###### InitBUS ################################################## 20DEC14 ######
// ################################################################################
void InitBUS()  { // Called from Setup()
  Wire.begin(MyAddress); 
  TWBR = 72;  // TWBR = 152 > 50 kHz  -- TWBR = 72 > 100 kHz (default)
  Wire.onReceive(ReceiveBUS);           // register event
  #ifdef DO_DEBUG
    printf("X2C setup ok..\n\r");
  #endif
  delay(50);
  //Report2Master();
}
// ################################################################################
// ###### ReceiveBUS ############################################### 29NOV14 ######
// ################################################################################
// Command = Bus/Afzender/Command/DatLen/D1/../DN/CC
void ReceiveBUS(int howMany){ 
  while (Wire.available() > 0) {
    byte MyIndex = 0;
    byte rcv_End = MAX_CMD_LEN;
    // rcv_Bus[rcv_Index] = 1; //BUS=1 voor X2C
    while ((Wire.available() > 0) && (MyIndex < rcv_End)) {
      rcv_Buf[rcv_Index][MyIndex++] = Wire.read();
      if ((MyIndex == 3) && (rcv_Buf[rcv_Index][2] < (MAX_RCV_Mask-3))) rcv_End = rcv_Buf[rcv_Index][2] + 5;
    }  
    int fCheck = CheckCommand(rcv_Index);      
    if ( fCheck > 0) {
       rcv_Index++;
       rcv_Index&=MAX_RCV_Mask;
    }
  }
}

// ################################################################################
// ###### SendBusX ################################################# 20DEC14 ######
// ################################################################################
void SendBusX(byte fBuf) {
    Wire.begin();   // Set as master
    Wire.setClock(100000UL);      //1412 was 10000
    //delay(10);
    Wire.beginTransmission(snd_Address[fBuf]);
    byte flength = Wire.write(snd_Buf[fBuf],snd_CC[fBuf]+2);    // Check:wire stopt bij 0x00 ???  
    byte retcode = Wire.endTransmission(true);
      #ifdef DO_DEBUG
        printf("Send X2C flength = %i, CCPlus1 = %i.\n\r", flength,snd_CC[fBuf]+1); 
      #endif
    if (retcode != 0) {
      #ifdef DO_DEBUG
        printf("failed X2C send.\n\r"); 
      #endif
      snd_Status[fBuf] = 2; //Failed
    }
    else {
      snd_Status[fBuf] = 1; //Send
    }
    Wire.begin(MyAddress);                // join X2C bus
    Wire.onReceive(ReceiveBUS);           // register event
}

void Report2Master() {
// ===== meld module : AA4104DOAODIAICC ======================
  NextReport = millis() + 1000000 + random(10000,20000);
  byte buf = snd_Index;
  snd_Index++;  snd_Index &= MAX_SND_Mask; 
  //snd_Bus[buf] = toBus;
  snd_Address[buf] = BUS_MASTER;
  snd_Buf[buf][0] = MyAddress;
  snd_Buf[buf][1] = 0x41;
  snd_Buf[buf][2] = 0x04;
  snd_Buf[buf][3] = LOC_PORT;
  snd_Buf[buf][4] = LOC_PORT;
  snd_Buf[buf][5] = LOC_DIGIN;
  snd_Buf[buf][6] = LOC_ANAIN;
  snd_Count[buf]= 0;
  SendBus(buf);
  buf = snd_Index;
  snd_Index++;   snd_Index &= MAX_SND_Mask; 
  snd_Address[buf] = BUS_MASTER;
  snd_Buf[buf][0] = MyAddress;
  snd_Buf[buf][1] = 0x44;
  snd_Buf[buf][2] = 0x07;
  snd_Buf[buf][3] = 'M';
  memcpy(&snd_Buf[buf][4], MyModel,6);
  snd_Count[buf]= 0;
  SendBus(buf);
  buf = snd_Index;
  snd_Index++;  snd_Index &= MAX_SND_Mask; 
  snd_Address[buf] = BUS_MASTER;
  snd_Buf[buf][0] = MyAddress;
  snd_Buf[buf][1] = 0x44;
  snd_Buf[buf][2] = 0x07;
  snd_Buf[buf][3] = 'V';
  memcpy(&snd_Buf[buf][4], MyVersion,6);
  snd_Count[buf]= 0;
  SendBus(buf);
}

