 /*
 * © Francesco Potortì 2013 - GPLv3 - Revision: 1.13
 *
 * Send an NTP packet and wait for the response, return the Unix time
 *
 * To lower the memory footprint, no buffers are allocated for sending
 * and receiving the NTP packets.  Four bytes of memory are allocated
 * for transmision, the rest is random garbage collected from the data
 * memory segment, and the received packet is read one byte at a time.
 * The Unix time is returned, that is, seconds from 1970-01-01T00:00.
 */
unsigned long inline ntpUnixTime (UDP &udp) {
  static int udpInited = udp.begin(123); // open socket on arbitrary port
  const char timeServer[] = "pool.ntp.org";  // NTP server 
  // TODO vervangen door 0.europe.pool.ntp.org ??
  // Only the first four bytes of an outgoing NTP packet need to be set
  // appropriately, the rest can be whatever.
  const long ntpFirstFourBytes = 0xEC0600E3; // NTP request header
  // Fail if WiFiUdp.begin() could not init a socket
  if (! udpInited) return 0;
  udp.flush();
  // Send an NTP request
  if (! (udp.beginPacket(timeServer, 123) // 123 is the NTP port
	 && udp.write((byte *)&ntpFirstFourBytes, 48) == 48
	 && udp.endPacket()))   return 0;	// sending request failed
  // Wait for response; check every pollIntv ms up to maxPoll times
  const int pollIntv = 150;		// poll every this many ms
  const byte maxPoll = 15;		// poll up to this many times
  int pktLen;				// received packet length
  for (byte i=0; i<maxPoll; i++) {
    if ((pktLen = udp.parsePacket()) == 48) break;
    delay(pollIntv);
  }
  if (pktLen != 48)  return 0;	// no correct packet received
  // Read and discard the first useless bytes
  // Set useless to 32 for speed; set to 40 for accuracy.
  const byte useless = 40;
  for (byte i = 0; i < useless; ++i)
    udp.read();
  // Read the integer part of sending time
  unsigned long time = udp.read();	// NTP time
  for (byte i = 1; i < 4; i++)
    time = time << 8 | udp.read();
  // Round to the nearest second if we want accuracy
  // The fractionary part is the next byte divided by 256: if it is
  // greater than 500ms we round to the next second; we also account
  // for an assumed network delay of 50ms, and (0.5-0.05)*256=115;
  // additionally, we account for how much we delayed reading the packet
  // since its arrival, which we assume on average to be pollIntv/2.
  time += (udp.read() > 115 - pollIntv/8);

  // Discard the rest of the packet
  udp.flush();
  return time - 2208988800ul;		// convert NTP time to Unix time
}

// bool IsDst(int day, int month, int dow)
// {
//    if (month < 3 || month > 10)  return false; 
//    if (month > 3 && month < 10)  return true; 
//    int previousSunday = day - dow;
//    if (month == 3) return previousSunday >= 25;
//    if (month == 10) return previousSunday < 25;
//    return false; // this line never gonna happend
// }


void DoTijdSync(bool FromRTC) {
  //if (RestartEth) {
  //  for (int i = 0; i < MAX_SOCK_NUM; i++) close(i);
  // }
  unsigned long unixTime = 0;
  byte fyear, fmonth, fdate, fDoW, fhour, fminute, fsecond;
  if (FromRTC == false ) {
    EthernetUDP udp;
    delay(10);
    unixTime = ntpUnixTime(udp);
    udp.stop(); // ??
    setTime(unixTime);
    #ifdef DO_DEBUG
        sprintf(sLine,"Tijd van NTP: YY: %i MM: %i DD: %i UU: %i MM: %i.\r\n", year(),month(), day(), hour(), minute()); Serial.print(sLine);
    #endif
  }
  if (unixTime > 0) {
    int fTshift = (TIMEZONE * 3600);
    setTime(unixTime+fTshift); // TODO Zomer/Winter
    #ifdef DO_DEBUG
      sprintf(sLine,"Tijd NA SHIFT: YY: %i MM: %i DD: %i UU: %i MM: %i.\r\n", year(),month(), day(), hour(), minute()); Serial.print(sLine);
    #endif
    RTClock.setSecond(second());//Set the second 
    RTClock.setMinute(minute());//Set the minute 
    RTClock.setHour(hour());  //Set the hour 
    RTClock.setDoW(weekday());    //Set the day of the week
    RTClock.setDate(day());  //Set the date of the month
    RTClock.setMonth(month());  //Set the month of the year
    RTClock.setYear(byte (year()-2000));  //Set the year (Last two digits of the year)
    RTClock.getTime(fyear, fmonth, fdate, fDoW, fhour, fminute, fsecond); 
    bool x12, xPM;
    fhour = RTClock.getHour(x12, xPM);
    // #ifdef DO_DEBUG
    //   sprintf(sLine,"Tijd naar RTC: YY: %i MM: %i DD: %i UU: %i MM: %i.\r\n", year(),month(), day(), hour(), minute()); Serial.print(sLine);
    //   sprintf(sLine,"Tijd van RTC: YY: %i MM: %i DD: %i UU: %i MM: %i.\r\n", fyear,fmonth, fdate, fhour, fminute); Serial.print(sLine);
    // #endif
  } else {
    // byte fyear, fmonth, fdate, fDoW, fhour, fminute, fsecond;
    RTClock.getTime(fyear, fmonth, fdate, fDoW, fhour, fminute, fsecond); 
    bool x12, xPM;
    fhour = RTClock.getHour(x12, xPM);
    if (FromRTC) {
      if (((fyear + 2000) == year()) && (fmonth == month()) && (fdate == day()) && (fhour == hour())) {
        sprintf(lLine,"Tijd van RTC: %02i/%02i/%02i %02i:%02i:%02i", fdate,fmonth,fyear,fhour,fminute,fsecond); lLog(lLine);
        setTime(fhour, fminute, fsecond,fdate,fmonth,(fyear + 2000));
      } else {
          sprintf(lLine,"Foute tijd van RTC: %02i/%02i/%02i %02i:%02i:%02i", fdate,fmonth,fyear,fhour,fminute,fsecond); lLog(lLine);
      }
    } else {
      setTime(fhour, fminute, fsecond,fdate,fmonth,(fyear + 2000));
      #ifdef DO_DEBUG
         sprintf(sLine,"Tijd van RTC: %02i/%02i/%02i %02i:%02i:%02i.\r\n", day(),month(),year(),hour(), minute(), second()); Serial.print(sLine);
      #endif
    }
  }
  // if (RestartEth) {
  //   webserver.begin();
  //   unsigned long thisTime = millis();
  //   for(int i=0;i<MAX_SOCK_NUM;i++) connectTime[i] = thisTime;
  // }

}

void DispTime() {
  if ((millis() - LastTime) > 900) {
    LastTime = millis();
    tim_Reeks[5] = year()-2000;
    tim_Reeks[4] = month();
    tim_Reeks[3] = day();
    tim_Reeks[2] = hour();
    tim_Reeks[1] = minute();
    tim_Reeks[0] = second();
    myLord.DST(tim_Reeks); // Adjust for DST for 7-segment display 
    TimeDisp[0] = tim_Reeks[2] / 10;
    TimeDisp[1] = tim_Reeks[2] % 10;
    TimeDisp[2] = minute() / 10;
    TimeDisp[3] = minute() % 10;
    TimeFlash = ! TimeFlash;
    tm1637.point(TimeFlash);
    tm1637.display(TimeDisp);
  }
}
