	SUBTTL	ST01 Module
        INCLUDE DEFS.INC
	include	sysinfo.inc

	include	debug.inc

EnableFast	equ	1	;Fast transfer mode
EnableBlock	equ	0	;Block transfer mode (not for 486)
BlockSize	equ	512	;Max block transfer size
WrtWait		equ	12	;Wait loop at fast write

;
; Register conventions:
;
; DS:	data segment
; ES:	available, not preserved
; FS:	port segment, preserved
; GS:	available, not preserved
;
; Caution: The device helper functions Block and Yield
; clear FS and GS - im Gegensatz zur Doku!
;

    if JR
PhysIO		equ	0C9A00h	;Physical address of ST01 I/O ports
    else
PhysIO		equ	0CFA00h	;Physical address of ST01 I/O ports
    endif
IRQnum		equ	5	;IRQ number
HostID		equ	6	;Host adapter SCSI ID
    if JR
TargetID	equ	4	;Target SCSI ID
    else
TargetID	equ	4	;Target SCSI ID
    endif

TimeSlice	equ	2000	;Time to sleep in execution phase

_cmd	equ	0000h	;Control/status port
_dat	equ	0200h	;Data port (1KB)

cmd	equ	byte ptr fs:[_cmd]	;usually
dat	equ	byte ptr fs:[_dat]

CEN	equ	80h	;enable
CIE	equ	40h	;interrupt enable
CPE	equ	20h	;parity enable
CARB	equ	10h	;start arbitration
CATN	equ	08h
CBSY	equ	04h
CSEL	equ	02h
CRST	equ	01h

SARB	equ	80h	;arbitration complete
SPE	equ	40h	;parity error
SSEL	equ	20h
SREQ	equ	10h
SCD	equ	08h
SIO	equ	04h
SMSG	equ	02h
SBSY	equ	01h

;	Message-In/Out Codes
MsgComplete	equ	00h
MsgSavePointer	equ	02h
MsgRestPointer	equ	03h
MsgDisconnect	equ	04h
MsgError	equ	05h
MsgAbort	equ	06h
MsgReject	equ	07h
MsgNop		equ	08h
MsgParity	equ	09h
MsgLinkedCompl	equ	0Ah
MsgLinkedComplF	equ	0Bh
MsgReset	equ	0Ch
MsgIdentify	equ	80h

;; MsgCanDisconn	equ	0h
MsgCanDisconn	equ	40h

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

StartData

	public	host_id
	public	target_id
	public	irq_num
	public	phys_addr

	public	sysinfo_seg

	extrn	DevHlpPtr:dword

;
; Set by command line switches
;
		even
phys_addr	dd	PhysIO		;Physical address of I/O ports
irq_num		dw	IRQnum		;IRQ number
host_id		db	1 shl HostID	;Host adapter SCSI ID mask
target_id	db	1 shl TargetID	;Target SCSI ID mask

;
; Other data
;
		even
port_seg	dw	0		;Protected mode I/O port selector
sysinfo_seg	dw	0		;Bimodal sysinfo segment address
yield_ptr	dd	0		;Bimodal ptr to yield flag
timeout		dd	0		;Start time
event_id	dd	0		;Dummy phys addr used as event ID
data_ptr	dd	0		;Virtual data transfer address
data_len	dw	0		;Requested transfer length
actual_len	dw	0		;Actual transfer length
cmdptr		dw	0		;Near pointer to next command byte
xstatus		dw	0		;Execute returns this on completion
status		db	0		;Status byte
wait_for_int	db	0		;Waiting for reselection interrupt
reconnect_flag	db	0		;Reconnection interrupt occurred
msg_byte	db	0		;Send this in message out phase
fast_xfer	db	0		;Fast block transfer mode

EndData

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

StartCode

	public	dev_interrupt
	public	command
	public	bus_reset
	public	device_reset
	public	init_io
	public	delay

	public	ST_INIT_null
ST_INIT_null:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; Set time to now + CX
;
set_time	proc near
	mov	es, sysinfo_seg
	movzx	ecx, cx
	add	ecx, es:[0].msec_timer
	mov	timeout, ecx
	ret
set_time	endp

