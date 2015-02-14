// #################################################################
// ###### ProcessCommand ###########################################
// #################################################################
void ProcessCommand() {
  if (rcv_Index == rcv_Procs) {  // Nikske te doen
     yield();
  }
  else {
    if (rcv_Buf[rcv_Procs][1] == 0x80) {
      byte fTodo = (snd_Index - snd_Procs) & MAX_SND_Mask;
      byte fStart = snd_Procs;
      for (byte x=fStart;x<(fStart+fTodo);x++) {
        byte y = x & MAX_SND_Mask;
        if ((snd_Address[y] == rcv_Buf[rcv_Procs][0]) && (snd_Buf[y][snd_CC[y]] == rcv_Buf[rcv_Procs][3])) {
          snd_Status[y] = 9;
          if (y == snd_Procs) {
            snd_Procs++;  snd_Procs&=MAX_SND_Mask;
          }
          x = fStart+fTodo;
        }
      }
    }
    else {
      // Prepare Answer 
      byte buf = snd_Index; 
      snd_Index++;  snd_Index &= MAX_SND_Mask; 
      // snd_Bus[buf] = rcv_Bus[rcv_Procs];
      snd_Address[buf] = rcv_Buf[rcv_Procs][0];  // Normaal altijd Master
      snd_Buf[buf][0] = MyAddress;
      snd_Buf[buf][1] = 0x80;  // Commando
      snd_Buf[buf][2] = 0x02;  // Lenght
      byte fAfzender = rcv_Buf[rcv_Procs][0];  // onthoud afzender...
      byte fDataLenght = rcv_Buf[rcv_Procs][2];
      snd_Buf[buf][3] = rcv_Buf[rcv_Procs][fDataLenght+3];  // CC
      byte fRC = 1; // RC
      #ifdef DO_DEBUG
        printf("Afz: 0x%02X, Command: 0x%02X, BUS_MASTER: 0x%02X, Lenght: 0x%02X.\n\r",fAfzender,rcv_Buf[rcv_Procs][1], BUS_MASTER,fDataLenght );
      #endif
      switch (rcv_Buf[rcv_Procs][1]) {
        case 0x01:         // Pas adres bus aan en reset bus
          if ((fAfzender == BUS_MASTER ) &&  (fDataLenght == 1)) { 
            if (MyAddress != rcv_Buf[rcv_Procs][3]) {
              EEPROM.write(1, 'A');
              EEPROM.write(2, rcv_Buf[rcv_Procs][3]);
              MyAddress = rcv_Buf[rcv_Procs][3];
              InitBUS();
            } else Queue('L',3,0,0,0,0,0,0);
            fRC = 0; // RC
          } 
          else fRC = 1; // RC
          break;
        case 0x02:        // Pas programma aan
          if ((fAfzender == BUS_MASTER ) &&  (fDataLenght == 7)) { 
            byte fPort = rcv_Buf[rcv_Procs][3]; 
            for (byte x = 0; x < 6 ; x++) {
              EEPROM.write((128+fPort*6+x),rcv_Buf[rcv_Procs][4+x]);
              Action[fPort][x] = rcv_Buf[rcv_Procs][4+x];
            }
            fRC = 0; // RC
          }
          else fRC = 1; // RC
          break;
        case 0x03:        // Schakel  Data = PPAA  PP=Poort AA=Actie  Return:PPVV  ENKEL MASTER???
          if ((fAfzender == BUS_MASTER ) &&  (fDataLenght == 3)) { 
            byte fRange = rcv_Buf[rcv_Procs][3]; 
            byte fPort  = rcv_Buf[rcv_Procs][4]; 
            Queue('H',1, fRange, fPort, rcv_Buf[rcv_Procs][5],0,0,0);
            fRC = 0; // RC
          }
          else fRC = 1; // RC
          break;
        case 0x04:        // Ontvang Module ID & Range van Master 
          if ((fAfzender == BUS_MASTER ) &&  (fDataLenght == 2)) { 
            MyModID = rcv_Buf[rcv_Procs][3]; 
            MyRange = rcv_Buf[rcv_Procs][4]; 
            fRC = 0; // RC
          }
          else fRC = 1; // RC
          break;
        case 0x05:        // Ontvang LocPrt2Out[LOC_PORT] van Master 
          if ((fAfzender == BUS_MASTER ) &&  (fDataLenght == 8)) { 
            LocPrt2Out[rcv_Buf[rcv_Procs][3]] = rcv_Buf[rcv_Procs][4]; 
            LocPrt2Out[rcv_Buf[rcv_Procs][5]] = rcv_Buf[rcv_Procs][6]; 
            LocPrt2Out[rcv_Buf[rcv_Procs][7]] = rcv_Buf[rcv_Procs][8]; 
            LocPrt2Out[rcv_Buf[rcv_Procs][9]] = rcv_Buf[rcv_Procs][10]; 
            fRC = 0; // RC
          }
          else fRC = 1; // RC
          break;
        case 0x06:        // Ontvang LocInp2Sens[LOC_DIGIN] van Master 
          if ((fAfzender == BUS_MASTER ) &&  (fDataLenght == 8)) { 
            LocInp2Sens[rcv_Buf[rcv_Procs][3]] = rcv_Buf[rcv_Procs][4]; 
            LocInp2Sens[rcv_Buf[rcv_Procs][5]] = rcv_Buf[rcv_Procs][6]; 
            LocInp2Sens[rcv_Buf[rcv_Procs][7]] = rcv_Buf[rcv_Procs][8]; 
            LocInp2Sens[rcv_Buf[rcv_Procs][9]] = rcv_Buf[rcv_Procs][10]; 
            fRC = 0; // RC
          }
          else fRC = 1; // RC
          break;
      }  //END SWITCH
      #ifdef DO_DEBUG
        printf("Command processed...\n\r");
      #endif
      snd_Buf[buf][4] = fRC;  // RC
      snd_Count[buf]= 0;
      SendBus(buf);
    }
    rcv_Procs++;
    rcv_Procs&=MAX_RCV_Mask;
  } 
}

// #################################################################
// ###### CheckCommand #############################################
// #################################################################
// Command = Bus/Afzender/Command/DatLen/D1/../DN/CC, return pos. CC, 0 = FOUT

int CheckCommand(byte fIndex) {
  byte myCC;
  byte fDataLenght = rcv_Buf[fIndex][2];
  //#ifdef DO_DEBUG
  //  printf("Check command  van bus %i.\n\r",rcv_Bus[fIndex]);
  //#endif
  if (fDataLenght > (MAX_CMD_LEN-4)) {  
    #ifdef DO_DEBUG
      printf("Datalenght Command te groot...\n\r");
    #endif
    return false;
  } else {
    myCC = rcv_Buf[fIndex][0] + rcv_Buf[fIndex][1] + fDataLenght;
    #ifdef DO_DEBUG
       printf("Command van 0x%X: 0x%X-0x%X-",rcv_Buf[fIndex][0],rcv_Buf[fIndex][1],fDataLenght);
    #endif
    byte fPos = 3;  
    while(fPos < fDataLenght + 3 )  {      
      #ifdef DO_DEBUG
         printf("0x%X-",rcv_Buf[fIndex][fPos]);
      #endif
      myCC += rcv_Buf[fIndex][fPos];
      fPos++; 
    }
    //
    #ifdef DO_DEBUG
       printf("0x%X\n\r", rcv_Buf[fIndex][fPos]);
    #endif
    if (myCC != rcv_Buf[fIndex][fPos]) return 0;
    return fPos;
  }  
}

