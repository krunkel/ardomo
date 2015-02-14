// #################################################################
// ###### Queue ####################################################
// #################################################################
// Zet in queue
void Queue(char qPriority, byte qType, byte q1, byte q2, byte q3, byte q4, byte q5, byte q6, byte qRepeat) {
   if (qPriority == 'H') {
     QueType[QueNext] = qType;
     QueParm[QueNext][1] = q1;
     QueParm[QueNext][2] = q2;
     QueParm[QueNext][3] = q3;
     QueParm[QueNext][4] = q4;
     QueParm[QueNext][5] = q5;
     QueParm[QueNext][6] = q6;
     QueIndex[QueNext] = qRepeat;
     QueNext++;
     QueNext&=Que_Mask;
   } else {
     QueLType[QueLNext] = qType;
     QueLRepeat[QueLNext] = qRepeat;
     QueLParm[QueLNext][1] = q1;
     QueLParm[QueLNext][2] = q2;
     QueLParm[QueLNext][3] = q3;
     QueLParm[QueLNext][4] = q4;
     QueLParm[QueLNext][5] = q5;
     QueLParm[QueLNext][6] = q6;
     QueLNext++;
     QueLNext&=Que_Mask;
   }
}
// #################################################################
// ###### ProcessQueue #############################################
// #################################################################
// Queue: 32 items 0-31: QueNext = Volgend leeg/ QueProc= Volgend uit te voeren 
void ProcessQueue() {
  // VerwerkQueue
  if ((QueNext == QueProc) && (snd_Procs == snd_Index) && (rcv_Index == rcv_Procs) && (QueLNext != QueLProc)) {
    Queue('H', QueLType[QueLProc],QueLParm[QueLProc][1],QueLParm[QueLProc][2],QueLParm[QueLProc][3],QueLParm[QueLProc][4],QueLParm[QueLProc][5],QueLParm[QueLProc][6],QueLRepeat[QueLProc]);
    QueLRepeat[QueLProc]--;
    if (QueLRepeat[QueLProc] == 0) { 
      QueLProc++;
      QueLProc&=Que_Mask;
    }
  }
  byte fModID;
  byte fBuf;
  byte fPtel;
  byte fSens;
  while ( QueNext != QueProc ) {
    switch (QueType[QueProc]) {
      case 1: 
        #ifdef DO_DEBUG
          sprintf(sLine,"queue Actie 1 - %i, %i, %i.\r\n",QueParm[QueProc][1],QueParm[QueProc][2],QueParm[QueProc][3]); Serial.print(sLine);
        #endif
        DoAction(QueParm[QueProc][1],QueParm[QueProc][2],QueParm[QueProc][3]);
        break;
      case 2:  // Zend module update (0x04) Module-ID, Range (Ox05) Poort > Output, (Ox06) Input > Sensor + forceer update programma
        fModID = QueParm[QueProc][1];
        snd_Bus[snd_Index] = mod_BUS[fModID];
        snd_Address[snd_Index] = mod_Address[fModID];
        snd_Buf[snd_Index][0] = MyAddress[mod_BUS[fModID]];
        snd_Buf[snd_Index][1] = 0x04;
        snd_Buf[snd_Index][2] = 0x02;
        snd_Buf[snd_Index][3] = fModID;
        snd_Buf[snd_Index][4] = mod_Range[fModID];
        snd_Count[snd_Index]= 0;
        snd_Status[snd_Index] = 2; // forceer resend via SendBusQueue
        snd_Index++;  snd_Index &= MAX_SND_Mask; 
        // Stuur Poort > Output koppeling
        snd_Bus[snd_Index] = mod_BUS[fModID];
        snd_Address[snd_Index] = mod_Address[fModID];
        snd_Buf[snd_Index][0] = MyAddress[mod_BUS[fModID]];
        snd_Buf[snd_Index][1] = 0x05;
        snd_Buf[snd_Index][2] = 0x08;
        fPtel = 0;
        for (byte x=1;x<=num_output;x++) {
          if (output_MOD[x] == fModID) {
            snd_Buf[snd_Index][3+fPtel*2] = output_PRT[x];
            snd_Buf[snd_Index][4+fPtel*2] = x;
            fPtel++;
            if (fPtel == 4) {
              snd_Count[snd_Index] = 0;
              snd_Status[snd_Index] = 2; // forceer resend via SendBusQueue
              snd_Index++;  snd_Index &= MAX_SND_Mask;
              // Volgende ...  
              snd_Bus[snd_Index] = mod_BUS[fModID];
              snd_Address[snd_Index] = mod_Address[fModID];
              snd_Buf[snd_Index][0] = MyAddress[mod_BUS[fModID]];
              snd_Buf[snd_Index][1] = 0x05;
              snd_Buf[snd_Index][2] = 0x08;
              fPtel = 0;
            }
          }  
        }  // end for
        if (fPtel > 0) {
          for (byte x=fPtel*2;x<7;x++) snd_Buf[snd_Index][4+x] = 0;
          snd_Count[snd_Index]= 0;
          snd_Status[snd_Index] = 2; // forceer resend via SendBusQueue
          snd_Index++;  snd_Index &= MAX_SND_Mask; 
        }
        // Stuur Input > sensor koppeling
        snd_Bus[snd_Index] = mod_BUS[fModID];
        snd_Address[snd_Index] = mod_Address[fModID];
        snd_Buf[snd_Index][0] = MyAddress[mod_BUS[fModID]];
        snd_Buf[snd_Index][1] = 0x06;
        snd_Buf[snd_Index][2] = 0x08;
        fPtel = 0;
        for (byte x=1;x<=num_sensor;x++) {
          if ((sensor_MOD[x] == fModID) && (sensor_ToMaster[x] > 0) ) {
            snd_Buf[snd_Index][3+fPtel*2] = sensor_PRT[x];
            snd_Buf[snd_Index][4+fPtel*2] = x;
            fPtel++;
            if (fPtel == 4) {
              snd_Count[snd_Index] = 0;
              snd_Status[snd_Index] = 2; // forceer resend via SendBusQueue
              snd_Index++;  snd_Index &= MAX_SND_Mask; 
              // Volgende ...  
              snd_Bus[snd_Index] = mod_BUS[fModID];
              snd_Address[snd_Index] = mod_Address[fModID];
              snd_Buf[snd_Index][0] = MyAddress[mod_BUS[fModID]];
              snd_Buf[snd_Index][1] = 0x06;
              snd_Buf[snd_Index][2] = 0x08;
              fPtel = 0;
            }
          }  
        }  // end for
        if (fPtel > 0) {
          for (byte x=fPtel*2;x<7;x++) snd_Buf[snd_Index][4+x] = 0;
          snd_Count[snd_Index]= 0;
          snd_Status[snd_Index] = 2; // forceer resend via SendBusQueue
          snd_Index++;  snd_Index &= MAX_SND_Mask; 
          }
        Queue('L',4,fModID,0,0,0,0,0,mod_NumIN[fModID]);   //UpdateProgrammaModule
        #ifdef DO_DEBUG
          sprintf(sLine,"Quetype = %i / Zend 0x04 en 0x05 naar Module\r\n",QueType[QueProc]); Serial.print(sLine);
        #endif
        break;
      case 3:   // Write new conf to SD
        char fLijn[80];
        MySD.remove("/mainprog/old/conf.2");
        MySD.rename("/mainprog/old/conf.1", "/mainprog/old/conf.2");
        MySD.rename("/mainprog/old/conf.txt", "/mainprog/old/conf.1");
        if (MySD.rename("/mainprog/conf.txt", "/mainprog/old/conf.txt")) {
          File cFile = MySD.open("/mainprog/conf.txt",  O_WRITE | O_CREAT);
          cFile.println("#conf.txt");
          sprintf(fLijn,"bus1modNext;%i",BUS_Next[1]); cFile.println(fLijn);
          sprintf(fLijn,"bus2modNext;%i",BUS_Next[2]); cFile.println(fLijn);
          cFile.println("#END#");
          cFile.close();
        }
        break;
      case 4:  // Zend programma update > module 
        fModID = QueParm[QueProc][1];
        fBuf = snd_Index;
        fSens=num_sensor;
        while ((fSens > 0) && ((sensor_MOD[fSens] != fModID) || (sensor_PRT[fSens] != QueIndex[QueProc]))) fSens--;
        if (fSens > 0) {
          snd_Index++;  snd_Index &= MAX_SND_Mask; 
          snd_Bus[fBuf] = mod_BUS[fModID];
          snd_Address[fBuf] = mod_Address[fModID];
          snd_Buf[fBuf][0] = MyAddress[mod_BUS[fModID]];
          snd_Buf[fBuf][1] = 0x02;  // Commando
          snd_Buf[fBuf][2] = 0x07;  // Lengte
          snd_Buf[fBuf][3] = sensor_PRT[fSens];
          for (byte x=0;x<6;x++) snd_Buf[fBuf][4+x]= sensor_Action[fSens][x];
          snd_Count[fBuf]= 0;
          SendBus(fBuf);
        }
        break;
      case 5:   // Pol module 
        fModID = QueIndex[QueProc];
        if (mod_BUS[fModID] > 0) {
          fBuf = snd_Index;
          snd_Index++;  snd_Index &= MAX_SND_Mask; 
          snd_Bus[fBuf] = mod_BUS[fModID];
          snd_Address[fBuf] = mod_Address[fModID];
          snd_Buf[fBuf][0] = MyAddress[mod_BUS[fModID]];
          snd_Buf[fBuf][1] = 0x01;  // Commando
          snd_Buf[fBuf][2] = 0x01;  // Lengte
          snd_Buf[fBuf][3] = mod_Address[fModID];
          snd_Count[fBuf]= 0;
          SendBus(fBuf);
        }
        break;
    } //END SWITCH
    QueProc++;
    QueProc&=Que_Mask;
    if (((QueNext - QueProc) & Que_Mask) < 20) return;  // 1 per keer tenzij queue > 20
    yield();
  }
}