;
; Compare time to saved time
;
cmp_time	proc near
	mov	es, sysinfo_seg
	mov	eax, es:[0].msec_timer
	cmp	eax, timeout
	ret
cmp_time	endp

;
; Wait CX msec
;
delay		proc near
	outtext	10, 'Delay '
	outword	10, cx
	outchar	10, '$'
	push	fs
	pusha
	mov	ax, cs
	mov	bx, offset delay
	sub	di, di
	mov	dh, 1		;not interruptible
	DevHlp	DevHlp_Block
	popa
	pop	fs
	ret
delay		endp

;
; Wait for event (interruptible), max CX msec
; Return: C=0:	event wakeup
;	  C=1:	Z=1:  timeout
;		Z=0:  interrupted
;
sleep		proc near
	outtext	5, 'Sleep '
	outword	5, cx
	push	fs
	push	di
	mov	ax, event_id._hi
	mov	bx, event_id._lo
	sub	di, di
	mov	dh, 0
	DevHlp	DevHlp_Block
	pop	di
	pop	fs
    if	trace
	pushf
	jc	short sl_1
	outtext	5, ': event$'
	jmp	short sl_2
sl_1:	outtext	5, ': timeout$'
sl_2:	popf
    endif
	ret
sleep		endp

yield		proc near
	push	dx
	push	bx
	les	bx, yield_ptr
	cmp	byte ptr es:[bx], 0
	je	short yield1
	outtext	5, 'Yield$'
	push	fs
	DevHlp	DevHlp_Yield
	pop	fs
yield1:	pop	bx
	pop	dx
yield2:	ret
yield		endp

;
; Wait for (status & DH) == DL, max CX msec
;	C=1: timeout
wait_for	proc near
	outtext	10, 'Wait '
	outword	10, cx
	outtext	10, ' for (status & '
	outbyte	10, dh
	outtext	10, ') == '
	outbyte	10, dl
	outchar	10, '$'
	pusha
	popa
	mov	al, cmd
	and	al, dh
	cmp	al, dl
	jne	short wf_wait
wf_ok:	clc
	ret
wf_wait:call	set_time
wf_loop:	;;!! call	yield
		mov	al, cmd
		and	al, dh
		cmp	al, dl
		je	short wf_ok
		call	cmp_time
		jb	short wf_loop
wf_to:	stc
	ret
wait_for	endp

waitfor		macro	mask, comp, time
	mov	dx, (mask) * 256 + (comp)
	mov	cx, time
	call	wait_for
		endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; Device interrupt
; - Check for proper reselection and wakeup driver if
;   waiting for reconnection.
; - Take care of race conditions in a system with
;   multiple host adapters on the same bus.
; - The CIE bit does not to seem to be what its name
;   suggests, so accept but ignore interrupts at other
;   times.
; - Cannot use waitfor, since Yield is unusable at
;   interrupt time.
;
dev_interrupt	proc far
	push	fs
	push	DevHlpPtr		;Setup for DevHlp calls.
	mov	bp, sp

	outtext	3, '-IRQ-'
	cmp	wait_for_int, 0		;Perhaps unexpected?
	mov	wait_for_int, 0
	je	int_ign

	smsw	ax			;Set I/O segment for each mode.
	test	al, 1
	jnz	short int_pm
int_rm:	mov	eax, phys_addr		;The interrupt can occur in real mode.
	shr	eax, 4
	mov	fs, ax
	jmp	short int_cont
int_pm:	mov	fs, port_seg
int_cont:

	mov	cmd, CPE		;Disable ST01 interrupts
	mov	ax, irq_num		;Allow all other interrupts
	DevHlp	DevHlp_EOI		;to reduce interrupt latency.

	test	cmd, SSEL		;The SEL line should be active since
	jz	int_bad			;it triggered the interrupt.

	mov	cx, 1000		;Wait for release of BSY for IO
int_1:	test	cmd, SBSY		;line and ID bits to become active.
	loopnz	int_1
	jnz	short int_bad

	mov	al, cmd			;Check for a valid reselection.
	test	al, SIO			;Is it a reselection?
	jz	short int_bad
	test	al, SPE			;Ignore reselections with bad parity.
	jnz	short int_bad

	mov	al, dat			;Check initiator & target IDs.
	xor	al, host_id
	xor	al, target_id
	jnz	short int_bad

	test	cmd, SSEL		;The SEL line should still be active
	jz	short int_bad		;but maybe another host adapter
					;got selected and the ID bits on the
					;data bus were garbage. In this case,
					;the SEL line should now be inactive.

	mov	cmd, CEN+CPE+CBSY	;Acknowledge reselection.
	mov	cx, 10000		;Wait for end of SEL.
