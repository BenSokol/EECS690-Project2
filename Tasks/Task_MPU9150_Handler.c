/**
* @Filename: Task_MPU9150_Handler.c
* @Author:   Kaiser Mittenburg and Ben Sokol
* @Email:    bensokol@me.com
* @Email:    kaisermittenburg@gmail.com
* @Created:  October 4nd, 2018
* @Modified:
* @Version:  1.0.0
*
* @Description: Periodically read and report accelerometer readings
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

#include "sensorlib/hw_ak8975.h"
#include "sensorlib/ak8975.h"
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


/************************************************
* Local task constant variables
************************************************/

// Program Constants


/************************************************
* Local task variables
************************************************/

//
// Acc variables
// The BMP180 control block
//
tMPU9150 sMPU9150;
uint32_t MPU9150Status = 0;
//
// A boolean that is set when an I2C transaction is completed.
//
volatile bool MPU9150SimpleDone = false;
//
// The number of BMP180 callbacks taken.
//
uint32_t MPU9150_Callbacks_Nbr = 0;
//
// Semaphore to indicate completion of an I/O operation
//
xSemaphoreHandle MPU9150_Semaphore;


uint32_t histogram_array[512];  // Data array


/************************************************
* Local task function declarations
************************************************/
extern void SimpleCallback(void* pvData, uint_fast8_t ui8Status);
extern void Task_MPU9150_Handler(void* pvParameters);
extern void MPU_report_data();
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
void MPU9150SimpleCallback(void* pvData, uint_fast8_t ui8Status) {
  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
  MPU9150_Callbacks_Nbr++;
  //
  // See if an error occurred.
  if (ui8Status != I2CM_STATUS_SUCCESS) {
    //
    // An error occurred, so handle it here if required.
    //
    UARTprintf(">>>>MPU9150 Error: %02X\n", ui8Status);
  }
  //
  // Indicate that the I2C transaction has completed.
  MPU9150SimpleDone = true;
  //
  // "Give" the MPU9150_Semaphore
  xSemaphoreGiveFromISR(MPU9150_Semaphore, &xHigherPriorityTaskWoken);
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
// The simple MPU9150 master driver example.
//
extern void Task_MPU9150_Handler(void* pvParameters) {
  // Initialize I2C7
  I2C7_Initialization();

  // Initialize Acc_Semaphore
  vSemaphoreCreateBinary(MPU9150_Semaphore);

  // Initialize the Acc.
  MPU9150SimpleDone = false;
  MPU9150Status = MPU9150Init(&sMPU9150, I2C7_Instance_Ref, 0x68, MPU9150SimpleCallback, 0);

  xSemaphoreTake(MPU9150_Semaphore, portMAX_DELAY);
  UARTprintf(">>>>MPU9150: Initialized!\n");

  //
  // Loop forever reading data from the MPU9150. Typically, this process
  // would be done in the background, but for the purposes of this example,
  // it is shown in an infinite loop.
  while (1) {
    float fAccelX = 0.0;
    float fAccelY = 0.0;
    float fAccelZ = 0.0;
    float fGyroX = 0.0;
    float fGyroY = 0.0;
    float fGyroZ = 0.0;


    char fAccelX_str[50];
    char fAccelY_str[50];
    char fAccelZ_str[50];
    char fGyroX_str[50];
    char fGyroY_str[50];
    char fGyroZ_str[50];

    // Request a reading from the Acc.
    MPU9150DataRead(&sMPU9150, MPU9150SimpleCallback, 0);
    xSemaphoreTake(MPU9150_Semaphore, portMAX_DELAY);
    //UARTprintf(">>>>MPU9150: Data Read!\n");

    // Get the new pressure and temperature reading.
    MPU9150DataAccelGetFloat(&sMPU9150, &fAccelX, &fAccelY, &fAccelZ);
    MPU9150DataGyroGetFloat(&sMPU9150, &fGyroX, &fGyroY, &fGyroZ);

    //ReportData_Item itemAccel;
    //itemAccel.TimeStamp = xPortSysTickCount;
    //itemAccel.ReportName = 91501;
    //itemAccel.ReportValueType_Flg = 0b0111;
    //itemAccel.ReportValue_0 = Float_to_Int32(fAccelX);
    //itemAccel.ReportValue_1 = Float_to_Int32(fAccelY);
    //itemAccel.ReportValue_2 = Float_to_Int32(fAccelZ);
    //itemAccel.ReportValue_3 = 0;

    // This sends copy of data
    //xQueueSend(ReportData_Queue, &itemAccel, 0);


    // Convert floats to strings because UARTprintf is unable to print float
    //sprintf(fAccelX_str, "%f", fAccelX);
    //sprintf(fAccelY_str, "%f", fAccelY);
    //sprintf(fAccelZ_str, "%f", fAccelZ);
    //sprintf(fGyroX_str, "%f", fGyroX);
    //sprintf(fGyroY_str, "%f", fGyroY);
    //sprintf(fGyroZ_str, "%f", fGyroZ);

    //UARTprintf(">>MPU9150Data: Accelerometer: X=%s; Y=%s; Z=%s\n", fAccelX_str, fAccelY_str, fAccelZ_str);
    //UARTprintf(">>MPU9150Data: Gyroscope: X=%s; Y=%s; Z=%s\n", fGyroX_str, fGyroY_str, fGyroZ_str);

    // Do something with the new pressure and temperature readings.
    vTaskDelay((SysTickFrequency * 1000) / 1000);
  }
}


extern void MPU_report_data() {
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

extern void MPU_zero_data() {
  uint32_t i = 0;
  for (i = 0; i < 512; ++i) {
    //histogram_array[i] = 0;
  }
}
