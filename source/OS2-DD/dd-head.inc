;##############################################################################
; $Id: dd-head.inc,v 1.3 1992/07/24 11:33:41 ak Exp $
;##############################################################################
; $Log: dd-head.inc,v $
; Revision 1.3  1992/07/24  11:33:41  ak
; OS/2 2.0
; BASEDEV drivers
; VGA debugging
;
; Revision 1.2  1992/01/06  20:10:04  ak
; *** empty log message ***
;
; Revision 1.1.1.1  1992/01/06  19:54:51  ak
; Alpha version.
;
; Revision 1.1  1992/01/06  19:54:50  ak
; Initial revision
;
;##############################################################################

	.seq
	.386p

	public	_header
	public	_end_code
	extrn	_strategy:near

	include	dd-segs.inc

ETEXT	segment	word public 'CODE' use16
_end_code	label byte
ETEXT	ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BegData	_DATA

CHR	= 8000H			; Bit set if character device
IDC	= 4000H			; Inter DD communications enabled
NIBM	= 2000H			; Bit set if NON-IBM Block Format
SHARE	= 1000H			; Set to support shrd dev. access
RM	= 0800H			; Set if Removable Media (Blk)
OPN	= 0800H			; Set if Device Open/Close (Char)
OS2L2	= 0100H			; OS/2 Function Level DD with IOCtl2
OS2L1	= 0080H			; OS/2 Function Level DD
CLK	= 0008H			; Clock Device
NUL	= 0004H			; Null Device
SCR	= 0002H			; Std Output Device (SCREEN)
KBD	= 0001H			; Std Input Device (KEYBOARD)

_header	dd	-1		; Pointer to next dd header
	dw	DD_ATTRIB	; Driver attributes
	dw	offset strategy	; Point to Strategy Routine
	dw	0		; Point to IDC Routine
	db	DD_NAME 	; Name Field (Must be 8 bytes)
	dq	0		; Reserved for OS/2

EndData	_DATA

BegCode _TEXT

strategy proc	far
	push	es
	push	bx
	call	_strategy
	add	sp, 4
	ret
strategy endp

EndCode _TEXT
