/*****************************************************************************
 * $Id: dd.h,v 1.4 1992/07/24 11:34:00 ak Exp $
 *****************************************************************************
 * $Log: dd.h,v $
 * Revision 1.4  1992/07/24  11:34:00  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
 *
 * Revision 1.3  1992/01/07  07:50:22  ak
 * DevHlp function which return error state only
 * now return byte instead of word. Corresponds
 * to SETxx.
 *
 * Revision 1.2  1992/01/06  22:48:44  ak
 * int_on/off() for != ZTC.
 *
 * Revision 1.1.1.1  1992/01/06  19:54:02  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:54:00  ak
 * Initial revision
 *
 *****************************************************************************
 * -DBASEDEV	for BASEDEV drivers
 *****************************************************************************/
 

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/

typedef unsigned char	byte;
typedef unsigned short	word;
typedef unsigned long	dword;

typedef dword		paddr;
typedef dword		handle;

typedef void		_near *	nearptr;
typedef void		_far * farptr;

typedef void		(_far * farfcn) (void);
typedef void		(_far * farptrfcn) (farptr);

#define Segment(p)	(word)((dword)(farptr)(p) >> 16)
#define Offset(p)	(word)((dword)(farptr)(p))
#define Pointer(s,o)	(farptr)((dword)(s) << 16 | (dword)(o))

/************************************************************************/
/*		Device driver interface definitions.			*/
/************************************************************************/

typedef struct ReqPkt ReqPkt;
struct ReqPkt {
	byte		len;				/* 00 */
	byte		unit;				/* 01 */
	byte		cmd;				/* 02 */
	word		status;				/* 03 */
	dword		reserved;			/* 05 */
	ReqPkt _far *	link;				/* 09 */
	union {
		struct {			/* INIT in */
			byte	_1_;			/* 0D */
			farptr	devhlp;			/* 0E */
			farptr	cmdline;		/* 12 */
			byte	firstUnit;		/* 16 */
		} ini;
		struct {			/* INIT out */
			byte	nunits;			/* 0D */
			word	endCode;		/* 0E */
			word	endData;		/* 10 */
			farptr	bpb;			/* 12 */
		} ino;
		struct {			/* READ/WRITE/VERIFY */
			byte	media;			/* 0D */
			paddr	addr;			/* 0E */
			word	count;			/* 12 */
			dword	sector;			/* 14 */
			word	fileno;			/* 18 */
		} rw;
		struct {
			byte	ch;		/* PEEK	*/
		} pk;
		struct {			/* OPEN/CLOSE */
			word	fileno;			/* 0D */
		} oc;
		struct {			/* IOCTL */
			byte	cat;			/* 0D */
			byte	fcn;			/* 0E */
			farptr	parm;			/* 0F */
			farptr	data;			/* 13 */
			word	fileno;			/* 17 */
			word	plen;			/* 19 */
			word	dlen;			/* 1B */
		} ioc;
	} u;
};

typedef ReqPkt _far * ReqPtr;

/* Status codes */
#define DONE	0x0100
#define BUSY	0x0200
#define DEVERR	0x4000
#define ERROR	0x8000

/* Error codes */
#define WriteProtect	0x00
#define UnknownUnit	0x01
#define DeviceNotReady	0x02
#define InvalidCommand	0x03
#define CRCError	0x04
#define BadReqStruct	0x05
#define SeekError	0x06
#define UnknownMedia	0x07
#define SectorNotFound	0x08
#define NoPaper		0x09
#define WriteFault	0x0A
#define ReadFault	0x0B
#define GeneralFailure	0x0C
#define ChangeDisk	0x0D
#define UncertainMedia	0x10
#define Interrupted	0x11
#define NoMonitors	0x12
#define InvalidParam	0x13

