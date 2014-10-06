	SUBTTL	Main Module
        INCLUDE DEFS.INC                    ; which includes some other .incs

	include	debug.inc

StartData

;====================================================
;
; Device Header -
;
;====================================================

TAPE_DEV LABEL WORD
	DD	-1			; SYSINIT handles the segment part
	DW	DEV_CHAR_DEV or DEV_PROTECT or DEV_OPEN or DEVLEV_2 or DEV_GIOCTL
					; Use equates in DEVHDR.INC
					; to define Attribute word
	DW	STRAT			; offset to strategy routine entry point
	DW	0			; IDC entry point
	DB	'TAPE$4  '		; Name of device
DPS	DB	8 DUP (?)		; More reserved

; End of the device header.
; 
;============================================================================

;
;  data structure used by this device driver
;

	EVEN
DISPTAB LABEL WORD

	DW	TAPE_INIT	;00: Init
	DW	TAPE_EXIT1	;01: Media Check (Block Devices Only)
	DW	TAPE_EXIT1	;02: Build BPB (Block Devices Only)
	DW	TAPE_EXIT1	;03: Reserved (used to be Ioctl Input)
	DW	TAPE_INPUT	;04: Input (Read)
	DW	TAPE_INPNOWAIT 	;05: Non-Destructive Input, no wait.
	DW	TAPE_INPSTATUS 	;06: Input Status
	DW	TAPE_INPFLUSH	;07: Input Flush
	DW	TAPE_OUTPUT	;08: Output (Write)
	DW	TAPE_OUTVERIFY	;09: Output (Write) with verify
	DW	TAPE_OUTSTATUS	;0A: Output Status
	DW	TAPE_OUTFLUSH	;0B: Output Flush
	DW	TAPE_EXIT1	;0C: Reserved (used to be Ioctl Output)
	DW	TAPE_OPEN	;0D: Device Open
	DW	TAPE_CLOSE	;0E: Device Close
	DW	TAPE_EXIT1	;0F: Removable Media
	DW	TAPE_GIOCTL	;10: Generic Ioctl
	DW	TAPE_EXIT1	;11: Reset Media
	DW	TAPE_EXIT1	;12: Get Logical Drive Map
	DW	TAPE_EXIT1	;13: Set Logical Drive Map
	DW	TAPE_EXIT1	;14: DeInstall
	DW	TAPE_EXIT1	;15: Port Access
	DW	TAPE_EXIT1	;16: Partitionable HDs
	DW	TAPE_EXIT1	;17: Logical Unit Mal
	DW	TAPE_EXIT1	;18: -
	DW	TAPE_EXIT1	;19: -
	DW	TAPE_EXIT1	;1A: -
	DW	TAPE_EXIT1	;1B: -

MAXCMD = (($ - DispTab)/2) - 1

	public	DevHlpPtr

		even
DevHlpPtr	dd	0
ReqPkt		dd	0

waiting		db	0	;someone is waiting
lock_flag	db	0	;[0]: parm, [1]: data

		even
para_handle	dd	0	;ioctl param handle
data_handle	dd	0	;ioctl data handle
data_ptr	dd	0	;data pointer

cmd_read	db	08h, 01h, 0, 0,  0, 0
cmd_write	db	0Ah, 01h, 0, 0,  0, 0
cmd_sense	db	03h, 00h, 0, 0, 16, 0
sense_data	db	16 dup(?)
cmd_block	db	10 dup(?)
sense_data_ptr	dd	offset sense_data

    if SCSI_LVL
errcode		equ	12	;SCSI standard
    else
errcode		equ	14	;Tandberg
    endif

EndData

StartIData
	public	end_data
end_data	db	0
EndIData

StartICode
	public	end_code
end_code	db	0
EndICode

StartCode

	extrn	command:near
	extrn	bus_reset:near
	extrn	device_reset:near
	extrn	delay:near

page
;============================================================================
;
; - TAPE_STRAT -
;
;	Device Driver strategy entry point.
;	Parameters	- es:bx = Pointer to the request block
;

STRAT	PROC	FAR
	push	DevHlpPtr		;Make DevHlp callable w/o using DS/ES
	mov	bp, sp

	mov	ax, es
	mov	fs, ax
	mov	di, bx

	cmp	fs:[di].PktCmd, 0	;INIT command? Debug is not yet
	je	short str_1		;initialized.

	outtext	2, 'Strategy '
	outbyte	2, fs:[di].PktCmd
	outchar	2, '$'

