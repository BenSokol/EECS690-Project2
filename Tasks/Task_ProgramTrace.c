/*--Task_ProgramTrace.c
 *
 *  Author:			Kaiser Mittenburg, Ben Sokol
 *	Organization:	KU/EECS/EECS 690
 *  Date:			August 30, 2018
 *
 *  Description:	Periodically traces current program memory location
 *
 */

#include	"inc/hw_ints.h"
#include	"inc/hw_memmap.h"
#include	"inc/hw_types.h"
#include	"inc/hw_uart.h"

#include	<stddef.h>
#include	<stdbool.h>
#include	<stdint.h>
#include	<stdarg.h>

#include	"driverlib/sysctl.h"
#include	"driverlib/pin_map.h"
#include	"driverlib/gpio.h"

#include	"FreeRTOS.h"
#include	"task.h"

extern void Task_ProgramTrace( void* pvParameters ) {

	const uint32_t	Interrupt_Frequency; //TODO


	while ( 1 ) {
        //
        // Periodically call interrupt
        //


		vTaskDelay( ( 500 * configTICK_RATE_HZ ) / 10000 );
	}
}



