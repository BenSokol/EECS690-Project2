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
#include	"driverlib/gpio.h"

#include	"FreeRTOS.h"
#include	"task.h"
#include    "semphr.h"


extern uint32_t Float_to_Int32( float theFloat );

xSemaphoreHandle Timer_0_A_Semaphore;

extern void Timer_0_A_ISR( void *pvParameters )
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    //Clear interrupt and do something
    TimerIntClear( TIMER0_BASE, TIMER_TIMA_TIMEOUT );
    /////////////////////////////////////////////////////
    UARTStdio_Initializaiton();
    UARTprintf( "test %i", Float_to_Int32( 321.012 ) );

    /////////////////////////////////////////////////////

    //Give the Timer_0_A_Semaphore back
    xSemaphoreGiveFromISR( Timer_0_A_Semaphore, &xHigherPriorityTaskWoken );
}

extern void Task_ProgramTrace( void* pvParameters ) {

	const uint32_t	Interrupt_Frequency; //TODO


	while ( 1 ) {
        //
        // Periodically call interrupt
        //


		vTaskDelay( ( 500 * configTICK_RATE_HZ ) / 10000 );
	}
}



