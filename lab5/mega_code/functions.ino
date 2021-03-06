#include "tcb.h"
#include "helpers.h"

bool dispBP = TRUE;
bool dispT = TRUE;
bool dispPR = TRUE;
bool dispRR = TRUE;
bool dispEKG = TRUE;

bool flashBP = FALSE;
bool flashT = FALSE;
bool flashPR = FALSE;

//bool flashBP = FALSE;
//bool flashT = TRUE;
//bool flashPR = TRUE;

bool refSelect = TRUE;
bool refMenu = TRUE;
bool refAnnu = TRUE;
bool refMeas = TRUE;
bool refDisp = TRUE;
bool ref = TRUE;
bool refCD = TRUE;
bool refG = TRUE;

bool genEKG = TRUE;

bool TSelected = FALSE;
bool BPSelected = FALSE;
bool PRSelected = FALSE;
bool RRSelected = FALSE;
bool EKGSelected = FALSE;
bool Disp = TRUE;
bool Disp2 = TRUE;

// By high we mean 15% out of range
bool bpHigh = FALSE;
bool tempHigh = FALSE;
bool prHigh = FALSE;
bool rrLow = FALSE;
bool rrHigh = FALSE;
bool EKGHigh = FALSE;

unsigned char bpOutOfRange = 0;
unsigned char tempOutOfRange = 0;
unsigned char pulseOutOfRange = 0;
unsigned char rrOutOfRange = 0;
unsigned char EKGOutOfRange = 0;



bool trIsReverse = FALSE, prIsReverse = FALSE, isEven = TRUE;



/*
*    @para: void* dataPtr, we assume it's MeasureData pointer; integer isEven, check if it's even;
*    isEven range: 0 or 1;
*    Increase temperatureRaw by 2 even number time, decrease by 1 odd times called before reach 50; 0 is even;
*    Then reverse the process until temperatureRaw falls below 15 
*    systolicPressRaw  - even: Increase by 3; - odd: decrease by 1; Range: no larger than 100
*    diastolicPressRaw -even: decrease by 2; - odd: increase by 1; Range: no less than 40
*    pulseRateRaw - even: decrease by 1; - odd: increase by 3; Range: 15-40
*    April, 22, 2019 by Kaiser
*/
void Measure(void* dataPtr)
{
    MeasureData md = *((MeasureData*) dataPtr);
    Serial1.write('s');
    if(dispT) { 
            Serial1.write('t');
            Serial1.write('p'); 
            if(Serial1.available()) {
                int newTemp = Serial1.read();
                if(moreThan15(newTemp, md.temperatureRawBuf)) {
                    shift(newTemp, 8, (md.temperatureRawBuf));
                }
            }
    }

    if(dispBP) { 
        Serial1.write('b'); 
        Serial1.write('u');
        if(Serial1.available()) {
            int newSys = Serial1.read();
            int newDia = Serial1.read();
            if(moreThan15(newSys, (md.bloodPressRawBuf) + 1) && moreThan15(newDia, md.bloodPressRawBuf)){
                shift(newSys, 16, (md.bloodPressRawBuf));
                shift(newDia, 16, (md.bloodPressRawBuf));
            }
        }
    }
    if(dispPR) { 
        Serial1.write('p');
        Serial1.write('l');
        if(Serial1.available()) {
            int newPr = Serial1.read();
            if(moreThan15(newPr, md.pulseRateRawBuf)) {
                shift(newPr, 8, (md.pulseRateRawBuf)); 
            }
        }
    }
    if(dispRR) { 
        Serial1.write('r');
        Serial1.write('l');
        if(Serial1.available()) { 
            int newRR = Serial1.read();
            if(moreThan15(newRR, md.pulseRateRawBuf)) {
                shift(newRR, 8, (md.respirationRateRawBuf));
            }
        }
    }
    Serial1.write('e');
    delay(200);
    //generateEKG();
    return;
}


double Fs = 0;

int i;
int fft = 0;
double f0 = 0;
int m_index = 0;
void generateEKG() {
    f0 = 200;
    Fs = 2.2 * f0;
    
    for (i = 0; i < 256; i++) {
        EKGRawBuf[i] = 32*sin(2*3.14*f0*i/Fs);
        //Serial.println(EKGRawBuf[i]);
        EKGImgBuf[i] = 0;
    }
    delay(1000);
    //Serial.println(EKGRawBuf[0]);
    m_index = optfft(EKGRawBuf, EKGImgBuf);
    fft = Fs *m_index /256 + 1;
    Serial.println(fft);
    shiftChar(fft, 16, EKGFreqBuf);
    Serial.println(EKGFreqBuf[0]);
    return;
    
}



