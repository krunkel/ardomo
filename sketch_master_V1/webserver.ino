// #################################################################
// ###### checkWebServer ###########################################
// #################################################################
void checkWebServer() {
  EthernetClient client = webserver.available();
  if(client) {
    DebugOn(1);
    boolean emptyLine = true;
    boolean firstLine = true;
    int tCount = 0;
    char tBuf[65];
    int r,t;
    char *pch;
    char methodBuffer[8];
    char requestBuffer[48];
    char pageBuffer[48];
    char paramBuffer[48];
    char protocolBuffer[9];
    char fileName[32];
    char fileType[4];
    int clientCount = 0;
    int loopCount = 0;
    while (client.connected()) {
      while(client.available()) {
        char c = client.read();
        if(firstLine && tCount < 63)  {  // onthoud eerste 63 pos van eerste lijn
          tBuf[tCount] = c;
          tCount++;
          tBuf[tCount] = 0;
        }
        // TODO stop eerste X pos van volgende regels in buffer om cookies te checken....  
        if (c == '\n' && emptyLine) {  // na lege regel, ga verder
          while(client.available()) client.read();
          int scanCount = sscanf(tBuf,"%7s %47s %8s",methodBuffer,requestBuffer,protocolBuffer);
          // GVK:Request-Line   = Method Request-URI HTTP-Version
          if(scanCount != 3) {
            #ifdef DO_DEBUG
              Serial.println(F("bad request"));
            #endif
            sendBadRequest(client);
            DebugOff(1);
            return;
          } // END if(scanCount != 3) 
          char* pch = strtok(requestBuffer,"?");
          if(pch != NULL) {
            strncpy(fileName,pch,31);
            strncpy(tBuf,pch,31);
            pch = strtok(NULL,"?");
            if(pch != NULL) {
              strcpy(paramBuffer,pch);
            }            
            else paramBuffer[0] = 0;            
          } // END if(pch != NULL)
          strtoupper(requestBuffer);
          strtoupper(tBuf);
          for(int x = 0; x < strlen(requestBuffer); x++) {
            if(strchr(legalChars,requestBuffer[x]) == NULL) {
              sendBadRequest(client);
              DebugOff(1);
              return;
            }            
          }
          //#ifdef DO_DEBUG
          //  sprintf(sLine, "file = %s.",requestBuffer); Serial.print(sLine);
          //#endif
          pch = strtok(tBuf,".");
          if (pch != NULL) {
            pch = strtok(NULL,".");
            if(pch != NULL) strncpy(fileType,pch,4);
            else fileType[0] = 0;
              //#ifdef DO_DEBUG
              //  printf("file type = %s", fileType);
              //#endif
          } //END if (pch != NULL)
          // #ifdef DO_DEBUG
          //    printf("method = %s", methodBuffer);
          // #endif
          if(strcmp(methodBuffer,"GET") != 0 && strcmp(methodBuffer,"HEAD") != 0) {
            sendBadRequest(client);
            DebugOff(1);
            return;
          }
          // #ifdef DO_DEBUG
          //   Serial.print(F("params = "));
          //   Serial.println(paramBuffer);
          //   Serial.print(F("protocol = "));
          //   Serial.println(protocolBuffer);
          // #endif          
          // if(strcmp(requestBuffer,"/MYTEST.PHP") == 0) {
          if (strcmp(fileType,"XML") == 0) {
            // =================================================================================
            // ========= RECEIVE & EXECUTE ACTION FROM WEBCLIENT ===============================
            // =================================================================================
            if (strcmp(requestBuffer,"/SETOUTP.XML") == 0) {
              byte fID;
              byte fACT;
              sscanf(paramBuffer, "%hi&%hi", &fID, &fACT);
              // TODO CHECK COMMAND
              Queue('H',1,1,fID, fACT,0,0,0,1);   
              #ifdef DO_DEBUG
                 sprintf(sLine, "Web aktie op poort %i Value:%i.\r\n", fID, fACT); Serial.print(sLine);
              #endif
              client.print(F("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n"));
              client.print(F("Content-Type: text/xml\r\n\r\n<respons>OK</respons>\r\n"));
              client.stop();
              DebugOff(1);
              return;
            } 
            // =================================================================================
            // ========= RECEIVE GETPORTS REQUEST & SEND XML AS ANSWER =========================
            // =================================================================================
            else if (strcmp(requestBuffer,"/GETPORTS.XML") == 0) {
              byte fLevel = (byte)atoi(paramBuffer);
              client.print(F("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n"));
              client.print(F("Content-Type: text/xml\r\n\r\n<ports>\r\n"));
              for (int i=0;i<num_output;i++) {
                if (output_Z[i] == fLevel) {
                  sprintf((tBuf), "<port id='%i'>\r\n<xx>%i</xx>\r\n<yy>%i</yy>\r\n",output_ID[i],output_X[i],output_Y[i]);
                  client.write(tBuf);
                  sprintf((tBuf), "<mod>%i</mod>\r\n<prt>%i</prt>\r\n<type>%i</type>\r\n",output_MOD[i],output_PRT[i],output_TYP[i]);
                  client.write(tBuf);
                  sprintf((tBuf), "<level>%i</level><graf>%i</graf><value>%i</value></port>\r\n",output_Z[i],output_GRAF[i],Value[(MyRange<<8) + i]);
                  client.write(tBuf);
                  //yield();
                }
              }
              client.print(F("</ports>\r\n"));
              client.stop();
              DebugOff(1);
              return;
            }
          }  // END if (strcmp(fileType,"XML") == 0)
          else if (strcmp(fileName,"/settcookie.htm") == 0) {
            client.print(F("HTTP/1.1 200 OK\r\n"));
            client.print(F("Content-Type: text/html\r\n"));
            client.print(F("Set-Cookie: client=ok; Max-Age=360000\r\n\r\n"));
            client.print(F("<html><head><title>-</title>"));
            client.print(F("<body>Cookie?</body></html>\r\n"));
            client.stop();  
            DebugOff(1);
            return;
          }
          else if (strcmp(fileName,"/status.htm") == 0) {
            // ###################################################
            // ################# STUUR Status ####################
            // ###################################################
            client.print(F("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n"));
            client.print(F("Content-Type: text/html\r\n\r\n"));
            client.print(F("<html><head><title>Status arDomo Master</title>"));
            client.print(F("<link rel='stylesheet' href='status.css'></head><body>\r\n"));
            tim_Reeks[5] = year()-2000;
            tim_Reeks[4] = month();
            tim_Reeks[3] = day();
            tim_Reeks[2] = hour();
            tim_Reeks[1] = minute();
            tim_Reeks[0] = second();
            myLord.DST(tim_Reeks); // Adjust for DST for 7-segment display 
            client.print(F("<table><caption>Systeem:</caption>\r\n"));
            client.print(F("<tr><th>ID</th><th>Waarde</th></tr>"));
            sprintf(tBuf, "<tr><td>Tijd</td><td>%02i:%02i:%02i - %02i/%02i/%04i</td></tr>",tim_Reeks[2],tim_Reeks[1],tim_Reeks[0],tim_Reeks[3],tim_Reeks[4],2000+tim_Reeks[5]); client.print(tBuf);
            byte uu = tim_Sunrise / 3600;
            byte mm = (tim_Sunrise % 3600) / 60;
            byte ss = (tim_Sunrise % 3600) % 60;
            sprintf(tBuf, "<tr><td>Zon op</td><td>%02i:%02i:%02i</td></tr>",uu,mm,ss); client.print(tBuf);
            uu = tim_Sunset / 3600;
            mm = (tim_Sunset % 3600) / 60;
            ss = (tim_Sunset % 3600) % 60;
            sprintf(tBuf, "<tr><td>Zon onder</td><td>%02i:%02i:%02i</td></tr>",uu,mm,ss); client.print(tBuf);
            sprintf(tBuf, "<tr><td>Que  Next</td><td>%i Proc: %i.</td></tr>",QueNext, QueProc); client.print(tBuf);
            sprintf(tBuf, "<tr><td>QueL Next</td><td>%i Proc: %i.</td></tr>",QueLNext, QueLProc); client.print(tBuf);
            sprintf(tBuf, "<tr><td>Snd  Next</td><td>%i Proc: %i.</td></tr>",snd_Index, snd_Procs); client.print(tBuf);
            sprintf(tBuf, "<tr><td>Rcv  Next</td><td>%i Proc: %i.</td></tr>",rcv_Index, rcv_Procs); client.print(tBuf);
            client.print(F("</table>\r\n"));
            client.print(F("<table><caption>Modules:</caption>\r\n"));
            client.print(F("<tr><th>ID</th><th>Bus</th><th>Addr</th><th>#Prt</th><th>#Out</th>"));
            client.print(F("<th>Range</th><th>Model</th><th>Versie</th><th>Ok</th><th>UU:MM      </th></tr>\r\n"));
            for (byte x=1;x<num_mod;x++) {
              sprintf(tBuf, "<tr><td>%i</td><td>0x%02X</td><td>0x%02X</td>",mod_ID[x],mod_BUS[x],mod_Address[x]); client.print(tBuf);
                           // 123456891123456892123456893123456894123456895123456896
              sprintf(tBuf, "<td>%i</td><td>%i</td><td>%i</td>",mod_NumIN[x],mod_NumOUT[x],mod_Range[x]); client.print(tBuf);
              sprintf(tBuf, "<td>%s</td><td>%s</td>",mod_Model[x],mod_Version[x]); client.print(tBuf);
              sprintf(tBuf, "<td>%i</td><td>%02i:%02i</td></tr>\r\n",mod_Ok[x],mod_LastCont[x][0],mod_LastCont[x][1]); client.print(tBuf);
            }
            client.print(F("</table>\r\n"));
            client.print(F("<table><caption>Outputs:</caption>\r\n"));
            client.print(F("<tr><th>ID</th><th>Type</th><th>Module</th><th>Poort</th><th>Logic</th>"));
            client.print(F("<th>Graf</th><th>X</th><th>Y</th><th>Level</th><th>Value</th></tr>\r\n"));
            for (byte x=1;x<num_output;x++) {
              sprintf(tBuf, "<tr><td>%i</td><td>%i</td><td>%i</td><td>%i</td>",output_ID[x],output_TYP[x],output_MOD[x],output_PRT[x]); client.print(tBuf);
                           // 123456891123456892123456893123456894123456895123456896
              sprintf(tBuf, "<td>%i</td><td>%i</td><td>%i</td><td>%i</td>",output_Logic[x],output_GRAF[x],output_X[x],output_Y[x]); client.print(tBuf);
              sprintf(tBuf, "<td>%i</td><td>%i</td></tr>\r\n",output_Z[x],Value[(MyRange<<8) + x]); client.print(tBuf);
            }
            client.print(F("</table>\r\n"));
            client.print(F("<table><caption>Timers:</caption>\r\n"));
            client.print(F("<tr><th>ID</th><th>A1</th><th>A2</th><th>A3</th><th>Today</th>"));
            client.print(F("<th>Done</th><th>Sectoday</th></tr>\r\n"));
            for (byte x=0;x<num_tim;x++) {
              sprintf(tBuf, "<tr><td>%i</td><td>%i</td><td>%i</td><td>%i</td>",x,tim_Act1[x],tim_Act2[x],tim_Act3[x]); client.print(tBuf);
                           // 123456891123456892123456893123456894123456895123456896
              sprintf(tBuf, "<td>%i</td><td>%i</td><td>%i</td></tr>",tim_Today[x],tim_Done[x],tim_SecToday[x]); client.print(tBuf);
            }
            client.print(F("</table>\r\n</body></html>\r\n"));
            client.stop();  
            DebugOff(1);
            return;
          }
          else {
            // ###################################################
            // STUUR SD CONTENT EN STOP ##########################
            // ###################################################
            // if(strcmp(fileName,"/") == 0) {
            //   strcpy(fileName,"/INDEX.HTM");
            //   strcpy(fileType,"HTM");
            //  }
            if(strlen(fileName) > 30 || strlen(fileName) < 4 ) {
              sendBadRequest(client);
              DebugOff(1);
              return;
            }
            else if(strlen(fileType) > 3 || strlen(fileType) < 1) {
              sendBadRequest(client);
              DebugOff(1);
              return;
            }
            else {
              #ifdef DO_DEBUG
                 sprintf(sLine, "Open %s.",fileName); Serial.print(sLine);
              #endif
              if (MySDfile.open(fileName, O_READ)) {
                // day=86400/week=604800
                client.print(F("HTTP/1.0 200 OK\r\nCache-Control:public,max-age=86400\r\n"));
                strcpy_P(tBuf,PSTR("Content-Type: "));
                if(strcmp(fileType,"HTM") == 0) strcat_P(tBuf,PSTR("text/html"));
                else if(strcmp(fileType,"PNG") == 0) strcat_P(tBuf,PSTR("image/png"));
                else if(strcmp(fileType,"JPG") == 0) strcat_P(tBuf,PSTR("image/jpeg"));
                else if(strcmp(fileType,"PHP") == 0) strcat_P(tBuf,PSTR("text/html"));
                else if(strcmp(fileType,"CSS") == 0) strcat_P(tBuf,PSTR("text/css"));
                else if(strcmp(fileType,"GIF") == 0) strcat_P(tBuf,PSTR("image/gif"));
                else if(strcmp(fileType,"JS") == 0) strcat_P(tBuf,PSTR("application/javascript"));
                else if(strcmp(fileType,"ICO") == 0) strcat_P(tBuf,PSTR("image/x-icon"));
                else if(strcmp(fileType,"PDF") == 0) strcat_P(tBuf,PSTR("application/pdf"));
                else if(strcmp(fileType,"ZIP") == 0) strcat_P(tBuf,PSTR("application/zip"));
                else strcat_P(tBuf,PSTR("text/plain"));
                strcat_P(tBuf,PSTR("\r\nConnection: close\r\n\r\n"));
                client.write(tBuf);

                if(strcmp(methodBuffer,"GET") == 0)  {
                  //#ifdef DO_DEBUG
                  //  Serial.print(F("send.."));
                  //#endif
                  // tBuf[60] = 0;
                  clientCount = 64;
                  while(clientCount == 64) {
                    clientCount = MySDfile.read(tBuf, 64);
                    //#ifdef DO_DEBUG
                    //  Serial.print(tBuf);
                    //#endif
                    client.write((byte*)tBuf,clientCount);
                  }
                }
                MySDfile.close();              
                SenderIP = client.getRemoteIP();
                client.stop();
                // LOG =====
                sprintf(lLine,"HTTP request: %s from %i.%i.%i.%i\0",fileName,SenderIP[0],SenderIP[1],SenderIP[2],SenderIP[3]); lLog(lLine);
                DebugOff(1);
                return;
              } //END IF FILE bestaat
              else {
                sendFileNotFound(client);
                DebugOff(1);
                return;
              }
            } //END IF Bestandsnaam OK
          } //END ELSE STUUR SD-File
          client.stop();
        } //END if (c == '\n' && emptyLine)
        else if (c == '\n') {  // 
          emptyLine = true;
          firstLine = false;
        } 
        else if (c != '\r') {
          emptyLine = false;
        }
        //yield();
      }
      loopCount++;
      // if 1000ms has passed since last packet //TODO 
      if(loopCount > 100) { //WEBUPDATE > 1000 naar 100
        // close connection
        client.stop();
        //#ifdef DO_DEBUG
        //   Serial.println("\r\nTimeout");
        //#endif
      }
      // delay 1ms for timeout timing
      //yield();
    }
    DebugOff(1);
  }
}

