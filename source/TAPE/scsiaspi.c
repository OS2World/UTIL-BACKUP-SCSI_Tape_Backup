/*****************************************************************************
 * $Id: scsiaspi.c,v 1.3 1992/10/04 22:15:38 ak Exp $
 *****************************************************************************
 * $Log: scsiaspi.c,v $
 * Revision 1.3  1992/10/04  22:15:38  ak
 * Updated DOS version, scsi_name() was missing. There seems to be some
 * request for GTAK10x (DOS) with DAT.
 *
 * Revision 1.2  1992/09/02  19:05:35  ak
 * Version 2.0
 * - EMX version
 * - AIX version
 * - SCSI-2 commands
 * - ADD Driver
 * - blocksize support
 *
 * Revision 1.1.1.1  1992/01/06  20:27:32  ak
 * Interface now based on ST01 and ASPI.
 * AHA_DRVR no longer supported.
 * Files reorganized.
 *
 * Revision 1.1  1992/01/06  20:27:30  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: scsiaspi.c,v 1.3 1992/10/04 22:15:38 ak Exp $";

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef OS2
 #error "ASPI interface not usable for OS/2"
#endif

#include <io.h>
#include <fcntl.h>
#include <dos.h>

#include "aspi.h"
#include "scsi.h"

static int		trace;
static int		handle;
static void		(_far * api)(SRB _far *);
SenseMode		senseMode = SenseKey;
static long		blocksize = 512;

#define ASPI(srb)	(*api)(srb)


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

static void
command(SRB *srb)
{
	int i;
	printf("SCSI op:");
	for (i = 0; i < srb->u.cmd.cdb_len; ++i)
		printf(" %02X", srb->u.cmd.cdb_st[i]);
	printf("\n");
}

static void
status(SRB *srb)
{
	int i;
	printf("SCSI status:");
	for (i = 0; i < srb->u.cmd.sense_len; ++i) {
		if (i && (i % 16) == 0)
			printf("\n            ");
		printf(" %02X", srb->u.cmd.cdb_st[srb->u.cmd.cdb_len + i]);
	}
	printf("\n");
}

void
scsi_init(void)
{
	union REGS	r;
	struct SREGS	sr;
	char		*cp;

#ifndef OS2
	if (_osmajor >= 10)
		fatal("Cannot use SCSI interface in OS/2 DOS mode\n");
#endif

	if ((handle = open("SCSIMGR$", O_BINARY|O_RDWR)) < 0)
		fatal("SCSI Manager not found\n");

	r.x.ax = 0x4402;			/* Ioctl Read */
	sr.ds  = FP_SEG((void _far *)&api);
	r.x.dx = FP_OFF((void _far *)&api);
	r.x.cx = 4;
	r.x.bx = handle;
	intdosx(&r, &r, &sr);
	if (r.x.cflag)
		fatal("SCSI Manager Ioctl failure\n");

	if ((cp = getenv("TAPEMODE")) != NULL)
		senseMode = atoi(cp);
}

void
scsi_file(int fd)
{
	scsi_init();
}

void
scsi_name(char *name)
{
	scsi_init();
}

void
scsi_term(void)
{
	close(handle);
}

void
scsi_trace(int level)
{
	trace = level;
}

void *
scsi_alloc(void)
{
	void *p = malloc(sizeof(SRB));
	if (p == NULL)
		fatal("No memory for SCSI Request Block\n");
}

void
scsi_free(void *p)
{
	free(p);
}

int
scsi_start(void *ctrl, 
	int target, int lun,
	void *cdb, int cdb_len, int sense_len,
	void far *data, long data_len,
	int readflag)
{
	SRB *srb = ctrl;

	srb->cmd		= SRB_Command;
	srb->status		= SRB_Busy;
	srb->ha_num		= 0;
	srb->flags		= data_len ? readflag ? SRB_Read : SRB_Write
				           : SRB_NoTransfer;
	srb->u.cmd.target	= target;
	srb->u.cmd.lun		= lun;
	srb->u.cmd.data_len	= data_len;
	srb->u.cmd.sense_len	= sense_len;
	srb->u.cmd.data_ptr	= data;
	srb->u.cmd.link_ptr	= 0;
	srb->u.cmd.cdb_len	= cdb_len;
	memcpy(srb->u.cmd.cdb_st+0, cdb, cdb_len);
	memset(srb->u.cmd.cdb_st+cdb_len, 0, sense_len);

	if (trace)
		command(srb);

	ASPI(srb);

	if (trace >= 2)
		printf("SRB status: %02X\n", srb->status);

	switch (srb->status) {
	case SRB_Busy:
	case SRB_Error:
		return ComeAgain;
	case SRB_Done:
		return NoError;
	default:
		return DriverError + srb->status;
	}
}

