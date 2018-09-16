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
#include    "inc/hw_sysctl.h"

#include    <stdlib.h>

#include	<stddef.h>
#include	<stdbool.h>
#include	<stdint.h>
#include	<stdarg.h>

#include	"driverlib/sysctl.h"
#include	"driverlib/pin_map.h"
#include    "driverlib/timer.h"
#include	"driverlib/gpio.h"

#include	"FreeRTOS.h"
#include	"task.h"
#include    "semphr.h"

#include    "Tasks/Task_ReportData.h"

#define DEBUG 0

extern volatile uint32_t xPortSysTickCount;

// Program Constants
const uint32_t PC_OFFSET       = 32;
const uint32_t LOAD_VALUE      = 50000;
const uint32_t PRE_SCALE_VALUE = 23;

const uint32_t MAX_ADDRESS     = 1<<15;  //32KiB

// 120mHz
//  (Period 8.33 nS * K) * M  = 10mS
//  M < 64k
//  1 < K < 256

//  1200000 = KM
//  M = 50,000  Load Value
//  K = 24      Pre-Scale (Should be one less than calculated)


extern uint32_t Get_Value_From_Stack( uint32_t );
xSemaphoreHandle Timer_0_A_Semaphore;
uint32_t data[512];
ReportData_Item* report_items[512];
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
uint32_t Current_PC = 0;
uint32_t Queue_Control = 0;


enum isr_status
{
    SHOULD_UPDATE,
    DO_NOT_UPDATE
} ISR_STATUS = DO_NOT_UPDATE;


extern void Timer_0_A_ISR()
{
    TimerIntClear( TIMER0_BASE, TIMER_TIMA_TIMEOUT );
    ISR_STATUS = SHOULD_UPDATE;
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
    /*if( xHigherPriorityTaskWoken )
    {
        //UARTprintf( "higher priority\n" );
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }*/
}

extern void Task_ProgramTrace( void* pvParameters ) {

	//const uint32_t	Interrupt_Frequency; //TODO

    //Initialize Semaphore
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

    ReportData_SetOutputFormat( Excel_CSV );
    uint32_t Current_Sys_Tick = xPortSysTickCount;
    uint32_t One_Second_Delta_Sys_Tick = 10000;
    uint32_t Stop_Sys_Tick = Current_Sys_Tick + ( 60 * One_Second_Delta_Sys_Tick );
    UARTprintf("BEGIN\n");

    uint32_t temp = 0;
    for(temp = 0; temp < 512; temp++)
    {
        data[temp] = 0;
    }

    while ( xPortSysTickCount < Stop_Sys_Tick  )
    {
        xSemaphoreTake( Timer_0_A_Semaphore, portMAX_DELAY );
        if( ISR_STATUS == SHOULD_UPDATE )
        {
            Queue_Control++;
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
    TimerDisable( TIMER0_BASE, TIMER_A );
    UARTprintf("DONE COLLECTING DATA\n");
    uint32_t i = 0;
    for( i = 0; i < 512; i++ )
    {
    ReportData_Item* item = (ReportData_Item*)malloc( sizeof( ReportData_Item ));
    item->TimeStamp = xPortSysTickCount;
    item->ReportName = 42;
    item->ReportValueType_Flg = 0x0;
    item->ReportValue_0 = i;
    item->ReportValue_1 = data[i];
    item->ReportValue_2 = 0;
    item->ReportValue_3 = 0;

    report_items[i] = item;

    if(i % 20 == 0)
    {
        UARTprintf("%u\n", i );
    }
    xQueueSend(  ReportData_Queue, item, 0 );

    }

    while(1)
    {
        //TRAP
    }

    //UARTprintf( "end\n" );
}