/*
*    @para: generic pointer dataPtr;
*    Assume the data pointer is of type ComputeData
*    Compute the corrected values of data
*    April 23, 2019 by Kaiser Sun
*    Change the type of corrected to char[]
*    April 24, 2019 by Kaiser Sun
*    May 24,2019 modified by Xinyu
*/
void Compute(void* dataPtr) {
    ComputeData comd = *((ComputeData*) dataPtr);
    //EKGData EKGD = *((EKGData*) data);
    if(dispT) {
        int correctedTemp = (*(comd.temperatureRawBuf)) * 0.75 + 5;
        shiftChar(correctedTemp, 8, (comd.tempCorrectedBuf));
    }
    if(dispBP) {
        int correctedDia = (*(comd.bloodPressRawBuf)) * 1.5 + 6;
        int correctedSys = (*(comd.bloodPressRawBuf + 1)) * 2 + 9;
        shiftChar(correctedSys, 16, (comd.bloodPressCorrectedBuf));
        shiftChar(correctedDia, 16, (comd.bloodPressCorrectedBuf));
    } 
    if(dispPR) {
        int correctedPr = (*(comd.pulseRateRawBuf)) * 3 + 8; 
        shiftChar(correctedPr, 8, (comd.prCorrectedBuf));
    }
    if(dispRR) {
        shiftChar((*(comd.respirationRateRawBuf))*3 + 7, 8, (comd.respirationRateCorBufPtr));
    }
    if (dispEKG && genEKG) {
        generateEKG();
        genEKG = false;
    }
    return;
}

int countt = 0;

