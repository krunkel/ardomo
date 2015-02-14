// #################################################################
// ###### InitBUS ##################################################
// #################################################################
void InitBUS()  { // Called from Setup()
  radio.begin();                           // Setup and configure rf radio
  radio.setChannel(1);
  radio.setAddressWidth(4);  
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  // radio.setDataRate(RF24_1MBPS);
  radio.setAutoAck(1);                     // Ensure autoACK is enabled
  //radio.enableAckPayload();              // Allow optional ack payloads
  radio.setRetries(2,15);                  // Optionally, increase the delay between retries & # of retries
  radio.setPayloadSize(MAX_CMD_LEN);   // GVK: Max Command = Bus/Afz/Com/06/x1/x2/x3/x4/x5/x6/CC (12??)
  radio.setCRCLength(RF24_CRC_8); 
  RF_MASTER_FULL[3] = BUS_MASTER;
  RF_CLIENT_FULL[3] = MyAddress;
  radio.openWritingPipe(RF_MASTER_FULL);
  radio.openReadingPipe(1,RF_CLIENT_FULL);
  radio.startListening();                  // Start listening
  radio.printDetails();                    // Dump the configuration of the rf unit for debugging
  radio.powerUp();                         //Power up the radio
  delay(50);
  //Report2Master();
}

// #################################################################
// ###### ReceiveBUS ###############################################
// #################################################################
void ReceiveBUS() {
  if(radio.available()){ 
    yield();
    byte MyIndex = 0;
    // rcv_Bus[rcv_Index] = 2; //BUS=1 voor X2C
    radio.read(&rcv_Buf[rcv_Index][0],MAX_CMD_LEN);
    //#ifdef DO_DEBUG
    //  printf("RF24 receive 0x%X-0x%X-0x%X-0x%X.\n\r",rcv_Buf[rcv_Index][0],rcv_Buf[rcv_Index][1],rcv_Buf[rcv_Index][2],rcv_Buf[rcv_Index][3]); 
    //#endif
    int fCheck = CheckCommand(rcv_Index);      
    if ( fCheck > 0) {
       rcv_Index++;
       rcv_Index&=MAX_RCV_Mask;
    }
  }
}
 
// #################################################################
// ###### SendBusX #################################################
// #################################################################
void SendBusX(byte fBuf) {
  //RF_CLIENT_FULL[3] = snd_Address[fBuf];      
  //radio.openWritingPipe(RF_CLIENT_FULL);  
  yield();
  radio.stopListening(); 
  // delay(5);  //GGG
  if (!radio.write( snd_Buf[fBuf], snd_CC[fBuf]+2 )){
    #ifdef DO_DEBUG
      printf("failed RF24 send.\n\r"); 
    #endif
    snd_Status[fBuf] = 2; //Failed
  }
  else {
    //#ifdef DO_DEBUG
    //  printf("RF24 send 0x%X-0x%X-0x%X-0x%X.\n\r",snd_Buf[fBuf][0],snd_Buf[fBuf][1],snd_Buf[fBuf][2],snd_Buf[fBuf][3]); 
    //#endif
    snd_Status[fBuf] = 1; //Send
  }
  radio.startListening();
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

