// #################################################################
// ###### Queue ####################################################
// #################################################################
// Zet in queue
void Queue(char qPriority, byte qType, byte q1, byte q2, byte q3, byte q4, byte q5, byte q6) {
   if (qPriority == 'H') {
     QueType[QueNext] = qType;
     QueParm[QueNext][1] = q1;
     QueParm[QueNext][2] = q2;
     QueParm[QueNext][3] = q3;
     QueParm[QueNext][4] = q4;
     QueParm[QueNext][5] = q5;
     QueParm[QueNext][6] = q6;
     QueNext++;
     QueNext&=Que_Mask;
   } else {
     QueLType[QueLNext] = qType;
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
  if ((QueNext == QueProc) && (snd_Procs == snd_Index) && (rcv_Index == rcv_Procs)) {
    if (NextReport < millis()) {
      Report2Master();
    } 
    if (QueLNext != QueLProc) {
      Queue('H', QueLType[QueLProc],QueLParm[QueLProc][1],QueLParm[QueLProc][2],QueLParm[QueLProc][3],QueLParm[QueLProc][4],QueLParm[QueLProc][5],QueLParm[QueLProc][6]);
      QueLProc++;
      QueLProc&=Que_Mask;
    }
  }
  while ( QueNext != QueProc ) {
    switch (QueType[QueProc]) {
      case 1: {   // DoAction
        printf("queue Actie 1 - %1, %2, %3./n/r",QueParm[QueProc][1],QueParm[QueProc][2],QueParm[QueProc][3]);
        DoAction(QueParm[QueProc][1],QueParm[QueProc][2],QueParm[QueProc][3]);
      }
      break;
      case 2: {   // Send Value port to Master
        if (LocPrt2Out[QueParm[QueProc][1]] > 0) {
          byte buf = snd_Index;
          snd_Index++; snd_Index &= MAX_SND_Mask; 
          //snd_Bus[buf] = toBus;
          snd_Address[buf] = BUS_MASTER;
          snd_Buf[buf][0] = MyAddress;
          snd_Buf[buf][1] = 0x43;
          snd_Buf[buf][2] = 0x02;
          snd_Buf[buf][3] = LocPrt2Out[QueParm[QueProc][1]];
          snd_Buf[buf][4] = PrtVal[QueParm[QueProc][1]];
          snd_Count[buf]= 0;
          SendBus(buf);
        }
        }
      break;
      case 3: {   // Send Status,....
        Report2Master();
      }
      break;
    }
    QueProc++;
    QueProc&=Que_Mask;
    if (((QueNext - QueProc) & Que_Mask) < 20) return;  // 1 per keer tenzij queue > 20
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
      if ((snd_Buf[y][1] == 0x80) || (snd_Status[y] == 9) || (snd_Count[y] > snd_MaxCount))  {  // Al ok of opgeven 
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
    snd_Status[fBuf] = 0;   // 1 = verzonden / 2 = resending / ??? Nodig
    byte fCheck = snd_Buf[fBuf][0] + snd_Buf[fBuf][1] + snd_Buf[fBuf][2];
    byte flength = snd_Buf[fBuf][2]+3;
    #ifdef DO_DEBUG
      printf("Send Address: %X: 0x%X-0x%X-0x%X",snd_Address[fBuf], snd_Buf[fBuf][0], snd_Buf[fBuf][1],snd_Buf[fBuf][2]);
    #endif
    for (byte x = 3; x <  flength ; x++) {
      fCheck += snd_Buf[fBuf][x];
      #ifdef DO_DEBUG
        printf("-0x%X",snd_Buf[fBuf][x]);
      #endif
    }
    snd_Buf[fBuf][flength] = fCheck;
    snd_CC[fBuf] = flength;
    #ifdef DO_DEBUG
      printf("-0x%X\n\r",fCheck);
    #endif
  }
  snd_Count[fBuf]++;
  snd_Time[fBuf] = millis();
  SendBusX(fBuf);
}