int_2:	test	cmd, SSEL
	loopnz	int_2
	jcxz	int_bad
	mov	cmd, CEN+CPE+CIE	;Now the targed controls BSY.

	mov	ax, event_id._hi	;Wakeup waiting driver.
	mov	bx, event_id._lo
	DevHlp	DevHlp_Run

	mov	reconnect_flag, 1	;Show the world.
	outtext	10, 'OK-'
	
int_r:	lea	sp, [bp+4]
	pop	fs
	clc				;The interrupt is ok.
	ret

int_ign:
	outtext	3, 'IGN-'
	mov	ax, irq_num
	DevHlp	DevHlp_EOI
	jmp	short int_r

int_bad:
	outtext	3, 'BAD-'
	mov	cmd, CPE+CIE		;Rearm for next (re-)selection.
	jmp	short int_r

dev_interrupt	endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; Initiate SCSI bus operations
;	C=1, AX=error-code
arbitrate	proc near
	outtext	3, 'Arbitration$'

arb_loop:
	mov	cmd, CPE		;Clear pending parity errors.
	mov	al, host_id		;Send host adapter SCSI ID.
	mov	dat, al
	mov	cmd, CEN+CPE+CARB	;Start bus arbitration.
	waitfor	SARB, SARB, 100		;Wait for success.
	jnc	short arb_select

	mov	cx, TimeSlice		;Takes long, go to sleep.
	call	sleep
	jnc	short arb_loop
	jz	short arb_loop

	mov	ax, STERR+11h		;Character I/O call interrupted.
	stc
	ret

arb_select:
	outtext	4, 'Select target$'

	mov	al, target_id		;Send both host & target SCSI IDs.
	or	dat, al
	mov	cmd, CEN+CPE+CATN+CSEL	;Signal target selection. Request
	waitfor	SBSY, SBSY, 250		;a message out phase via ATN.
	jc	short dev_not_ready
	mov	cmd, CEN+CPE+CATN	;Now target controls SEL.

	outtext	4, 'Arbitration done$'
	clc
	ret

dev_not_ready:
	outtext	1, 'Initiation error$'
	mov	ax, STERR+02h
	stc
	ret

arbitrate	endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; Disconnected. Wait for reselection
; Interrupts are disabled on entry.
;	C=1, AX=error-code
reconnect	proc near
	outtext	3, 'Waiting for reselection$'
res_loop:
	cli
	cmp	reconnect_flag, 0	;Check the flag to be sure
	jne	res_ok			;the interrupt didn't get lost.
	mov	cx, 1000		;Wait for reselection.
	call	sleep
	sti
	jnc	short res_ok		;Event?
	jnz	short res_int		;Interrupt?
	jmp	short res_loop		;Timeout.
res_ok:
	sti
	outtext	3, 'Reconnected$'
	mov	wait_for_int, 0
	clc
	ret
res_int:
	outtext	1, 'Reconnection interrupted$'
	mov	ax, STERR+11h		;Character I/O call interrupted.
	mov	wait_for_int, 0
	stc
	ret
reconnect	endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; On SCSI bus. Execute transfer as controlled by target.
;	C=1, AX=error-code on error
;
execute		proc near
	outtext	3, 'Execute$'

	push	di
	push	si
	mov	xstatus, 0
	jmp	x_loop

	even			;CD  IO  MSG	phase
x_table	dw	x_data_out	;0   0   0	data out
	dw	x_inv_bus	;0   0   1	-invalid-
	dw	x_data_in	;0   1   0	data in
	dw	x_inv_bus	;0   1   1	-invalid-
	dw	x_cmd_out	;1   0   0	command
	dw	x_msg_out	;1   0   1	message out
	dw	x_stat_in	;1   1   0	status
	dw	x_msg_in 	;1   1   1	message in

