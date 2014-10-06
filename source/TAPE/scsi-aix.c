/*****************************************************************************
 * $Id: scsi-aix.c,v 1.2 1992/09/12 18:10:50 ak Exp $
 *****************************************************************************
 * $Log: scsi-aix.c,v $
 * Revision 1.2  1992/09/12  18:10:50  ak
 * Added scsi_name
 * Added device name support to tctl.c
 *
 * Revision 1.1  1992/09/02  19:05:17  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: scsi-aix.c,v 1.2 1992/09/12 18:10:50 ak Exp $";

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/devinfo.h>
#include <sys/scsi.h>
#include <sys/tape.h>

#define _far

#include "scsi.h"
#include "tapedrvr.h"

#define CDB_LEN		12
#define SENSE_LEN	40

typedef unsigned char	BYTE;

typedef struct SCB { int dummy; } SCB;

static BYTE	sense_cmd[6] = { CmdRequestSense, 0, 0, 0, 0, 0 };
static int	hdev;
static int	trace = 0;
static int	blocksize = 512;
enum SenseMode	senseMode = Sensekey;

static void
fatal(char *msg, ...)
{
	va_list ap;
	fprintf(stderr, "Fatal SCSI error: ");
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end (ap);
	perror(" ");
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

void
scsi_name(char *name)
{
	char *cp;

	if (!name) {
		name = getenv("TAPE");
		if (name == NULL)
			fatal("Missing environment name TAPE\n");
	}
	while (*name == '+')
		++name;

	hdev = openx(name, O_RDWR, 0, SC_DIAGNOSTIC);
	if (hdev < 0)
		fatal("Cannot access device %s", name);

	if ((cp = getenv("TAPEMODE")) != NULL)
		senseMode = atoi(cp);
}

void
scsi_file(int fd)
{
	char *cp;

	hdev = fd;

	if ((cp = getenv("TAPEMODE")) != NULL)
		senseMode = atoi(cp);
}

void
scsi_init(void)
{
	scsi_name(NULL);
}

void
scsi_term(void)
{
	close(hdev);
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

int
scsi_cmd(int target, int lun,
	void *cdb, int cdb_len,	void *sense, int sense_len,
	void _far *data, long data_len,
	int readflag)
{
	BYTE cmd, *sptr;
	int rc, ern;
	struct sc_iocmd io;

	if (cdb_len > CDB_LEN || sense_len > SENSE_LEN)
		return SenseKey+IllegalRequest;

	if (trace)
		command(cdb, cdb_len);

	io.data_length = data_len;
	io.buffer = data;
	io.timeout_value = 2 * 3600;
	io.flags = data_len ? (readflag ? B_READ : B_WRITE) : 0;
	io.command_length = cdb_len;
	memcpy(io.scsi_cdb, cdb, cdb_len);
	rc = ioctl(hdev, STIOCMD, &io);
	ern = errno;

	if (trace >= 2) {
		if (rc < 0)
			fprintf(stderr, "ioctl errno %d\n", errno);
		if (io.status_validity == 1)
			fprintf(stderr, "SCSI bus status %X\n", io.scsi_bus_status);
		if (io.status_validity == 2)
			fprintf(stderr, "adapter status %X\n", io.adapter_status);
	}

	if (rc >= 0)
		return NoError;
	if (ern != EIO)
		return SystemError + ern;
	if (io.status_validity == 1 && io.scsi_bus_status != CheckStatus)
		return StatusError + io.scsi_bus_status;
	if (io.status_validity == 2)
		return HostError + io.adapter_status;

	/* CheckStatus */

	if (io.scsi_cdb[0] == CmdRequestSense)
		return StatusError + CheckStatus;

	io.data_length = sense_len;
	io.buffer = sense;
	io.timeout_value = 20;
	io.flags = B_READ;
	io.command_length = sizeof sense_cmd;
	memcpy(io.scsi_cdb, sense_cmd, sizeof sense_cmd);
	io.scsi_cdb[4] = sense_len;
	rc = ioctl(hdev, STIOCMD, &io);
	ern = errno;

	if (rc < 0)
		return SystemError + ern;
	if (io.status_validity == 1)
		return StatusError + io.scsi_bus_status;
	if (io.status_validity == 2)
		return HostError + io.adapter_status;
	
	if (trace)
		status(sense, sense_len);

	sptr = (BYTE *)sense;
	if ((sptr[0] & 0x7E) != 0x70)
		return sptr[0] ? ExtendedError + (sptr[0] & 0x7F) : 0;
	if (sense_len <= 2)
		return UnknownError;
	return sptr[2] ? SenseKey + (sptr[2] & 0x0F) : 0;
}

int
scsi_start(void *dcb, 
	int target, int lun,
	void *cdb, int cdb_len, int sense_len,
	void _far *data, long data_len,
	int readflag)
{
	return DriverError;
}

int
scsi_wait(void *dcb, void *sense, int wait)
{
	return DriverError;
}

int
scsi_reset(int target, int lun, int bus)
{
	return DriverError;
}

long
scsi_get_blocksize(int target, int lun)
{
	return blocksize;
}

long
scsi_set_blocksize(int target, int lun, long size)
{
	int rc;
	struct stchgp chgp;

	chgp.st_ecc = ST_NOECC;
	chgp.st_blksize = size;
	rc = ioctl(hdev, STIOCHGP, &chgp);
	if (rc < 0)
		return SystemError + errno;
	blocksize = size;
	return blocksize;
}

int
scsi_set_trace(int level)
{
	return DriverError;
}

char *
scsi_error(int code)
{
	static char text[80], *cp;

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
		sprintf(text, "Operation not implemented");
		break;
	case ErrorClass(SystemError):
		cp = strerror(ErrorCode(code));
		if (cp)
			strcpy(text, cp);
		else
			sprintf(text, "System error: %u", ErrorCode(code));
		break;
	case ErrorClass(HostError):
		sprintf(text, "Host adapter error: %u", ErrorCode(code));
		break;
	default:
		sprintf(text, "Other error: %04X", code);
		break;
	}
	return text;
}