/*
*    @para: generic pointer dataPtr;
*    Assume the data pointer is of type DisplayData
*    Display the data on the TFT display
*    April 23, 2019 by Kaiser Sun
*    May 24, 2019 modified by Xinyu
*/
int index = 15;
void Display(void* dataPtr) {
    // TODO: change the color of display!
    // Serial.println("run display");
    index = 15;
    DisplayData dd = *((DisplayData*) dataPtr);
    // Setup of tft display
    if (refDisp) {
      tft.setCursor(0, 0);
      tft.setTextColor(CYAN);
      tft.setTextSize(2);
      tft.println("        Mobile Doctor");
      tft.setTextColor(WHITE);
      //tft.setTextSize(1.9);
      if (dispBP) {
        tft.println("Systolic Pressure: ");
        tft.println("Diastolic Pressure: ");
      }
      if (dispT) {
        tft.println("Temperature: ");
      }
      if (dispPR) {
        tft.println("Pulse Rate: "); 
      }
      if (dispRR) {
        tft.println("Respiration Rate: ");
      }
      if (dispEKG) {
        tft.println("EKG: ");
      }
      if (*(dd.Mode) == 0) {
        tft.print("Battery: ");
      }
      refDisp = false;
    }
    
    tft.fillRect(225,15,400,118,BLACK);
    tft.setTextSize(2);

    // Display Pressure
    if(dispBP) {
        if(bpOutOfRange == 0) {
            tft.setTextColor(GREEN);
            flashBP = false;
        } else if (bpHigh) { 
          tft.setTextColor(RED);
          flashBP = false;
        } else {
           tft.setTextColor(YELLOW);
           flashBP = true;
        }
        
        tft.setCursor(225, index);
        BPindex = index;
        tft.print(*(dd.bloodPressCorrectedBuf + 1));
        tft.print(" mmHg");
        index += 16;
        tft.setCursor(225, index);
        tft.print(*(dd.bloodPressCorrectedBuf));
        tft.print(" mmHg");
      
    }

    // print temperature
    if(dispT) {
        if(tempOutOfRange == 0) {
            tft.setTextColor(GREEN);
            flashT = false;
        } else {
            if(tempHigh) {
              tft.setTextColor(RED);
              flashT = false;
            } else {
              tft.setTextColor(YELLOW);
              flashT = true;
            }
        }

        index += 16;
        Tindex = index;
        tft.setCursor(225, index);
        tft.print(*(dd.tempCorrectedBuf));
        tft.print(" C");
    }

    // Display pulse
    if(dispPR) {
        if(pulseOutOfRange == 0) {
            tft.setTextColor(GREEN);
            flashPR = false;
        } else {
            if(prHigh) {
              tft.setTextColor(RED);
              flashPR = false;
            } else {
              tft.setTextColor(YELLOW);
              flashPR = true;
            }
        }

        index += 16;
        PRindex = index;
        tft.setCursor(225, index);
        tft.print(*(dd.prCorrectedBuf));
        tft.print("BPM");
    }

    // Display respiration rate
    if(dispRR) {
        if(rrOutOfRange == 0) {
            tft.setTextColor(GREEN);
        } else {
            if(rrHigh) {
              tft.setTextColor(RED);
            } else {
              tft.setTextColor(YELLOW);
            }
        }
        index += 16;
        tft.setCursor(225, index);
        tft.print(*(dd.respirationRateCorrectedBuf));
        tft.print(" /s");
    }

    if (dispEKG) {
        if (EKGOutOfRange == 0) {
            tft.setTextColor(GREEN);
        } else {
            if (EKGHigh) {
                tft.setTextColor(RED);
            } else {
                tft.setTextColor(YELLOW);
            }
        }
        index += 16;
        tft.setCursor(225, index);
        tft.print(*(dd.EKGFreqBuf));
        tft.print("Hz");

    }
    

    // Display battery status
    if(*(dd.batteryStatePtr) > 20) {
        tft.setTextColor(GREEN);
    } else {
        tft.setTextColor(RED);
    }

    if (*(dd.Mode) == 0) {
//      tft.print("Battery: ");
      index += 16;
      tft.setCursor(225, index);
      tft.println(*(dd.batteryStatePtr)); 
      tft.setTextSize(1.5);
      tft.setTextColor(RED);
      tft.fillRect(0,130,400,30,BLACK);
      tft.setCursor(0, 135);
      if(tempHigh) {
          tft.println("Your body temperature is too high! Calm down QWQ");
      }
  
      if (bpHigh) {
          tft.println("Your blood pressure is too high! Calm down QWQ");
      }
      
      if(prHigh) {
          tft.println("Your heart beat is too slow. Do something exciting 0w0");
      }
    }

    // End
    tft.setTextSize(1.4);
    tft.setTextColor(CYAN);
    tft.setCursor(0, 163);
    tft.println("                   CSE 474 Inc.");
    return;
}


/*
*    @param: generic pointer dataPtr;
*    assume the dataPtr is of type dataPtr;
*    if the data are out of range, diplay with red;
*    April 23, 2019 by Kaiser Sun
*/
void WarningAlarm(void* dataPtr) {
    WarningAlarmData wad = *((WarningAlarmData*) dataPtr);
    if (*(wad.temperatureRawBuf) > 43.7*1.05 || *(wad.temperatureRawBuf) < 41.5*0.95) {
        tempOutOfRange = 1;
        tempHigh = isTHight(float(*(wad.temperatureRawBuf)));
        if(tempHigh) { tWCounter++; }
    } else {
        tempOutOfRange = 0;
    }
    if(*(wad.bloodPressRawBuf + 1) > 60.5*1.05 || *(wad.bloodPressRawBuf) > 49.3*1.05 || *(wad.bloodPressRawBuf + 1) < 55.5*0.95 || *(wad.bloodPressRawBuf) < 42.7*0.95) {
        bpOutOfRange = 1;
        //sys: 1, Dia: 0
        bpHigh = isBPHigh(*(wad.bloodPressRawBuf + 1), *(wad.bloodPressRawBuf));
        if(bpHigh) { bpWCounter++; }
    } else {
        bpOutOfRange = 0;
    }
    if(*(wad.pulseRateRawBuf) < 17.3*1.05 || *(wad.pulseRateRawBuf) > 30.7*0.95) {
        pulseOutOfRange = 1;
        prHigh = isPRHigh(float(*(wad.pulseRateRawBuf)));
        if(prHigh) { prWCounter++; }
    } else {
        pulseOutOfRange = 0;
    }
    if(*(wad.rrRawBuf) < 1.67*0.95 || *(wad.rrRawBuf) > 6*1.05) {
        rrOutOfRange = 1;
        rrHigh = isRRHigh(float(*(wad.rrRawBuf)));
        if(rrHigh) {rrWCounter++;}
    } else {
        rrOutOfRange = 0;
    }
    if(*(wad.EKGFreqBuf) < 35*0.95 || *(wad.EKGFreqBuf) > 3750*1.05) {
        EKGOutOfRange = 1;
        EKGHigh = isEKGHigh(float(*(wad.EKGFreqBuf)));
        if(EKGHigh) {ekgWCounter++;}
    } else {
      EKGOutOfRange = 0;
    }
    return;  
}
/*
*    @param: generic pointer dataPtr;
*    Assume the dataPtr is of type StatusData;
*    BatteryState shall decrease by 1 each it is called;
*    April 23, 2019 by Kaiser Sun
*/
void Status(void* dataPtr) {
    StatusData sd = *((StatusData*) dataPtr);
    if(*(sd.batteryStatePtr) > 0) {
        *(sd.batteryStatePtr) -= 1;
    }
    return;
}