x_inv_bus:
	outtext	1, 'X: Invalid bus state$'
	call	bus_reset		;Bus reset required.
	mov	ax, STERR+0Ch		;General failure.
	jmp	x_error

x_parity:
	outtext	1, 'X: Parity error$'
	mov	cmd, CEN+CPE+CATN	;Request message phase
	mov	msg_byte, MsgError	;to inform the target of the problem.
	jmp	x_loop			;Target should terminate the command.

x_loop:	mov	bl, cmd			;First check for next byte.
	outtext	20, 'X_Loop '
	outbyte	20, bl
	outchar	20, '$'
	test	bl, SBSY
	jz	x_not_bsy
	test	bl, SREQ
	jz	short x_wait
	test	bl, SPE
	jnz	short x_parity
x_jump:	and	bx, SCD+SIO+SMSG	;Execute next byte or block.
	jmp	x_table[bx]

x_wait:	mov	cx, 100			;Not yet avail., setup for timeout.
	call	set_time
x_test:	mov	bl, cmd			;Wait for next byte or timeout.
	outtext	20, 'X_Loop '
	outbyte	20, bl
	outchar	20, '$'
	test	bl, SBSY
	jz	x_not_bsy
	test	bl, SREQ
	jnz	short x_jump
	test	bl, SPE
	jnz	x_parity
	call	yield
	call	cmp_time
	jb	short x_test

	mov	cx, TimeSlice		;Takes a long time
	call	sleep			;so give up timeslices.
	jnc	short x_test
	jnz	short x_interrupted

	mov	al, msg_byte		;Timeout.
	test	al, MsgIdentify		;Message out phase pending?
	jnz	short x_test
	cmp	al, MsgReject
	je	short x_test
x_no_response:
	outtext	1, 'X: No response$'	;Target did not respond to message out.
	call	bus_reset		;phase request. Reset the bus.
	mov	ax, STERR+02h		;Device not ready
	jmp	x_error

x_interrupted:
	outtext	1, 'X: Interrupted$'
	cmp	msg_byte, MsgAbort	;Repeated?
	je	short x_no_response
	mov	cmd, CEN+CPE+CIE+CATN	;Request message phase
	mov	msg_byte, MsgAbort	;to abort the running command.
	mov	xstatus, STERR+11h	;Character I/O call interrupted.
	jmp	x_loop

x_not_bsy:
	cmp	msg_byte, MsgReset	;Bus free due to reset message?
	je	short x_retx
	cmp	msg_byte, MsgAbort	;Bus free due to abort message?
	je	short x_retx
	outtext	1, 'X: Not BSY$'	;No, a real failure.
	mov	ax, STERR+0Ch		;General failure.
	jmp	x_error

x_ovf:
	mov	cmd, CEN+CPE+CIE+CATN	;Request message phase
	outtext	1, 'X: Overflow$'
	mov	msg_byte, MsgAbort	;to abort the running command.
	mov	xstatus, STERR+0Ch	;General failure.
	mov	data_len, 0
	jmp	x_loop

x_error:
	pop	si
	pop	di
	stc
	ret

x_retx:	mov	ax, xstatus
	test	ax, ax
	jnz	short x_error
x_ret:	pop	si
	pop	di
	clc
	ret

x_data_in:
	cld
    if	EnableFast
	cmp	fast_xfer, 0
	jne	x_fast_in
    endif
	mov	al, dat
	sub	data_len, 1
	jc	x_ovf
	outtext	5, 'Slow data in: '
	outbyte	5, al
	outtext	5, ' at '
	outword	5, actual_len
	outchar	5, '$'
	les	di, data_ptr
	stos	byte ptr es:[di]
	mov	data_ptr._off, di
	inc	actual_len
	jmp	x_loop

    if	EnableFast
x_fi_0:	mov	fast_xfer, 0
	jmp	x_ovf
x_fast_in:
	mov	cx, data_len
	cmp	cx, BlockSize
	jb	short x_fi_1
	mov	cx, BlockSize
x_fi_1:	jcxz	x_fi_0
	outtext	4, 'Fast data in: '
	outword	4, cx
	outtext	4, ' at '
	outword	4, actual_len
	outchar	4, '$'
	add	actual_len, cx
	sub	data_len, cx
	mov	si, _dat
	les	di, data_ptr
	rep movs byte ptr es:[di], byte ptr fs:[si]
	mov	data_ptr._off, di
	jmp	x_loop
    endif

