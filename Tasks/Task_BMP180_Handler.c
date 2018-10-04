/**
* @Filename: Task_BMP180_Handler.c
* @Author:   Kaiser Mittenburg and Ben Sokol
* @Email:    bensokol@me.com
* @Email:    kaisermittenburg@gmail.com
* @Created:  October 2nd, 2018
* @Modified:
* @Version:  1.0.0
*
* @Description: Periodically read and report temperature and pressure
*
* Copyright (C) 2018 by Kaiser Mittenburg and Ben Sokol. All Rights Reserved.
*/

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "Drivers/I2C7_Handler.h"
#include "Drivers/UARTStdio_Initialization.h"
#include "Drivers/uartstdio.h"

#include "sensorlib/bmp180.h"
#include "sensorlib/hw_bmp180.h"
#include "sensorlib/i2cm_drv.h"

#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"

#include "Tasks/Task_ReportData.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define DEBUG

extern int32_t Float_to_Int32( float theFloat );
/************************************************
* External variables
************************************************/

// Access to current Sys Tick
extern volatile long int xPortSysTickCount;

//*****************************************************************************
//
// The speed of the processor clock in Hertz, which is therefore the speed of the
// clock that is fed to the peripherals.
//
//*****************************************************************************
extern uint32_t g_ulSystemClock;

//
// SysTickClock Frequency
//
#define SysTickFrequency configTICK_RATE_HZ


/************************************************
* External functions declarations
************************************************/


/************************************************
* Local task constant types
************************************************/
typedef enum ISR_STATUS_t {
  COLLECTING,      // Should ISR collect data
  DONE_COLLECTING  // ISR is done collecting data, reporting
} ISR_STATUS_t;


/************************************************
* Local task constant variables
************************************************/

// Program Constants


/************************************************
* Local task variables
************************************************/

//
// BMP180 variables
// The BMP180 control block
//
tBMP180 sBMP180;
uint32_t BMP180Status = 0;
//
// A boolean that is set when an I2C transaction is completed.
//
volatile bool BMP180SimpleDone = false;
//
// The number of BMP180 callbacks taken.
//
uint32_t BMP180_Callbacks_Nbr = 0;
//
// Semaphore to indicate completion of an I/O operation
//
xSemaphoreHandle BMP180_Semaphore;


uint32_t histogram_array[512];  // Data array

// Status of our PC value. Controls when data is collected/reported
ISR_STATUS_t BMP_current_ISR_Status = COLLECTING;

uint32_t BMP_start_Sys_Tick = 0;  // Sys_Tick when ISR starts collecting
uint32_t BMP_stop_Sys_Tick = 0;   // Sys_Tick when ISR needs to stop collecting


/************************************************
* Local task function declarations
************************************************/
extern void BMP180SimpleCallback(void* pvData, uint_fast8_t ui8Status);
extern void Task_BMP180_Handler(void* pvParameters);
extern void BMP_report_data();
extern void zero_histogram_array();

/************************************************
* Local task function definitions
************************************************/

/*************************************************************************
* Function Name: Timer_0_A_ISR
* Description:   Interrupt Service Routine used to profile tasks
* Parameters:    N/A
* Return:        void
*************************************************************************/
//
// The function that is provided by this example as a callback when BMP180
// transactions have completed.
void BMP180SimpleCallback(void* pvData, uint_fast8_t ui8Status) {
  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
  BMP180_Callbacks_Nbr++;
  //
  // See if an error occurred.
  if (ui8Status != I2CM_STATUS_SUCCESS) {
    //
    // An error occurred, so handle it here if required.
    //
    UARTprintf(">>>>BMP180 Error: %02X\n", ui8Status);
  }
  //
  // Indicate that the I2C transaction has completed.
  BMP180SimpleDone = true;
  //
  // "Give" the BMP180_Semaphore
  xSemaphoreGiveFromISR(BMP180_Semaphore, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/*************************************************************************
* Function Name: Task_ProgramTrace
* Description:   Task used to initialize Timer_0_A_ISR
* Parameters:    void* pvParameters;
* Return:        void
*************************************************************************/
//================================================================
//
// The simple BMP180 master driver example.
//
extern void Task_BMP180_Handler(void* pvParameters) {
  // Initialize I2C7
  I2C7_Initialization();

  // Initialize BMP180_Semaphore
  vSemaphoreCreateBinary(BMP180_Semaphore);

  // Initialize the BMP180.
  BMP180SimpleDone = false;
  BMP180Status = BMP180Init(&sBMP180, I2C7_Instance_Ref, 0x77, BMP180SimpleCallback, 0);
  xSemaphoreTake(BMP180_Semaphore, portMAX_DELAY);
  UARTprintf(">>>>BMP180: Initialized!\n");

  //
  // Loop forever reading data from the BMP180. Typically, this process
  // would be done in the background, but for the purposes of this example,
  // it is shown in an infinite loop.
  while (1) {
    float fTemperature = 0.0;
    float fPressure = 0.0;
    char fTemperatureStr[50];
    char fPressureStr[50];

    // Request a reading from the BMP180.
    BMP180DataRead(&sBMP180, BMP180SimpleCallback, 0);
    xSemaphoreTake(BMP180_Semaphore, portMAX_DELAY);
    UARTprintf(">>>>BMP180: Data Read!\n");

    // Get the new pressure and temperature reading.
    BMP180DataPressureGetFloat(&sBMP180, &fPressure);
    BMP180DataTemperatureGetFloat(&sBMP180, &fTemperature);

    // Convert floats to strings because UARTprintf is unable to print float
    sprintf(fPressureStr, "%f", fPressure);
    sprintf(fTemperatureStr, "%f", fTemperature);
    UARTprintf(">>BMPData: Temperature: %s; Pressure: %s;\n", fTemperatureStr, fPressureStr );

    // Do something with the new pressure and temperature readings.
    vTaskDelay((SysTickFrequency * 1000) / 1000);
  }
}


extern void BMP_report_data() {
  uint32_t i = 0;
  for (i = 0; i < 512; ++i) {
    //ReportData_Item item;
    //item.TimeStamp = xPortSysTickCount;
    //item.ReportName = 42;
    //item.ReportValueType_Flg = 0x0;
    //item.ReportValue_0 = i;
    //item.ReportValue_1 = histogram_array[i];
    //item.ReportValue_2 = 0;
    //item.ReportValue_3 = 0;

    // This sends copy of data
    //xQueueSend(ReportData_Queue, &item, 0);
  }
}

extern void BMP_zero_data() {
  uint32_t i = 0;
  for (i = 0; i < 512; ++i) {
    //histogram_array[i] = 0;
  }
}
