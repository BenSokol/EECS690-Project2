/**
* @Filename: Task_BMP180_Handler.c
* @Author:   Kaiser Mittenburg and Ben Sokol
* @Email:    ben@bensokol.com
* @Email:    kaisermittenburg@gmail.com
* @Created:  October 2nd, 2018 [1:10pm]
* @Modified: October 4th, 2018 [2:55pm]
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


/************************************************
* External variables
************************************************/
// Access to current SysTick
extern volatile long int xPortSysTickCount;

// SysTickClock Frequency
#define SysTickFrequency configTICK_RATE_HZ


/************************************************
* Local constant variables
************************************************/
// The I2C Address of the BMP180
const int BMP180_ADDRESS = 0x77;


/************************************************
* Local task variables
************************************************/
// The BMP180 control block
tBMP180 sBMP180;

// A boolean that is set when an I2C transaction is completed.
volatile bool BMP180SimpleDone = false;

// The number of BMP180 callbacks taken.
uint32_t BMP180_Callbacks_Nbr = 0;

// Semaphore to indicate completion of an I/O operation
xSemaphoreHandle BMP180_Semaphore;


/************************************************
* Local task function declarations
************************************************/
extern void BMP180SimpleCallback(void* pvData, uint_fast8_t ui8Status);
extern void Task_BMP180_Handler(void* pvParameters);


/************************************************
* Local task function definitions
************************************************/

/*************************************************************************
* Function Name: BMP180SimpleCallback
* Description:   Callback Routine for the BMP180 for when
*                transactions have completed
* Parameters:    void* pvData
*                uint_fast8_t ui8Status
* Return:        void
*************************************************************************/
void BMP180SimpleCallback(void* pvData, uint_fast8_t ui8Status) {
  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
  BMP180_Callbacks_Nbr++;

  if (ui8Status != I2CM_STATUS_SUCCESS) {
    // An error occurred
    UARTprintf(">>>>BMP180 Error: %02X\n", ui8Status);
  }
  // Indicate that the I2C transaction has completed.
  BMP180SimpleDone = true;

  // "Give" the BMP180_Semaphore
  xSemaphoreGiveFromISR(BMP180_Semaphore, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/*************************************************************************
* Function Name: Task_BMP180_Handler
* Description:   Task to report Temperature and Pressure from
*                BMP180 sensor
* Parameters:    void* pvParameters;
* Return:        void
*************************************************************************/
extern void Task_BMP180_Handler(void* pvParameters) {
  // Initialize UART
  UARTStdio_Initialization();

  // Initialize I2C7
  I2C7_Initialization();

  // Initialize BMP180_Semaphore
  vSemaphoreCreateBinary(BMP180_Semaphore);

  // Initialize the BMP180.
  BMP180SimpleDone = false;
  BMP180Init(&sBMP180, I2C7_Instance_Ref, BMP180_ADDRESS, BMP180SimpleCallback, 0);
  xSemaphoreTake(BMP180_Semaphore, portMAX_DELAY);
  UARTprintf(">>>>BMP180: Initialized!\n");

  // Loop forever reading and reporting data from the BMP180.
  while (1) {
    float fTemperature = 0.0;
    float fPressure = 0.0;

    // Request a reading from the BMP180.
    BMP180DataRead(&sBMP180, BMP180SimpleCallback, 0);
    xSemaphoreTake(BMP180_Semaphore, portMAX_DELAY);

    // Get the new pressure and temperature reading.
    BMP180DataPressureGetFloat(&sBMP180, &fPressure);
    BMP180DataTemperatureGetFloat(&sBMP180, &fTemperature);

    ReportData_Item pressureItem;
    pressureItem.TimeStamp = xPortSysTickCount;
    pressureItem.ReportName = 0002;
    pressureItem.ReportValueType_Flg = 0b0001;
    pressureItem.ReportValue_0 = *(int32_t*)&fPressure;
    pressureItem.ReportValue_1 = fPressure;
    pressureItem.ReportValue_2 = 0;
    pressureItem.ReportValue_3 = 0;

    ReportData_Item tempItem;
    tempItem.TimeStamp = xPortSysTickCount;
    tempItem.ReportName = 0003;
    tempItem.ReportValueType_Flg = 0b0001;
    tempItem.ReportValue_0 = *(int32_t*)&fTemperature;
    tempItem.ReportValue_1 = 0;
    tempItem.ReportValue_2 = 0;
    tempItem.ReportValue_3 = 0;

    // Send ReportData_Items to queue to print
    xQueueSend(ReportData_Queue, &pressureItem, 0);
    xQueueSend(ReportData_Queue, &tempItem, 0);

    // Delay
    vTaskDelay((SysTickFrequency * 1000) / 1000);
  }
}
