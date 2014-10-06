;##############################################################################
; $Id: header.asm,v 1.1.1.1 1992/09/02 18:58:23 ak Exp $
;##############################################################################
; $Log: header.asm,v $
; Revision 1.1.1.1  1992/09/02  18:58:23  ak
; Initial checkin of OS/2 ADD based SCSI tape device driver.
;
; Revision 1.1  1992/09/02  18:58:21  ak
; Initial revision
;
;##############################################################################
DD_ATTRIB	equ	OS2L2+IDC+SHARE+OPN+CHR
DD_NAME		equ	'????????'

include dd-head.inc

	end
