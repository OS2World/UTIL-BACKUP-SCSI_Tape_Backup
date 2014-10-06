;##############################################################################
; $Id: lowlevel.asm,v 1.5 1992/07/24 11:36:30 ak Exp $
;##############################################################################
; $Log: lowlevel.asm,v $
; Revision 1.5  1992/07/24  11:36:30  ak
; OS/2 2.0
; BASEDEV drivers
; VGA debugging
;
; Revision 1.4  1992/01/06  22:49:28  ak
; *** empty log message ***
;
; Revision 1.3  1992/01/06  22:48:17  ak
; int_on/off() for != ZTC.
;
; Revision 1.2  1992/01/06  20:10:00  ak
; *** empty log message ***
;
; Revision 1.1.1.1  1992/01/06  19:54:55  ak
; Alpha version.
;
; Revision 1.1  1992/01/06  19:54:53  ak
; Initial revision
;
;##############################################################################

	.386p

	public	__acrtused
	public	_devhlp
	public	_ctors

	public	_AllocGDTSelector 
	public	_AllocPhys        
	public	_AllocReqPacket      
	public	_AttachDD
	public	_Block            
	public	_DevDone          
	public	_EOI              
	public	_FreePhys         
	public	_FreeReqPacket    
	public	_GetDOSVar        
	public	_InternalError    
	public	_Lock             
	public	_PhysToGDTSelector
	public	_PhysToUVirt 
	public	_ProtToReal       
	public	_PullParticular   
	public	_PullReqPacket    
	public	_PushReqPacket    
	public	_QueueFlush       
	public	_QueueInit        
	public	_QueueRead        
	public	_QueueWrite       
	public	_RealToProt       
	public	_RegisterStackUsage
	public	_ResetTimer       
	public	_Run              
	public	_SemClear         
	public	_SemHandle        
	public	_SemRequest       
	public	_SendEvent        
	public	_SetIRQ           
	public	_SetROMVector     
	public	_SetTimer         
	public	_SortReqPacket    
	public	_TCYield          
	public	_TickCount        
	public	_Unlock           
	public	_VerifyAccess     
	public	_VirtToPhys       
	public	_Yield

	public	_PhysToVirt
	public	_UnPhysToVirt

	public	_inProtMode
	public	_inInitPhase
	public	_int_on
	public	_int_off

	include	macros.inc
	include	dd-segs.inc

	assume	cs:CGROUP, ds:DGROUP, es:nothing, ss:nothing

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BegData	_DATA

__acrtused	dw	0	;C .OBJ files want one.  They never
				;use it.

_devhlp		dd	0	;devhlp vector

EndData	_DATA
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BegCode	_TEXT

;
; Helper functions
;

DevHlp_SchedClock	=  0	;  0	Called each timer tick
DevHlp_DevDone		=  1	;  1	Device I/O complete
DevHlp_Yield		=  2	;  2	yield CPU if resched set
DevHlp_TCYield		=  3	;  3	yield to time critical task

DevHlp_Block		=  4	;  4	Block on event
DevHlp_Run		=  5	;  5	Unblock process

DevHlp_SemRequest	=  6	;  6	claim a semaphore
DevHlp_SemClear 	=  7	;  7	release a semaphore
DevHlp_SemHandle	=  8	;  8	obtain a semaphore handle

DevHlp_PushRequest	=  9	;  9	Push the request
DevHlp_PullRequest	= 10	;  A	Pull next request from Q
DevHlp_PullParticular	= 11	;  B	Pull a specific request
DevHlp_SortRequest	= 12	;  C	Push request in sorted order

DevHlp_AllocReqPacket	= 13	;  D	allocate request packet
DevHlp_FreeReqPacket	= 14	;  E	free request packet

DevHlp_QueueInit	= 15	;  F	Init/Clear char queue
DevHlp_QueueFlush	= 16	; 10	flush queue
DevHlp_QueueWrite	= 17	; 11	Put a char in the queue
DevHlp_QueueRead	= 18	; 12	Get a char from the queue

DevHlp_Lock		= 19	; 13	Lock segment
DevHlp_Unlock		= 20	; 14	Unlock segment

DevHlp_PhysToVirt	= 21	; 15	convert physical address to virtual
DevHlp_VirtToPhys	= 22	; 16	convert virtual address to physical
DevHlp_PhysToUVirt	= 23	; 17	convert physical to LDT

DevHlp_AllocPhys	= 24	; 18	allocate physical memory
DevHlp_FreePhys 	= 25	; 19	free physical memory

