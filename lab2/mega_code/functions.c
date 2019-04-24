#include <stdio.h>
#include "tcb.h"
#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library

#define	RED     0xF800
#define	GREEN   0x07E0
#define	BLACK   0x0000


int temperatureRaw, systolicPressRaw, diastolicPressRaw, pulseRateRaw, batteryState;

typedef struct
{
    void (*myTask)(void*);
    void* taskDataPtr; 
} TCB;


typedef struct
{
    int* temperatureRawPtr;
    int* systolicPressRawPtr;
    int* diastolicPressRawPtr;
    int* pulseRateRawPtr;
} MeasureData;

typedef struct
{
    int* temperatureRawPtr;
    int* systolicPressRawPtr;
    int* diastolicPressRawPtr;
    int* pulseRateRawPtr;
    int* tempCorrectedPtr;
    int* sysPressCorrectedPtr;
    int* diasCorrectedPtr;
    int* prCorrectedPtr;
} ComputeData;

typedef struct
{
    int* tempCorrectedPtr;
    int* sysPressCorrectedPtr;
    int* diasCorrectedPtr;
    int* prCorrectedPtr;
    int* batteryStatePtr;
} DisplayData;

typedef struct
{
    int* temperatureRawPtr;
    int* systolicPressRawPtr;
    int* diastolicPressRawPtr;
    int* pulseRateRawPtr;
    int* batteryStatePtr; 
} WarningAlarmData;

typedef struct
{
    int* batteryStatePtr;
} StatusData;

typedef struct
{

} SchedulerData;


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
int trIsReverse = 0, prIsReverse = 0;
void Measure(void* dataPtr, int isEven, int trIsReverse, int prIsReverse)
{
    // dereference the data pointer;
    md = *((MeasureData*) dataPtr);  
        // When the function is executed even times;
        // Update the data;
        if(isEven == 1) {
            if(*(md.systolicPressRawPtr) <= 100) {
                *(md.systolicPressRawPtr) += 3;
            }
            if(*(md.diastolicPressRawPtr) >= 40) {
                *(md.diastolicPressRawPtr) -= 2;
            }
            if(trIsReverse == 0) {
                *(md.temperatureRawPtr) += 2;
            } else {
                *(md.temperatureRawPtr) -= 2;
            }
            if(prIsReverse == 0) {
                *(md.pulseRateRawPtr) -= 1;
            } else {
                *(md.pulseRateRawPtr) += 1;
            }
        } else {
            if(*(md.systolicPressRawPtr) <= 100) {
                *(md.systolicPressRawPtr) -= 1;
            }
            if(*(md.diastolicPressRawPtr) >= 40) {
                *(md.diastolicPressRawPtr) += 1;
            }
            if(trIsReverse == 0) {
                *(md.temperatureRawPtr) -= 2;
            } else {
                *(md.temperatureRawPtr) += 2;
            }
            if(prIsReverse == 0) {
                *(md.pulseRateRawPtr) += 1;
            } else {
                *(md.pulseRateRawPtr) -= 1;
            }
        }

        // Check if should reverse the process;
        if(*(md.temperatureRawPtr) > 50) {
            trIsReverse = 1;
        } else if (*(md.temperatureRawPtr) < 15)
        {
            trIsReverse = 1;
        }

        if(*(md.pulseRateRawPtr) > 40) {
            prIsReverse = 1;
        } else if (*(md.pulseRateRawPtr) < 15)
        {
            prIsReverse = 0;
        }

        return;
}

/*
*    @para: generic pointer dataPtr;
*    Assume the data pointer is of type ComputeData
*    Compute the corrected values of data
*    April 23, 2019 by Kaiser Sun
*/
void Compute(void* dataPtr) {
    comd = *((ComputeData*) dataPtr);
    *(comd.tempCorrectedPtr) = (*(comd.temperatureRawPtr)) * 0.75 + 5;
    *(comd.sysPressCorrectedPtr) =(*(comd.systolicPressRawPtr)) * 2 + 9;
    *(comd.diasCorrectedPtr) = (*(comd.diastolicPressRawPtr)) * 1.5 + 6;
    *(comd.prCorrectedPtr) = (*(comd.pulseRateRaw)) * 3 + 8;
    return;
}

/*
*    @para: generic pointer dataPtr;
*    Assume the data pointer is of type DisplayData
*    Display the data on the TFT display
*    April 23, 2019 by Kaiser Sun
*/
void Display(void* dataPtr) {
    // Setup of tft display
    tft.fillScreen(BLACK);
    tft.setCursor(0, 0);
    tft.setTextColor(GREEN);
    tft.setTextSize(3);
    // Pointer dereference
    dd = *((DisplayData*) dataPtr);
    // Display
    tft.print("Systolic Pressure: ");
    //TODO: need to make sure whether use raw or corrected
    tft.print(*(dd.systolicPressRawPtr));
    tft.print("mmHg   Diastolic Pressure: ");
    tft.print(*(dd.diastolicPressRawPtr));
    tft.println(" mmHg");
    // print temperature, pulserate, battery Status
    tft.print("Temperature: ");
    tft.print(*(dd.temperatureRawPtr));
    tft.print("C   Pulse Rate: ");
    tft.print(*(dd.pulseRateRawPtr));
    tft.print("BPM   Battery: ");
    tft.print(*(dd.batteryStatePtr)); 
}

/*
*    @param: generic pointer dataPtr;
*    assume the dataPtr is of type dataPtr;
*    if the data are out of range, diplay with red;
*/
void WarningAlarm(void* dataPtr) {
    wad = *((WarningAlarmData*) dataPtr);
    // Change all measurement value?
    
}

/*
*    @param: generic pointer dataPtr;
*    Assume the dataPtr is of type StatusData;
*    BatteryState shall decrease by 1 each it is called;
*/
void Status(void* dataPtr) {
    
}