int
scsi_wait(void *ctrl, void *sense, int wait)
{
	SRB *srb = ctrl;
	int i, slen, ret;
	unsigned char *sptr;

	for (;;) {
		if (trace >= 2)
			printf("SRB status: %02X\n", srb->status);

		switch (srb->status) {
		case SRB_Busy:
			if (wait) {
				union REGS r;

				/* DESQview - give up timeslice */
				r.x.ax = 0x1000;
				int86(0x15, &r, &r);

				/* Windows 3+ - give up timeslice */
				r.x.ax = 0x1680;
				int86(0x2F, &r, &r);

				continue;
			}
			return ComeAgain;
		case SRB_Error:
			break;
		case SRB_Done:
			return NoError;
		default:
			return DriverError + srb->status;
		}
		break;
	}

	if (srb->u.cmd.ha_status) {
		if (trace)
			printf("SCSI host adapter error: %02X\n",
				srb->u.cmd.ha_status);
		return HostError + srb->u.cmd.ha_status;
	}
	switch (srb->u.cmd.target_status) {
	case GoodStatus:
		return NoError;
	case CheckStatus:
		break;
	default:
		if (trace)
			printf("SCSI completion status: %02X\n",
				srb->u.cmd.target_status);
		return StatusError + srb->u.cmd.target_status;
	}

	slen = srb->u.cmd.sense_len ? srb->u.cmd.sense_len : 14;
	memcpy(sense, srb->u.cmd.cdb_st + srb->u.cmd.cdb_len, slen);

	if (trace)
		status(srb);

	sptr = (unsigned char *)sense;
	if ((sptr[0] & 0x7E) != 0x70)
		return ExtendedError + (sptr[0] & 0x7F);
	if (srb->u.cmd.sense_len <= 2)
		return UnknownError;
	return SenseKey + (sptr[2] & 0x0F);
}

int
scsi_cmd(int target, int lun,
	void *cdb, int cdb_len,	void *sense, int sense_len,
	void far *data, long data_len,
	int readflag)
{
	SRB srb;
	int ret;

	ret = scsi_start(&srb,
			target, lun,
			cdb, cdb_len, sense_len,
			data, data_len,
			readflag);
	while (ret == ComeAgain)
		ret = scsi_wait(&srb, sense, 1);
	return ret;
}

int
scsi_reset(int target, int lun, int bus)
{
	SRB srb;

	srb.cmd			= SRB_Reset;
	srb.status		= SRB_Busy;
	srb.ha_num		= 0;
	srb.flags		= SRB_NoTransfer;
	srb.u.res.target	= target;
	srb.u.res.lun		= lun;

	ASPI(&srb);

	for (;;) {
		switch (srb.status) {
		case SRB_Busy:
			continue;
		case SRB_Done:
			return NoError;
		default:
			return DriverError + srb.status;
		}
	}
}

long
scsi_get_blocksize(int target, int lun)
{
	return blocksize;
}

long
scsi_set_blocksize(int target, int lun, long size)
{
	long old = blocksize;
	blocksize = size;
	return old;
}

int
scsi_set_trace(int level)
{
	int old = trace;
	trace = level;
	return old;
}

char *
scsi_error(int code)
{
	static char text[80];
	static ErrorTable driverTab[] = {
		SRB_Aborted,	"Command aborted",
		SRB_BadAbort,	"Could not abort",
		SRB_InvalidCmd,	"Invalid ASPI request",
		SRB_InvalidHA,	"Invalid host adapter",
		SRB_BadDevice,	"Device not installed",
		-1
	};
	static ErrorTable hostTab[] = {
			/* Adaptec 154x host adapter status */
		SRB_NoError,		"No error",
		SRB_Timeout,		"Selection timeout",
		SRB_DataLength,		"Data length error",
		SRB_BusFree,		"Unexpected bus free",
		SRB_BusSequence,	"Target bus sequence failure",
		-1
	};

	if (code == 0)
		return "No error";
	if (code == ComeAgain)
		return "Busy";
	switch (ErrorClass(code)) {
	case SenseKey:
		return senseTab[code & 0x0F];
	case ExtendedError:
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
	case StatusError:
		sprintf(text, "Target status: %s",
			find_error(targetStatusTab, ErrorCode(code)));
		break;
	case DriverError:
		sprintf(text, "Driver error: %s",
			find_error(driverTab, ErrorCode(code)));
		break;
	case SystemError:
		sprintf(text, "System error: %u", ErrorCode(code));
		break;
	case HostError:
		sprintf(text, "Host adapter error: %s",
			find_error(hostTab, ErrorCode(code)));
		break;
	default:
		sprintf(text, "Other error: %04X", code);
		break;
	}
	return text;
}