// #################################################################
// ###### SendBusQueue #############################################
// #################################################################
void SendBusQueue() {
  if (snd_Procs != snd_Index) {
    byte fTodo = (snd_Index - snd_Procs) & MAX_SND_Mask;
    byte fStart = snd_Procs;
    for (byte x=fStart;x<(fStart+fTodo);x++) {
      byte y = x & MAX_SND_Mask;
      if ((snd_Buf[y][1] == 0x80) || (snd_Status[y] == 9) || (snd_Count[y] > snd_MaxCount))  { 
        if (snd_Count[y] > snd_MaxCount) {
          numFailed++;
          sprintf(logFailed,"Send Failed: #%i, Last: Bus:%i Addr:%i Comm:%i Len:%i",numFailed, snd_Bus[y],  snd_Address[y], snd_Buf[y][1], snd_Buf[y][2]);
        }
        snd_Status[y] = 9;
        if (snd_Procs == y) {
          snd_Procs++;  snd_Procs &= MAX_SND_Mask;
        }
      }
      else {
        bool bResend = false;
        if (snd_Status[y] == 2) bResend = true ; //Failed > sturen direct terug
        else if (snd_Status[y] == 1) {
          unsigned long timepassed = millis() - snd_Time[y];
          if (timepassed > Resend_TimeOut) bResend = true;
        }
        if (bResend) {
          SendBus(y);
          return;  // slechts 1 resend per loop;
        } 
      }
    }
  }
}

