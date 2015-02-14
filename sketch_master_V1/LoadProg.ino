// ################################################################################
// ###### READ PROG FROM SD #######################################################
// ################################################################################
void LoadProg() {  
  File myFile;
  int tCount = 0;
  char lijn[256];
  char reeksA[30];
  char reeksB[30];
  byte len=0;
  #ifdef DO_DEBUG
    Serial.println("Start loading prog from SD...");
  #endif 
// =============================================================================
// ====== 0/conf.txt ===========================================================
// =============================================================================
  lijn[0]=0;
  myFile = MySD.open("/mainprog/conf.txt", O_READ);
  while ((len = myFile.fgets(lijn, sizeof(lijn))) > 0) {
    char fString[20]; 
    int fVal;
    if (lijn[0] != '#') {
      // 4;Toilet;1;1;4;1;100;52;1   = ID,Naam,Type,Module,Poort,Graf,X,Y,Level 
      sscanf(lijn, "%[^;];%i", &fString, &fVal);
      //#ifdef DO_DEBUG
      //  sprintf(sLine,"Lijn : %s / fString: %s / fVal: %i.\r\n", lijn, fString, fVal); Serial.print(sLine);
      //#endif
      if (strcmp(fString,"bus1modNext") == 0) BUS_Next[1] = (byte)fVal;
      else if (strcmp(fString,"bus2modNext") == 0) BUS_Next[2] = (byte)fVal; 
    } 
  }
  #ifdef DO_DEBUG
    sprintf(sLine,"Bus 2 Next = 0x%X.\r\n", BUS_Next[2]); Serial.print(sLine);
  #endif
  myFile.close();
// ================================================================================
// ====== 1/modules.csv ===========================================================
// ================================================================================
// #modules.csv
// #ID,Bus,Address,Input,Output,Model,Versie
  byte fTel=0; 
  lijn[0]=0;
  myFile = MySD.open("/mainprog/modules.csv", O_READ);
  while ((len = myFile.fgets(lijn, sizeof(lijn))) > 0) {
    if (lijn[0] != '#') { 
      sscanf(lijn, "%hi,%hi,%hi,%hi,%hi,%[^,],%[^,]", &mod_ID[fTel], &mod_BUS[fTel], &mod_Address[fTel], &mod_NumIN[fTel], &mod_NumOUT[fTel], &mod_Model[fTel], &mod_Version[fTel]);
      mod_Model[fTel][5] = 0;
      mod_Version[fTel][5] = 0;
      mod_Range[fTel] = 1;
      mod_Ok[fTel] = false;
      // mod_Model[fTel][0] = '-';mod_Model[fTel][1] = 0;
      // mod_Version[fTel][0] = '-';mod_Version[fTel][1] = 0;
      #ifdef DO_DEBUG
        sprintf(sLine,"Module(%i) ID: %i Address: %i.\r\n", fTel,mod_ID[fTel],mod_Address[fTel]); Serial.print(sLine);
      #endif
      fTel++;
      yield();
    }   
  }
  myFile.close();
  num_mod = (byte)fTel;
// ================================================================================
// ====== 2/outputs.csv ===========================================================
// ================================================================================
// #outputs.csv - Type  1=A/U - 2=Analoog/PWM,,,,,,,,,,
// #ID,Omschr,ToMaster,Type,Module,Poort,Graf,X,Y,Level,Logic
  fTel = 0; 
  lijn[0]=0;
  myFile = MySD.open("/mainprog/outputs.csv", O_READ);
  while ((len = myFile.fgets(lijn, sizeof(lijn))) > 0) {
    if (lijn[0] != '#') {
      sscanf(lijn, "%hi,%*[^,],%hi,%hi,%hi,%hi,%hi,%hi,%hi,%hi,%[^,]", &output_ID[fTel], &output_ToMaster[fTel], &output_TYP[fTel], &output_MOD[fTel], &output_PRT[fTel], &output_GRAF[fTel], &output_X[fTel], &output_Y[fTel], &output_Z[fTel], &reeksA);
      output_Logic[fTel] = PutInArr(reeksA);
      #ifdef DO_DEBUG
        sprintf(sLine,"Output(%i) ID: %i Module:%i Address: %i Level: %i Reeks: %s",fTel,output_ID[fTel],output_MOD[fTel],output_PRT[fTel],output_Z[fTel], reeksA); Serial.print(sLine);
        printArr(output_Logic[fTel]);
      #endif
      fTel++;
    } 
  }
  myFile.close();
  num_output = (byte)fTel; 
// ================================================================================
// ====== 3/sensors.csv ===========================================================
// ================================================================================
// #sensors.csv,,,,,,,,,,,
// #ID,Omschr,ToMaster,Module,Type,Poort,Graf,X,Y,Level,Logic,Actie
// 1,Alles uit,1,1,1,1,0,0,0,0,{},{254/1/1/0/0/0}
  fTel = 0; 
  lijn[0]=0;
  myFile = MySD.open("/mainprog/sensors.csv", O_READ);
  while ((len = myFile.fgets(lijn, sizeof(lijn))) > 0) {
    if (lijn[0] != '#') {
      sscanf(lijn, "%hi,%*[^,],%hi,%hi,%hi,%hi,%hi,%hi,%hi,%hi,%[^,],%[^,]", &sensor_ID[fTel], &sensor_ToMaster[fTel], &sensor_MOD[fTel], &sensor_TYP[fTel], &sensor_PRT[fTel], &sensor_GRAF[fTel], &sensor_X[fTel], &sensor_Y[fTel], &sensor_Z[fTel], &reeksA, &reeksB);
      sensor_Logic[fTel] = PutInArr(reeksA);
      PutInBytes(&sensor_Action[fTel][0],reeksB);
      #ifdef DO_DEBUG
        sprintf(sLine,"Sensor(%i) ID: %i Module:%i Address: %i Level: %i ReeksA: %s ReeksB: %s",fTel,sensor_ID[fTel],sensor_MOD[fTel],sensor_PRT[fTel],sensor_Z[fTel], reeksA, reeksB); Serial.print(sLine);
        printArr(sensor_Logic[fTel]);
        sprintf(sLine,"Sensor Actie (%i) [%i/%i/%i/%i/%i/%i].\r\n",fTel,sensor_Action[fTel][0],sensor_Action[fTel][1],sensor_Action[fTel][2],sensor_Action[fTel][3],sensor_Action[fTel][4],sensor_Action[fTel][5]); Serial.print(sLine);
      #endif
      if (sensor_MOD[fTel]==1) { //Master
        LocInp2Sens[sensor_PRT[fTel]] = fTel;
        for (byte x=0;x<6;x++) Action[sensor_PRT[fTel]][x] = sensor_Action[fTel][x];
      }
      fTel++;
    } 
  }
  myFile.close();
  num_sensor = (byte)fTel; 
// ================================================================================
// ====== 4/timers.csv ============================================================
// ================================================================================
// #timers.csv,,,,,,,,
// #YYYY,MM,DD,HH,Min,Sec,Act1,Act2,Act3
// # Elke dag zonsondergang+15 Buiten1 aan,,,,,,,,
// 1,129,33,224,15,0,254,9,1
  fTel = 0; 
  lijn[0]=0;
  myFile = MySD.open("/mainprog/timers.csv", O_READ);
  while ((len = myFile.fgets(lijn, sizeof(lijn))) > 0) {
    if (lijn[0] != '#') { 
      sscanf(lijn, "%i,%hi,%hi,%hi,%hi,%hi,%hi,%hi,%hi", &tim_Year[fTel],&tim_Month[fTel],&tim_Day[fTel],&tim_Hour[fTel],&tim_Min[fTel],&tim_Sec[fTel],&tim_Act1[fTel],&tim_Act2[fTel],&tim_Act3[fTel]);
      #ifdef DO_DEBUG
        sprintf(sLine,"Timer ID: %i Year: %i Action: 0x%hx-%hx-%hi.\r\n", fTel,tim_Year[fTel],tim_Act1[fTel],tim_Act2[fTel],tim_Act3[fTel]); Serial.print(sLine);
      #endif
      fTel++;
      yield();
    }   
  }
  num_tim = fTel;
// ================================================================================
// ====== 5/logic.csv =============================================================
// ================================================================================
// #logic.csv,#1=AND/2=OR/,,,,,,,,
// #ID,Type,Input,Output,Val1,Val2,ActRange,ActPort,ActAction,ActRange,ActPort,ActAction
// 1,1,"{17/18}",{},1,19,2,1,19,2
  fTel = 0; 
  lijn[0]=0;
  //reeksA[0] = 0;
  //reeksB[0] = 0;
  byte fTmp;
  myFile = MySD.open("/mainprog/logic.csv", O_READ);
  while ((len = myFile.fgets(lijn, sizeof(lijn))) > 0) {
    if (lijn[0] != '#') { 
      sscanf(lijn, "%hi,%hi,%[^,],%[^,],%i,%i,%hi,%hi,%hi,%hi,%hi,%hi", &fTmp,&log_pType[fTel],&reeksA,&reeksB,&log_Val1[fTel],&log_Val2[fTel],&log_Act1Range[fTel],&log_Act1Port[fTel],&log_Act1Act[fTel],&log_Act2Range[fTel],&log_Act2Port[fTel],&log_Act2Act[fTel]);
      log_pInp[fTel] = PutInArr(reeksA);
      log_pOut[fTel] = PutInArr(reeksB);
      #ifdef DO_DEBUG
        sprintf(sLine,"Logic ID: %i Type: %i InpArr: %i OutArr: %i Actie1: %i.\r\n", fTel,log_pType[fTel],log_pInp[fTel],log_pOut[fTel],log_Act1Act[fTel]); Serial.print(sLine);
        printArr(log_pInp[fTel]);
        printArr(log_pOut[fTel]);
      #endif
      fTel++;
      yield();
    }   
  }
  num_log = fTel;
// ================================================================================
// ====== 6/moods.csv =============================================================
// ================================================================================
// #moods.csv,,
// #ID,Omschr,
// 0,Dummy,{}
// 1,Alles uit,{254/3/1:254/5/1}

// byte num_moods;
// byte moo_Lenght[MAX_MOODS];
// short moo_Pointer[MAX_MOODS];
// byte moo_ElRange[MAX_MOODS_EL];
// byte moo_ElPort[MAX_MOODS_EL];
// byte moo_ElValue[MAX_MOODS_EL];
//short NextFreeMooElItem = 1;

  fTel = 0; 
  lijn[0]=0;
  //reeksA[0] = 0;
  //reeksB[0] = 0;
  //byte fTmp;
  myFile = MySD.open("/mainprog/moods.csv", O_READ);
  while ((len = myFile.fgets(lijn, sizeof(lijn))) > 0) {
    if (lijn[0] != '#') { 
      sscanf(lijn, "%hi,", &fTmp);
      byte fPos = 0;
      while (lijn[fPos] != '{') fPos++;
      fPos++;
      byte fNumTriplet = 0;
      byte fNumItem = 0;
      char* fItem;
      fItem = strtok(&lijn[fPos], "/:}");
      #ifdef DO_DEBUG
        sprintf(sLine,"Start: %s{", fItem); Serial.print(sLine);
      #endif
      while (fItem != NULL) {
        byte tVal = (byte)atoi(fItem);
        if (fNumItem == 0) moo_ElRange[NextFreeMooElItem] = tVal;
        else if (fNumItem == 1) moo_ElPort[NextFreeMooElItem] = tVal;
        else if (fNumItem == 2) moo_ElValue[NextFreeMooElItem] = tVal;
        #ifdef DO_DEBUG
          sprintf(sLine,"%i=%i.", fNumItem,tVal); Serial.print(sLine);
        #endif
        fNumItem++;
        if (fNumItem == 3) {
          fNumItem = 0;
          NextFreeMooElItem++;
          fNumTriplet++;
        }
        fItem = strtok(NULL, "/:}");
      }
      #ifdef DO_DEBUG
        sprintf(sLine,"}\r\n"); Serial.print(sLine);
      #endif
      moo_Lenght[fTmp] = fNumTriplet;
      if (fNumTriplet > 0) moo_Pointer[fTmp] = NextFreeMooElItem - fNumTriplet;
      else moo_Pointer[fTmp] = 0;
      #ifdef DO_DEBUG
        sprintf(sLine,"Mood ID: %i Lengte: %i Pointer: %i.\r\n", fTmp,moo_Lenght[fTmp],moo_Pointer[fTmp]); Serial.print(sLine);
      #endif
      fTel++;
      yield();
    }   
  }
  num_moods = fTmp + 1;
  #ifdef DO_DEBUG
    Serial.println(F("End loading prog from SD"));
  #endif
}