/*
*   This function is called when the TFT is in menu mode
*   May 9, 2019 by Kaiser
*   May 24, 2019 modified by Xinyu
*/
void menu(KeypadData* dataPtr) {
    //refMenu = false;
    KeypadData d = *dataPtr;

    if (refMenu) {
      tft.setCursor(0, 0);
      tft.fillScreen(BLACK);
      drawSub(70, 0, dispBP);
      drawSub(70, 48, dispPR);
      drawSub(70, 96, dispT);
      drawSub(70, 144, dispRR);
      drawSub(70, 192, dispEKG);
      tft.fillRect(0, 0, 70, 240, MAGENTA);
      tft.setTextSize(2);
      tft.setTextColor(BLACK);
      tft.setCursor(5, 100);
      tft.print("Exit");
      refMenu = false;
    }
    if (TSelected) {
      drawSub(70, 96, dispT);
      TSelected = false;
    }
    if (BPSelected) {
      drawSub(70, 0, dispBP);
      BPSelected = false;
    }
    if (PRSelected) {
      drawSub(70, 48, dispPR);
      PRSelected = false;
    }
    if (RRSelected) {
      drawSub(70, 144, dispRR);
      RRSelected = false;
    }
    if (EKGSelected) {
      drawSub(70, 192, dispEKG);
      EKGSelected = false;
    }

    // Get point
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    // If we have point selected
    if (p.z > ts.pressureThreshhold) {
    // scale from 0->1023 to tft.width
        p.x = (tft.width() - map(p.x, TS_MINX, TS_MAXX, tft.width(), 0));
        p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));

        if(T(p.x, p.y)) {
            TSelected = true;
            if(dispT) {
                dispT = FALSE;
            } else {
                dispT = TRUE;
            }
        }
        if(BP(p.x, p.y)) {
            BPSelected = true;
            if(dispBP) {
                dispBP = FALSE;
            } else {
                dispBP = TRUE;
            }
        }
        if(PR(p.x, p.y)) {
            PRSelected = true;
            if(dispPR) {
                dispPR = FALSE;
            } else {
                dispPR = TRUE;
            }
        }
        if(RR(p.x, p.y)) {
            RRSelected = true;
            if(dispRR) {
                dispRR = FALSE;
            } else {
                dispRR = TRUE;
            }
        }
        if (EKG(p.x, p.y)) {
            EKGSelected = true;
            if (dispEKG) {
              dispEKG = false;
            } else {
              dispEKG = true;
            }
        }
        if(QUIT1(p.x, p.y)) {
            *(d.measurementSelectionPtr) = 0;
            refSelect = true;
            refDisp = true;
            //Select((void*) &d);
        }
    }
    //refMenu = true;
    return;
}