DevHlp_SetROMVector	= 26	; 1A	set a ROM service routine vector
DevHlp_SetIRQ		= 27	; 1B	set an IRQ interrupt
DevHlp_UnSetIRQ 	= 28	; 1C	unset an IRQ interrupt

DevHlp_SetTimer 	= 29	; 1D	set timer request handler
DevHlp_ResetTimer	= 30	; 1E	unset timer request handler

DevHlp_MonitorCreate	= 31	; 1F	create a monitor
DevHlp_Register 	= 32	; 20	install a monitor
DevHlp_DeRegister	= 33	; 21	remove a monitor
DevHlp_MonWrite 	= 34	; 22	pass data records to monitor
DevHlp_MonFlush 	= 35	; 23	remove all data from stream

DevHlp_GetDOSVar	= 36	; 24	Return pointer to DOS variable
DevHlp_SendEvent	= 37	; 25	an event occurred
DevHlp_ROMCritSection	= 38	; 26	ROM Critical Section
DevHlp_VerifyAccess	= 39	; 27	Verify access to memory
DevHlp_RAS		= 40	; 28	Put info in RAS trace buffer

DevHlp_AttachDD		= 42	; 2A	Attach a device driver

DevHlp_InternalError	= 43	; 2B	Panic

DevHlp_AllocGDTSel	= 45	; 2D	Allocate GDT Selectors
DevHlp_PhysToGDTSel	= 46	; 2E	Convert phys addr to GDT sel
DevHlp_RealToProt	= 47	; 2F	Change from real to protected mode
DevHlp_ProtToReal	= 48	; 30	Change from protected to real mode

DevHlp_EOI		= 49	; 31	Send EOI to PIC
DevHlp_UnPhysToVirt	= 50	; 32	mark completion of PhysToVirt
DevHlp_TickCount	= 51	; 33	modify timer

DevHlp_GetLIDEntry	= 34h
DevHlp_FreeLIDEntry	= 35h
DevHlp_ABIOSCall	= 36h
DevHlp_ABIOSCommonEntry	= 37h

DevHlp_RegisterStackUse = 38h

DevHlp_VideoPause	= 3Ch
DevHlp_DispMsg		= 3Dh

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; OS/2 2.0

DevHlp_SegRealloc	= 62	; 3E    Realloc DD protect mode segment
DevHlp_PutWaitingQueue	= 63	; 3F    Put I/O request on waiting queue
DevHlp_GetWaitingQueue	= 64	; 40    Get I/O request from waiting queue
;DevHlp_PhysToSys	= 65	; 41    Address conversion for the AOX
;DevHlp_PhysToSysHook	= 66	; 42    Address conversion for the AOX
DevHlp_RegisterDeviceClass = 67	; 43    Register DC entry point

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; OS/2 2.0 32-bit

DevHlp_RegisterPDD	= 80	; 50    Register PDD entry point with
				;       VDM manager for later PDD-VDD
				;	communication
DevHlp_RegisterBeep	= 81	; 51	register PTD beep service
				;	entry point with kernel
DevHlp_Beep		= 82	; 52	preempt beep service via PTD

DevHlp_FreeGDTSelector	= 83	; 53	Free allocated GDT selector

DevHlp_PhysToGDTSel	= 84	; 54	Convert Phys Addr to GDT sel
				;	with given access
				;   BUGBUG: TEMPORARY!!!

DevHlp_VMLock		= 85	; 55	Lock linear address range

DevHlp_VMUnlock 	= 86	; 56	Unlock address range

DevHlp_VMAlloc		= 87	; 56	Allocate memory

DevHlp_VMFree		= 88	; 58	Free memory or mapping

DevHlp_VMProcessToGlobal = 89	; 59	Create global mapping to process
				;	memory

DevHlp_VMGlobalToProcess = 90	; 5A	Create process mapping to global
				;	memory

DevHlp_VirtToLin	= 91	; 5B Convert virtual address to linear

DevHlp_LinToGDTSelector = 92	; 5C Convert linear address to virtual

DevHlp_GetDescInfo	= 93	; 5D Return descriptor information

DevHlp_LinToPageList	= 94	; 5E build pagelist array from lin addr

DevHlp_PageListToLin	= 95	; 5F map page list array to lin addr

DevHlp_PageListToGDTSelector = 96 ; 60 map page list array to GDT sel.

DevHlp_RegisterTmrDD	= 97	; 61 Register TMR Device Driver.

