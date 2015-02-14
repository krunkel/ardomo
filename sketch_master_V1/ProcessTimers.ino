// #################################################################
// ###### ProcessTimers ############################################
// #################################################################
// byte num_tim;
// byte tim_ID[MAX_TIMERS];
// int  tim_Year[MAX_TIMERS];
// byte tim_Month[MAX_TIMERS];
// byte tim_Day[MAX_TIMERS];
// byte tim_Hour[MAX_TIMERS];
// byte tim_Min[MAX_TIMERS];
// byte tim_Sec[MAX_TIMERS];
// char tim_Act[MAX_TIMERS][ACT_LENGHT];
// bool tim_Today[MAX_TIMERS];
/* iYYYYxMMxDDxUUxMMxSSxActie   Actie = Type/ID/Act
int(tim_Year),byte(tim_Month),byte(tim_Day),byte(tim_Hour),byte(tim_Min),byte(tim_Sec),string tim_Act
     YYYY < 2000 = elke x jaar (
     MM < 128   = Maand, > 128 elke x maand
     DD = 3 hoogste bits  000 Fix dag maand / 001 = elke x dagen - 001.00001 (33) = elke dag
                          010 = dag van de week - 0(64)=zond,6(70)=zat 
                          011 = .00001 (97)= weekend  .00010 (98) = weekdag    
     UU 3 hoogste bits: 000  Fix / OO1 elke x uur /  100  Sunrise -tijd / 101  Sunrise +tijd / 110  Sunset -tijd / 111  Sunset +tijd
     MM < 128   = Min, > 128 elke x minuten
     SS < 128   = Sec, > 128 elke x seconden
*/