str_loop:
	cmp	ReqPkt._seg, 0		;Driver busy?
	jne	str_busy

str_1:	mov	ReqPkt._off, di		;Save the bimodal address
	mov	ReqPkt._seg, fs		;of the request packet.
	movzx	eax, fs:[di].PktCmd	;Get command from packet and execute.
	cmp	al, MAXCMD
	ja	str_bad
	call	DISPTAB[eax*2]

str_x:	xor	ax, STDONE		;Mark operation complete.
	mov	fs:[di].PktStatus, ax

	outtext	2, 'Strategy return '
	outword	2, ax
	outchar	2, '$'

	cmp	waiting, 0		;Release waiting threads.
	je	short str_x1
	outtext	3, 'Wakeup waiting driver$'
	mov	ax, ReqPkt._seg
	mov	bx, ReqPkt._off
	mov	ReqPkt._seg, 0
	DevHlp	DevHlp_Run
	jmp	short str_r
str_x1:	mov	ReqPkt._seg, 0
str_r:	lea	sp, [bp+4]
	ret

str_busy:				;Driver is busy. Wait.
	outtext	2, 'Driver busy$'
	inc	waiting
	push	fs
	push	di
	mov	ax, ReqPkt._seg		;Use bimodal pointer of running
	mov	bx, ReqPkt._off		;request packet as event ID.
	mov	di, 0			;No timeout.
	mov	cx, 1000
	mov	dh, 0			;Interruptible.
	DevHlp	DevHlp_Block
	pop	di
	pop	fs
	pushf
	dec	waiting
	popf
	jnc	str_loop		;Retry on event or timeout wakeup.
	jz	str_loop

	mov	ax, STDONE+STERR+11h	;Character I/O call interrupted.
	mov	fs:[di].PktStatus, ax
	jmp	short str_r

str_bad:
	mov	ax, STERR+03h		;Unknown command.
	jmp	str_x

STRAT	    ENDP

TAPE_EXIT1	proc near
	mov	ax, STERR+03h		;Unknown command.
	ret
TAPE_EXIT1	endp

;====================================================
;
; Operations
; ----------
;
; In:	FS:DI	= ptr to request block
; Out:	AX	= exit code (incl. DONE)
;
;====================================================

TAPE_INPUT	proc near	;04: Input (Read)
	smsw	ax			;Protected mode only.
	test	al, 1
	jz	short terr_1
	mov	dx, offset cmd_read
	jmp	rw_command
terr_1:	mov	ax, STERR+01h		;Unknown unit.
	ret
TAPE_INPUT	endp

TAPE_INPNOWAIT	proc near	;05: Non-Destructive Input, no wait.
	mov	ax, STBUSY	;busy = none avail
	ret
TAPE_INPNOWAIT	endp

TAPE_INPSTATUS	proc near	;06: Input Status
	mov	ax, STBUSY	;busy = none avail
	ret
TAPE_INPSTATUS	endp

TAPE_INPFLUSH	proc near	;07: Input Flush
	sub	ax, ax
	ret
TAPE_INPFLUSH	endp

TAPE_OUTPUT	proc near	;08: Output (Write)
	smsw	ax			;Protected mode only.
	test	al, 1
	jz	short terr_1
	mov	dx, offset cmd_write
	jmp	rw_command
TAPE_OUTPUT	endp

TAPE_OUTVERIFY	proc near	;09: Output (Write) with verify
	smsw	ax			;Protected mode only.
	test	al, 1
	jz	short terr_1
	mov	dx, offset cmd_write
	jmp	rw_command
TAPE_OUTVERIFY	endp

TAPE_OUTSTATUS	proc near	;0A: Output Status
	sub	ax, ax		;no wait
	cmp	waiting, 0
	je	short ost_r
	mov	ax, STBUSY	;busy + done = wait required
ost_r:	ret
TAPE_OUTSTATUS	endp

TAPE_OUTFLUSH	proc near	;0B: Output Flush
	sub	ax, ax
	ret
TAPE_OUTFLUSH	endp

TAPE_OPEN	proc near	;0D: Device Open
	smsw	ax		;Protected mode only.
	test	al, 1
	jz	short terr_1
	sub	ax, ax
	ret
TAPE_OPEN	endp

TAPE_CLOSE	proc near	;0E: Device Close
	sub	ax, ax
	ret
TAPE_CLOSE	endp