x_data_out:
	cld
    if	EnableFast
	cmp	fast_xfer, 0
	jne	x_fast_out
    endif
	sub	data_len, 1
	jc	x_do_0
	les	si, data_ptr
	lods	byte ptr es:[si]
	mov	data_ptr._off, si
	mov	dat, al
	outtext	5, 'Slow data out: '
	outbyte	5, al
	outtext	5, ' at '
	outword	5, actual_len
	outchar	5, '$'
	inc	actual_len
	jmp	x_loop
x_do_0:	mov	dat, 0
	jmp	x_ovf

    if	EnableFast
x_fo_0:	mov	fast_xfer, 0
	jmp	x_ovf
x_fast_out:
	mov	cx, data_len
	cmp	cx, BlockSize
	jb	short x_fo_1
	mov	cx, BlockSize
x_fo_1:	jcxz	x_fo_0
	outtext	4, 'Fast data out: '
	outword	4, cx
	outtext	4, ' at '
	outword	4, actual_len
	outchar	4, '$'
	add	actual_len, cx
	sub	data_len, cx
	lgs	si, data_ptr
	mov	ax, fs
	mov	es, ax
	mov	di, _dat
     if EnableBlock
	rep	movs byte ptr es:[di], byte ptr gs:[si]
     else
x_fo_2:	movs	byte ptr es:[di], byte ptr gs:[si]
	push	cx
	mov	cx, WrtWait
	loop	$
	pop	cx
	loop	x_fo_2
     endif
	mov	data_ptr._off, si
	jmp	x_loop
    endif

x_msg_in:
	outtext	4, 'Message in: '
	cli
	mov	wait_for_int, 1		;Enable full interrupt management,
	mov	reconnect_flag, 0
	mov	cmd, CEN+CPE+CIE	;don't miss reselection interrupt.
	mov	al, dat
	cmp	al, MsgDisconnect
	je	short x_disconnect
	mov	wait_for_int, 0		;Not a disconnect, forget about
	sti				;interrupts.
	cmp	al, MsgComplete
	je	short x_complete
	test	al, MsgIdentify		;Check for other known messages.
	jnz	short x_msg_ok
	cmp	al, MsgSavePointer
	je	short x_msg_ok
	cmp	al, MsgRestPointer
	je	short x_msg_ok
	mov	cmd, CEN+CPE+CIE+CATN	;Unknown message, request message out
	mov	msg_byte, MsgReject	;phase and signal message reject.
	outtext	4, 'Rejected '
	outbyte	4, al
x_msg_ok:
	outchar	4, '$'
	jmp	x_loop

x_disconnect:
	outtext	4, 'Disconnect$'
	waitfor	SBSY, 0, 100		;Wait until bus free
	mov	cmd, CPE+CIE		;	and release bus.
	call	reconnect
	jnc	x_loop
	push	ax			;Reconnection failed, abort command.
	call	abort
	pop	ax
	jmp	x_error

x_complete:
	outtext	4, 'Complete$'
	waitfor	SBSY, 0, 100		;Wait until bus free
	mov	cmd, CPE+CIE		;	and release bus.
	jmp	x_retx

x_msg_out:
	outtext	4, 'Message out: '
	outbyte	4, msg_byte
	outchar	4, '$'
	mov	cmd, CEN+CPE+CIE	;Release message phase request.
	mov	al, msg_byte
	mov	dat, al
	jmp	x_loop

x_cmd_out:
	mov	si, cmdptr
	lodsb
	mov	dat, al
	mov	cmdptr, si
	outtext	5, 'Command out: '
	outbyte	5, al
	outchar	5, '$'
	jmp	x_loop

x_stat_in:
	mov	al, dat
	mov	status, al
	outtext	4, 'Status in: '
	outbyte	4, al
	outchar	4, '$'
	jmp	x_loop

execute		endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; Reset SCSI bus.
;
bus_reset	proc near
	push	fs
	mov	fs, port_seg
	outtext	2, 'Bus reset$'
	mov	cmd, CEN+CPE+CRST
	mov	cx, 100
	call	delay
	mov	cmd, 0
	mov	cx, 100
	call	delay
	pop	fs
	clc
	ret
