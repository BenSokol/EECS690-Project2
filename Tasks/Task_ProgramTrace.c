/*--Task_ProgramTrace.c
 *
 *  Author:			Kaiser Mittenburg, Ben Sokol
 *	Organization:	KU/EECS/EECS 690
 *  Date:			August 30, 2018
 *
 *  Description:	Periodically traces current program memory location
 *
 */
#include "Drivers/UARTStdio_Initialization.h"
#include "Drivers/uartstdio.h"
#include "driverlib/interrupt.h"


#include	"inc/hw_ints.h"
#include	"inc/hw_memmap.h"
#include	"inc/hw_types.h"
#include	"inc/hw_uart.h"
#include  "inc/hw_sysctl.h"

#include  <stdlib.h>
#include	<stddef.h>
#include	<stdbool.h>
#include	<stdint.h>
#include	<stdarg.h>

#include	"driverlib/sysctl.h"
#include	"driverlib/pin_map.h"
#include  "driverlib/timer.h"
#include	"driverlib/gpio.h"

#include	"FreeRTOS.h"
#include	"task.h"
#include  "semphr.h"

#include  "Tasks/Task_ReportData.h"

#define DEBUG 0

// Access to current Sys Tick
extern volatile uint32_t xPortSysTickCount;

// Program Constants
// We operate at 120 MHz, which gives a period of 8.33 nS
// The equation 8.33nS * K * M = 10mS
// The LOAD_VALUE (M) must be < 64k, 50,000 chosen arbitrarily
// The PRE_SCALE_VALUE (K) must be < 256. Solving, K = 24
// Since K is zero indexed, K = 23
// We are only interested in memory <= 32KiB which is 2^15
const uint32_t PC_OFFSET       = 32;
const uint32_t LOAD_VALUE      = 50000;
const uint32_t PRE_SCALE_VALUE = 23;
const uint32_t MAX_ADDRESS     = 1<<15;  //32KiB

// Assembly function to get PC from the stack
extern uint32_t Get_Value_From_Stack( uint32_t );

// The semaphore
xSemaphoreHandle Timer_0_A_Semaphore;

// Histogram
uint32_t data[512];

// Pointers to keep our Report_Items alive
ReportData_Item* report_items[512];

// Extracted fro the ISR to keep stack items to a minimum
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

// The current memory address obtained from the PC
uint32_t Current_PC = 0;

// Status of our PC value. Controls pushing duplicate values
enum isr_status
{
    SHOULD_UPDATE,
    DO_NOT_UPDATE
} ISR_STATUS = DO_NOT_UPDATE;

//////////////////////////////////////////////////////////////////////////////
// Subroutines
//////////////////////////////////////////////////////////////////////////////
extern void Timer_0_A_ISR()
{
    TimerIntClear( TIMER0_BASE, TIMER_TIMA_TIMEOUT );

    // We have a new value to save to the histogram
    ISR_STATUS = SHOULD_UPDATE;

    // Get the value from the PC
    Current_PC = Get_Value_From_Stack( PC_OFFSET );

    //
    // "Give" the Timer_0_A_Semaphore
    //
    xSemaphoreGiveFromISR( Timer_0_A_Semaphore, &xHigherPriorityTaskWoken );
    //
    // If xHigherPriorityTaskWoken was set to true,
    // we should yield. The actual macro used here is
    // port specific.
    //
    if( xHigherPriorityTaskWoken )
    {
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

extern void Task_ProgramTrace( void* pvParameters )
{
    //Initialize Semaphore and setup Timer
    vSemaphoreCreateBinary( Timer_0_A_Semaphore );

    SysCtlPeripheralEnable( SYSCTL_PERIPH_TIMER0 );

    IntRegister( INT_TIMER0A, Timer_0_A_ISR );

    TimerConfigure( TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC );

    TimerPrescaleSet( TIMER0_BASE, TIMER_A, PRE_SCALE_VALUE );

    TimerLoadSet( TIMER0_BASE, TIMER_A, LOAD_VALUE );

    TimerIntEnable( TIMER0_BASE, TIMER_TIMA_TIMEOUT );

    //Enable Timer_0_A interrupt in NVIC
    IntEnable( INT_TIMER0A );

    // Enable (Start) Timer
    TimerEnable( TIMER0_BASE, TIMER_A );

    // Set data report to Excel format
    ReportData_SetOutputFormat( Excel_CSV );

    // Calculate a stopping time (one minute)
    uint32_t Current_Sys_Tick = xPortSysTickCount;
    uint32_t One_Second_Delta_Sys_Tick = 10000;
    uint32_t Stop_Sys_Tick = Current_Sys_Tick + ( 60 * One_Second_Delta_Sys_Tick );

#if DEBUG
    UARTprintf("BEGIN\n");
#endif

    // Init the data array
    uint32_t temp = 0;
    for( temp = 0; temp < 512; temp++ )
    {
        data[temp] = 0;
    }

    // Add values to the histogram when appropriate
    while ( xPortSysTickCount < Stop_Sys_Tick )
    {
        xSemaphoreTake( Timer_0_A_Semaphore, portMAX_DELAY );
        if( ISR_STATUS == SHOULD_UPDATE )
        {
            if( Current_PC > 0 && Current_PC < MAX_ADDRESS )
            {
                data[Current_PC >> 6]++;
            }

            #if DEBUG
                UARTprintf( "test %X %u %u \n", Current_PC, Current_PC, Current_PC >> 6 );
                if( xPortSysTickCount >= Next_Sys_Tick )
                {
                    UARTprintf("NEXT\n");
                    Next_Sys_Tick += One_Second_Delta_Sys_Tick;
                }
            #endif
            ISR_STATUS = DO_NOT_UPDATE;
        }
    }
    // Disable the timer and take the semaphore
    TimerDisable( TIMER0_BASE, TIMER_A );
    xSemaphoreTake( Timer_0_A_Semaphore, portMAX_DELAY );

    #if DEBUG
      UARTprintf("DONE COLLECTING DATA\n");
    #endif

    // Create Report_Items for the data
    uint32_t i = 0;
    for( i = 0; i < 512; i++ )
    {
      ReportData_Item* item = (ReportData_Item*)malloc( sizeof( ReportData_Item ) );
      item->TimeStamp = xPortSysTickCount;
      item->ReportName = 42;
      item->ReportValueType_Flg = 0x0;
      item->ReportValue_0 = i;
      item->ReportValue_1 = data[i];
      item->ReportValue_2 = 0;
      item->ReportValue_3 = 0;

      report_items[i] = item;

      #if DEBUG
        if( i % 20 == 0 )
        {
            UARTprintf( "%u\n", i );
        }
      #endif

      xQueueSend(  ReportData_Queue, item, 0 );
    }

    while(1)
    {
        //TRAP HERE
    }
}