void sendFileNotFound(EthernetClient thisClient) {
  char tBuf[64];
  strcpy_P(tBuf,PSTR("HTTP/1.0 404 File Not Found\r\n"));
  thisClient.write(tBuf);
  strcpy_P(tBuf,PSTR("Content-Type: text/html\r\nConnection: close\r\n\r\n"));
  thisClient.write(tBuf);
  strcpy_P(tBuf,PSTR("<html><body><H1>FILE NOT FOUND</H1></body></html>"));
  thisClient.write(tBuf);
  thisClient.stop();  
//  #ifdef DO_DEBUG
//    Serial.println(F("disconnected"));
//  #endif
}

void sendBadRequest(EthernetClient thisClient) {
  char tBuf[64];
  strcpy_P(tBuf,PSTR("HTTP/1.0 400 Bad Request\r\n"));
  thisClient.write(tBuf);
  strcpy_P(tBuf,PSTR("Content-Type: text/html\r\nConnection: close\r\n\r\n"));
  thisClient.write(tBuf);
  strcpy_P(tBuf,PSTR("<html><body><H1>BAD REQUEST</H1></body></html>"));
  thisClient.write(tBuf);
  thisClient.stop();  
//  #ifdef DO_DEBUG
//    printf("disconnected\r\n");
//  #endif
}

