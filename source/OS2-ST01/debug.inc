	.xlist

StartData

    ifndef debug_def
	extrn	tracelvl:byte
    endif

EndData

StartCode

    ifndef debug_def
	extrn	out_char:near
	extrn	out_text:near
	extrn	out_byte:near
    endif

outtext		macro	level, text
	local	l1
    if	trace
	cmp	tracelvl, level
	jb	short l1
	call	out_text
	irp	param, <text>
	db	param
	endm
	db	0
l1:
    endif
		endm

outchar		macro	level, char
	local	l1
    if	trace
	cmp	tracelvl, level
	jb	short l1
    	push	ax
	mov	al, char
	call	out_char
	pop	ax
l1:
    endif
		endm

outbyte		macro	level, byte
	local	l1
    if	trace
	cmp	tracelvl, level
	jb	short l1
    	push	ax
	mov	al, byte
	call	out_byte
	pop	ax
l1:
    endif
		endm

outword		macro	level, word
	local	l1
    if	trace
	cmp	tracelvl, level
	jb	short l1
	push	ax
	mov	ax, word
	push	ax
	mov	al, ah
	call	out_byte
	pop	ax
	call	out_byte
	pop	ax
l1:
    endif
		endm

display		macro	x, y
	mov	al, x
	not	al
	mov	dx, 3BCh
	out	dx, al
		endm

EndCode

	.list
