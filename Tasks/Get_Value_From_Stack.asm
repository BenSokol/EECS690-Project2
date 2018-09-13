;;*****************************************************************************
;;
;;	Get_Value_From_Stack.asm
;;
;;		Author: 		Kaiser Mittenburg, Ben Sokol
;;		Organization:	KU/EECS/EECS 690
;;		Date:			2018-09-11
;;		Version:		1.0
;;
;;		Purpose:		Return the value from the stack given an offset
;;
;;		Notes:
;;
;;*****************************************************************************



;;

;;	Declare sections and external references

		.global		Get_Value_From_Stack	; Declare entry point as a global symbol

;;	No constant data

;;	No variable allocation

;;	Program instructions

		.text								; Program section

Get_Value_From_Stack:						; Entry point

		LDR		R0,[SP,R0]
		BX		LR
		.end