// #################################################################
// ###### SendBus ##################################################
// #################################################################
void SendBus(byte fBuf) {   //Buf = Afz/Comm/Len/X1..Xn)
  if (snd_Count[fBuf] == 0) {
    snd_Status[fBuf] = 0;
    byte fCheck = snd_Buf[fBuf][0] + snd_Buf[fBuf][1] + snd_Buf[fBuf][2];
    byte flength = snd_Buf[fBuf][2]+3;
    #ifdef DO_DEBUG
       sprintf(sLine,"Commando NAAR: 0x%X: 0x%X-0x%X-0x%X-",snd_Address[fBuf],snd_Buf[fBuf][0],snd_Buf[fBuf][1],snd_Buf[fBuf][2]); Serial.print(sLine);
    #endif
    for (byte x = 3; x <  flength ; x++) {
      fCheck += snd_Buf[fBuf][x];
      #ifdef DO_DEBUG
        sprintf(sLine, "-0x%X",snd_Buf[fBuf][x]); Serial.print(sLine);
      #endif
    }
    snd_Buf[fBuf][flength] = fCheck;
    snd_CC[fBuf] = flength;
    #ifdef DO_DEBUG
       sprintf(sLine, "-0x%X\r\n",fCheck); Serial.print(sLine);
    #endif
  }
  #ifdef DO_DEBUG
    sprintf(sLine, "Send buf(%i)(rep# %i) naar bus:0x%X, Address: 0x%X: 0x%X-0x%X-0x%X",fBuf,snd_Count[fBuf],snd_Bus[fBuf],snd_Address[fBuf], snd_Buf[fBuf][0], snd_Buf[fBuf][1],snd_Buf[fBuf][2]); Serial.print(sLine);
  #endif
  snd_Count[fBuf]++;
  snd_Time[fBuf] = millis();
  if (snd_Bus[fBuf] == 1) SendX2C(fBuf);
  else if (snd_Bus[fBuf] == 2) SendRF(fBuf);
}



