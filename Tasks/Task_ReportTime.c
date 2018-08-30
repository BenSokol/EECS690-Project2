/*--Task_ReportTime.c
 *
 *  Author:			Gary J. Minden
 *	Organization:	KU/EECS/EECS 388
 *  Date:			February 23, 2016
 *
 *  Description:	Reports SysTickCount via printf.
 *
 */


#include	<stddef.h>
#include	<stdbool.h>
#include	<stdint.h>
#include	<stdarg.h>

#include	"Tasks/Task_ReportData.h"

#include	"FreeRTOS.h"
#include	"task.h"

//
//	External references
//
extern volatile uint32_t xPortSysTickCount;

extern int32_t Float_to_Int32( float theFloat );

extern void Task_ReportTime( void *pvParameters ) {

	ReportData_Item		theTimeReport;

	//
	//	No set-up necessary
	//

	//
	//	Delay a while to let ReportData_Queue to initialize.
	//	This is a hack.
	//
	vTaskDelay( 2 * configTICK_RATE_HZ );

	//
	//	Enter task loop
	//
	while ( 1 ) {

		theTimeReport.TimeStamp = xPortSysTickCount;
		theTimeReport.ReportName = 1;
		theTimeReport.ReportValueType_Flg = 0x05;
		theTimeReport.ReportValue_0 = Float_to_Int32( -55.67 );
		theTimeReport.ReportValue_1 = 98760;
		theTimeReport.ReportValue_2 = Float_to_Int32( 100.450 );
		theTimeReport.ReportValue_3 = -65430;

		xQueueSend(  ReportData_Queue, &theTimeReport, 0 );

		vTaskDelay( 2 * configTICK_RATE_HZ );
	}
}