/* Char-device command codes */
#define DevInit		0x00	/* DEVICE init, ring 3 */
#define DevRead		0x04
#define DevPeek		0x05
#define DevInputStatus	0x06
#define DevInputFlush	0x07
#define DevWrite	0x08
#define DevWriteVerify	0x09
#define DevOutputStatus	0x0A
#define DevOutputFlush	0x0B
#define DevOpen		0x0D
#define DevClose	0x0E
#define DevIOCtl	0x10
#define DevDeinstall	0x14
#define DevPortaccess	0x15
#define DevBaseInit	0x1B	/* 2.0 BASEDEV init, ring 0 */
#define DevShutdown	0x1C	/* 2.0 */

/* IOCtl categories */
#define IOCtlAsync	1
#define IOCtlPointer	2
#define IOCtlVideo	3
#define IOCtlScreen	4
#define IOCtlKeyboard	5
#define IOCtlPrinter	6
#define IOCtlMouse	7
#define IOCtlLogDisk	8
#define IOCtlPhysDisk	9	
#define IOCtlMonitor	10
#define IOCtlGeneral	11

/* IOCtlGeneral function codes */
#define GenFlushInput	 0x01
#define GenFlushOutput	 0x02
#define GenSessionSwitch 0x41
#define GenQueryMonitor	 0x60

typedef struct DevHeader {
	dword		link;
	word		attr;
	word		entries [2];
	byte		name [8];
	word		protCS, protDS;
	word		realCS, realDS;
} DevHeader;

extern DevHeader	header;		/* caution: near address is NULL */

/************************************************************************/
/*		Device helper functions.				*/
/************************************************************************/

#define Infinite	0xFFFFFFFFL

extern word	AllocGDTSelector (farptr sels, word nsels);
extern paddr	AllocPhys	 (dword size);
extern ReqPtr	AllocReqPkt	 (word waitflag);
extern byte	AttachDD	 (nearptr name, nearptr area);
extern int	Block		 (dword id, dword time, word interruptible);
extern void	DevDone		 (ReqPtr reqpkt);
extern void	DispMsg		 (farptr msgtab);
extern void	EOI		 (word irqnum);
extern byte	FreePhys	 (paddr);
extern void	FreeReqPacket	 (ReqPtr);
extern farptr	GetDOSVar	 (word varnum, word cx);
extern void	InternalError	 (farptr msg, word len);
extern handle	Lock		 (word seg, word flags);
extern word	PhysToGDTSelector(paddr addr, word len, word sel);
extern farptr	PhysToUVirt	 (paddr addr, word len, word type, word tag);
extern word	ProtToReal	 (void);
extern byte	PullParticular	 (nearptr queue, ReqPtr reqpkt);
extern ReqPtr	PullReqPacket	 (nearptr queue);
extern word	PushReqPacket	 (nearptr queue, ReqPtr reqpkt);
extern void	QueueFlush	 (nearptr queue);
extern void	QueueInit	 (nearptr queue);
extern int	QueueRead	 (nearptr queue);
extern byte	QueueWrite	 (nearptr queue, word ch);
extern byte	RealToProt	 (void);
extern byte	RegisterStackUsage(nearptr data);
extern byte	ResetTimer	 (farfcn handler);
extern word	Run		 (dword id);
extern word	SemClear	 (handle);
extern handle	SemHandle	 (dword semid, word inuse);
extern word	SemRequest	 (handle h, dword timeout);
extern byte	SendEvent	 (word event, word arg);
extern byte	SetIRQ		 (farfcn handler, word irqnum, word shared);
extern dword	SetROMVector	 (farfcn handler, word intnum, word saveDS);
extern byte	SetTimer	 (farfcn handler);
extern void	SortReqPacket	 (nearptr queue, ReqPtr reqpkt);
extern void	TCYield		 (void);
extern byte	TickCount	 (farfcn handler, word tick_count);
extern byte	Unlock		 (handle);
extern byte	VerifyAccess	 (farptr addr, word length, word type);
extern paddr	VirtToPhys	 (farptr addr);
extern void	Yield		 (void);

/* 2.0 only: */
extern farptr	PhysToVirt	 (paddr addr, word len);
extern void	UnPhysToVirt	 (void);

