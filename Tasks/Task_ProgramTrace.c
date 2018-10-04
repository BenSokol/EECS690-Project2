/**
* @Filename: Task_ProgramTrace.c
* @Author:   Kaiser Mittenburg and Ben Sokol
* @Email:    bensokol@me.com
* @Email:    kaisermittenburg@gmail.com
* @Created:  August 30th, 2018 [1:35pm]
* @Modified: September 17th, 2018 [1:07pm]
* @Version:  1.0.0
*
* @Description: Periodically traces current program memory location
*
* Copyright (C) 2018 by Kaiser Mittenburg and Ben Sokol. All Rights Reserved.
*/

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "Drivers/UARTStdio_Initialization.h"
#include "Drivers/uartstdio.h"

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
#define ENABLE_OUTPUT FALSE

/************************************************
* External variables
************************************************/

// Access to current Sys Tick
extern volatile long int xPortSysTickCount;


/************************************************
* External functions declarations
************************************************/

// Assembly function to get PC from the stack
extern uint32_t Get_Value_From_Stack(uint32_t);

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
// We operate at 120 MHz, which gives a period of 8.33 nS
// The equation 8.33nS * K * M = 10mS
// The LOAD_VALUE (M) must be < 64k, 50,000 chosen arbitrarily
// The PRE_SCALE_VALUE (K) must be < 256. Solving, K = 24
// Since K is zero indexed, K = 23
// We are only interested in memory <= 32KiB which is 2^15
const uint32_t PC_OFFSET = 32;
const uint32_t LOAD_VALUE = 50000;
const uint32_t PRE_SCALE_VALUE = 23;
const uint32_t SIZE_OF_HISTOGRAM_ARRAY = 512;  // (512 << 6) == 32KiB
const uint32_t ONE_SECOND_DELTA_SYS_TICK = 10000;
const uint32_t REPORT_FREQUENCY_IN_SECONDS = 60;


/************************************************
* Local task variables
************************************************/

// The semaphore
xSemaphoreHandle Timer_0_A_Semaphore;

uint32_t histogram_array[512];  // Data array

// Extracted from the ISR to keep stack items to a minimum
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

// The current memory address obtained from the PC
uint32_t current_PC = 0;

// Status of our PC value. Controls when data is collected/reported
ISR_STATUS_t current_ISR_Status = COLLECTING;

uint32_t start_Sys_Tick = 0;  // Sys_Tick when ISR starts collecting
uint32_t stop_Sys_Tick = 0;   // Sys_Tick when ISR needs to stop collecting

uint32_t current_Histogram_Report = 0; // How many reports have been output


/************************************************
* Local task function declarations
************************************************/
extern void Timer_0_A_ISR();
extern void Task_ProgramTrace(void* pvParameters);
extern void report_histogram_data();
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
extern void Timer_0_A_ISR() {
  TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

  if (current_ISR_Status == COLLECTING) {
    // Get the value from the PC
    current_PC = Get_Value_From_Stack(PC_OFFSET);
    current_PC = floor( current_PC / 64.0 );

    // Validate Current_PC value is within size of array and store
    if (current_PC < SIZE_OF_HISTOGRAM_ARRAY) {
      // Increment Bin for Current_PC
      histogram_array[current_PC]++;
    }
    else {
      #if ENABLE_OUTPUT
          // Current_PC is out of range. In theory should never enter this else statement
            UARTprintf("ERROR: ( Current_PC / 64 ) >= %i", SIZE_OF_HISTOGRAM_ARRAY);
          }
          else {
            UARTprintf("ERROR: ( Current_PC / 64 ) < 0");
          }
          UARTprintf(" (Current_PC = %u\n");
      #endif
    }

    if (xPortSysTickCount > stop_Sys_Tick) {
      current_ISR_Status = DONE_COLLECTING;
    }
  }

  // "Give" the Timer_0_A_Semaphore
  xSemaphoreGiveFromISR(Timer_0_A_Semaphore, &xHigherPriorityTaskWoken);

  // If xHigherPriorityTaskWoken was set to true, we should yield.
  // The actual macro used here is port specific.
  // if (xHigherPriorityTaskWoken) {
  //   portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  // }
}


/*************************************************************************
* Function Name: Task_ProgramTrace
* Description:   Task used to initialize Timer_0_A_ISR
* Parameters:    void* pvParameters;
* Return:        void
*************************************************************************/
extern void Task_ProgramTrace(void* pvParameters) {
  //Initialize Semaphore and setup Timer
  vSemaphoreCreateBinary(Timer_0_A_Semaphore);

  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

  IntRegister(INT_TIMER0A, Timer_0_A_ISR);

  TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);

  TimerPrescaleSet(TIMER0_BASE, TIMER_A, PRE_SCALE_VALUE);

  TimerLoadSet(TIMER0_BASE, TIMER_A, LOAD_VALUE);

  TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

  //Enable Timer_0_A interrupt in NVIC
  IntEnable(INT_TIMER0A);

  // Enable (Start) Timer
  TimerEnable(TIMER0_BASE, TIMER_A);

  // Set data report to Excel format
  ReportData_SetOutputFormat(Excel_CSV);

  // Init the data array
  zero_histogram_array();

  // Set start time based on current systick, stop time = 1 minute later.
  start_Sys_Tick = xPortSysTickCount;
  stop_Sys_Tick = start_Sys_Tick + (REPORT_FREQUENCY_IN_SECONDS * ONE_SECOND_DELTA_SYS_TICK);

  // Add values to the histogram when appropriate
  while (1) {
    xSemaphoreTake(Timer_0_A_Semaphore, portMAX_DELAY);
    if (current_ISR_Status == DONE_COLLECTING) {
      current_Histogram_Report++;

      #if ENABLE_OUTPUT
        UARTprintf("DONE COLLECTING (%u)- BEGIN OUTPUT\n", current_Histogram_Report);
      #endif
      report_histogram_data();

      // Zero array to make sure overflow doesnt happen
      zero_histogram_array();

      // Reset time to start collecting again for 1 minute
      start_Sys_Tick = xPortSysTickCount;
      stop_Sys_Tick = start_Sys_Tick + (60 * ONE_SECOND_DELTA_SYS_TICK);

      // Set flag current_ISR_Status to start collecting again
      current_ISR_Status = COLLECTING;
    }
  }
}


extern void report_histogram_data() {
  uint32_t i = 0;
  for (i = 0; i < 512; ++i) {
    ReportData_Item item;
    item.TimeStamp = xPortSysTickCount;
    item.ReportName = 42;
    item.ReportValueType_Flg = 0x0;
    item.ReportValue_0 = i;
    item.ReportValue_1 = histogram_array[i];
    item.ReportValue_2 = 0;
    item.ReportValue_3 = 0;

    // This sends copy of data
    #if ENABLE_OUTPUT
      xQueueSend(ReportData_Queue, &item, 0);
    #endif
  }
}

extern void zero_histogram_array() {
  uint32_t i = 0;
  for (i = 0; i < 512; ++i) {
    histogram_array[i] = 0;
  }
}