DevHlp_RegisterPerfCtrs = 98	; 62 Register device driver perf. ctrs (PVW).

DevHlp_AllocateCtxHook	= 99	; 63 Allocate a context hook

DevHlp_FreeCtxHook	= 100	; 64 Free a context hook

DevHlp_ArmCtxHook	= 101	; 65 Arm a context hook

DevHlp_VMSetMem 	= 102	; 66H commit/decommit memory

DevHlp_OpenEventSem	= 103	; 67H open an event semaphore

DevHlp_CloseEventSem	= 104	; 68H close an event semaphore

DevHlp_PostEventSem	= 105	; 69H post an event semaphore

DevHlp_ResetEventSem	= 106	; 6AH reset an event semaphore

DevHlp_RegisterFreq	= 107	; 6BH	register PTD freq service
				;	entry point with kernel

DevHlp_DynamicAPI	= 108	; 6CH add a dynamic API
;
; Request bit definitions used in DevHlp_DynamicAPI
DHDYNAM_16B_CALLGATE	= 1	; 1 = 16 bit callgate, 0 = 32 bit
DHDYNAM_16B_APIRTN	= 2	; 1 = 16 bit API rtn,  0 = 32 rtn

DevHlp_ProcRun2 	= 109	; 6DH  Unblock process via procrun2
;
DevHlp_CreateInt13VDM	= 110	; 6EH Create Int13 VDM (Internal Only) OEMINT13


p	equ	[bp+4]

; (word *address, word nselectors) -> word 0 if ok, errcode if not
_AllocGDTSelector proc near
	.enter	<es,di>
	les	di, p+0
	mov	cx, p+4
	.devhlp	AllocGDTSel
	.rzero
	.return	<es,di>
_AllocGDTSelector endp

; (dword size) -> physaddr if ok, 0 if not
_AllocPhys proc near
	.enter	<>
	mov	ax, p+2
	mov	bx, p+0
	.devhlp	AllocPhys
	.rnull	ax, bx
	.return	<>
_AllocPhys endp

; (word waitflag) -> farptr if ok, 0 if not
_AllocReqPacket proc near
	.enter	<es>
	mov	dh, p+0
	.devhlp	AllocReqPacket
	.rnull	es, bx
	.return	<es>
_AllocReqPacket endp

; (nearptr name, nearptr area) -> error
_AttachDD proc near
	.enter	<di>
	mov	bx, p+0
	mov	di, p+2
	.devhlp	AttachDD
	.rflag
	.return	<di>
_AttachDD endp

; (dword id, dword time, word interruptible)
;	-> 0 if normal, -1 if timeout, awake-code > 0 if unusual wakeup
_Block proc near
	.enter	<di>
	mov	bx, p+0
	mov	ax, p+2
	mov	cx, p+4
	mov	di, p+6
	mov	dh, p+8
	.devhlp	Block
	jnc	short bl_evt
	jz	short bl_tim
	mov	ah, 0
	jmp	short bl_ret
bl_tim:	mov	ax, -1
	jmp	short bl_ret
bl_evt:	sub	ax, ax
bl_ret:	.return	<di>
_Block endp

; (farptr reqpkt)
_DevDone proc near
	.enter	<es>
	les	bx, p+0
	.devhlp	DevDone
	.return	<es>
_DevDone endp

; (farptr msgtable)
_DispMsg proc near
	.enter	<es,di>
	les	di, p+0
	sub	bx, bx
	.devhlp	DispMsg
	.return	<es,di>
_DispMsg endp

; (word irqnum)
_EOI proc near
	.enter	<>
	mov	al, p+0
	.devhlp	EOI
	.return	<>
_EOI endp

; (physaddr address) -> error
_FreePhys proc near
	.enter	<>
	mov	bx, p+0
	mov	ax, p+2
	.devhlp	FreePhys
	.rflag
	.return	<>
_FreePhys endp

; (farptr reqpkt)
_FreeReqPacket proc near
	.enter	<es>
	les	bx, p
	.devhlp	FreeReqPacket
	.return	<es>
_FreeReqPacket endp

; (word varnum, word cx) -> farptr or 0
_GetDOSVar proc near
	.enter	<>
	mov	al, p
	mov	cx, p+2
	.devhlp	GetDOSVar
	.rnull	ax, bx
	.return	<>
_GetDOSVar endp

; (farptr msg, word msglen)
_InternalError proc near
	lds	si, p
	mov	di, p+4
	mov	ax, ds
	mov	es, ax
	mov	dl, DevHlp_InternalError
	call	es:_devhlp
