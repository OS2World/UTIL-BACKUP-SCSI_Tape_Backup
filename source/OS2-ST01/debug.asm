	SUBTTL	Debug Module
        INCLUDE DEFS.INC

	debug_def equ 1
	include	debug.inc

ScreenH	equ	0Bh
ScreenL	equ	0000h

;;;;;;;;;;;;;;;;;;;;;;;;;;;;

StartData

	public	tracelvl

line		dw	0
col		dw	0
hex		db	'0123456789ABCDEF'
tracelvl	db	1

EndData

;;;;;;;;;;;;;;;;;;;;;;;;;;;;

StartCode

	public	out_char
	public	out_text
	public	out_byte
	public	init_debug

out_char	proc near
    if	trace
	push	gs
	push	fs
	push	es
	pusha

	push	ax

	mov	ax, ScreenH			;Screen -> ES:DI
	mov	bx, ScreenL
	mov	cx, 25 * 80 * 2
	mov	dh, 1
	DevHlp	DevHlp_PhysToVirt
	jc	short oc_f

	pop	ax
	
	cmp	al, '$'
	je	short oc_lf
	cmp	al, 0Ah
	jne	short oc_nrm

oc_lf:	mov	col, 0				;LF = CR-LF
	inc	line
	cmp	line, 25
	jne	short oc_r
	dec	line
	lea	si, [di + (80 * 2)]		;Scroll
	mov	cx, 24 * 80
	cld
	rep movs word ptr es:[di], word ptr es:[si]
	mov	cx, 80
	mov	ax, 0720h
	rep stos word ptr es:[di]
	jmp	short oc_r

oc_nrm:	imul	bx, line, 80
	add	bx, col
	shl	bx, 1
	mov	ah, 07h
	mov	es:[di+bx], ax
	cmp	col, 79
	je	short oc_r
	inc	col

oc_r:	DevHlp	DevHlp_UnPhysToVirt

oc_f:	popa
	pop	es
	pop	fs
	pop	gs
    endif
	ret
out_char	endp


out_text	proc near
    if	trace
	xchg	si, [esp]
	push	ax
ot1:		lods	byte ptr cs:[si]
		cmp	al, 0
		je	ot2
		call	out_char
		jmp	ot1
ot2:	pop	ax
	xchg	si, [esp]
    endif
	ret
out_text	endp


out_byte	proc near
    if	trace
	push	bx
	mov	ah, al
	shr	al, 4
	mov	bx, offset hex
	xlat
	call	out_char
	mov	al, ah
	and	al, 0Fh
	xlat
	call	out_char
	pop	bx
    endif
	ret
out_byte	endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;

init_debug	proc near
	outtext	2, 'DBG init$'
init_r:	clc
	ret
init_debug	endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;

EndCode

	end
