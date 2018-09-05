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


extern int32_t Float_to_Int32();
xSemaphoreHandle Timer_0_A_Semaphore;
extern uint32_t data[256];



extern void Timer_0_A_ISR()
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    //Clear interrupt and do something
    TimerIntClear( TIMER0_BASE, TIMER_TIMA_TIMEOUT );
    /////////////////////////////////////////////////////
    //UARTprintf( "In the ISR!\n" );

    UARTprintf( "test %X \n", Float_to_Int32() );


    /////////////////////////////////////////////////////

    //Give the Timer_0_A_Semaphore back
    xSemaphoreGiveFromISR( Timer_0_A_Semaphore, &xHigherPriorityTaskWoken );
}

extern void Task_ProgramTrace( void* pvParameters ) {

	//const uint32_t	Interrupt_Frequency; //TODO

    //Initialize Semaphore
    vSemaphoreCreateBinary( Timer_0_A_Semaphore );

    SysCtlPeripheralEnable( SYSCTL_PERIPH_TIMER0 );

    IntRegister( INT_TIMER0A, Timer_0_A_ISR );

    TimerConfigure( TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC );

    TimerPrescaleSet( TIMER0_BASE, TIMER_A, 9 );

    TimerLoadSet( TIMER0_BASE, TIMER_A, 50000 );

    //Enable Timer_0_A interrupt in NVIC
    IntEnable( INT_TIMER0A );

    // Enable (Start) Timer
    TimerEnable( TIMER0_BASE, TIMER_A );

	while ( 1 ) {
        //
        // Periodically call interrupt
        //
	    xSemaphoreTake( Timer_0_A_Semaphore, portMAX_DELAY );


		//vTaskDelay( ( 500 * configTICK_RATE_HZ ) / 10000 );
	}
}