void ProcessTimers() {
  // VerwerkTimers
  // int(tim_Year),byte(tim_Month),byte(tim_Day),byte(tim_Hour),byte(tim_Min),byte(tim_Sec),string tim_Act
  long fSecofDay = (hour() * 60 + minute()) * 60 + second();
  if (day() != TimerDay) { // Eerste ProcessTimers van de dag
    // =================================================================
    // ====== ProcessTimers == 1ste run van de dag =====================
    // =================================================================
    #ifdef DO_DEBUG
      Serial.println("Timer eerste run dag...");
    #endif
    logOk = false;
    // TODO Update 2b62b868-0015-4f85-84e7-b5907a6de673 https://www.duckdns.org/update?domains=ben&token=2b62b868-0015-4f85-84e7-b5907a6de673&ip=
    // TODO Correctie Zomer/Winter
    tim_Sun[3] = day();
    tim_Sun[4] = month();
    tim_Sun[5] = year()-2000;
    myLord.SunRise(tim_Sun);
    tim_Sunrise = tim_Sun[0] + tim_Sun[1]*60 + tim_Sun[2]*3600; // Seconden na middernacht
    myLord.SunSet(tim_Sun); 
    tim_Sunset = tim_Sun[0] + tim_Sun[1]*60 + tim_Sun[2]*3600;  // Seconden na middernacht
    // Bepaal welke timerrecords vandaag spelen en zet tim_Today[]
    // LOG =====
    sprintf(lLine,"Eerste run - Zon op: %02i:%02i Zon onder: %02i:%02i.\0",tim_Sunrise/3600,(tim_Sunrise%3600)/60,tim_Sunset/3600,(tim_Sunset%3600)/60); lLog(lLine);
    for(int i=0;i<num_tim;i++) {
      tim_Today[i] = false;
      tim_Done[i] = false;
      if ((tim_Year[i] == year()) || ((tim_Year[i] < 2000) && ((year() % tim_Year[i]) == 0))) {
        if ((tim_Month[i] == month()) || ((tim_Month[i] > 128) && ((month() % (tim_Month[i] & B01111111)) == 0))) {
          byte dType = tim_Day[i] >> 5;
          byte dDay = tim_Day[i] & B00011111;
          if (((dType == 0 ) && (day() == dDay)) || ((dType == 1 ) && ((day() % dDay) == 0))) tim_Today[i] = true;
          else if ((dType == 2 ) && (weekday() == dDay))  tim_Today[i] = true;
          else if ((dType == 3 ) && ((dDay == 1) && ((weekday() == 1) || (weekday() == 7)))) tim_Today[i] = true;
          else if ((dType == 3 ) && ((dDay == 2) && ((weekday() > 1) && (weekday() < 7)))) tim_Today[i] = true;
        }  
      }  
      #ifdef DO_DEBUG
        if (tim_Today[i]) { sprintf(sLine,"Timer %i voor vandaag...\r\n",i); Serial.print(sLine);}
      #endif
      if ((tim_Today[i]) && ((tim_Hour[i] >> 5) > 3)){ // bereken tim_SecToday[i] als sunrise/sunset timer
        byte hType = tim_Hour[i] >> 5;
        byte hHour = tim_Hour[i] & B00011111;
        long fDelta = (hHour * 60 + tim_Min[i]) * 60 + tim_Sec[i];
        switch (hType) {     //   100  Sunrise -tijd / 101  Sunrise +tijd / 110  Sunset -tijd / 111  Sunset +tijd
          case B100:  tim_SecToday[i] = tim_Sunrise - fDelta; break;
          case B101:  tim_SecToday[i] = tim_Sunrise + fDelta; break;
          case B110:  tim_SecToday[i] = tim_Sunset - fDelta; break;
          case B111:  tim_SecToday[i] = tim_Sunset + fDelta; break;
        }
      }
    }  
    TimerDay = day();   // Eerste run van de dag afgelopen.....
  }
  if ((snd_Procs == snd_Index) && (rcv_Index == rcv_Procs)) { 
    // =================================================================
    // ====== ProcessTimers == controleer alle timers===================
    // =================================================================
    for(int i=0;i<num_tim;i++) {
      if (tim_Today[i]) {    // enkel als relevant vandaag...
          // Verwerk deze timer
        byte hType = tim_Hour[i] >> 5;
        byte hHour = tim_Hour[i] & B00011111;
        if (((hType == 0 ) && (hour() == hHour)) || ((hType == 1 ) && ((hour() % hHour) == 0))) {     //gewoon uur
          if ((tim_Min[i] == minute()) || ((tim_Min[i] > 128) && ((minute() % (tim_Min[i] & B01111111)) == 0))) {
            if ((tim_Sec[i] == second()) || ((tim_Sec[i] > 128) && ((second() % (tim_Sec[i] & B01111111)) == 0))) {
               if (!tim_Done[i]) {
                   // LOG =====
                 sprintf(lLine,"Timer %i.\0",i); lLog(lLine);
                 #ifdef DO_DEBUG
                   sprintf(sLine,"Timer %i NU!!!\r\n",i); Serial.print(sLine);
                 #endif
                 DoAction (tim_Act1[i],tim_Act2[i],tim_Act3[i]);
                 tim_Done[i] = true;
               }        
            } else tim_Done[i] = false;
          } else tim_Done[i] = false;
        }  
        else {
          tim_Done[i] = false; 
          if ((tim_SecToday[i] > 0 ) && (fSecofDay >= tim_SecToday[i]) && ((fSecofDay-15) <  tim_SecToday[i])) {            
            DoAction (tim_Act1[i],tim_Act2[i],tim_Act3[i]);
            tim_Today[i] = false;  // slechts 1 keer per dag....
            sprintf(lLine,"Timer %i.\0",i); lLog(lLine);
          }
        }
      }
    }
  }
  // =================================================================
  // ====== ProcessTimers == Als geen ander werk =====================
  // =================================================================
  if ((QueNext == QueProc) && (snd_Procs == snd_Index) && (rcv_Index == rcv_Procs) && (QueLNext == QueLProc) && ((millis() - LastLogic) > 5000)) { 
    // ================================
    // ====== controleer DELAYS =======
    // ================================
    for(byte i=0;i<MAX_DELAY;i++) {
      if (delay_act[i]) {
        if (delay_go[i] < millis()) {
          delay_act[i] = false;
          delay_logic[i] = 0;
          DoAction (delay_ActRange[i],delay_ActPort[i],delay_ActAct[i]);
        }
      }
    }
    LastLogic = millis();
    if (log_pType[log2Pol] != 20) Dologic(log2Pol);
      // yield();
    log2Pol++;
    if (log2Pol > num_log)  log2Pol = 1;
    // =================================================================
    // ====== ProcessTimers == elke 10 minuten ========================
    // =================================================================
    byte fTen = minute() / 10; 
    if (fTen != TimerTen) {
      // === POL MODULES ===
      Queue('L',5,0,0,0,0,0,0,num_mod);  
      //mod2Pol++;
      //if (mod2Pol > num_mod) mod2Pol = 1;
      TimerTen = minute() / 10;
    }  
    // =================================================================
    // ====== ProcessTimers == elk uur =================================
    // =================================================================
    if ((minute() == 27) && (TimerHour != hour())) {
      DoTijdSync(true);
      TimerHour = hour();
    }  
  }  
}

