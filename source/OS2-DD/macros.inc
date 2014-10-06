;##############################################################################
; $Id: macros.inc,v 1.3 1992/07/24 11:36:45 ak Exp $
;##############################################################################
; $Log: macros.inc,v $
; Revision 1.3  1992/07/24  11:36:45  ak
; OS/2 2.0
; BASEDEV drivers
; VGA debugging
;
; Revision 1.2  1992/01/07  07:51:00  ak
; DevHlp function which return error state only
; now return byte instead of word. Corresponds
; to SETxx.
;
; Revision 1.1.1.1  1992/01/06  19:55:01  ak
; Alpha version.
;
; Revision 1.1  1992/01/06  19:55:00  ak
; Initial revision
;
;##############################################################################

regsave	macro	list, reg
	local	r
	irp	r, <list>
		ifidn	<r>, <reg>
			push	r
		endif
	endm
	endm

regrest	macro	list, reg
	local	r
	irp	r, <list>
		ifidn	<r>, <reg>
			pop	r
		endif
	endm
	endm

.enter	macro	regs
	push	bp
	mov	bp, sp
	regsave	<regs>, es
	regsave	<regs>, ds
	regsave	<regs>, bp
	regsave	<regs>, di
	regsave	<regs>, si
	endm

.return	macro	regs
	regrest	<regs>, si
	regrest	<regs>, di
	regrest	<regs>, bp
	regrest	<regs>, ds
	regrest	<regs>, es
	leave
	ret
	endm

.rnull	macro	rh, rl
	local	lbl
	mov	dx, rh
	mov	ax, rl
	jnc	short lbl
	sub	dx, dx
	sub	ax, ax
lbl:
	endm

.rzero	macro
	local	lbl
	jc	short lbl
	sub	ax, ax
lbl:
	endm

.rflag	macro
	setc	al
	endm

.rminus	macro
	local	lbl
	jnc	short lbl
	mov	ax, -1
lbl:
	endm

.devhlp	macro	name
	mov	dl, DevHlp_&name
	call	_devhlp
	endm