;====================================================
;
; Generic IOCtl
;
;====================================================

TAPE_GIOCTL	proc near	;10: Generic Ioctl
	push	si

	outtext	2, 'IOCtl '
	outbyte	2, fs:[di].GIOCategory
	outchar	2, ' '
	outbyte	2, fs:[di].GIOFunction
	outchar	2, '$'

	mov	lock_flag, 0

	movzx	eax, fs:[di].GIOFunction
	cmp	fs:[di].GIOCategory, 80h	;Own IOCtl codes?
	je	short gio_cat_own
	cmp	fs:[di].GIOCategory, 11		;general IOCtl codes?
	je	short gio_cat_11

gio_bad:
	test	fs:[di].GIOFunction, 80h	;Ignore invalid command?
	jnz	short gio_r0
	mov	ax, STERR+03h			;No, unknown command.
	jmp	short gio_r

gio_cat_11:				;Category 11: general device control.
	cmp	al, 01h			;Flush input.
	je	short gio_r0
	cmp	al, 02h			;Flush output.
	je	short gio_r0
	cmp	ax, 41h			;Session switch notification?
	je	short gio_r0
	cmp	ax, 60h			;Query monitor support?
	mov	ax, STERR+12h		;Monitor calls not supported.
	je	short gio_r
	jmp	gio_bad

	even
giotab	dw	gio_scsi_slow		;0
	dw	gio_scsi_fast		;1
	dw	gio_bus_reset		;2
	dw	gio_dev_reset		;3
	dw	gio_set_trace		;4

gio_cat_own:				;Own IOCtl commands.
	and	al, 1Fh
	cmp	al, 04h
	ja	short gio_bad
	jmp	giotab[eax*2]

gio_scsi_slow:				;SCSI command - slow transfer.
	mov	al, 0
	call	gen_scsi_cmd
	jmp	short gio_r

gio_scsi_fast:				;SCSI command - fast transfer.
	mov	al, 1
	call	gen_scsi_cmd
	jmp	short gio_r

gio_bus_reset:				;SCSI bus reset.
	call	bus_reset
	jc	short gio_r
	jmp	short gio_r0

gio_dev_reset:				;SCSI device reset.
	call	device_reset
	jc	short gio_r
	jmp	short gio_r0

gio_set_trace:
    if	trace
	push	di
	mov	ax, fs:[di].GIOParaPack._seg
	mov	di, fs:[di].GIOParaPack._off
	mov	cx, 1
	mov	dl, 0			;read-only
	DevHlp	DevHlp_VerifyAccess
	pop	di
	jc	gio_seg
	les	bx, fs:[di].GIOParaPack
	mov	al, es:[bx]
	mov	tracelvl, al
    endif
	jmp	short gio_r0

gio_r0:	sub	ax, ax
gio_r:	push	ax

	test	lock_flag, 1		;Unlock parameter and data segment.
	jz	short gio_r1
	mov	ax, para_handle._hi
	mov	bx, para_handle._lo
	DevHlp	DevHlp_Unlock
gio_r1:
	test	lock_flag, 2
	jz	short gio_r2
	mov	ax, data_handle._hi
	mov	bx, data_handle._lo
	DevHlp	DevHlp_Unlock
gio_r2:
	mov	lock_flag, 0
	pop	ax
	pop	si
	ret

gio_seg:mov	ax, STERR+13h		;iInvalid parameter.
	jmp	short gio_r
TAPE_GIOCTL	endp

;====================================================
;
; Command Subroutines
;
;====================================================

;
; Request sense
;
; AX = device driver error status
; CX = number from information bytes
;
req_sense	proc near
	push	si

	mov	al, 0				;Execute "request sense"
	mov	ebx, sense_data_ptr
	mov	cx, 16
	mov	dx, offset cmd_sense
	call	command
	jc	reqs_no_stat
	cmp	cx, 4
	jb	reqs_no_stat

    if	trace ge 1
	outtext	1, 'Sense data:'
	mov	si, offset sense_data
reqs_0:		lodsb
		outchar	1, ' '
		outbyte	1, al
		loop	reqs_0
	outchar	1, '$'
    endif

	mov	al, sense_data+0		;Check for unexpected
	and	al, 7Fh				;small sense block
	cmp	al, 70h
	jne	short reqs_short

	cmp	sense_data+errcode, 0Ah		;Insufficient Capacity?
	jne	short reqs_std
	mov	al, sense_data+2		;EOM set but sense key
	and	al, 4Fh				;set to "NO SENSE"?
	cmp	al, 40h
	je	short reqs_ew