/*
*    @param: KeypadData pointer, dataPtr
*    Submethod of select; when called, goes into announciation mode
*    May 10th by Kaiser
*    May 24, 2019 modified by Xinyu
*/
void anno(KeypadData* dataPtr) {
    //refAnnu = false;
    KeypadData d = *dataPtr;
    // Draw exit button
    
    if (refAnnu) {
      tft.fillScreen(BLACK);
      tft.fillRect(0, 180, 330, 60, RED);
      tft.setTextSize(2);
      tft.setTextColor(BLUE);
      tft.setCursor(150, 200);
      tft.print("Exit2");
  
      refAnnu = false;
    }
    //(*disp.myTask)(disp.taskDataPtr);
    
    // getPoint
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    // If we have point selected
    if (p.z > ts.pressureThreshhold) {
        // scale from 0->1023 to tft.width
        p.x = (tft.width() - map(p.x, TS_MINX, TS_MAXX, tft.width(), 0));
        p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));
        if(QUIT2(p.x, p.y)) {
            *(d.alarmAcknowledgePtr) = 0;
            refSelect = true;
            refDisp = true;
            
        }
    }


    return;
}

/*
*   @param: generic pointer dataPtr
*   Assume the dataPtr is of type keyData
*   Goes into mode select; from mode select, we can select or 
*   get announciation data
*   May 8, 2019 by Kaiser Sun
*   May 9, 2019 rewrote by Kaiser
*   May 24, 2019 modified by Xinyu
*/
void Select(void* dataPtr) {
    //refSelect = false;
    KeypadData kd = *((KeypadData*) dataPtr);
    // When it's select mode

       if (refSelect) {
          tft.fillScreen(WHITE);
          tft.fillCircle(55, 60, 50, GREEN);
          // Not used button
          tft.fillCircle(160, 60, 50, CYAN);
          // Upper: not used button
          tft.setTextSize(2);
          tft.setTextColor(WHITE);
          tft.setCursor(30, 55);
          tft.print("Menu");
          tft.fillCircle(265, 60, 50, YELLOW);
          // Not used button
          tft.setCursor(140, 55);
          tft.print("Meas");
          //tft.setTextColor(WHITE);
          tft.fillCircle(55, 180, 50, PINK);
          tft.setCursor(40, 175);
          tft.print("TM");
          tft.fillCircle(160, 180, 50, PURPLE);
          tft.setCursor(120, 175);
          tft.print("Call Dr");
          tft.fillCircle(265, 180, 50, BLUE);
          tft.setCursor(245, 175);
          tft.print("Game");
          // Upper: not used button
          tft.setCursor(230, 55);
          tft.print("Announ");
          refSelect = false; 
        }
        digitalWrite(13, HIGH);
        TSPoint p = ts.getPoint();
        digitalWrite(13, LOW);
        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);
        if (p.z > ts.pressureThreshhold) {
        // scale from 0->1023 to tft.width
            p.x = (tft.width() - map(p.x, TS_MINX, TS_MAXX, tft.width(), 0));
            p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));
            //Serial.println(MENU(p.x, p.y));
            if(ANN(p.x, p.y)) {
                // If it's announciation, turn to display
                *(kd.alarmAcknowledgePtr) = 1;
                refAnnu = true;
                refDisp = true;
            }
            if(MENU(p.x, p.y)) {
                *(kd.measurementSelectionPtr) = 1;
                refMenu = true;
                refDisp = true;
            } 
            if (MEAS(p.x, p.y)) {
               *(kd.displaySelectionPtr) = 1;
               refDisp = true;
               refMeas = true;
            }
            if (TM (p.x, p.y)) {
               *(kd.displayTMPtr) = 1;
               refDisp = true;
               ref = true;
               //Serial.println(*(kd.displayTMPtr));
               //blank();
            } 
            if (EM (p.x, p.y)) {
              *(kd.displayEmergencyPtr) = 1;
              refDisp = true;
              refCD = true;
              //blank();
            }
            if (Game (p.x, p.y)) {
              *(kd.displayGamePtr) = 1;
              refDisp = true;
              refG = true;
              //blank();
            }
        }
    
    return;
}

/*
*   Extra credit feature; It will call doctor when the button is pressed
*   June 8, Kaiser
*/
void callDoctor() {
    if (refCD) {
      //Serial.println("The patient is calling you!");
      
      tft.setTextSize(3);
      
      tft.fillScreen(BLACK);
      tft.fillRect(0, 180, 330, 60, RED);
      tft.setTextSize(2);
      tft.setTextColor(BLUE);
      tft.setCursor(150, 200);
      tft.print("Exit");
      tft.setTextColor(WHITE);
      tft.setCursor(10, 80);
      tft.print("Calling the Doctor......");
      delay(1000);
      tft.fillRect(0, 0, 330, 150, BLACK);
      tft.setCursor(10, 80);
      tft.print("Please wait for your doctor.");
      
      //Serial.println("The patient is calling you!");
      
      refCD = false;
    }

    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    
    // If we have point selected
    if (p.z > ts.pressureThreshhold) {
        // scale from 0->1023 to tft.width
        p.x = (tft.width() - map(p.x, TS_MINX, TS_MAXX, tft.width(), 0));
        p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));
        if(QUIT3(p.x, p.y)) {
            //*(displayTMPtr) = 0;
            *(displayEmergencyPtr) = 0;
            //*(displayGamePtr) = 0;
            refSelect = true;
            refDisp = true;
        }
    } 
    
    return;
}

