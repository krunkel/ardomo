// #################################################################
// ###### InitRF ###################################################
// #################################################################
void InitRF()  { // Called from Setup()
  radio.begin();                           // Setup and configure rf radio
  radio.setChannel(1);
  radio.setAddressWidth(4);  
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  // radio.setDataRate(RF24_1MBPS);
  radio.setAutoAck(1);                     // Ensure autoACK is enabled
  //radio.enableAckPayload();                // Allow optional ack payloads
  radio.setRetries(2,15);                  // Optionally, increase the delay between retries & # of retries
  radio.setPayloadSize(MAX_CMD_LEN);   // GVK: Max Command = Afz/Com/Len/08/x1/x2/../x7/x8/CC (12)
  radio.setCRCLength(RF24_CRC_8); 
  RF_MASTER_FULL[3] = BUS_MASTER[2];
  RF_CLIENT_FULL[3] = BUS_DEFAULT[2];
  radio.openWritingPipe(RF_CLIENT_FULL);  
  radio.openReadingPipe(1,RF_MASTER_FULL);
  radio.startListening();                 // Start listening
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
  radio.powerUp();                        //Power up the radio
}

// #################################################################
// ###### ReceiveRF ################################################
// #################################################################
void ReceiveRF() {
  if(radio.available()){ 
    DebugOn(2);
    yield();
    byte MyIndex = 0;
    // byte rcv_End = MAX_RCV_Mask
    rcv_Bus[rcv_Index] = 2; //BUS=1 voor X2C
    radio.read(&rcv_Buf[rcv_Index][0],MAX_CMD_LEN);
    #ifdef DO_DEBUG
      sprintf(sLine,"RF Receive MyIndex = %i, Buf0 = 0x%X.\r\n",MyIndex,rcv_Buf[rcv_Index][0]); Serial.print(sLine);
    #endif
    int fCheck = CheckCommand(rcv_Index);      
    if ( fCheck > 0) {
       rcv_Index++;
       rcv_Index&=MAX_RCV_Mask;
    }
    DebugOff(2);
  }
}
 
void SendRF(byte fBuf) {
  //DebugOn(3);
  RF_CLIENT_FULL[3] = snd_Address[fBuf];      
  radio.openWritingPipe(RF_CLIENT_FULL);  
  yield();
  radio.stopListening(); 
  if (!radio.write( snd_Buf[fBuf], snd_CC[fBuf]+1)){
    #ifdef DO_DEBUG
      sprintf(sLine,"failed RF24 send.\r\n"); Serial.print(sLine); 
    #endif
    snd_Status[fBuf] = 2; //Failed
  }
  else {
    snd_Status[fBuf] = 1; //Send
  }
  radio.startListening();
  //DebugOff(3);
}