reqs_std:
	mov	al, sense_data+errcode		;Translate error class & code.
	jmp	short reqs_scan
reqs_short:
	mov	al, sense_data+0
reqs_scan:
	and	al, 7Fh
	mov	bx, offset reqs_t
	mov	cx, reqs_tlen
reqs_1:		mov	ah, cs:[bx+0]
		mov	dx, cs:[bx+1]
		add	bx, 3
		cmp	al, ah
		loopnz	reqs_1
	jnz	short reqs_unknown
	mov	ax, dx

reqs_r:	mov	cx, word ptr sense_data+5
	xchg	ch, cl
	pop	si
	ret

reqs_unknown:
	mov	ax, STERR+07h			;Any other: unknown media.
	jmp	reqs_r

reqs_no_stat:
	mov	ax, STERR+07h			;No sense block
	mov	cx, 0FFFFh
	pop	si
	ret

reqs_ew:					;Early EOT indication
	outtext	1, 'Early Warning$'
	mov	ax, STERR+09h			;"Out of paper" :-)
	jmp	short reqs_r

sxlat	macro	sense, system
	db	sense
	dw	system
	endm

reqs_t	label	byte
	sxlat 00h, 0	     ;no sense			no error
	sxlat 02h, STERR+02h ;hardware error		drive not ready	
	sxlat 04h, STERR+02h ;drive not ready		drive not ready
	sxlat 09h, STERR+02h ;media not loaded		drive not ready
	sxlat 0Ah, STERR+0Ah ;insufficient capacity	write fault
	sxlat 11h, STERR+04h ;uncorrectable data error	CRC error
	sxlat 14h, STERR+08h ;no record found		sector not found
	sxlat 17h, STERR+00h ;write protected		write protect violation
	sxlat 19h, STERR+04h ;bad block found		CRC error
	sxlat 1Ch, 0         ;file mark detected	EOF
	sxlat 1Dh, STERR+04h ;compare error		CRC error
	sxlat 20h, STERR+03h ;invalid command		unknown command
	sxlat 30h, STERR+0Dh ;unit attention		change disk
	sxlat 33h, STERR+0Ah ;append error		write fault
	sxlat 34h, STERR+0Bh ;read EOM			read fault
reqs_tlen	equ	($ - offset reqs_t) / 3

req_sense	endp

;
; Generic R/W command
; DX = command record
;
rw_command	proc near
	push	si

	outtext	3, 'R/W '
	outword	3, fs:[di].IOpData._hi
	outword	3, fs:[di].IOpData._lo
	outchar	3, ' '
	outword	3, fs:[di].IOcount
	outchar	3, '$'

	push	dx			;Make virtual address
	mov	ax, fs:[di].IOpData._hi
	mov	bx, fs:[di].IOpData._lo
	mov	cx, fs:[di].IOcount
	mov	si, data_ptr._seg
	DevHlp	DevHlp_PhysToGDTSel
	pop	dx
	mov	ax, STERR+13h		;Invalid parameter.
	jc	rw_e

	test	cx, 01FFh		;Verify block alignment
	mov	ax, STERR+13h		;Invalid parameter.
	stc
	jnz	short rw_e

	mov	al, ch			;Insert block count into command.
	shr	al, 1
	mov	bx, dx
	mov	[bx+4], al

	mov	al, 1			;Run the command.
	mov	ebx, data_ptr
	call	command
	jc	short rw_ret
	cmp	dl, 00h
	je	short rw_ret

	stc
	cmp	dl, 02h			;Check status?
	mov	ax, STERR+02h		;No -> not ready.
	jne	short rw_ret
	call	req_sense		;Do check status.
	shl	cx, 9			;Convert number of blocks not handled
	sub	fs:[di].IOcount, cx	;to number of bytes actually xferred.
	pop	si
	ret

rw_ret:	mov	fs:[di].IOcount, cx	;Number of bytes actually transferred.
	pop	si
	ret

rw_e:	mov	fs:[di].IOcount, 0
	pop	si
	ret
rw_command	endp

