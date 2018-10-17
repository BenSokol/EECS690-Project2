/*--Task_ReportData.c
 *
 *  Author:			Gary J. Minden
 *	Organization:	KU/EECS/EECS 690
 *  Date:			March 13, 2016 (B60313)
 *
 *  Description:	Transmits data via UART to PC.
 *
 *  Modification:	2016-09-22 (B60922)
 *  				Changed number of value to 4 from 2.
 *
 *  Modification:
 *  Author:			Gary J. Minden
 *  Organization:		KU/EECS/EECS 690
 *  Date:			2017-09-17 (B70917)
 *  Description:	(1) Added a C_Format capability
 *  				(2) Added an output type specifier for int32_t or
 *  					float. The output type for a specific
 *  					value is determined from the corresponding
 *  					bit in the output type field, ReportType_Flg.
 *  					ReportType_Flg<i> determines the output type
 *  					for ReportValue_<i>. If the ReportType_Flg<i>
 *  					is '0', the output type is int32_t. If the
 *  					ReportType_Flg<i> is '1', the output type
 *  					is int32_t.
 *  				(3) Added a global subroutine to
 *  					set the output format
 *
 */

#include	<stddef.h>
#include	<stdbool.h>
#include	<stdint.h>
#include	<stdarg.h>

#include	"Drivers/UARTStdio_Initialization.h"
#include	"Drivers/uartstdio.h"
#include	"Tasks/Task_ReportData.h"

#include	<stdio.h>

#include	"FreeRTOS.h"
#include	"task.h"
#include	"queue.h"

//
//	Define the ReportData Queue
//
extern QueueHandle_t ReportData_Queue = NULL;

//
//	Define output format and subroutine to set output format.
//
ReportData_OutputFormat		ReportData_CurrentFormat = Excel_CSV;

extern void ReportData_SetOutputFormat( ReportData_OutputFormat newFormat ) {

	ReportData_CurrentFormat = newFormat;
}

//
//	Define the ReportData Task
//
#define		NbrValues			4
#define		FormattedStringSize	32

extern void Task_ReportData( void *pvParameters ) {

	ReportData_Item			theReport;
	BaseType_t				ReportQueue_Status;

	typedef			 	char	FormattedString_t[FormattedStringSize];
	FormattedString_t	FormattedStrings[NbrValues];
	uint32_t			Value_Idx;

	typedef			union ValueType { int32_t Integer; float Float; } ValueType_t;
	ValueType_t		Values[NbrValues];


	//
	//	Ensure UARTStdio is initialized
	//
	UARTStdio_Initialization();

	UARTprintf( ">>>>ReportData: Initializing.\n" );

	//
	//	Define ReportData_Queue
	//
	ReportData_Queue = xQueueCreate( 1024, sizeof( ReportData_Item ) );

	UARTprintf( ">>>>ReportData: Queue Handle: %p\n", ReportData_Queue );

	while ( 1 )	{


		//
		//	Try to read ReportItem from ReportData_Queue.
		//	If a ReportData_Item is returned, print the contents
		//		to the UART via UARTStdioPrintf
		//
		ReportQueue_Status = xQueueReceive( ReportData_Queue,
											&theReport,
											1 * portTICK_PERIOD_MS );

//		UARTprintf( ">>>>ReportData: Queue Receive: %d\n", ReportQueue_Status );


		if ( ReportQueue_Status == pdTRUE ) {

			//
			//	First, copy the values to the ValueType_t
			//	union. This is necessary to handle both
			//	int32_t and float values.
			//	Convert the values to strings.
			//
			Values[0].Integer = theReport.ReportValue_0;
			Values[1].Integer = theReport.ReportValue_1;
			Values[2].Integer = theReport.ReportValue_2;
			Values[3].Integer = theReport.ReportValue_3;

			for ( Value_Idx = 0; Value_Idx < NbrValues; Value_Idx++ ) {
				if ( theReport.ReportValueType_Flg & (1 << Value_Idx) ) {

					//
					//	Value type is float
					//
					sprintf( FormattedStrings[Value_Idx], "%+#8.3F", Values[Value_Idx].Float );
					} else {

					//
					//	Value type is int32_t
					//
					sprintf( FormattedStrings[Value_Idx], "%+#08d", Values[Value_Idx].Integer );
					}

			}


					switch ( ReportData_CurrentFormat ) {

						//
						//	Output in Excel Comma Separated format
						//
						case Excel_CSV:
							UARTprintf( "%08d,%04d,%s,%s,%s,%s\n",
										theReport.TimeStamp, theReport.ReportName,
										FormattedStrings[0], FormattedStrings[1],
										FormattedStrings[2], FormattedStrings[3] );
							break;

						//
						//	Output in Mathematica List format
						//
						case Mathematica_List:
							UARTprintf( "{ %08d, %04d, %s, %s, %s, %s },\n",
										theReport.TimeStamp, theReport.ReportName,
										FormattedStrings[0], FormattedStrings[1],
										FormattedStrings[2], FormattedStrings[3] );

							break;

						//
						//	Output in C white space format
						//
						case C_Format:
							UARTprintf( "%08d %04d %s %s %s %s\n",
										theReport.TimeStamp, theReport.ReportName,
										FormattedStrings[0], FormattedStrings[1],
										FormattedStrings[2], FormattedStrings[3] );
							break;

					}

				}

		vTaskDelay( 100 );

	}

}
