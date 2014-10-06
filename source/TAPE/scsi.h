/*****************************************************************************
 * $Id: scsi.h,v 1.3 1992/09/12 18:11:02 ak Exp $
 *****************************************************************************
 * $Log: scsi.h,v $
 * Revision 1.3  1992/09/12  18:11:02  ak
 * Added scsi_name
 * Added device name support to tctl.c
 *
 * Revision 1.2  1992/09/02  19:05:32  ak
 * Version 2.0
 * - EMX version
 * - AIX version
 * - SCSI-2 commands
 * - ADD Driver
 * - blocksize support
 *
 * Revision 1.1.1.1  1992/01/06  20:27:28  ak
 * Interface now based on ST01 and ASPI.
 * AHA_DRVR no longer supported.
 * Files reorganized.
 *
 * Revision 1.1  1992/01/06  20:27:26  ak
 * Initial revision
 *
 *****************************************************************************/

/************************************************************************
 *		S C S I   C o m m a n d   D e f i n i t i o n s		*
 *				(SCSI-2)				*
 ************************************************************************/

/*
 * Command Codes
 *	Codes 00..1F:  6 bytes
 *	Codes 20..5F: 10 bytes
 *	Codes 60..9F: reserved
 *	Codes A0..BF: 12 bytes
 *	Codes C0..FF: vendor specific
 *
 */
	/* General commands: */
#define CmdTestUnitReady	0x00
#define CmdRequestSense		0x03
#define CmdInquiry		0x12
#define CmdModeSelect		0x15
#define CmdModeSense		0x1A
#define CmdReserveUnit		0x16
#define CmdReleaseUnit		0x17
#define CmdCopy			0x18
#define CmdReceiveDiagnostics	0x1C
#define CmdSendDiagnostics	0x1D
#define CmdCompare		0x39
#define CmdCopyAndVerify	0x3A
#define CmdWriteBuffer		0x3B
#define CmdReadBuffer		0x3C
#define CmdChangeDefinition	0x40
#define CmdLogSelect		0x4C
#define CmdLogSense		0x4D
#define CmdModeSelect2		0x55
#define CmdModeSense2		0x5A
	/* Random access device commands: */
#define RanRezeroUnit		0x01
#define RanFormatUnit		0x04
#define RanReassignBlocks	0x07
#define RanRead			0x08
#define RanWrite		0x0A
#define RanSeek			0x0B
#define RanReserve		0x16
#define RanRelease		0x17
#define RanStartStopUnit	0x1B
#define RanRequestDiagnostics	0x1C
#define RanSendDiagnostics	0x1D
#define RanPreventMediaRemoval	0x1E
#define RanReadCapacity		0x25
#define RanRead2		0x28
#define RanWrite2		0x2A
#define RanSeek2		0x2B
#define RanWriteAndVerify	0x2E
#define RanVerify		0x2F
#define RanSearchDataHigh	0x30
#define RanSearchDataEqual	0x31
#define RanSearchDataLow	0x32
#define RanSetLimits		0x33
#define RanPreFetch		0x34
#define RanSynchronizeCache	0x35
#define RanLockUnloadCache	0x36
#define RanReadDefectData	0x37
#define RanReadLong		0x3E
#define RanWriteLong		0x3F
#define RanWriteSame		0x41
	/* Sequential access device commands: */
#define SeqRewind		0x01
#define SeqRequestBlockAddress	0x02	/* Tandberg specific */
#define SeqReadBlockLimits	0x05
#define SeqRead			0x08
#define SeqWrite		0x0A
#define SeqSeek			0x0C	/* Tandberg specific */
#define SeqReadReverse		0x0F
#define SeqWriteFilemarks	0x10
#define SeqSpace		0x11
#define SeqVerify		0x13
#define SeqRecoverBufferedData	0x14
#define SeqCopy			0x18
#define SeqErase		0x19
#define SeqLoad			0x1B
#define SeqRequestDiagnostics	0x1C
#define SeqSendDiagnostics	0x1D
#define SeqPreventMediaRemoval	0x1E
#define SeqLocate		0x2B
#define SeqReadPosition		0x34
	/* Processor type device commands (Adaptec AHA154x/1640): */
#define ProcReceive		0x08
#define ProcSend		0x0A

/*
 * Status bits.
 */
#define VADD	0x80
#define FM	0x80
#define EOM	0x40

/*
 * SCSI Command Sense Keys:
 */
				/* QTA (Quantum,Tandberg,Adaptec) */
