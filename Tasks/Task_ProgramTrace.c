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

#define DEBUG 1
#define PC_OFFSET 32
#define LOAD_VALUE 50000
#define PRE_SCALE_VALUE 23

// 120mHz
//  (Period 8.33 nS * K) * M  = 10mS
//  M < 64k
//  1 < K < 256

//  1200000 = KM
//  M = 50,000  Load Value
//  K = 24      Pre-Scale (Should be one less than calculated)


extern int32_t Get_Value_From_Stack( int32_t );
xSemaphoreHandle Timer_0_A_Semaphore;
extern uint32_t data[256];



extern void Timer_0_A_ISR()
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    TimerIntClear( TIMER0_BASE, TIMER_TIMA_TIMEOUT );

    uint32_t pc = Get_Value_From_Stack( PC_OFFSET );
#if DEBUG
    UARTprintf( "test %X %u \n", pc, pc );
#endif
    //
    // "Give" the Timer_0_A_Semaphore
    //
    xSemaphoreGiveFromISR( Timer_0_A_Semaphore,
    &xHigherPriorityTaskWoken );
    //
    // If xHigherPriorityTaskWoken was set to true,
    // we should yield. The actual macro used here is
    // port specific.
    //
    if( xHigherPriorityTaskWoken )
    {
        //UARTprintf( "higher priority\n" );
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

extern void Task_ProgramTrace( void* pvParameters ) {

	//const uint32_t	Interrupt_Frequency; //TODO

    //Initialize Semaphore
    vSemaphoreCreateBinary( Timer_0_A_Semaphore );

    SysCtlPeripheralEnable( SYSCTL_PERIPH_TIMER0 );

    IntRegister( INT_TIMER0A, Timer_0_A_ISR );

    TimerConfigure( TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC );

    TimerPrescaleSet( TIMER0_BASE, TIMER_A, 23 );

    TimerLoadSet( TIMER0_BASE, TIMER_A, 50000 );
    TimerIntEnable( TIMER0_BASE, TIMER_TIMA_TIMEOUT );
    //Enable Timer_0_A interrupt in NVIC
    IntEnable( INT_TIMER0A );

    // Enable (Start) Timer
    TimerEnable( TIMER0_BASE, TIMER_A );

    while (1)
    {
        xSemaphoreTake( Timer_0_A_Semaphore, portMAX_DELAY );
    }

    UARTprintf( "end\n" );
}