void  strtoupper(char* aBuf) {
  for(int x = 0; x<strlen(aBuf);x++) {
    aBuf[x] = toupper(aBuf[x]);
  }
}

byte socketStat[MAX_SOCK_NUM];
void checkSockStatus() {
  unsigned long thisTime = millis();
  for (int i = 0; i < MAX_SOCK_NUM; i++) {
    uint8_t s = W5100.readSnSR(i);
    if(s == 0x17) {
      if(socketStat[i] == 0x14) {
        connectTime[i] = thisTime;
      }
      else if(socketStat[i] == 0x17) {
        if(thisTime - connectTime[i] > 15000UL) {  // was 30000UL
          close(i);
        }
      }
    }
    else connectTime[i] = thisTime;
    socketStat[i] = W5100.readSnSR(i);
  }
}

void SendGoogle(char* Message) {
// https://docs.google.com/forms/d/1NH2TwnL3zNI_SOZ6OjbWVWQDsc-Xwsj_R5lyuYswZdw/viewform
// post entry.554888789:Test3
}

// #################################################################
// ###### lLog #####################################################
// #################################################################
void lLog(char* lMessage) {
  char lPref[24];  //YYYY/MM/DD UU:MM:SS - 0
  char logFile[20];
  if (!logOk) {
    time_t t = now() - 4000;  
    sprintf(logFile, "/log/%04i%02i%02i.txt",year(t),month(t),day(t));
    MySD.rename("/log/log.txt", logFile); // als niet ok, blijf log.txt gebruiken.
    logOk = true;
  } 
  File MyLogfile;
  MyLogfile = MySD.open("/log/log.txt", O_WRONLY | O_APPEND | O_CREAT);
  sprintf(lPref, "20%02i/%02i/%02i %02i:%02i:%02i - ",tim_Reeks[5],tim_Reeks[4],tim_Reeks[3],tim_Reeks[2],tim_Reeks[1],tim_Reeks[0]);
  MyLogfile.print(lPref);
  MyLogfile.println(lMessage);
  if (logFailed[0] > 0) {
    MyLogfile.print(lPref);
    MyLogfile.println(logFailed);
    logFailed[0] = 0;
    numFailed = 0;
  }  
  MyLogfile.close();
}