void game() {
    if (refG) {
      tft.setCursor(0, 20);
      tft.setTextColor(WHITE);
      tft.setTextSize(2);
      tft.fillScreen(BLACK);
      tft.print("Select the block that has green words to escape this page.");
      tft.setTextColor(BLUE);
      tft.setCursor(230, 120);
      tft.print("GREEN");
      // Green words
      tft.setTextColor(GREEN);
      tft.setCursor(10, 120);
      tft.print("BLUE");
      refG = false;
    }
        TSPoint p = ts.getPoint();
        digitalWrite(13, LOW);
        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);
        if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
        // scale from 0->1023 to tft.width
            p.x = (tft.width() - map(p.x, TS_MINX, TS_MAXX, tft.width(), 0));
            p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));
            if(G(p.x, p.y)) {
              //*(displayTMPtr) = 0;
              *(displayGamePtr) = 0;
              //*(displayGamePtr) = 0;
              refSelect = true;
              refDisp = true;
            }
        } 
    return;
}

void blank() {
    if (ref) {
      tft.fillScreen(WHITE);
      tft.fillRect(0, 180, 330, 60, RED);
      tft.setTextSize(3);
      tft.setCursor(10, 90);
      tft.setTextColor(RED);
      tft.print("Make Way for Ambulance !!!");
      tft.setTextColor(BLUE);
      tft.setCursor(150, 200);
      tft.setTextSize(2);
      tft.print("Exit");
      ref = false;
      delay(500);
    } else {

      tft.fillCircle(150, 90, 60, RED);
      tft.fillRect(0, 0, 330, 180, WHITE);
    }
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    
    // If we have point selected
    if (p.z > ts.pressureThreshhold) {
        // scale from 0->1023 to tft.width
        p.x = (tft.width() - map(p.x, TS_MINX, TS_MAXX, tft.width(), 0));
        p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));
        if(QUIT3(p.x, p.y)) {
            *(displayTMPtr) = 0;
            *(displayEmergencyPtr) = 0;
            *(displayGamePtr) = 0;
            refSelect = true;
            refDisp = true;
        }
    } 

    return;
}


/*
*    @param: KeypadData pointer, dataPtr
*    Submethod of select; when called, goes into 
*    May 24th by Xinyu
*/
void Measurement(KeypadData* dataPtr) {
   
    KeypadData d = *dataPtr;
    // Draw exit button
    
    if (refMeas) {
      tft.fillScreen(BLACK);
      tft.fillRect(0, 180, 330, 60, RED);
      tft.setTextSize(2);
      tft.setTextColor(BLUE);
      tft.setCursor(150, 200);
      tft.print("Exit3");
      refMeas = false;
    }

    // getPoint
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    
    // If we have point selected
    if (p.z > ts.pressureThreshhold) {
        // scale from 0->1023 to tft.width
        p.x = (tft.width() - map(p.x, TS_MINX, TS_MAXX, tft.width(), 0));
        p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));
        if(QUIT3(p.x, p.y)) {
            *(d.displaySelectionPtr) = 0;
            refSelect = true;
            refDisp = true;
        }
    } 

    return;
}


