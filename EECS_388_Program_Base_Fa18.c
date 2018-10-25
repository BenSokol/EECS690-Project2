/**
* @Filename: EECS_388_Program_Base_Fa18.c
* @Author:   Dr. Gary Minden
* @Email:    gminden@ittc.ku.edu
* @Modified By: Kaiser Mittenburg and Ben Sokol
* @Email:    ben@bensokol.com
* @Email:    kaisermittenburg@gmail.com
* @Created:  October 4th, 2018 [1:02pm]
* @Modified: October 18th, 2018 [6:53am]
* @Version:  2.0.0
*
* @Description: Periodically read and report accelerometer and
*               gyroscope readings
*
* Copyright (C) 2018 by Dr. Gary Minden, Kaiser Mittenburg, and Ben Sokol.
* All Rights Reserved.
*/

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"

#include "Drivers/I2C7_Handler.h"
#include "Drivers/Processor_Initialization.h"
#include "Drivers/UARTStdio_Initialization.h"
#include "Drivers/uartstdio.h"

#include "FreeRTOS.h"
#include "task.h"

extern void Task_Blink_LED_PortN_1(void *pvParameters);
extern void Task_ReportTime(void *pvParameters);
extern void Task_ReportData(void *pvParameters);
extern void Task_ProgramTrace(void *pvParameters);
extern void Task_BMP180_Handler(void *pvParameters);
extern void Task_MPU9150_Handler(void *pvParameters);

int main(void) {
  Processor_Initialization();
  UARTStdio_Initialization();

  // Create a task to blink LED, PortN_1
  xTaskCreate(Task_Blink_LED_PortN_1, "Blinky", 32, NULL, 1, NULL);

  // Create a task to report data.
  xTaskCreate(Task_ReportData, "ReportData", 512, NULL, 1, NULL);

  // Create a task to report SysTickCount
  xTaskCreate(Task_ReportTime, "ReportTime", 512, NULL, 1, NULL);

  // Create a task to program trace
  xTaskCreate(Task_ProgramTrace, "Trace", 512, NULL, 1, NULL);

  // Create a task to report temperature and pressure
  xTaskCreate(Task_BMP180_Handler, "Pressure", 512, NULL, 1, NULL);

  // Create a task to report acceleration and gyroscope
  xTaskCreate(Task_MPU9150_Handler, "Accelerometer", 512, NULL, 1, NULL);

  UARTprintf("FreeRTOS Starting!\n");

  //Start FreeRTOS Task Scheduler
  vTaskStartScheduler();

  while (1) {
  }
}
