// #################################################################
// ###### InitX2C ##################################################
// #################################################################
void InitX2C()  { // Called from Setup()
  Wire1.begin(MyAddress[1]);    // join X2C bus (SDA/SCL op pin 20/21)
    #ifdef DO_DEBUG
      sprintf(sLine,"X2C MyAddress = %i.\r\n",MyAddress[1]); Serial.print(sLine);
    #endif
  Wire1.setClock(100000UL);     //1412 was 10000
  Wire1.onReceive(ReceiveX2C);   // register event
  #ifdef DO_DEBUG
    Serial.println("X2C setup ok..");
  #endif
}

// #################################################################
// ###### ReceiveX2C ###############################################
// #################################################################
// Command = Bus/Afzender/Command/DatLen/D1/../DN/CC
// INTERRUPT !
void ReceiveX2C(int howMany){
  DebugOn(4);
  while (Wire1.available() > 0) {
    byte MyIndex = 0;
    byte rcv_End = MAX_CMD_LEN;
    #ifdef DO_DEBUG
      sprintf(sLine,"X2C Receive MyIndex = %i, rcv_End = %i.\r\n",MyIndex,rcv_End); Serial.print(sLine);
    #endif
    rcv_Bus[rcv_Index] = 1; //BUS=1 voor X2C
    // rcv_Buf[rcv_Index][MyIndex++] = 1; //BUS=1 voor X2C
    while ((Wire1.available() > 0) && (MyIndex < rcv_End)) {
      rcv_Buf[rcv_Index][MyIndex++] = Wire1.read();
      //#ifdef DO_DEBUG
      //  printf("0x%X",rcv_Buf[rcv_Index][MyIndex-1]);
      //#endif
      if ((MyIndex == 3) && (rcv_Buf[rcv_Index][2] < (MAX_RCV_Mask-3))) rcv_End = rcv_Buf[rcv_Index][2] + 5;
    }  
    //#ifdef DO_DEBUG
    //  printf("X2C Receive MyIndex = %i, rcv_End = %i.\r\n",MyIndex,rcv_End);
    //#endif
    int fCheck = CheckCommand(rcv_Index);      
    if ( fCheck > 0) {
       rcv_Index++;
       rcv_Index&=MAX_RCV_Mask;
    }
  }
  DebugOff(4);
}

// #################################################################
// ###### SendX2C ##################################################
// #################################################################
void SendX2C(byte fBuf) {
    Wire1.begin();   // Set as master
    Wire1.setClock(100000UL);      //1412 was 10000
    //delay(10);
    Wire1.beginTransmission(snd_Address[fBuf]);
    byte flength = Wire1.write(snd_Buf[fBuf],snd_CC[fBuf]+1);    // Check:wire stopt bij 0x00 ???  
    byte retcode = Wire1.endTransmission(true);
    if (retcode != 0) {
      #ifdef DO_DEBUG
        Serial.println("failed X2C send."); 
      #endif
      snd_Status[fBuf] = 2; //Failed
    }
    else {
      snd_Status[fBuf] = 1; //Send
    }
    Wire1.begin(MyAddress[1]);                // join X2C bus
    Wire1.onReceive(ReceiveX2C);           // register event
}