short PutInArr(char* aBuf) { // string "{n1/n2/n3/n4,..}" return ArrNumber
  short fArr = NextFreeArr;
  arr_ptr[fArr] = NextFreeArrItem;
  byte fNumItem = 0;
  char* fItem;
  if (aBuf[1] == '}') {
    return 0;
  } else {
    fItem = strtok(&aBuf[1], "/:}");
    while (fItem != NULL) {
      array[arr_ptr[fArr] + fNumItem] = (short)atoi(fItem);
      fNumItem++;
      fItem = strtok(NULL, "/:}");
    }
    if (fNumItem > 0) {
      arr_len[fArr]=fNumItem;
      NextFreeArr++;
      NextFreeArrItem += fNumItem;
      return fArr;
    }
    else return 0;
  }
}

void PutInBytes(byte fByte[6], char* aBuf) { // string "{n1;n2;n3;n4,..}" return ArrNumber
  byte fNumItem = 0;
  char* fItem;
  fItem = strtok(&aBuf[1], "/:}");
  //#ifdef DO_DEBUG
  //  sprintf(sLine,"Array %i (%s):", fArr,aBuf); Serial.print(sLine);
  //#endif
  while (fItem != NULL) {
    fByte[fNumItem] = (byte)atoi(fItem);
    fNumItem++;
    fItem = strtok(NULL, "/:}");
  }
  for (byte x=fNumItem;x<6;x++) fByte[x] = 0;
}

void printArr(short fArr) { 
  if (fArr > 0) {
    Serial.print("[");
    for (byte x=0;x < arr_len[fArr];x++) {
      sprintf(sLine,"%i,", array[arr_ptr[fArr] + x]); Serial.print(sLine);
    }
    Serial.print("]\r\n");
  }
}


