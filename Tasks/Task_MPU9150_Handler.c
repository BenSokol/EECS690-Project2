/**
* @Filename: Task_MPU9150_Handler.c
* @Author:   Kaiser Mittenburg and Ben Sokol
* @Email:    ben@bensokol.com
* @Email:    kaisermittenburg@gmail.com
* @Created:  October 4th, 2018 [1:02pm]
* @Modified: October 4th, 2018 [2:54pm]
* @Version:  1.0.0
*
* @Description: Periodically read and report accelerometer and
*               gyroscope readings
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

#include "sensorlib/ak8975.h"
#include "sensorlib/hw_ak8975.h"
#include "sensorlib/hw_mpu9150.h"
#include "sensorlib/mpu9150.h"

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
// The I2C Address of the MPU9150
const int MPU9150_ADDRESS = 0x68;


/************************************************
* Local task variables
************************************************/
// The MPU9150 control block
tMPU9150 sMPU9150;

// A boolean that is set when an I2C transaction is completed.
volatile bool MPU9150SimpleDone = false;

// The number of MPU9150 callbacks taken.
uint32_t MPU9150_Callbacks_Nbr = 0;

// Semaphore to indicate completion of an I/O operation
xSemaphoreHandle MPU9150_Semaphore;


/************************************************
* Local task function declarations
************************************************/
extern void MPU9150SimpleCallback(void* pvData, uint_fast8_t ui8Status);
extern void Task_MPU9150_Handler(void* pvParameters);


/************************************************
* Local task function definitions
************************************************/

/*************************************************************************
* Function Name: MPU9150SimpleCallback
* Description:   Callback Routine for the MPU9150 for when
*                transactions have completed
* Parameters:    void* pvData
*                uint_fast8_t ui8Status
* Return:        void
*************************************************************************/
void MPU9150SimpleCallback(void* pvData, uint_fast8_t ui8Status) {
  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
  MPU9150_Callbacks_Nbr++;

  if (ui8Status != I2CM_STATUS_SUCCESS) {
    // An error occurred
    UARTprintf(">>>>MPU9150 Error: %02X\n", ui8Status);
  }
  // Indicate that the I2C transaction has completed.
  MPU9150SimpleDone = true;

  // "Give" the MPU9150_Semaphore
  xSemaphoreGiveFromISR(MPU9150_Semaphore, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/*************************************************************************
* Function Name: Task_MPU9150_Handler
* Description:   Task to report Gyroscope and Accelerometer from
*                MPU9150 sensor
* Parameters:    void* pvParameters;
* Return:        void
*************************************************************************/
extern void Task_MPU9150_Handler(void* pvParameters) {
  // Initialize UART
  UARTStdio_Initialization();

  // Initialize I2C7
  I2C7_Initialization();

  // Initialize MPU9150_Semaphore
  vSemaphoreCreateBinary(MPU9150_Semaphore);

  // Initialize the MPU9150.
  MPU9150SimpleDone = false;
  MPU9150Init(&sMPU9150, I2C7_Instance_Ref, MPU9150_ADDRESS, MPU9150SimpleCallback, 0);
  xSemaphoreTake(MPU9150_Semaphore, portMAX_DELAY);
  UARTprintf(">>>>MPU9150: Initialized!\n");

  // Loop forever reading and reporting data from the MPU9150.
  while (1) {
    float fAccelX = 0.0;
    float fAccelY = 0.0;
    float fAccelZ = 0.0;
    float fGyroX = 0.0;
    float fGyroY = 0.0;
    float fGyroZ = 0.0;

    // Request a reading from the MPU9150.
    MPU9150DataRead(&sMPU9150, MPU9150SimpleCallback, 0);
    xSemaphoreTake(MPU9150_Semaphore, portMAX_DELAY);

    // Get the new accelerometer and pressure reading.
    MPU9150DataAccelGetFloat(&sMPU9150, &fAccelX, &fAccelY, &fAccelZ);
    MPU9150DataGyroGetFloat(&sMPU9150, &fGyroX, &fGyroY, &fGyroZ);

    // Create ReportData_Item for Acceleration
    ReportData_Item itemAccel;
    itemAccel.TimeStamp = xPortSysTickCount;
    itemAccel.ReportName = 0004;
    itemAccel.ReportValueType_Flg = 0b0111;
    itemAccel.ReportValue_0 = *(int32_t*)&fAccelX;
    itemAccel.ReportValue_1 = *(int32_t*)&fAccelY;
    itemAccel.ReportValue_2 = *(int32_t*)&fAccelZ;
    itemAccel.ReportValue_3 = 0;

    // Create ReportData_Item for Gyroscope
    ReportData_Item itemGyro;
    itemGyro.TimeStamp = xPortSysTickCount;
    itemGyro.ReportName = 0005;
    itemGyro.ReportValueType_Flg = 0b0111;
    itemGyro.ReportValue_0 = *(int32_t*)&fGyroX;
    itemGyro.ReportValue_1 = *(int32_t*)&fGyroY;
    itemGyro.ReportValue_2 = *(int32_t*)&fGyroZ;
    itemGyro.ReportValue_3 = 0;

    // Send ReportData_Items to queue to print
    xQueueSend(ReportData_Queue, &itemAccel, 0);
    xQueueSend(ReportData_Queue, &itemGyro, 0);

    // Delay
    vTaskDelay((SysTickFrequency * 1000) / 1000);
  }
}
