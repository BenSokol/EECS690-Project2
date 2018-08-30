/*--Task_ReportData.h
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
 *  				(2) Added an output type, int32_t or
 *  					float.
 *  				(3) Added a global subroutine to
 *  					set the output format
 *
 */

#ifndef TASKS_TASK_REPORTDATA_H_
#define TASKS_TASK_REPORTDATA_H_

#include	<stddef.h>
#include	<stdbool.h>
#include	<stdint.h>
#include	<stdarg.h>

#include	"utils/uartstdio.h"		//	Access original *.h file 
									//	as the uartstdio.c file does

#include	"FreeRTOS.h"
#include	"task.h"
#include	"queue.h"


typedef enum { Excel_CSV, Mathematica_List, C_Format } ReportData_OutputFormat;

//
//	Define the ReportData Task subroutines
//
extern void Task_ReportData( void *pvParameters );
extern void ReportData_SetOutputFormat( ReportData_OutputFormat newOutputFormat );

//
//	Define the ReportData Queue
//
extern QueueHandle_t ReportData_Queue;
//
//	Define a structure to hold a data report
//
typedef struct  {	uint32_t				TimeStamp;
					uint32_t				ReportName;
					uint32_t				ReportValueType_Flg;
					int32_t					ReportValue_0;
					int32_t					ReportValue_1;
					int32_t					ReportValue_2;
					int32_t					ReportValue_3; } ReportData_Item;

#endif /* TASKS_TASK_REPORTDATA_H_ */
