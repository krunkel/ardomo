// ################################################################################
// ###### ProcessCommand ########################################### 16DEC14 ######
// ################################################################################
// Commando's Master > Client: 0x01: 0x01: Zet address 0x02: Verander prog 0x03: Schakel 0x04: Zet module parm 0x05: Poort-Output koppeling
// Commando's Client > Master: 0x41: Melden / 0x42: do actie / 0x43: zet value / 0x44 geef M(Model) V(Versie)
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
        if ((snd_Bus[y] == rcv_Bus[rcv_Procs]) && (snd_Address[y] == rcv_Buf[rcv_Procs][0]) && (snd_Buf[y][snd_CC[y]] == rcv_Buf[rcv_Procs][3])) {
          snd_Status[y] = 9;
          if (y == snd_Procs) {
            snd_Procs++;  snd_Procs &= MAX_SND_Mask;
          }
          x = fStart+fTodo;
        }
      }
    }
    else {
      // Prepare Answer 
      byte buf = snd_Index;
      snd_Index++;  snd_Index &= MAX_SND_Mask; 
      snd_Bus[buf] = rcv_Bus[rcv_Procs];
      snd_Address[buf] = rcv_Buf[rcv_Procs][0];
      snd_Buf[buf][0] = MyAddress[snd_Bus[buf]];
      snd_Buf[buf][1] = 0x80;  // Commando
      snd_Buf[buf][2] = 0x02;  // Lenght
      byte fAfzender = rcv_Buf[rcv_Procs][0];  // onthoud afzender...
      byte fDataLenght = rcv_Buf[rcv_Procs][2];
      snd_Buf[buf][3] = rcv_Buf[rcv_Procs][fDataLenght+3];  // CC
      byte fRC = 1; // RC
      byte fModID = 0;
      switch (rcv_Buf[rcv_Procs][1]) {
        case 0x41:         // aanmelding van  module : CC = 41 > meld module : AA4104DOAODIAICC
          #ifdef DO_DEBUG
            sprintf(sLine,"Bus Default = 0x%X, Afzender : 0x%X , Next = 0x%X.\r\n",fAfzender, BUS_DEFAULT[rcv_Bus[rcv_Procs]],BUS_Next[rcv_Bus[rcv_Procs]]); Serial.print(sLine);
          #endif
          if ((fAfzender == BUS_DEFAULT[rcv_Bus[rcv_Procs]] ) &&  (fDataLenght == 4)) {   // Module gebruikt default > stuur nieuw addres
            snd_Bus[snd_Index] = rcv_Bus[rcv_Procs];
            snd_Address[snd_Index] = fAfzender;
            snd_Buf[snd_Index][0] = MyAddress[rcv_Bus[rcv_Procs]];
            snd_Buf[snd_Index][1] = 0x01;  // Commando
            snd_Buf[snd_Index][2] = 0x01;  // Lengte
            snd_Buf[snd_Index][3] = BUS_Next[rcv_Bus[rcv_Procs]];
            snd_Count[snd_Index]= 0;
            snd_Status[snd_Index] = 2; // forceer resend via SendBusQueue
            snd_Index++;  snd_Index &= MAX_SND_Mask; 
            // SendBus(buf); // Zend via SendBusQueue()
            BUS_Next[rcv_Bus[rcv_Procs]]++;
            Queue('L',3,0,0,0,0,0,0,1);  // Write conf.txt to SD
          }
          else {
            for (byte x=1;x<=num_mod;x++) {
              if ((mod_BUS[x] == rcv_Bus[rcv_Procs]) && (mod_Address[x] == fAfzender)) {
                fModID = x;
                // LOG ====
                sprintf(lLine,"Com 41 van module %i.\0",fModID); lLog(lLine);
                mod_Ok[fModID] = true;
                mod_LastCont[fModID][0] = hour();
                mod_LastCont[fModID][1] = minute();
                Queue('L',2,fModID,0,0,0,0,0,1);   //Update Module
                x = num_mod;
              }
            }
          }
          fRC = 0;
          break;
        case 0x42:        // Module vraagt actie: AA4203RRPPAAC
          if (fDataLenght == 3) { 
            byte fRange = rcv_Buf[rcv_Procs][3]; 
            byte fOutput = rcv_Buf[rcv_Procs][4]; 
            byte fAction = rcv_Buf[rcv_Procs][5]; 
            #ifdef DO_DEBUG
              sprintf(sLine,"fOutput = %i fAction = %i .\r\n", fOutput ,fAction); Serial.print(sLine);
            #endif
            Queue('H',1,fRange, fOutput, fAction,0,0,0,1);
            sprintf(cItem,"FM:%02i:%02i:%02i",rcv_Bus[rcv_Procs],fAfzender,fOutput); CLog(cItem);
            fRC = 0x00;  //RC
          }
          break;
        case 0x43:         // Module geeft nieuwe waarde voor Output
          if (fDataLenght == 2) { 
            byte fOutput = rcv_Buf[rcv_Procs][3]; 
            byte fValue = rcv_Buf[rcv_Procs][4]; 
            // DigOutV[fOutput] = (byte)fValue;
            #ifdef DO_DEBUG
              sprintf(sLine,"Zet Value %i op  = %i .\r\n", fOutput ,fValue); Serial.print(sLine);
            #endif            
            Value[(MyRange<<8) + fOutput] = fValue;
            if (output_Logic[fOutput] != 0) {
              for (byte x = 0; x < arr_len[output_Logic[fOutput]]; x++) Dologic(array[arr_ptr[output_Logic[fOutput]] + x]);
            }
            fRC = 0; // RC
          }
          break;
        case 0x44:         // Module geeft geef M(Model) V(Versie)
          if (fDataLenght == 7) { 
            for (byte x=1;x<=num_mod;x++) {
              if ((mod_BUS[x] == rcv_Bus[rcv_Procs]) && (mod_Address[x] == fAfzender)) {
                if (rcv_Buf[rcv_Procs][3] == 'M') memcpy(&mod_Model[x], &rcv_Buf[rcv_Procs][4],6);
                if (rcv_Buf[rcv_Procs][3] == 'V') memcpy(&mod_Version[x], &rcv_Buf[rcv_Procs][4],6);
                x = num_mod;  //einde for
              }
            }
            fRC = 0; // RC
          }
          break;
        case 0x45:         // Module geeft nieuwe waarde voor Sensor
          if (fDataLenght == 2) { 
            byte fSensor = rcv_Buf[rcv_Procs][3]; 
            byte fValue = rcv_Buf[rcv_Procs][4]; 
            // DigOutV[fOutput] = (byte)fValue;
            #ifdef DO_DEBUG
              sprintf(sLine,"Zet Sensor %i op  = %i .\r\n", fSensor ,fValue); Serial.print(sLine);
            #endif            
            Value[((2+MyRange)<<8) + fSensor] = fValue;
            if (sensor_Logic[fSensor] != 0) {
              for (byte x = 0; x < arr_len[sensor_Logic[fSensor]]; x++) Dologic(array[arr_ptr[sensor_Logic[fSensor]] + x]);
            }
            fRC = 0; // RC
          }
          break;
      }  //END SWITCH
      #ifdef DO_DEBUG
        Serial.println("Command processed...");
      #endif
      snd_Buf[buf][4] = fRC;  // RC
      snd_Count[buf]= 0;
      SendBus(buf);
    }
    rcv_Procs++;
    rcv_Procs&=MAX_RCV_Mask;
  } 
}

