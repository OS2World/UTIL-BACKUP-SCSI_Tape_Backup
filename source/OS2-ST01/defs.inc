	.386P			; Enable 80286 protect-mode instructions

;	.sall
	.xlist

;trace	equ	1
trace	equ	0

include dosmac.inc
include devsym.inc
include devhlp.inc
include error.inc

SegPointer struc
	_off	dw	?
	_seg	dw	?
SegPointer ends

DoubleWord struc
	_lo	dw	?
	_hi	dw	?
DoubleWord ends

DATA	SEGMENT WORD PUBLIC 'DATA' use16
DATA	ENDS
IDATA	SEGMENT WORD PUBLIC 'DATA' use16
IDATA	ENDS
CODE	SEGMENT WORD PUBLIC 'CODE' use16
CODE	ENDS
ICODE	SEGMENT WORD PUBLIC 'CODE' use16
ICODE	ENDS

CGROUP	GROUP	CODE, ICODE
DGROUP	GROUP	DATA, IDATA	

StartData macro
DATA	SEGMENT WORD PUBLIC 'DATA'
	endm
EndData	macro
DATA	ENDS
	endm
StartIData macro
IDATA	SEGMENT WORD PUBLIC 'DATA'
	endm
EndIData macro
IDATA	ENDS
	endm
StartCode macro
CODE	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:CODE,DS:DATA,ES:NOTHING,SS:NOTHING,FS:NOTHING,GS:NOTHING
	endm
EndCode	macro
CODE	ENDS
	endm
StartICode macro
ICODE	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:ICODE,DS:DATA,ES:NOTHING,SS:NOTHING,FS:NOTHING,GS:NOTHING
	endm
EndICode macro
ICODE	ENDS
	endm

DevHlp	macro	function
	mov	dl,function
	call	dword ptr ss:[bp]
	endm

	.list
