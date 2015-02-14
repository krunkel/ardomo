/*
	01 (AND)  #IN,#OUT,0,0,ActFalse,ActTrue,0xVA,0xLO
 	02 (OR)  #IN,#OUT,0,0,,ActFalse,ActTrue,0xVA,0xLO
 	03 (NAND)  #IN,#OUT,0,0,,ActFalse,ActTrue,0xVA,0xLO
 	04 (NOR)  #IN,#OUT,0,0,,ActFalse,ActTrue,0xVA,0xLO
 	05 (XOR)  #IN,#OUT,0,0,,ActFalse,ActTrue,0xVA,0xLO

	10 (HYST)  #IN,#OUT,TresholdDown,TresholdDown,INPUT???,ActLow,ActHigh,0xVA,0xLO
	11 (COUNT) #IN,#OUT,TresholdDown,TresholdDown,INPUT???,ActLow,ActHigh,0xVA,0xLO

	20 (DELAY)  #IN,#OUT,DelayOn,DelayOff,INPUT,ActLow,ActHigh,0xVA,0xLO  Delay = Short = max 32768 sec

*/
// uint16_t log_In[]  Ingang > Pointer naar int REEKS log_Port
// byte log_pType[log_Port] > type poort (1=AND/2=OR/3=...
// int log_pInp[log_Port] > Pointer naar int REEKS log_Port
// poort > poort
// poort > output
// input > value > poort > output
// ...   > value > poort > output

// byte log_pType[lNum]
// short log_pInp[lNum]
// short log_pOut[lNum]
// byte log_Act1Range[lNum]
// byte log_Act1Port[lNum]
// byte log_Act1Act[lNum]
// byte log_Act2Range[lNum]
// byte log_Act2Port[lNum]
// byte log_Act2Act[lNum]
// byte arr_len[MAX_ARRAY]
// short arr_ptr[MAX_ARRAY]
// short array[MAX_ARRAY*4] //?

void Dologic(short lNum) {
  if (lNum > 0) {
    byte lVal = 0;
    bool DoAct = false;
    //#ifdef DO_DEBUG
    //  sprintf(sLine, "Logic:%i Type:%i Input:%i Actie1:%i/%i. Values:", lNum, log_pType[lNum], log_pInp[lNum],log_Act1Port[lNum], log_Act1Act[lNum]); Serial.print(sLine);
    //  printArr(log_pInp[lNum]);
    //#endif
    switch (log_pType[lNum]) {
      case 1:  // And
        for (byte x = 0; x < arr_len[log_pInp[lNum]]; x++)  {
          if (Value[array[arr_ptr[log_pInp[lNum]] + x]] > 0) lVal++;
          //#ifdef DO_DEBUG
          //  sprintf(sLine, " %i (%i),", Value[array[arr_ptr[log_pInp[lNum]] + x]], array[arr_ptr[log_pInp[lNum]] + x]); Serial.print(sLine);
          //#endif
        }
        if (lVal == arr_len[log_pInp[lNum]]) {
          DoAct = true;
          lVal = true;
        }
        break;
      case 2:  // Or
        for (byte x = 0; x < arr_len[log_pInp[lNum]]; x++)  {
          if (Value[array[arr_ptr[log_pInp[lNum]] + x]] > 0) lVal++;
        }
        if (lVal > 1) {
          DoAct = true;
          lVal = true;
        }
        break;
      case 20:  // Delay A1 Delay A2
        byte x = 0;
        while ((delay_logic[x] != lNum) && (delay_act[x]) && (x < MAX_DELAY)) x++; 
        if (x < MAX_DELAY) {
          if (log_Val1[lNum] == 0) {          
            DoAction (log_Act1Range[lNum],log_Act1Port[lNum],log_Act1Act[lNum]);
          } else {
            delay_act[x] = true;
            delay_logic[x] = lNum;
            // delay_start[x] = millis();
            delay_go[x] = millis() + (log_Val1[lNum] * 1000);
            delay_ActRange[x] = log_Act1Range[lNum];
            delay_ActPort[x] = log_Act1Port[lNum];
            delay_ActAct[x] = log_Act1Act[lNum];
            x++;
          }
          while ((delay_logic[x] != lNum) && (delay_act[x]) && (x < MAX_DELAY)) x++; 
          if (x < MAX_DELAY) {
            delay_act[x] = true;
            delay_logic[x] = lNum;
            // delay_start[x] = millis();
            delay_go[x] = millis() + (log_Val2[lNum] * 1000);
            delay_ActRange[x] = log_Act2Range[lNum];
            delay_ActPort[x] = log_Act2Port[lNum];
            delay_ActAct[x] = log_Act2Act[lNum];
            x++;
            while (x < MAX_DELAY) {
              if (delay_logic[x] == lNum) {
                delay_act[x] = false;
                delay_logic[x] = 0;
              }
              x++;
            }
          }
        }
        DoAct = false;
        lVal = Value[(5 << 8) + lNum];
        break;
    }  // end switch
    if (DoAct && (log_Act1Act[lNum] > 0)) {
      DoAction(log_Act1Range[lNum], log_Act1Port[lNum], log_Act1Act[lNum]);
      #ifdef DO_DEBUG
        sprintf(sLine, "Aktie!= Logic:%i Range:%i Port:%i Actie:%i.\r\n", lNum, log_Act1Range[lNum], log_Act1Port[lNum], log_Act1Act[lNum]); Serial.print(sLine);
      #endif
    }
    if ((DoAct == false) && (log_Act2Act[lNum] > 0) && (log_pType[lNum] < 20)) {
      DoAction(log_Act2Range[lNum], log_Act2Port[lNum], log_Act2Act[lNum]);
      #ifdef DO_DEBUG
        sprintf(sLine, "Aktie (false)!= Logic:%i Range:%i Port:%i Actie:%i.\r\n", lNum, log_Act2Range[lNum], log_Act2Port[lNum], log_Act2Act[lNum]); Serial.print(sLine);
      #endif
    }
    if (lVal != Value[(5 << 8) + lNum]) {
      Value[(5 << 8) + lNum] = lVal;
      for (byte x = 0; x < arr_len[log_pOut[lNum]]; x++) {
        yield();
        if (array[arr_ptr[log_pOut[lNum]] + x] > 0) Dologic(array[arr_ptr[log_pOut[lNum]] + x]);
      }
    }
  }
}