_InternalError endp

; (word seg, word flags) -> dword handle or 0
_Lock proc near
	.enter	<>
	mov	ax, p+0
	mov	bx, p+2
	.devhlp	Lock
	.rnull	ax, bx
	.return	<>
_Lock endp

; (physaddr address, word length, word selector) -> 0 or errcode
_PhysToGDTSelector proc near
	.enter	<si>
	mov	bx, p+0
	mov	ax, p+2
	mov	cx, p+4
	mov	si, p+6
	.devhlp	PhysToGDTSel
	.rzero
	.return	<si>
_PhysToGDTSelector endp

; (physaddr address, word length, word type, word tag) -> farptr or 0
_PhysToUVirt proc near
	.enter	<es, si>
	mov	bx, p+0
	mov	ax, p+2
	mov	cx, p+4
	mov	dh, p+6
	mov	si, p+8
	.devhlp	PhysToUVirt
	.rnull	es, bx
	.return	<es, si>
_PhysToUVirt endp

; () -> 0 or errcode
_ProtToReal proc near
	.devhlp	ProtToReal
	.rzero
	ret
_ProtToReal endp

; (nearptr queue, farptr reqpkt) -> error
_PullParticular proc near
	.enter	<es,si>
	mov	si, p+0
	les	bx, p+2
	.devhlp	PullParticular
	.rflag
	.return	<es,si>
_PullParticular endp

; (nearptr queue) -> farptr reqpkt or 0
_PullReqPacket proc near
	.enter	<es,si>
	mov	si, p+0
	.devhlp	PullRequest
	.rnull	es, bx
	.return	<es,si>
_PullReqPacket  endp

; (nearptr queue, farptr reqpkt)
_PushReqPacket proc near
	.enter	<es,si>
	mov	si, p+0
	les	bx, p+2
	.devhlp	PushRequest
	.return	<es,si>
_PushReqPacket  endp

; (nearptr queue)
_QueueFlush proc near
	.enter	<si>
	mov	si, p+0
	.devhlp	QueueFlush
	.return	<si>
_QueueFlush endp

; (nearptr queue)
_QueueInit proc near
	.enter	<si>
	mov	si, p+0
	.devhlp	QueueInit
	.return	<si>
_QueueInit endp

; (nearptr queue) -> word char or -1
_QueueRead proc near
	.enter	<>
	mov	bx, p+0
	.devhlp	QueueRead
	.rminus
	.return	<>
_QueueRead endp

; (nearptr queue, word char) -> error
_QueueWrite proc near
	.enter	<>
	mov	bx, p+0
	mov	al, p+2
	.devhlp	QueueWrite
	.rflag
	.return	<>
_QueueWrite endp

; () -> error
_RealToProt proc near
	.devhlp	RealToProt
	.rflag
	ret
_RealToProt endp

; (nearptr data) -> error
_RegisterStackUsage proc near
	.enter	<>
	mov	bx, p+0
	.devhlp	RegisterStackUse
	.rflag
	.return	<>
_RegisterStackUsage endp

; (farfcn handler) -> error
_ResetTimer proc near
	.enter	<>
	mov	ax, p+0
	.devhlp	ResetTimer
	.rflag
	.return	<>
_ResetTimer endp

; (dword id) -> word count_awakened
_Run proc near
	.enter	<>
	mov	bx, p+0
	mov	ax, p+2
	.devhlp	Run
	.return	<>
_Run endp

; (dword handle) -> 0 or errcode
_SemClear proc near
	.enter	<>
	mov	bx, p+0
	mov	ax, p+2
	.devhlp	SemClear
	.rzero
	.return	<>
_SemClear endp

; (dword semid, word inuse) -> dword handle or 0
_SemHandle proc near
	.enter	<>
	mov	bx, p+0
	mov	ax, p+2
	mov	dh, p+4
	.devhlp	SemHandle
	.rnull	ax, bx
	.return	<>
_SemHandle endp

; (dword handle, dword time) -> 0 or errcode
_SemRequest proc near
	.enter	<di>
	mov	bx, p+0
	mov	ax, p+2
	mov	cx, p+4
	mov	di, p+6
	.devhlp	SemRequest
	.rzero
	.return	<di>
_SemRequest endp

; (word event, word arg) -> error
_SendEvent proc near
	.enter	<>
	mov	al, p+0
	mov	bx, p+2
	.devhlp	SendEvent
	.rflag
	.return	<>
_SendEvent endp

