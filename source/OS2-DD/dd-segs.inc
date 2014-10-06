;##############################################################################
; $Id: dd-segs.inc,v 1.2 1992/07/24 11:33:51 ak Exp $
;##############################################################################
; $Log: dd-segs.inc,v $
; Revision 1.2  1992/07/24  11:33:51  ak
; OS/2 2.0
; BASEDEV drivers
; VGA debugging
;
; Revision 1.1.1.1  1992/01/06  19:55:08  ak
; Alpha version.
;
; Revision 1.1  1992/01/06  19:55:07  ak
; Initial revision
;
;##############################################################################

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Data segments
;
_DATA	segment word public 'DATA' use16
_DATA	ends
;
;Function pointers to far constructors
XIFB	segment	word public 'DATA' use16
XIFB	ends
XIF	segment	word public 'DATA' use16
XIF	ends
XIFE	segment	word public 'DATA' use16
XIFE	ends
;Function pointers to near constructors
XIB	segment	word public 'DATA' use16
XIB	ends
XI	segment	word public 'DATA' use16
XI	ends
XIE	segment	word public 'DATA' use16
XIE	ends
;Function pointers to far destructors
XCFB	segment	word public 'DATA' use16
XCFB	ends
XCF	segment	word public 'DATA' use16
XCF	ends
XCFE	segment	word public 'DATA' use16
XCFE	ends
;Function pointers to near destructors
XCB	segment	word public 'DATA' use16
XCB	ends
XC	segment	word public 'DATA' use16
XC	ends
XCE	segment	word public 'DATA' use16
XCE	ends
;
CONST	segment word public 'CONST' use16
CONST	ends
UDATA	segment word public 'BSS' use16
UDATA	ends
_BSS	segment word public 'BSS' use16
_BSS	ends
;
DGROUP	group	_DATA,XIFB,XIF,XIFE,XIB,XI,XIE,XCB,XC,XCE,XCFB,XCF,XCFE,CONST,UDATA,_BSS

;
; Code segments
;
_TEXT	segment	word public 'CODE' use16
_TEXT	ends
ETEXT	segment	word public 'CODE' use16
ETEXT	ends
ITEXT	segment	word public 'CODE' use16
ITEXT	ends
;
CGROUP	GROUP	_TEXT,ETEXT,ITEXT

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

BegCode	macro	segm
segm	segment	word public 'CODE' use16
	endm

EndCode	macro	segm
segm	ends
	endm
	
BegData	macro	segm
segm	segment	word public 'DATA' use16
	endm

EndData	macro	segm
segm	ends
	endm
	