/*
*   @param: data pointer of communications data
*   Receive the command from remote subsystem
*   change the display status; 's' is start of message
*   'e' is end of message
*   Order: temperature, pressure, pulserate, respiration rate
*   May 29th, 2019 by Kaiser
*/
char t = 'q';
void Communications(void* dataPtr) {
    CommunicationsData cd = *((CommunicationsData*) dataPtr);
    if(Serial.available() > 0) {
        if(Serial.read() == 's') {
            int i = 0;
            t = Serial.read();
            while(t != 'e') {
                // When they did not read end of message, keep loop
                if(t == 't') {
                    Serial.write("Temperature: ");
                    Serial.println(*(cd.tempCorrectedBuf));
                    Serial.println(" ");
                    if(dispT) { dispT = false; } else {dispT = true; }
                }
                if(t == 'b') {
                    Serial.write("Systolic Pressure: ");
                    Serial.println(*(cd.bloodPressCorrectedBuf + 1));
                    Serial.println(" ");
                    Serial.write("Diastolic Pressure: ");
                    Serial.println(*(cd.bloodPressCorrectedBuf));
                    Serial.println(" ");
                    if(dispBP) { dispBP = false; } else { dispBP = true; }
                }
                if(t == 'p') {
                    Serial.write("Pulse Rate: ");
                    Serial.println(*(cd.prCorrectedBuf));
                    Serial.println(" ");
                    if(dispPR) { dispPR = false; } else { dispPR = true; }
                }
                if(t == 'r') {
                    Serial.write("Respiration Rate: ");
                    Serial.println(*(cd.respirationRateCorBufPtr));
                    Serial.println(" ");
                    if(dispRR) {dispRR = false; } else { dispRR = true; }
                }
                t = Serial.read();
            }
        }
        t = 'e';
    }
    return;
}


/*
*    @param: array of TCB taskQ
*    pre: assume length(taskQ) = 5
*    helper method of scheduler, used to measure the time
*    that each task takes to execute; should be commented
*    after development;
*    April 23, 2019 by Kaiser Sun
*    May 12, 2019 modified by Xinyu
*/
void schedulerTest() {
        TCB* taskQueue = front;

        while (taskQueue != NULL) {
            run(taskQueue);
            taskQueue = taskQueue->next;
        }
}

char* doctorName = "Kaiser";
char* patientName = "Shouta";
/*
*    Display data on remote system
*    Share the data with display
*    TODO: Will be called each 5 seconds
*/
void remoteCommunicationDisplay(void* dataPtr) {
  // Print out product name, doctor name, and patient name
  Serial.println("__________________Mobile__Doctor______________________");
  Serial.println("____________0w0_Are_You_Healthy_Today?________________");
  Serial.println("Mobile Doctor 0W0");
  Serial.println(doctorName);
  Serial.println(patientName);
  // Dereference data
  DisplayData dd = *((DisplayData*) dataPtr);
  // Display temperature
  if(dispT) {
    Serial.print("Temperature:        ");
    Serial.print(*(dd.tempCorrectedBuf));
    Serial.println(" C");
  }
  if(dispBP) {
    Serial.print("Systolic Pressure: ");
    Serial.print(*(dd.bloodPressCorrectedBuf + 1));
    Serial.println("mmHg");
    Serial.print("Diastolic Pressure: ");
    Serial.print(*(dd.bloodPressCorrectedBuf));
    Serial.println("mmHg");
  }
  if(dispPR) {
    Serial.print("Pulse rate:         ");
    Serial.print(*(pulseRateRawBuf));
    Serial.println("BPM");
  }
  if(dispEKG) {
    Serial.print("EKG:                ");
    // TODO: makesure if the EKG is right
    Serial.print(*(EKGFreqBuf));
    Serial.println("Hz");
  }
  Serial.print("Battery:              ");
  Serial.println(*(batteryStatePtrr));
  if(tempHigh) {
    Serial.println("Your body temperature is too high! Calm down QWQ");
  }
  
  if (bpHigh) {
    Serial.println("Your blood pressure is too high! Calm down QWQ");
  }
      
  if(prHigh) {
    Serial.println("Your heart beat is too slow. Do something exciting 0w0");
  }
  Serial.println("__________________________________________CSE474_Inc._____");
  Serial.println(" ");
  Serial.println(" ");
  return;
}