#define NoSense		0x0	/* QT  No sense code / Invalid command (A) */
#define RecoveredData	0x1	/* Q   Recovered data error */
#define NotReady	0x2	/* QT  Unit not ready */
#define MediaError	0x3	/* QT  Unrecoverable data error */
#define Hardware	0x4	/* QT  Hardware error */
#define IllegalRequest	0x5	/* QTA Illegal parameter */
#define UnitAttention	0x6	/* QTA Reset, Media changed, Mode changed, ..*/
#define WriteProtected	0x7	/*  T  Write protected */
#define BlankCheck	0x8	/*  T  Logical end-of-media detected */
#define VendorUnique	0x9	/* Q   Look at additional sense code */
#define CopyAborted	0xA	/*  T  Error in copy command */
#define AbortedCommand	0xB	/* QTA Aborted command / Parity error (A) */
#define Reserved_SC_1	0xC	/*     ? */
#define MediaOverflow	0xD	/*  T  Beyond physical end-of-media */
#define Miscompare	0xE	/*  T  Verification failed */
#define Reserved_SC_2	0xF	/*     ? */

/*
 * SCSI Status Byte:
 */
#define GoodStatus		0x00	/* Command successful, good status */
#define CheckStatus		0x02	/* Command failed, check status */
#define ConditionMet		0x04	/* Search/prefetch operation satisfied */
#define BusyStatus		0x08	/* Drive busy */
#define Intermediate		0x10	/* Linked command successful */
#define IntermediateCondMet	0x14	/* Intermediate + ConditionMet */
#define ReservationConflict	0x18	/* Reservation conflict */
#define CommandTerminated	0x22	/* Terminated by TERMINATE I/O PROCESS */
#define QueueFull		0x28	/* Command Queue is full */

/************************************************************************
 *			S C S I   C o m m a n d s			*
 ************************************************************************/

#ifndef ASM_DECL

#if OS2 >= 2 || defined(unix)
  #define _far
#endif

extern void scsi_init(void);
extern void scsi_name(char *name);
extern void scsi_file(int fd);
extern void scsi_term(void);

extern void * scsi_alloc(void);		/* allocate control block */
extern void   scsi_free(void *);	/* deallocate control block */

extern void scsi_trace(int);

extern int scsi_cmd (int target, int lun,
		    void *cdb, int cdb_len, void *sense, int sense_len,
		    void _far *data, long data_len,
		    int readflag);

extern int scsi_start (void *dcb,
		    int target, int lun,
		    void *cdb, int cdb_len, int sense_len,
		    void _far *data, long data_len,
		    int readflag);
extern int scsi_wait (void *dcb, void *sense, int wait);

extern int scsi_reset (int target, int lun, int bus);

extern long scsi_get_blocksize (int target, int lun);
extern long scsi_set_blocksize (int target, int lun, long size);
extern int scsi_set_trace (int level);

/*
 * Sense data mode:
 */
typedef enum SenseMode {
	Sensekey	= 0,
	TDC3600		= 1,
	SCSI2		= 2
} SenseMode;

extern SenseMode	senseMode;

typedef struct ErrorTable {
	unsigned	code;
	char		*text;
} ErrorTable;

extern char *		senseTab[];
extern ErrorTable	targetStatusTab[];
extern ErrorTable	tdc3600ercd[];
extern ErrorTable	tdc3600xercd[];
extern ErrorTable	scsi2asc[];
extern ErrorTable	addErrorTab[];

extern char *	scsi_error (int);
extern char *	find_error (ErrorTable *, unsigned);

#define ErrorClass(x)	((x) & ~0x07FF)
#define ErrorCode(x)	((x) & 0x07FF)
#define NoError		0			/* completed without error */
#define SenseKey	(-0x8000L+0x0800)	/* SCSI sense key code, see below */
#define ExtendedError	(-0x8000L+0x1000)	/* SCSI sense error class + error code */
						/* only if standard sense block received */
#define StatusError	(-0x8000L+0x1800)	/* SCSI status byte */
#define DriverError	(-0x8000L+0x2000)	/* Driver error code */
#define SystemError	(-0x8000L+0x2800)	/* System error code */
#define HostError	(-0x8000L+0x3000)	/* host adapter error */
#define UnknownError	(-0x8000L+0x3800)	/* Cannot find error code */
#define TapeError	(-0x8000L+0x4000)	/* -> tape.h */
#define MappedError	(-0x8000L+0x4800)	/* Packed ADD error codes */
#define ComeAgain	-1			/* Command not yet completed */

#endif
