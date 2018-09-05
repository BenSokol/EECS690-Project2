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

		.global		Float_to_Int32			; Declare entry point as a global symbol

;;	No constant data

;;	No variable allocation

;;	Program instructions

		.text								; Program section

Float_to_Int32:								; Entry point

;;
;;	Save necessary registers
;;
;;	Since this subroutine does not use registers other than R0,
;;	  and we do not call another subroutine, we don't need to save
;;	  any registers.
;;
;;	Allocate local variables on the Stack
;;
;;	Since this subroutine does not have local variables,
;;	  no Stack space need be allocated.
;;
;;	This subroutine returns the input argument in R0.

		;;ADDS R0,R15
;;
		MOV R0, PC
		BX			LR		; Branch to the program address in the Link Register
		;B          B3
        ;STW    B3, *A4
        ;NOP    4
		.end