bus_reset	endp

;
; Reset device.
;
device_reset	proc near
	push	fs
	mov	fs, port_seg
	outtext	2, 'Device reset$'
	mov	msg_byte, MsgReset
	call	arbitrate
	jc	short drst_r
	call	execute
drst_r:	pop	fs
	ret
device_reset		endp

;
; Abort command.
abort		proc near
	push	fs
	mov	fs, port_seg
	outtext	2, 'Abort command$'
	mov	msg_byte, MsgAbort
	call	arbitrate
	jc	short abrt_r
	call	execute
abrt_r:	pop	fs
	ret
abort		endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; Execute SCSI command
; In:
;	AL	= 0 for slow, 1 for fast transfer
;	EBX	= virtual data transfer address
;	CX	= expected data length
;	DX	= pointer to command record
; Out:
;	C	= set on error
;	AX	= error code, 0 if no error
;	CX	= number of data bytes actually transferred
;	DL	= status byte, 0FFh if no status available
;
command		proc near
	push	fs
	push	es

    if	trace
	pusha
	outtext	2, 'Command, len='
	outword	2, cx
	outtext	2, ', cmd:'
	mov	si, dx
	mov	cx, 6
cmd1:	lodsb
	outchar	2, ' '
	outbyte	2, al
	loop	cmd1
	outchar	2, '$'
	popa
    endif

	mov	fast_xfer, al
	mov	data_ptr, ebx
	mov	data_len, cx
	mov	cmdptr, dx
	mov	actual_len, 0
	mov	status, 0FFh
	mov	fs, port_seg

	mov	msg_byte, MsgIdentify+MsgCanDisconn+0
	call	arbitrate
	jc	short cmd_e
	call	execute
	jc	short cmd_e

	mov	dl, status
	mov	cx, actual_len
	outtext	2, 'Command status='
	outbyte	2, dl
	outtext	2, ', length='
	outword	2, cx
	outchar	2, '$'

	pop	es
	pop	fs
	clc
	ret

cmd_e:	mov	dl, status
	mov	cx, actual_len
	outtext	2, 'Command error='
	outword	2, ax
	outtext	2, ', status='
	outbyte	2, dl
	outtext	2, ', length='
	outword	2, cx
	outchar	2, '$'

	pop	es
	pop	fs
	stc
	ret
command		endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;

init_io		proc near
	push	di
	push	si

	outtext	2, 'ST_Tape IO init$'

	mov	ax, phys_addr._hi	;Clear cmd register.
	mov	bx, phys_addr._lo
	mov	cx, 1000h
	mov	dh, 1
	DevHlp	DevHlp_PhysToVirt
	jc	init_r
	mov	byte ptr es:[di+_cmd], 0
	DevHlp	DevHlp_UnPhysToVirt

	mov	al, 1			;Get sysinfo segment (for msec timer).
	DevHlp	DevHlp_GetDOSVar
	jc	init_r
	mov	es, ax
	mov	ax, es:[bx]
	mov	sysinfo_seg, ax

	mov	al, 8			;Get pointer to yield flag.
	DevHlp	DevHlp_GetDOSVar
	jc	init_r
	mov	yield_ptr._seg, ax
	mov	yield_ptr._off, bx

	sub	si, si			;The physical address of the driver's
	DevHlp	DevHlp_VirtToPhys	;data segment is used as event ID.
	mov	event_id._hi, ax
	mov	event_id._lo, bx

	mov	ax, ds			;Allocate port I/O segment selector.
	mov	es, ax
	mov	di, offset port_seg
	mov	cx, 1
	DevHlp	DevHlp_AllocGDTSel
	jc	short init_r

	mov	ax, phys_addr._hi	;Set port I/O segment address.
	mov	bx, phys_addr._lo
	mov	cx, 1000h
	mov	si, port_seg
	DevHlp	DevHlp_PhysToGDTSel
	jc	short init_r

	mov	ax, offset dev_interrupt
	mov	bx, irq_num 		;Set interrupt handler.
	mov	dh, 0
	DevHlp	DevHlp_SetIRQ
	jc	short init_r

init_r:	pop	si
	pop	di
	ret
init_io		endp

EndCode

	end