/*
*    Execute only at the Setup function once
*    Start the system timer, arrange the taskQueue
*    May 22, 2019 by Kaiser
*/
void startUp() {
    // Setup the data structs
  meaD = MeasureData{temperatureRawPtrr, bloodPressRawPtrr, pulseRateRawPtrr, respirationRateRawPtr, measurementSelectionPtr, EKGFreqBufPtr};
  //EKGD = EKGData{EKGRawBuf, EKGImgBuf, EKGFreqBuf};
  cD = ComputeData{temperatureRawPtrr, bloodPressRawPtrr, pulseRateRawPtrr, respirationRateRawPtr, EKGRawBufPtr, EKGImgBufPtr, EKGFreqBufPtr, tempCorrectedPtrr, bloodPressCorrectedPtrr, pulseRateCorrectedPtrr, respirationRateCorPtr,measurementSelectionPtr};
  dDa = DisplayData{ModePtrr, tempCorrectedPtrr, bloodPressCorrectedPtrr, pulseRateCorrectedPtrr, respirationRateCorPtr, EKGFreqBufPtr, batteryStatePtrr};
  wAD = WarningAlarmData{temperatureRawPtrr, bloodPressRawPtrr, pulseRateRawPtrr, respirationRateRawPtr, EKGFreqBufPtr, batteryStatePtrr};
  sD = StatusData{batteryStatePtrr};
  kD = KeypadData{localFunctionSelectPtr, measurementSelectionPtr, alarmAcknowledgePtr, commandPtr, remoteFunctionSelectPtr, measurementResultSelectionPtr, displaySelectionPtr, displayTMPtr, displayEmergencyPtr, displayGamePtr};
  comD = CommunicationsData{tempCorrectedPtrr, bloodPressCorrectedPtrr, pulseRateCorrectedPtrr, respirationRateCorPtr, EKGFreqBufPtr};
// Setup the TCBs
  meas = {&Measure, &meaD};
  //gene = {&generateEKG, &EKGD};
  comp = {&Compute, &cD};
  disp = {&Display, &dDa};
  alar = {&WarningAlarm, &wAD};
  stat = {&Status, &sD};
  remDisp = {&remoteCommunicationDisplay, &dDa};
  //keyp = {&Select, &kD};
  com = {&Communications, &comD};
  
  // Setup task queue
  insertLast(&meas);
  insertLast(&comp);
  //insertLast(&gene);
  insertLast(&stat);
  insertLast(&alar);
  insertLast(&com);
  //insertLast(&keyp);
  time1 = millis();
  timeb = time1;
  timeRD1 = time1;
  timeEmer1 = time1;
  // taskQueue[4] = disp;
  //run(&keyp);
  //generateEKG();

}


void flash() {
  if (dispBP && flashBP) {
    if (counterBP == 1) {
      tft.fillRect(225, BPindex, 400, 30, BLACK);
    } else {
      tft.setTextColor(YELLOW);
      tft.fillRect(225, BPindex, 400, 30, BLACK);
      tft.setTextSize(2);
      tft.setCursor(225, BPindex);
      tft.print(*(bloodPressCorrectedBuf + 1));
      tft.print(" mmHg");
      tft.setCursor(225, BPindex + 16);
      tft.print(*(bloodPressCorrectedBuf));
      tft.print(" mmHg");
    }
  }
  
  if (dispPR && flashPR) {
    if (counterPR == 4) {
      tft.fillRect(225, PRindex, 400, 15, BLACK);
    } else if (counterPR == 8){
      tft.setTextColor(YELLOW);
      tft.fillRect(225, PRindex, 400, 15, BLACK);
      tft.setTextSize(2);
      tft.setCursor(225, PRindex);
      tft.print(*(pulseRateCorrectedPtrr));
      tft.print("BPM");
    }
  }
  if (dispT && flashT) {
    if (counterT == 2) {
      tft.fillRect(225, Tindex, 400, 15, BLACK);
    } else if (counterT == 4) {
      tft.setTextColor(YELLOW);
      tft.fillRect(225, Tindex, 400, 15, BLACK);
      tft.setTextSize(2);
      tft.setCursor(225, Tindex);
      // Serial.println(Tindex);
        
      tft.print(*(tempCorrectedBuf));
      tft.print(" C");
    }
  }
  
}



/*
*    @param: int index, TCB* taskQ;
*    pre: index < length(taskQ);
*    helper method of sechdule function;
*    April 23, 2019 by Kaiser Sun
*/
void run(TCB* taskQ) {
    // Call the function in the taskQ;
    (*taskQ->myTask)(taskQ->taskDataPtr);
}

/*
*   Called when user having an emergency situation
*/
void emergency() {
  Serial.println("EMERGENCY!!!");
  // Display website
  Serial.println("https://www.911.gov/");
  tft.fillScreen(RED);
}