; (farfcn handler, word irqnum, word shared) -> error
_SetIRQ proc near
	.enter	<>
	mov	ax, p+0
	mov	bx, p+4
	mov	dh, p+6
	.devhlp	SetIRQ
	.rflag
	.return	<>
_SetIRQ endp

; (farfcn handler, word intnum, nearCSptr saveDS) -> dword previous
_SetROMVector proc near
	.enter	<si>
	mov	ax, p+0
	mov	bx, p+4
	mov	si, p+6
	.devhlp	SetROMVector
	.rnull	ax, dx
	.return	<si>
_SetROMVector endp

; (farfcn handler) -> error
_SetTimer proc near
	.enter	<>
	mov	ax, p+0
	.devhlp	SetTimer
	.rflag
	.return	<>
_SetTimer endp

; (nearptr queue, farptr reqpkt)
_SortReqPacket proc near
	.enter	<es,si>
	mov	si, p+0
	les	bx, p+2
	.devhlp	SortRequest
	.return	<es,si>
_SortReqPacket endp

; ()
_TCYield proc near
	.devhlp	TCYield
	ret
_TCYield endp

; (farfcn handler, word tick_count) -> error
_TickCount proc near
	.enter	<>
	mov	ax, p+0
	mov	bx, p+4
	.devhlp	TickCount
	.rflag
	.return	<>
_TickCount endp

; (dword handle) -> error
_Unlock proc near
	.enter	<>
	mov	bx, p+0
	mov	ax, p+2
	.devhlp	Unlock
	.rflag
	.return	<>
_Unlock endp

; (farptr address, word length, word type) -> error
_VerifyAccess proc near
	.enter	<di>
	mov	ax, p+2
	mov	cx, p+4
	mov	di, p+0
	mov	dh, p+6
	.devhlp	VerifyAccess
	.rflag
	.return	<di>
_VerifyAccess endp

; (farptr address) -> physaddr
_VirtToPhys proc near
	.enter	<es,ds,si>
	mov	ax, ds
	mov	es, ax
	lds	si, p+0
	mov	dl, DevHlp_VirtToPhys
	call	es:_devhlp
	.rnull	ax, bx
	.return	<es,ds,si>
_VirtToPhys endp

; ()
_Yield proc near
	.devhlp	Yield
	ret
_Yield endp


; (farptr paddr, word len) -> farptr
_PhysToVirt proc near
	.enter	<es,di>
	mov	ax, p+2
	mov	bx, p+0
	mov	cx, p+4
	mov	dh, 1
	.devhlp	PhysToVirt
	.rnull	es, di
	.return	<es,di>
_PhysToVirt endp

; ()
_UnPhysToVirt proc near
	mov	dh, 1
	.devhlp	UnPhysToVirt
	ret
_UnPhysToVirt endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_inProtMode proc near
	smsw	ax
	and	ax, 1
	ret
_inProtMode endp

_inInitPhase proc near
	smsw	ax
	and	ax, 1
	jz	short in1
	mov	ax, cs
	and	ax, 3
in1:	ret
_inInitPhase endp

_int_on proc near
	sti
_int_on endp

_int_off proc near
	sti
_int_off endp

EndCode	_TEXT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BegCode	ITEXT

; Clear BSS & c_common segments
; Perform static constructors
; Go backwards through the list so we do what was linked in last, first.
; Return length of data segment

_ctors	proc near
	push	di
	;Clear BSS segment
	mov	di, offset DGROUP:UDATA	;start of _BSS
	mov	ax, ds			;end of _BSS = end of data segment
	lsl	cx, ax
	inc	cx
	push	cx
	sub	cx, di
	jcxz	short ct0
	mov	ax, ds
	mov	es, ax
	sub	al, al
	cld
	rep	stosb

	;Call far ctors
ct0:	mov	di, offset DGROUP:XIFE
ctfar:	cmp	di, offset DGROUP:XIFB
	jbe	short ct2
	sub	di, 4
	cmp	dword ptr [di], 0
	jz	short ctfar
	call	dword ptr [di]
	jmp	short ctfar

	;Call near ctors
ct2:	mov	di, offset DGROUP:XIE
ctnear:	cmp	di, offset DGROUP:XIB
	jbe	short ct3
	sub	di, 2
	mov	cx, [di]
	jcxz	short ctnear
	call	cx
	jmp	short ctnear

ct3:	pop	ax		;length of data segment
	pop	di
	ret
_ctors	endp

EndCode	ITEXT

	end