;
; Verify and lock ioctl parameter block.
;
vfy_para	proc near
	cmp	fs:[di].GIOParaLength, 0
	je	vfyp_1
	and	lock_flag, not 1
	mov	ax, fs:[di].GIOParaPack._seg
	mov	bh, 0				;Lock, short-term, any memory.
	mov	bl, 0				;Block until available.
	DevHlp	DevHlp_Lock
	jc	short vfyp_1
	mov	para_handle._hi, ax
	mov	para_handle._lo, bx
	or	lock_flag, 1
	push	di
	mov	cx, fs:[di].GIOParaLength	;Verify access.
	mov	ax, fs:[di].GIOParaPack._seg
	mov	di, fs:[di].GIOParaPack._off
	mov	dh, 0				;Read-only.
	DevHlp	DevHlp_VerifyAccess
	pop	di
vfyp_1:	ret
vfy_para	endp

;
; Verify and lock ioctl data block.
; The data block can be NULL.
;
vfy_data	proc near
	cmp	fs:[di].GIODataLength, 0
	je	vfyd_1
	and	lock_flag, not 2
	movzx	eax, fs:[di].GIODataLength	;NULL pointer & length?
	or	eax, fs:[di].GIODataPack
	jz	short vfyd_1
	mov	ax, fs:[di].GIODataPack._seg
	mov	bh, 0				;Lock, short-term, any memory.
	mov	bl, 0				;Block until available.
	DevHlp	DevHlp_Lock
	jc	short vfyd_1
	mov	data_handle._hi, ax
	mov	data_handle._lo, bx
	or	lock_flag, 2
	push	di
	mov	cx, fs:[di].GIODataLength	;Verify access.
	mov	ax, fs:[di].GIODataPack._seg
	mov	di, fs:[di].GIODataPack._off
	mov	dh, 1				;Read+write.
	DevHlp	DevHlp_VerifyAccess
	pop	di
vfyd_1:	ret
vfy_data	endp

;
; Generic IOCTL,  Subfunction "general SCSI command"
; AL = 0 for slow, 1 for fast xfer
;
gen_scsi_cmd	proc near
	push	ax				;Slow/fast flag.

	call	vfy_para
	jc	short gen_e1
	call	vfy_data
	jc	short gen_e1

	mov	cx, fs:[di].GIOParaLength	;Copy command to data segment.
	cmp	cx, 10
	ja	short gen_e1
	push	di
	push	si
	lgs	si, fs:[di].GIOParaPack
	mov	ax, ds
	mov	es, ax
	mov	di, offset cmd_block
	cld
	rep	movs byte ptr es:[di], byte ptr gs:[si]
	pop	si
	pop	di

	pop	ax				;Slow/fast flag
	mov	ebx, fs:[di].GIODataPack	;Data pointer
	mov	cx, fs:[di].GIODataLength
	mov	dx, offset cmd_block		;Execute command
	call	command
	jc	short gen_r
	cmp	dl, 0				;Is ok, test SCSI status byte.
	je	short gen_r
	mov	al, 80h				;Not ok, return SCSI status byte.
	or	al, dl
	jmp	short gen_r

gen_e1:	add	sp, 2
gen_e:	mov	ax, STERR+13h			;Invalid parameters.
gen_r:	ret
gen_scsi_cmd	endp

;====================================================
;
; Initialization
;
;====================================================

	extrn	init_io:near
	extrn	init_debug:near

TAPE_INIT	proc near	;00: Init
	mov	eax, fs:[di].InitDevHlp	;Set device helper vector.
	mov	DevHlpPtr, eax
	mov	ss:[bp], eax

	mov	sense_data_ptr._seg, ds	;Fixup.

	push	di			;Allocate selector for data pointer.
	mov	ax, ds
	mov	es, ax
	mov	di, offset data_ptr._seg
	mov	cx, 1
	DevHlp	DevHlp_AllocGDTSel
	pop	di
	jc	init_e

;	les	bx, fs:[di].InitParms
;	FIXME: Interpret command line.

	call	init_debug
	jc	init_e

	call	init_io
	jc	init_e

 	outtext	2, 'ST_Tape Init done$'

init_r:	mov	fs:[di].InitcUnit, 0
	mov	fs:[di].InitEcode, offset end_code
	mov	fs:[di].InitEdata, offset end_data
	mov	fs:[di].InitpBPB, 0
	sub	ax, ax
	ret

init_e:	mov	fs:[di].InitcUnit, 0
	mov	fs:[di].InitEcode, 0
	mov	fs:[di].InitEdata, 0
	mov	ax, STERR+0Ch
	ret
TAPE_INIT	endp

EndCode

	end
