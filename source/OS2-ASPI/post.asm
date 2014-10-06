;##############################################################################
; $Id: post.asm,v 2.1 1992/11/14 21:00:20 ak Exp $
;##############################################################################
; $Log: post.asm,v $
; Revision 2.1  1992/11/14  21:00:20  ak
; OS/2 2.00.1 ASPI
;
; Revision 1.1.1.1  1992/01/06  20:16:24  ak
; Alpha version.
;
; Revision 1.1  1992/01/06  20:16:22  ak
; Initial revision
;
;##############################################################################
	.386p

	include	dd-segs.inc

	extrn	_post:near
	public	_postEntry

BegCode	_TEXT

	assume	cs:CGROUP, ds:DGROUP, es:nothing, ss:nothing

_postEntry proc far
	push	bp
	mov	bp, sp
	pusha
	push	es		;ASPI spec
	mov	ds, [bp+6]
	push	dword ptr [bp+8]
	call	_post
	add	sp, 4
	pop	es		;ASPI spec
	popa
	pop	bp
	ret
_postEntry endp

EndCode	_TEXT

	end