// ################################################################################
// ###### CheckCommand ############################################# 16DEC14 ######
// ################################################################################
// Command = Afzender/Command/DatLen/D1/../DN/CC, return pos. CC, 0 = FOUT
//              0

int CheckCommand(byte fIndex) {
  byte myCC;
  byte fDataLenght = rcv_Buf[fIndex][2];
  if (fDataLenght > (MAX_CMD_LEN-4)) {
    #ifdef DO_DEBUG
      Serial.println("Datalenght Command te groot...");
    #endif
    return false;
  } else {
    myCC = rcv_Buf[fIndex][0] + rcv_Buf[fIndex][1] + fDataLenght;
    #ifdef DO_DEBUG
       sprintf(sLine,"Commando VAN  0x%X: 0x%X-0x%X-",rcv_Buf[fIndex][0],rcv_Buf[fIndex][1],fDataLenght); Serial.print(sLine);
    #endif
    byte fPos = 3;  
    while(fPos < fDataLenght + 3 )  {      
      #ifdef DO_DEBUG
         sprintf(sLine,"0x%X-",rcv_Buf[fIndex][fPos]); Serial.print(sLine);
      #endif
      myCC += rcv_Buf[fIndex][fPos];
      fPos++; 
    }
    //
    #ifdef DO_DEBUG
      sprintf(sLine, "0x%X\r\n", rcv_Buf[fIndex][fPos]); Serial.print(sLine);
    #endif
    if (myCC != rcv_Buf[fIndex][fPos]) return 0;
    return fPos;
  }  
}

