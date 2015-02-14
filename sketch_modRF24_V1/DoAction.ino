// #################################################################
// ###### DoAction #################################################
// #################################################################
/* Action : 
   OnRange  = Range (0 = Local Port/ 1 - ... OutputRange / 128 = Local Function / 129 = Mood Master... 
   OnPort   = Local Port/Output - Local Port: 0x0... = Digital / 0x1... = Analog
   DoAction = action: 0 = none / 1 = toggle / 2 = off / 3 = on / 4 - 127 = fix val / 128 - 191 = delta + / 192 - 192 = delta - 
*/
void DoAction(byte OnRange, byte OnPort, byte xAction) {
  if (OnRange == 0) {    // LOKALE POORT 
    #ifdef DO_DEBUG
      printf("On Port: %i Do Action: %i.\n\r",OnPort,xAction);
    #endif
    int OldValue = PrtVal[OnPort];
    int NewValue = OldValue;
    switch (xAction) {     // action: 0 = none / 1 = toggle / 2 = off / 3 = on / 4 - 127 = fix val / 128 - 191 = delta + / 192 - 192 = delta -
      case 1: //Toggle
        if (PrtType[OnPort] == 1) {
           if (OldValue > 0 ) NewValue = UIT;
           else NewValue = AAN;  
        }
        else if (PrtType[OnPort] == 2) {
           if (OldValue > 0 ) {
              PrtPrevVal[OnPort] = (byte)OldValue;
              NewValue = 0;
           }
           else NewValue = PrtPrevVal[OnPort];  
        }
        break; 
      case 2: //Uit 
        NewValue = UIT;
        break; 
      case 3: 
        NewValue = AAN; // Voor analoog hier een oude waarde??
        break;
      default: 
        if (xAction & B10000000) {
          byte fDelta = xAction & B00111111; 
          if (xAction & B01000000) {   // UP
            NewValue += fDelta;
            if (NewValue > 255) NewValue = 255;
          }
          else {                        //DOWN
            NewValue -= fDelta;
            if (NewValue < 0) NewValue = 0;
          }
        }
        else NewValue = xAction<<1;       //FIX VALUE
    }  //SWITCH  
    if (NewValue != OldValue) {
      Queue('H',2,OnPort,0,0,0,0,0);
      PrtVal[OnPort] = (byte)NewValue;
      // TODO ENKEL OP MASTER
      // Value[OnOutput] = (byte)NewValue;
      if (PrtType[OnPort] == 1) {
         if (PrtVal[OnPort] == UIT)  digitalWrite(PrtPin[OnPort], LOW);
         else digitalWrite(PrtPin[OnPort], HIGH);  
      }
      else if (PrtType[OnPort] == 2) analogWrite(PrtPin[OnPort],(byte)NewValue);
      #ifdef DO_DEBUG
        printf("Switch on Port: %i Value: %i.\n\r",OnPort, NewValue);
      #endif
    } 
  }
  else  {     // Geen lokale poort > Stuur naar Master
    byte buf = snd_Index;
    snd_Index++;  snd_Index &= MAX_SND_Mask; 
    //snd_Bus[buf] = toBus;
    snd_Address[buf] = BUS_MASTER;
    snd_Buf[buf][0] = MyAddress;
    snd_Buf[buf][1] = 0x42;
    snd_Buf[buf][2] = 0x03;
    snd_Buf[buf][3] = OnRange;
    snd_Buf[buf][4] = OnPort;
    snd_Buf[buf][5] = xAction;
    snd_Count[buf]= 0;
    SendBus(buf);
  }
}

// #################################################################
// ###### ReadMyPins ###############################################
// #################################################################
void ReadMyPins ()   {
  for (byte port = 1; port < LOC_DIGIN ; port++) {
    if ((millis() - PortStateStart[port]) > 50) {
      long fDelta = 0;
      PortState[port] = digitalRead(DigInp[port]); 
      if (PortState[port] == LOW && PortPrevStat[port] == HIGH)  {
        PortStart[port] = millis();   // Start pressing
        PortLastIncr[port] = millis();   // LastIncr
        PortStateStart[port] = millis();
        #ifdef DO_DEBUG
          printf("Input down start op:%i.\n\r", port);
        #endif
      }
      else fDelta = millis() - PortStart[port];       
      if ((PortState[port] == LOW) && (PortPrevStat[port] == LOW)  && (Action[port][5] > 0)) {  // Cont pressing
        if ( fDelta > LongClick) {
          if ( PortLastIncr[port] == PortStart[port] ) {  // Only Once 
             PortLastIncr[port] = millis();
             DoAction (Action[port][3], Action[port][4], Action[port][5]);
          }
          else if ( Action[port][5] & B10000000 ) {       // Repeat INC     
            if ( millis() - PortLastIncr[port] > LongIncr ) {
              PortLastIncr[port] = millis();
              DoAction (Action[port][3], Action[port][4], Action[port][5]);
            }
          }
          // #ifdef DO_DEBUG
          //   printf("Lang in:%i.\n\r", port);
          // #endif
        } 
      }
      if ((PortState[port] == HIGH) && (PortPrevStat [port] == LOW)){  // End pressing
        PortStateStart[port] = millis();
        if ((millis() - PortStart[port]) < LongClick) {
          if (Action[port][2] > 0) DoAction(Action[port][0], Action[port][1], Action[port][2]);  // Actie 1  queue naar master: opdracht of status
        }
        else if (Action[port][5] & B10000000) Action[port][5] = Action[port][5] ^ B01000000;      //Toggle Up/down bij einde longpress
        #ifdef DO_DEBUG
          printf("Input up op:%i.\n\r", port);
        #endif
      }
      if (PortPrevStat[port] != PortState[port]) {
        PortPrevStat[port] = PortState[port];
        if (LocInp2Sens[port] > 0) {
          byte buf = snd_Index;
          byte fVal = 0;
          if (PortState[port] == HIGH) fVal = 255;
          snd_Index++;  snd_Index &= MAX_SND_Mask; 
          snd_Address[buf] = BUS_MASTER;
          snd_Buf[buf][0] = MyAddress;
          snd_Buf[buf][1] = 0x45;
          snd_Buf[buf][2] = 0x02;
          snd_Buf[buf][3] = LocInp2Sens[port];
          snd_Buf[buf][4] = fVal;
          snd_Count[buf]= 0;
          SendBus(buf);
        }
      }
    }
  }
}