/* Dos variables */
#define DosVar_SysinfoSeg	1
#define DosVar_LocinfoSeg	2
#define DosVar_DumpVector	4
#define DosVar_RebootVector	5
#define DosVar_MSATSVector	6
#define DosVar_Yield		7
#define DosVar_TCYield		8
#define DosVar_InterruptLevel	13	/* 2.0 */
#define DosVar_DeviceClassTable	14	/* 2.0 */

/* Lock flags */
#define LockShort	0x0000
#define LockLong	0x0100
#define LockLongHigh	0x0300
#define LockShortVerify	0x0400
/* Lock option - or-ed in */
#define LockNowait	0x0001

/* Event numbers */
#define EventSMMouse		0	/* Session Manager - mouse */
#define EventCtrlBreak		1	/* Ctrl-Break */
#define EventCtrlC		2	/* Ctrl-C */
#define EventCtrlScrollLock	3	/* Ctrl-ScrollLock */
#define EventCtrlPrintScreen	4	/* Ctrl-PrtSc */
#define EventShftPrintScreen	5	/* Shift-PrtSc */
#define EventSMKeyboard		6	/* Session Manager - keyboard */

/* VerifyAccess types */
#define VerifyRead	0
#define VerifyWrite	1

/* Character queue */
typedef struct CharQueue {
	word	size;		/* Size of queue in bytes */
	word	index;		/* Index of next char out */
	word	count;		/* Count of characters in the queue */
	byte	base [1];	/* Queue buffer */
} CharQueue;

/* AttachDD data record */
typedef struct IDCEntry {
	dword	real;
	word	realDS;
	dword	prot;
	word	protDS;
} IDCEntry;

/* 2.0 ADD entry point table */
typedef struct DevClassTableEntry {
	farptrfcn	entry;		/* ADD vector */
	word		flags;	
	char		name[16];
} DevClassTableEntry;
typedef struct DevClassTable {
	word		count;		/* number of entries */
	word		maxCount;	/* ??? */
	DevClassTableEntry entries[1];
} DevClassTable;

/************************************************************************/
/*		Support functions.					*/
/************************************************************************/

extern word	inProtMode (void);
extern word	inInitPhase (void);

#ifdef __ZTC__
# define int_on()	asm(0xFB)	/* STI	*/
# define int_off()	asm(0xFA)	/* CLI	*/
#else
  extern void int_on();
  extern void int_off();
#endif

#ifdef BASEDEV
  extern void	put2c (byte);
  extern void	put2s (char _far *);
  extern void	put2x (dword);
  extern void	put2d (long);
  #define putc	put2c
  #define puts	put2s
  #define putx	put2x
  #define putd	put2d
  #define _STI_ddebug _STI_d2debug
#else
  extern void	putc (byte);
  extern void	puts (char _far *);
  extern void	putx (dword);
  extern void	putd (long);
#endif

#ifdef DEBUG
 extern int	trace;
 #define PUTC(n,c) (trace >= n && putc(c))
 #define PUTS(n,s) (trace >= n && puts(s))
 #define PUTX(n,x) (trace >= n && putx(x))
 #define PUTD(n,x) (trace >= n && putd(x))
#else
 #define PUTC(n,c)
 #define PUTS(n,s)
 #define PUTX(n,x)
 #define PUTD(n,x)
#endif

extern dword	video_address;		/* set this to B0000 or B8000 */
					/* default is B8000 */

/************************************************************************/
/*		Driver specific functions.				*/
/************************************************************************/

/* Device driver dependant functions */
extern word	DrvInit		(char _far *);
extern word	DrvInitBase	(char _far *);
extern word	DrvRead		(ReqPtr);
extern word	DrvPeek		(ReqPtr);
extern word	DrvWrite	(ReqPtr, int);	/* 1: verify */
extern word	DrvInputStatus	(void);
extern word	DrvInputFlush	(void);
extern word	DrvOutputStatus	(void);
extern word	DrvOutputFlush	(void);
extern word	DrvOpen		(ReqPtr);
extern word	DrvClose	(ReqPtr);
extern word	DrvIOCtl	(ReqPtr);

/************************************************************************/

#ifdef __cplusplus
}
#endif

