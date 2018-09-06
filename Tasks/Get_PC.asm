;;*****************************************************************************
;;
;;	Float_to_Int32.asm
;;
;;		Author: 		Gary J. Minden
;;		Organization:	KU/EECS/EECS 690
;;		Date:			2017-10-06 (B71006)
;;		Version:		1.0
;;
;;		Purpose:		Copy float bits to int32_t.
;;
;;		Notes:
;;
;;*****************************************************************************
;;

;;
;;	This subroutine computes a value based on one input arguement.
;;	The arguments is assumed to be in CPU registes R0
;;	  (aka A1).
;;	The result is returned in R0.
;;

;;	Declare sections and external references

		.global		Get_PC			; Declare entry point as a global symbol

;;	No constant data

;;	No variable allocation

;;	Program instructions

		.text								; Program section

Get_PC:								; Entry point

		MOV R0, PC
		BX			LR		; Branch to the program address in the Link Register
		.end
