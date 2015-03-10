// #################################################################
// ###### DoAction #################################################
// #################################################################
/* Action :        &
   OnRange  = Range (0 = Local Port/ 1 - ... OutputRange / 128 = Local Function / 129 = Mood Master... 
   OnPort   = Local Port/Output - Local Port: 0x0... = Digital / 0x1... = Analog
   DoAction = action: 0 = none / 1 = toggle / 2 = off / 3 = on / 4 - 127 = fix val / 128 - 191 = delta + / 192 - 192 = delta - 
*/
void DoAction(byte OnRange, byte OnPort, byte xAction) {
  byte OnOutput =  OnPort;
  if ((OnRange == MyRange) && (output_MOD[OnPort] == MyModID)) {
     OnRange = 0;
     OnPort = output_PRT[OnPort];
  }  
  if (OnRange == 0) {    // LOKALE POORT 
    #ifdef DO_DEBUG
      sprintf(sLine,"On Port: %i Do Action: %i.\r\n",OnPort,xAction); Serial.print(sLine);
    #endif
    int OldValue = PrtVal[OnPort];
    int NewValue = OldValue;
    switch (xAction) {     // action: 0 = none / 1 = toggle / 2 = off / 3 = on / 4 - 127 = fix val / 128 - 191 = delta+ / 192 - 192 = delta-
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
        else NewValue = xAction<<1;       //Vaste waarde
    }  //SWITCH  
    if (NewValue != OldValue) {
      PrtVal[OnPort] = (byte)NewValue;
      // TODO ENKEL OP MASTER
      Value[(MyRange<<8) + OnPort] = (byte)NewValue;
      if (PrtType[OnPort] == 1) {
         if (PrtVal[OnPort] == UIT)  digitalWrite(PrtPin[OnPort], LOW);
         else digitalWrite(PrtPin[OnPort], HIGH);  
      }
      else if (PrtType[OnPort] == 2) analogWrite(PrtPin[OnPort],(byte)NewValue);
      #ifdef DO_DEBUG
        sprintf(sLine, "Switch on Port: %i Value: %i Logic:%i.\r\n",OnPort,NewValue,output_Logic[OnPort]); Serial.print(sLine);
      #endif
      if (output_Logic[OnPort] != 0) {
        for (byte x = 0; x < arr_len[output_Logic[OnPort]]; x++) Dologic(array[arr_ptr[output_Logic[OnPort]] + x]);
      }
    } 
  } // END (OnRange == 0)
  else if (OnRange == 254) {    // Mood 
  // moo_Lenght[MAX_MOODS]; moo_Pointer[MAX_MOODS]; moo_ElRange[MAX_MOODS_EL]; moo_ElPort[MAX_MOODS_EL]; moo_ElValue[MAX_MOODS_EL];
    for (byte x = 0; x < moo_Lenght[OnPort];x++) {
      short fEl = moo_Pointer[OnPort] + x;
      if (moo_ElValue[fEl] > 0) {
        Queue('L',1,moo_ElRange[fEl], moo_ElPort[fEl], moo_ElValue[fEl],0,0,0,1);
        // yield();
      }  
    }
  } // END (OnRange == 254)
  else if (OnRange == 255) {    // Value 
  } // END (OnRange == 255)
  else  {   // Geen lokale poort > Stuur naar Module (of Master vanaf module)
    //char fSend[32];
    if (mod_Ok[output_MOD[OnPort]]) {
      if (! (((xAction == 2) && (Value[(MyRange<<8) + OnPort] = 0)) || ((xAction == 3) && (Value[(MyRange<<8) + OnPort] > 0)))) {
        byte toBus = mod_BUS[output_MOD[OnPort]];
        byte toPort = output_PRT[OnPort];
        byte buf = snd_Index;
        snd_Index++;  snd_Index &= MAX_SND_Mask; 
        #ifdef DO_DEBUG
          sprintf(sLine,"Stuur actie naar module:%i bus:%i,%i voor poort:%i actie:%i.\r\n",output_MOD[OnPort],toBus,mod_Address[output_MOD[OnPort]],toPort,xAction); Serial.print(sLine);
        #endif
        snd_Bus[buf] = toBus;
        snd_Address[buf] = mod_Address[output_MOD[OnPort]];
        snd_Buf[buf][0] = MyAddress[toBus];
        snd_Buf[buf][1] = 0x03;  // Commando
        snd_Buf[buf][2] = 0x03;  // Lengte
        snd_Buf[buf][3] = 0;     // Lokaal op bestemming
        snd_Buf[buf][4] = toPort;
        snd_Buf[buf][5] = xAction;
        snd_Count[buf]= 0;
        SendBus(buf);
      }
    }
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
          sprintf(sLine,"Input down start op:%i.\r\n", port); Serial.print(sLine);
        #endif
      }
      else fDelta = millis() - PortStart[port];     

      if ((PortState[port] == LOW) && (PortPrevStat[port] == LOW) && (Action[port][5] > 0)) {  // Cont pressing
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
          //   printf("Lang in:%i.\r\n", port);
          // #endif
        } 
      }
      if (PortState[port] == HIGH && PortPrevStat [port] == LOW){  // End pressing
        PortStateStart[port] = millis();
        if ((millis() - PortStart[port]) < LongClick) {
          if (Action[port][2] > 0) {
            DoAction(Action[port][0], Action[port][1], Action[port][2]);  // Actie 1  queue naar master: opdracht of status
            sprintf(cItem,"PI:%02i:%02i:%02i",Action[port][0], Action[port][1], Action[port][2]); CLog(cItem);
          }
        }
        else if (Action[port][5] & B10000000) Action[port][5] = Action[port][5] ^ B01000000;      //Toggle Up/down bij einde longpress
      }
      if (PortPrevStat[port] != PortState[port]) {
        PortPrevStat[port] = PortState[port];
        if (LocInp2Sens[port] > 0) {
          byte fVal = 0;
          if (PortState[port] == HIGH) fVal = 255;
          byte fSensor = LocInp2Sens[port];
          #ifdef DO_DEBUG
            sprintf(sLine,"Zet Sensor %i op  = %i .\r\n", fSensor ,fVal); Serial.print(sLine);
          #endif            
          Value[((2+MyRange)<<8) + fSensor] = fVal;
          if (sensor_Logic[fSensor] != 0) {
            for (byte x = 0; x < arr_len[sensor_Logic[fSensor]]; x++) Dologic(array[arr_ptr[sensor_Logic[fSensor]] + x]);
          }
        }
      }
    }
  }
}

