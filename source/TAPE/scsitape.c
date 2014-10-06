/*****************************************************************************
 * $Id: scsitape.c,v 1.4 1992/10/14 18:35:14 ak Exp $
 *****************************************************************************
 * $Log: scsitape.c,v $
 * Revision 1.4  1992/10/14  18:35:14  ak
 * IBM SCSI driver.
 *
 * Revision 1.3  1992/09/12  18:10:52  ak
 * Added scsi_name
 * Added device name support to tctl.c
 *
 * Revision 1.2  1992/09/02  19:05:43  ak
 * Version 2.0
 * - EMX version
 * - AIX version
 * - SCSI-2 commands
 * - ADD Driver
 * - blocksize support
 *
 * Revision 1.1.1.1  1992/01/06  20:27:14  ak
 * Interface now based on ST01 and ASPI.
 * AHA_DRVR no longer supported.
 * Files reorganized.
 *
 * Revision 1.1  1992/01/06  20:27:12  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: scsitape.c,v 1.4 1992/10/14 18:35:14 ak Exp $";

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#define INCL_DOSFILEMGR
#define INCL_DOSDEVICES
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_ERRORS
#include <os2.h>

#ifdef __EMX__
#ifndef _far
#define _far
#endif
#endif

#include "scsi.h"
#include "aspi.h"
#include "tapedrvr.h"

#define CDB_LEN		12
#define SENSE_LEN	40

#if OS2 >= 2
  typedef ULONG		Word;
#else
  typedef USHORT	Word;
#endif

typedef struct SCB {
	void *		cdb;
	void _far *	data;
	long		data_len;
	BYTE		target;
	BYTE		lun;
	BYTE		cdb_len;
	BYTE		sense_len;
	BYTE		readflag;
	BYTE		sense [SENSE_LEN];
	unsigned	error;
} SCB;

static HFILE	hdev;
static BYTE	sense_cmd[6] = { CmdRequestSense, 0, 0, 0, 0, 0 };
static TID	thread_id;
#if OS2 >= 2
  static HMTX	mutex;
  static HEV	done;
#else
  static BYTE	thread_stack[4096];
  static ULONG	sema;
#endif
static SCB	*thread_scb;
static BYTE	trace = 0;
static BYTE	drvLevel = ST01driver;
enum SenseMode	senseMode = Sensekey;

static void
fatal(char *msg, ...)
{
	va_list ap;
	fprintf(stderr, "Fatal SCSI error: ");
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end (ap);
	fprintf(stderr, "\n");
	exit (2);
}

command(BYTE *cdb, int len)
{
	int i;
	fprintf(stderr, "SCSI op:");
	for (i = 0; i < len; ++i)
		fprintf(stderr, " %02X", cdb[i]);
	fprintf(stderr, "\n");
}

status(BYTE *sense, int len)
{
	int i;
	fprintf(stderr, "SCSI status:");
	for (i = 0; i < len; ++i) {
		if (i && (i % 16) == 0)
			fprintf(stderr, "\n            ");
		fprintf(stderr, " %02X", sense[i]);
	}
	fprintf(stderr, "\n");
}

static void
init()
{
	BYTE data[2];
	char *cp;
#if OS2 >= 2
	ULONG datalen = 2;

	if (DosDevIOCtl(hdev, IOCtlCategory, IOCtlLevel,
				0, 0, 0,  data, datalen, &datalen) == 0)
#else
	if (DosDevIOCtl2(data, 2, 0, 0, IOCtlLevel, IOCtlCategory, hdev) == 0)
#endif
	{
		drvLevel = data[0];
		senseMode = data[1];
		if (trace)
			fprintf(stderr, "Device driver level: %d, sense mode: %d\n",
				drvLevel, senseMode);
	} else {
		if ((cp = getenv("TAPEMODE")) != NULL)
			senseMode = atoi(cp);
		if (trace)
			fprintf(stderr, "Device driver level: 0, sense mode: %d\n",
				senseMode);
	}
}

void
scsi_file(int fd)
{
	hdev = fd;
	init();
}

void
scsi_name(char *name)
{
	Word rc, action;
	char *cp;

	if (!name) {
		name = getenv("TAPE");
		if (name == NULL)
			fatal("Missing environment name TAPE\n");
	}
	while (*name == '+')
		++name;

#if OS2 >= 2
	rc = DosOpen((PSZ)name, &hdev, &action, 0L, 0,
		FILE_OPEN, OPEN_ACCESS_READWRITE+OPEN_SHARE_DENYNONE+OPEN_FLAGS_FAIL_ON_ERROR, 0);
#else
	rc = DosOpen((PSZ)name, &hdev, &action, 0L, 0,
		FILE_OPEN, OPEN_ACCESS_READWRITE+OPEN_SHARE_DENYNONE+OPEN_FLAGS_FAIL_ON_ERROR, 0L);
#endif
	if (rc)
		fatal("Cannot access device %s, return code %u", name, rc);
	init();
}

void
scsi_init(void)
{
	scsi_name(NULL);
}

void
scsi_term(void)
{
	DosClose(hdev);
}

void
scsi_trace(int level)
{
	trace = level;
}

void *
scsi_alloc(void)
{
	void *p = malloc(sizeof(SCB));
	if (p == NULL)
		fatal("No memory for SCSI Control Block\n");
}

void
scsi_free(void *p)
{
	free(p);
}

static int
mapDriverError(unsigned rc)
{
	if (rc < 0xFF00)
		return SystemError + ErrorCode(rc);
	rc &= 0xFF;

	switch (drvLevel) {
	case ST01driver:
		return DriverError + ErrST01Driver + (rc & ErrMask);
	case ADDdriver:
		if (rc < ErrTapeDriver)
			return MappedError + rc;
		break;
	case SCSIdriver:
		if (rc < 0x20)
			return DriverError + 0x100 + rc;
		if (rc < 0x40)
			return UnknownError + (rc - 0x20);
		if (rc < 0x50)
			return StatusError + ((rc - 0x40) << 1);
		if (rc < 0x80)
			return HostError + 0x100 + (rc - 0x50);
		if (rc < 0xA0)
			return DriverError + 0x100 + rc;
	}

	switch (rc & ErrSource) {
	case ErrTargetStatus:
		return StatusError + (rc & ErrMask);
	case ErrST01Driver:
	case ErrASPIDriver1:
	case ErrASPIDriver2:
	case ErrTapeDriver:
		return DriverError + rc;
	case ErrHostAdapter:
		return HostError + (rc & ErrMask);
	default:
		return UnknownError + rc;
	}
}

int
scsi_cmd(int target, int lun,
	void *cdb, int cdb_len,	void *sense, int sense_len,
	void _far *data, long data_len,
	int readflag)
{
	BYTE	cmd, *sptr;
	Word	fcn, rc, rc2, len;
#if OS2 >= 2
	ULONG	parmlen, datalen;
#endif

	if (cdb_len > CDB_LEN || sense_len > SENSE_LEN)
		return SenseKey+IllegalRequest;

	cmd = *(BYTE *)cdb;
	fcn = (cmd == SeqRead || cmd == SeqWrite) ? IOCtlFast : IOCtlSlow;
	if (drvLevel)
		fcn += readflag ? IOCtlRead : IOCtlWrite;
	if (trace) {
		fprintf(stderr, "\tIOCtl=%02X, ", fcn);
		command(cdb, cdb_len);
	}
#if OS2 >= 2
	parmlen = cdb_len;
	datalen = data_len;
	rc = DosDevIOCtl(hdev, IOCtlCategory, fcn,
			cdb, parmlen, &parmlen,  data, datalen, &datalen);
#else
	if (data_len)
		rc = DosDevIOCtl2(data, (Word)data_len, cdb, cdb_len,
				fcn, IOCtlCategory, hdev);
	else
		rc = DosDevIOCtl2(0, 0, cdb, cdb_len,
				fcn, IOCtlCategory, hdev);
#endif

	if (trace >= 2)
		fprintf(stderr, "IOCtl return %X\n", rc);
	if (drvLevel != ST01driver && rc == 0)
		return 0;
	if (cmd == CmdRequestSense)
		return rc ? mapDriverError(rc) : 0;
	if (rc)
		switch (drvLevel) {
		case ST01driver:
			if (rc != 0xFF00+CheckStatus)
				return mapDriverError(rc);
			break;
		case ASPIdriver:
			if (rc != 0xFF00+ErrTargetStatus+CheckStatus)
				return mapDriverError(rc);
			break;
		case SCSIdriver:
			if (rc != 0xFF41)
				return mapDriverError(rc);
			break;
		}

#if OS2 >= 2
	parmlen = sizeof sense_cmd;
	datalen = sense_len;
	rc2 = DosDevIOCtl(hdev, IOCtlCategory, drvLevel ? IOCtlSense : IOCtlSlow,
			sense_cmd, parmlen, &parmlen, sense, datalen, &datalen);
	if (trace && sense_len != datalen) {
		fprintf(stderr, "Got %d sense bytes\n", datalen);
		sense_len = datalen;
	}
#else
	rc2 = DosDevIOCtl2(sense, (Word)sense_len, sense_cmd, sizeof sense_cmd,
			drvLevel ? IOCtlSense : IOCtlSlow, IOCtlCategory, hdev);
#endif
	if (rc2)
		return mapDriverError(rc ? rc : rc2);

	if (trace)
		status(sense, sense_len);

	sptr = (BYTE *)sense;
	if ((sptr[0] & 0x7E) != 0x70)
		return sptr[0] ? ExtendedError + (sptr[0] & 0x7F) : 0;
	if (sense_len <= 2)
		return UnknownError;
	return sptr[2] ? SenseKey + (sptr[2] & 0x0F) : 0;
}

#pragma check_stack(off)

static void _far
#if OS2 >= 2
  thread_fcn(ULONG dummy)
#else
  thread_fcn(void)
#endif
{
	for (;;) {
		DosSuspendThread(thread_id);
		if (thread_scb) {
			thread_scb->error = scsi_cmd(thread_scb->target,
				thread_scb->lun,
				thread_scb->cdb, thread_scb->cdb_len,
				thread_scb->sense, thread_scb->sense_len,
				thread_scb->data, thread_scb->data_len,
				thread_scb->readflag);
#if OS2 >= 2
			DosPostEventSem(done);
			DosReleaseMutexSem(mutex);
#else
			DosSemClear((HSEM)&sema);
#endif
		}
	}
}

#pragma check_stack()

int
scsi_start(void *dcb, 
	int target, int lun,
	void *cdb, int cdb_len, int sense_len,
	void _far *data, long data_len,
	int readflag)
{
	SCB *scb = (SCB *)dcb;
#if OS2 >= 2
	ULONG npost;
#endif

	if (sense_len > SENSE_LEN)
		return SenseKey+IllegalRequest;

	if (thread_id == 0) {
		Word rc;
#if OS2 >= 2
		rc = DosCreateThread(&thread_id, thread_fcn, 0, 0, 8192);
		if (rc)
			fatal("Cannot create thread, return code %u", rc);
		rc = DosCreateMutexSem(0, &mutex, 0, 0);
		if (rc)
			fatal("Cannot create mutex sema, return code %u", rc);
		rc = DosCreateEventSem(0, &done, 0, 0);
		if (rc)
			fatal("Cannot create event sema, return code %u", rc);
#else
		rc = DosCreateThread(thread_fcn, &thread_id, thread_stack+4096);
		if (rc)
			fatal("Cannot create thread, return code %u", rc);
		DosSemClear((HSEM)&sema);
#endif
	}

	if (thread_scb)
		return SystemError+ErrorCode(ERROR_DEVICE_IN_USE);

#if OS2 >= 2
	DosRequestMutexSem(mutex, SEM_INDEFINITE_WAIT);
#else
	DosSemWait((HSEM)&sema, -1);
#endif

	scb->target    = target;
	scb->lun       = lun;
	scb->cdb       = cdb;
	scb->cdb_len   = cdb_len;
	scb->data      = data;
	scb->data_len  = data_len;
	scb->sense_len = sense_len;
	scb->readflag  = readflag;
	thread_scb = scb;

#if OS2 >= 2
	DosResetEventSem(done, &npost);
#else
	DosSemSet((HSEM)&sema);
#endif
	DosResumeThread(thread_id);

	return ComeAgain;
}

int
scsi_wait(void *dcb, void *sense, int wait)
{
	SCB *scb = (SCB *)dcb;
	int r;

	if (scb != thread_scb)
		return SystemError+ErrorCode(ERROR_INVALID_HANDLE);
#if OS2 >= 2
	if (DosWaitEventSem(done, SEM_INDEFINITE_WAIT))
#else
	if (DosSemWait((HSEM)&sema, wait ? -1 : 0))
#endif
		return ComeAgain;
	thread_scb = 0;
	memcpy(sense, scb->sense, scb->sense_len);
	r = scb->error;
	return r;
}

int
scsi_reset(int target, int lun, int bus)
{
#if OS2 >= 2
	Word rc = DosDevIOCtl(hdev, IOCtlCategory, bus ? IOCtlBusReset : IOCtlDevReset,
			0, 0, 0,  0, 0, 0);
#else
	Word rc = DosDevIOCtl(0, 0,
		bus ? IOCtlBusReset : IOCtlDevReset, IOCtlCategory, hdev);
#endif
	if (rc)
		return mapDriverError(rc);
	return NoError;
}

long
scsi_get_blocksize(int target, int lun)
{
	long sz;
#if OS2 >= 2
	ULONG dlen;
	Word rc = DosDevIOCtl(hdev, IOCtlCategory, IOCtlBlocksize,
			0, 0, 0,  &sz, sizeof sz, &dlen);
#else
	Word rc = DosDevIOCtl2(&sz, sizeof sz,  0, 0,
			IOCtlBlocksize, IOCtlCategory, hdev);
#endif
	return rc ? mapDriverError(rc) : sz;
}

long
scsi_set_blocksize(int target, int lun, long size)
{
	long sz = size;
#if OS2 >= 2
	ULONG plen, dlen;
	Word rc = DosDevIOCtl(hdev, IOCtlCategory, IOCtlBlocksize,
			&size, sizeof size, &plen,  &sz, sizeof sz, &dlen);
#else
	Word rc = DosDevIOCtl2(&sz, sizeof sz,  &size, sizeof size,
			IOCtlBlocksize, IOCtlCategory, hdev);
#endif
	return rc ? mapDriverError(rc) : sz;
}

int
scsi_set_trace(int level)
{
	unsigned char lvl = level;
#if OS2 >= 2
	ULONG plen, dlen;
	Word rc = DosDevIOCtl(hdev, IOCtlCategory, IOCtlTrace,
			&level, sizeof level, &plen,  &lvl, sizeof lvl, &dlen);
#else
	Word rc = DosDevIOCtl2(&lvl, sizeof lvl,  &level, sizeof level,
			IOCtlTrace, IOCtlCategory, hdev);
#endif
	return rc ? mapDriverError(rc) : lvl;
}

char *
scsi_error(int code)
{
	static char text[80];
	static ErrorTable driverTab[] = {
			/* ST01 driver error codes */
		ErrST01Driver+0x02,	"Device not ready",
		ErrST01Driver+0x0C,	"Bus sequence error",
		ErrST01Driver+0x13,	"Parameter error",

			/* ASPI status codes */
		ErrASPIDriver1+0x00,	"Busy",
		ErrASPIDriver1+0x01,	"Done",
		ErrASPIDriver1+0x02,	"Aborted",
		ErrASPIDriver1+0x03,	"Bad aborted",
		ErrASPIDriver1+0x04,	"Error",
		ErrASPIDriver1+0x10,	"Busy POST",
		ErrASPIDriver2+0x00,	"Invalid ASPI request",
		ErrASPIDriver2+0x01,	"Invalid host adapter",
		ErrASPIDriver2+0x02,	"Device not installed",

			/* ASPITAPE driver error codes */
		TapeInvalidFcn,		"Invalid ioctl cat/fcn code",
		TapeInvalidParm,	"Invalid parm pointer/length",
		TapeInvalidData,	"Invalid data pointer/length",
		TapeNoSenseData,	"No sense data",

			/* OS2SCSI.DMD driver error codes */
				/* general driver error codes */
		0x100,			"Write protected",
		0x101,			"Unknown unit",
		0x102,			"Device not ready",
		0x103,			"Unknown command",
		0x104,			"CRC error",
		0x105,			"Bad request struct length",
		0x10A,			"Write fault",
		0x10B,			"Read fault",
		0x10C,			"Generail failure",
		0x10D,			"Change disk",
		0x110,			"Uncertain media",
		0x114,			"Device in use",
				/* OS2SCSI.DMD specific codes */
		0x180,			"Device error",
		0x181,			"Timeout error",
		0x182,			"Unusual wakeup",
		0x183,			"DevHlp error",
		0x184,			"Request block not available",
		0x185,			"Maximum device support exceeded",
		0x186,			"Interrupt level not available",
		0x187,			"Device not available",
		0x188,			"More IRQ levelsthan adapters",
		0x189,			"Device busy",
		0x18A,			"Request sense failed",
		0x18B,			"Intelligent buffer not supported",

		-1
	};
	static ErrorTable hostTab[] = {
			/* Adaptec 154x host adapter status */
		SRB_NoError,		"No error",
		SRB_Timeout,		"Selection timeout",
		SRB_DataLength,		"Data length error",
		SRB_BusFree,		"Unexpected bus free",
		SRB_BusSequence,	"Target bus sequence failure",
			/* IBM SCSI adapter error */
		0x101,			"Bus reset",
		0x102,			"Interface fault",
		0x110,			"Selection timeout",
		0x111,			"Unexpected bus free",
		0x113,			"Bus sequence error",
		0x120,			"Short data",
		-1
	};

	if (code == 0)
		return "No error";
	if (code == ComeAgain)
		return "Busy";
	switch (ErrorClass(code)) {
	case ErrorClass(SenseKey):
		return senseTab[code & 0x0F];
	case ErrorClass(ExtendedError):
		switch (senseMode) {
		case TDC3600:
			sprintf(text, "Error code: %s",
				find_error(tdc3600ercd, ErrorCode(code)));
			break;
		default:
			sprintf(text, "Additional sense code: %02X",
				ErrorCode(code));
		}
		break;
	case ErrorClass(StatusError):
		sprintf(text, "Target status: %s",
			find_error(targetStatusTab, ErrorCode(code)));
		break;
	case ErrorClass(DriverError):
		sprintf(text, "Driver error: %s",
			find_error(driverTab, ErrorCode(code)));
		break;
	case ErrorClass(SystemError):
		sprintf(text, "System error: %u", ErrorCode(code));
		break;
	case ErrorClass(HostError):
		sprintf(text, "Host adapter error: %s",
			find_error(hostTab, ErrorCode(code)));
		break;
	case ErrorClass(MappedError):
		sprintf(text, "ADD error code: %s",
			find_error(addErrorTab, ErrorCode(code)));
		break;
	default:
		sprintf(text, "Other error: %04X", code);
		break;
	}
	return text;
}
