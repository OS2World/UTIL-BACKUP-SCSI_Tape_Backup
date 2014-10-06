/*****************************************************************************
 * $Id: tctl.c,v 1.4 1992/10/12 19:48:41 ak Exp $
 *****************************************************************************
 * $Log: tctl.c,v $
 * Revision 1.4  1992/10/12  19:48:41  ak
 * Upper/lower case was significant.
 *
 * Revision 1.3  1992/09/12  18:10:59  ak
 * Added scsi_name
 * Added device name support to tctl.c
 *
 * Revision 1.2  1992/09/02  19:05:40  ak
 * Version 2.0
 * - EMX version
 * - AIX version
 * - SCSI-2 commands
 * - ADD Driver
 * - blocksize support
 *
 * Revision 1.1.1.1  1992/01/06  20:27:09  ak
 * Interface now based on ST01 and ASPI.
 * AHA_DRVR no longer supported.
 * Files reorganized.
 *
 * Revision 1.1  1992/01/06  20:27:08  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: tctl.c,v 1.4 1992/10/12 19:48:41 ak Exp $";

/*
 *	tctl.c
 *
 * Tape Control.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#ifndef unix
#include <io.h>
#endif
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "scsi.h"
#include "tape.h"
#include "scsitape.h"

#define NBLOCKS	 60

#define INT16(x) ((x)[0] << 8 | (x)[1])
#define INT24(x) ((unsigned long)INT16(x) << 8 | (x)[2])
#define INT32(x) (INT24(x) << 8 | (x)[3])

#ifdef __EMX__
  #define strtoul	strtol
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

static ExtSenseData	sense;
static unsigned char	cdb [12];
static unsigned char	inq [255];
static unsigned char	mode [255];
static unsigned char	buffer [NBLOCKS][512];
static int		trace = 0;
static FILE *		out;

extern int		scsiLevel;

typedef int	(*fcnptr)(void _far *, long);

#ifdef unix

int
strnicmp(char *x, char *y, unsigned n)
{
	while (n--) {
		char xx = isupper(*x) ? tolower(*x) : *x;
		char yy = isupper(*y) ? tolower(*y) : *y;
		if (xx < yy)
			return -1;
		if (xx > yy)
			return 1;
		if (xx == '\0')
			return 0;
	}
	return 0;
}

int
stricmp(char *x, char *y)
{
	return strnicmp(x, y, UINT_MAX);
}

#endif

static void
print_inquiry(void)
{
	int k;

	if ((inq[0] & 0x1F) == 0x01)
		fprintf(out, "Tape device");
	else
		fprintf(out, "Device type %02X", inq[0] & 0x1F);
	if (inq[0] & 0xE0)
		fprintf(out, ", qualifier %X", inq[0] >> 5);
	if (inq[1] & 0x7F)
		fprintf(out, ", modifier %02X", inq[1] & 0x7F);
	fprintf(out, ", %sremovable\n", (inq[1] & 0x80) ? "" : "not ");
	fprintf(out, "ISO %d, ECMA %d, ANSI %d\n",
		inq[2] >> 6 & 3, inq[2] >> 3 & 7, inq[2] >> 0 & 3);
	fprintf(out, "can%s terminate i/o process\n", (inq[3] & 0x40) ? "" : "not");
	if (inq[3] & 0x0F)
		fprintf(out, "response data format is SCSI-%d\n", inq[3] & 0x0F);
	else
		fprintf(out, "old response data format (perhaps CCS)\n");
	fprintf(out, "relative addressing %ssupported\n", (inq[7] & 0x80) ? "" : "not ");
	if (inq[7] & 0x40)
		fprintf(out, "32-bit bus width supported\n");
	if (inq[7] & 0x20)
		fprintf(out, "16-bit bus width supported\n");
	fprintf(out, "synchronous transfers %ssupported\n", (inq[7] & 0x10) ? "" : "not ");
	fprintf(out, "linked commands %ssupported\n", (inq[7] & 0x08) ? "" : "not ");
	fprintf(out, "caching %ssupported\n", (inq[7] & 0x04) ? "" : "not ");
	fprintf(out, "command queueing %ssupported\n", (inq[7] & 0x02) ? "" : "not ");
	if (!(inq[7] & 0x01))
		fprintf(out, "soft reset is hard reset\n");
	fprintf(out, "identification '%.48s'\n", inq+8);
	fprintf(out, "\n");
}

static void
dump(unsigned char *p, int len)
{
	int r, c;
	for (r = 0; r < len; r += 16) {
		fprintf(out, "\t");
		for (c = r; c < r + 16; c += 1)
			if (c < len)
				fprintf(out, "%02X ", p[c]);
			else
				fprintf(out, "   ");
		fprintf(out, "  ");
		for (c = r; c < r + 16; c += 1)
			if (c < len)
				fprintf(out, "%c", isprint(p[c]) ? p[c] : '.');
		fprintf(out, "\n");
	}
}

static void
print_mode(int page, int skip)
{
	int e, b, k, df;
	static char * pageval[] = {
		"current",
		"changeable",
		"default",
		"saved"
	};
	static char * ifcid[] = {
		"SCSI (X3.131)",
		"SMI (X3.91M-1987",
		"ESDI (X3.170)",
		"IPI-2 (X3.130-1986; X3T9.3(87-002",
		"IPI-3 (X3.132-1987; X3.147-1988",
	};

	if (skip)
		b = 4 + mode[3];
	else {
		fprintf(out, "Header:\tmedia type %02X", mode[1]);
		if (mode[2] & 0x80)
			fprintf(out, ", write protected");
		if (mode[2] & 0x70)
			fprintf(out, ", buffer mode %d", (mode[2] & 0x70) >> 4);
		else
			fprintf(out, ", unbuffered");
		fprintf(out, ", speed code %d\n", mode[2] & 0xF);
		fprintf(out, "\n");
		if (trace)
			dump(mode, 4);

		for (b = 4; b < 4 + mode[3]; b += 8) {
			fprintf(out, "Block:\tdensity code %02X, %ld blocks of %ld bytes\n",
				mode[b+0], INT24(mode+b+1), INT24(mode+b+5));
			if (trace)
				dump(mode+4+b, 8);
		}
	}
	
	fprintf(out, "Page %02X (%s values, %ssavable): ",
		page & 0x3F,
		pageval[(page & 0xC0) >> 6],
		(mode[b+0] & 0x80) ? "" : "not ");

	switch(k = mode[b+0] & 0x3F) {

	case 0x00:
		fprintf(out, "Vendor specific\n");
		dump(mode+b, 12);
		return;

	case 0x01:
		fprintf(out, "Read-Write Error Recovery\n");
		fprintf(out, "\t%stransfer block on error\n",
			mode[b+2] & 0x20 ? "" : "do not ");
		fprintf(out, "\tearly recovery %sabled\n",
			mode[b+2] & 0x08 ? "en" : "dis");
		fprintf(out, "\t%spost recovered errors\n",
			mode[b+2] & 0x04 ? "" : "do not ");
		fprintf(out, "\t%sterminate transfer on error\n",
			mode[b+2] & 0x02 ? "" : "do not ");
		fprintf(out, "\terror correction %sabled\n",
			mode[b+2] & 0x08 ? "en" : "dis");
		fprintf(out, "\twrite retry count: %d\n", mode[b+8]);
		fprintf(out, "\tpost log ready %sabled\n",
			mode[b+9] & 0x80 ? "en" : "dis");
		fprintf(out, "\tattach log to sense %sabled\n",
			mode[b+9] & 0x80 ? "en" : "dis");
		fprintf(out, "\trelative treshold: %d\n", mode[b+9] & 0x0F);
		break;

	case 0x02:
		fprintf(out, "Disconnect-Reconnect\n");
		fprintf(out, "\tbuffer full ratio: %u\n", mode[b+2]);
		fprintf(out, "\tbuffer empty ratio: %u\n", mode[b+3]);
		fprintf(out, "\tbus inactivity limit: %lu\n", INT16(mode+b+4));
		fprintf(out, "\tdisconnect time limit: %lu\n", INT16(mode+b+6));
		fprintf(out, "\tconnect time limit: %lu\n", INT16(mode+b+8));
		fprintf(out, "\tmaximum burst size: %lu\n", INT16(mode+b+10));
		if (mode[b+1] >= 13 && mode[b+12] & 3)
			fprintf(out, "\tdon't disconnect within transfer\n");
		return;

	case 0x09:
		fprintf(out, "Periperal Device\n");
		fprintf(out, "\tinterface: ");
		if ((k = INT16(mode+b+2)) < 5)
			fprintf(out, "%s\n", ifcid[k]);
		else
			fprintf(out, "%04X\n", k);
		break;

	case 0x0A:
		fprintf(out, "Control Mode\n");
		break;

	case 0x10:
		fprintf(out, "Device Configuration\n");
		if (mode[b+2] & 0x40)
			fprintf(out, "\tchange active partition to %d\n", mode[b+3]);
		if (mode[b+2] & 0x20)
			fprintf(out, "\tchange active format to %02X\n", mode[b+2] & 0x1F);
		if (mode[b+4] || mode[b+5])
			fprintf(out, "\twrite buffer full ratio %d"
			       ", read buffer empty ratio %d\n",
					mode[b+4], mode[b+5]);
		if ((k = INT16(mode+b+6)) != 0)
			fprintf(out, "\twrite delay %d.%1d sec\n", k / 10, k % 10);
		if (mode[b+8] & 0x80)
			fprintf(out, "\tsupports data buffer recovery\n");
		if (mode[b+8] & 0x40)
			fprintf(out, "\tsupports block IDs\n");
		if (mode[b+8] & 0x20)
			fprintf(out, "\trecognizes setmarks\n");
		if (mode[b+8] & 0x10)
			fprintf(out, "\tautomatic velocity control\n");
		if (mode[b+8] & 0x0C)
			fprintf(out, "\tstops pre-read on %d consecutive filemarks\n",
				mode[b+8] >> 2 & 3);
		else
			fprintf(out, "\tno stop on consecutive filemarks\n");
		fprintf(out, "\tbuffer recovery in %s order\n",
				(mode[b+8] & 0x02) ? "LIFO" : "FIFO");
		if (mode[b+8] & 0x01)
			fprintf(out, "\treturns %s at early warning position for read and write\n",
				(mode[b+19] & 0x08) ? "EOM" : "VOLUME OVERFLOW");
		else
			fprintf(out, "\tdoes not report early warning on read\n");
		fprintf(out, "\tgap size %02X\n", mode[b+9]);
		fprintf(out, "\t%s data at early warning\n",
			(mode[b+10] & 0x08) ? "synchronizes (flushes)" : "retains");
		switch (mode[b+10] & 0xF0) {
		case 0x10: fprintf(out, "\tdefault EOD generation\n"); break;
		case 0x30: fprintf(out, "\tformat defined EOD generation\n"); break;
		case 0x50: fprintf(out, "\tsee SOCF (\"consecutive filemarks\")\n"); break;
		case 0x70: fprintf(out, "\tEOD not supported\n"); break;
		default:   fprintf(out, "\tEOD generation disabled\n"); break;
		}
		fprintf(out, "\tbuffer size reduced by %d at early warning\n",
			INT24(mode+b+11));
		if (mode[b+1] > 14)
			if (mode[b+14])
				fprintf(out, "\tcompression mode %d\n", mode[b+14]);
			else
				fprintf(out, "\tno compression\n");
		break;

	case 0x11:
		fprintf(out, "Medium Partition (1)\n");
		fprintf(out, "\t%d additional partitions supported\n", mode[b+2]);
		if (mode[b+4] & 0x80)
			fprintf(out, "\tfixed partition assignments\n");
		else if (mode[b+4] & 0x40)
			fprintf(out, "\tdefine %d device partitions\n",
				mode[b+3]);
		else if (mode[b+4] & 0x20) {
			static char * psum[] = {"bytes","KB","MB","?"};
			fprintf(out, "\tdefine %d partitions (%s):",
				psum[mode[b+4] >> 3 & 3]);
			for (k = 0; k < mode[b+3]; ++k)
				fprintf(out, " %d", INT16(mode+b+8+2*k));
			fprintf(out, "\n");
		}
		switch (mode[b+5]) {
		case 0x00: fprintf(out, "\tdoes not recognize format or partitions\n"); break;
		case 0x01: fprintf(out, "\trecognizes format\n"); break;
		case 0x02: fprintf(out, "\trecognizes partitions\n"); break;
		case 0x03: fprintf(out, "\trecognizes format and partitions\n"); break;
		default:   fprintf(out, "\tformat recognition: %02X\n");
		}
		break;

	case 0x12:
		fprintf(out, "Medium Partition (2)\n");
		break;

	case 0x13:
		fprintf(out, "Medium Partition (3)\n");
		break;

	case 0x14:
		fprintf(out, "Medium Partition (4)\n");
		break;

	case 0x20:
		fprintf(out, "Miscellaneous Parameters\n");
		fprintf(out, "\tforced streaming count: %u\n", INT16(mode+b+2));
		fprintf(out, "\tcopy sense allocation: %u\n", mode[b+4]);
		fprintf(out, "\tcopy disconnect %sabled\n",
			mode[b+5] & 1 ? "dis" : "en");
		switch (mode[b+6]) {
		case 0:	fprintf(out, "\tauto load\n"); break;
		case 1:	fprintf(out, "\tauto retension\n"); break;
		case 2:	fprintf(out, "\tno auto load, no auto retension\n"); break;
		}
		fprintf(out, "\tpower-up retension delay: %u.%u sec\n",
			mode[b+7] / 10, mode[b+7] % 10);
		fprintf(out, "\tfast space mode %sabled\n",
			mode[b+8] & 1 ? "en" : "dis");
		break;

	default:
		if (k < 0x15)
			fprintf(out, "Reserved\n");
		else if (k == 0x3F)
			fprintf(out, "All pages\n");
		else
			fprintf(out, "Vendor specific\n");
		break;
	}
	dump(mode+b, 2 + mode[b+1]);
}

int
read_tape(FILE *file)
{
	int r;
	long actual, count = 0, size = tape_get_blocksize();
	if (size <= 0)
		size = 512;
	else if (size > sizeof buffer) {
		fprintf(out, "blocksize %ld too large\n", size);
		exit(1);
	} else
		size = (sizeof buffer / size) * size;
	do {
		r = tape_read(buffer, size, &actual);
		if (actual != 0 && actual != TapeUndefLength) {
			if (fwrite(buffer, 1, actual, file) != actual) {
				perror("writing");
				exit(1);
			}
			count += actual;
		}
	} while (r == 0);
	fflush(file);
	if (ferror(file)) {
		perror("reading");
		exit(1);
	}
	fprintf(out, "	%ld bytes read\n", count);
	return r;
}

int
write_tape(FILE *file)
{
	int n, r;
	long count = 0;
	long actual;
	long blocksize = tape_get_blocksize();
	long size = blocksize;
	if (blocksize < 0)
		return blocksize;
	if (size <= 0)
		size = 512;
	else if (size > sizeof buffer) {
		fprintf(out, "blocksize %ld too large\n", size);
		exit(1);
	} else
		size = (sizeof buffer / size) * size;
	while ((n = fread(buffer, 1, size, file)) > 0) {
		if (n % blocksize) {
			int ob = n % blocksize;
			int nb = blocksize - ob;
			memset(buffer[0] + ob, 0, nb);
			fprintf(out, "\t%d null bytes appended to align to block boundary\n", nb);
			n += nb;
		}
		r = tape_write(buffer, n, &actual);
		if (actual != TapeUndefLength) {
			count += actual;
		}
		if (r)
			return r;
	}
	if ((r = tape_filemark(0, 0, NULL)) < 0)
		return r;
	if (ferror(file)) {
		perror("writing");
		exit(1);
	}
	fprintf(out, "	%ld bytes written\n", count);
	return 0;
}

int
eq(const char *p, const char *q, int len)
{
	int n = strlen(p);
	if (n < len)
		return 0;
	return (isupper(*q) ? strncmp(p, q, n) : strnicmp(p, q, n)) == 0;
}

void
help(void)
{
	static char *text[] = {
		"tape <option|command>+",
		"",
		"Options:",
		"	-0..7		Device TAPE$<n>",
		"	-Wait		always wait for completion (default)",
		"	-Nowait		don't wait for completion",
		"	-Abort		abort on errors (default)",
		"	-Stdout		print on stdout",
		"	-Ignore		ignore errors",
		"	-Trace		trace mode",
		"",
		"General commands:",
		"	Load		Load, rewind",
		"	RETension	Load, retension",
		"	Unload		Unload, rewind",
		"	UNLOADEnd	Unload, position to end of tape",
		"	REWind		Rewind",
		"	STatus		Print status",
		"	Inquiry		Print inquiry data",
		"	MOde [<n>]	Print mode page <n> (default=all)",
		"	File [<n>]	Seek files forward/backward (default=1)",
		"	Block [<n>]	Seek blocks forward/backward (default=1)",
		"	TEll		Current block number (TDC specific)",
		"	SEek <n>	Seek block <n> (TDC specific)",
		"	ENd		Position to logical end of media",
		"	BLOCKSize <n>	Set blocksize, 0 = variable",
		"	MARK [<n>]	Write <n:1> filemarks",
		"	SETMark [<n>]	Write <n:1> setmarks",
		"	ERASE		Erase tape",
		"	Verify [<n>]	Verify <n> files (default=1)",
		"	REad <file>|-	Read to file|stdout",
		"	WRite <file>|-	Write from file|stdin",
		"	RESET		Device reset",
		"	BUSRESET	SCSI bus reset",
		"	TRACE <n>	Set driver trace mode",
		"",
		"	SPeed <n>	Set speed code",
		"	SET <x>		Set mode page(s)",
		"		<x> split into items by comma or blank",
		"		items:	save	save page(s)",
		"			scsi-1	page format is SCSI-1",
		"			scsi-2	page format is SCSI-2",
		"			<hex>	mode data",
		"		page format defaults to the driver mode",
		"		or TAPEMODE, if set",
		"",
		"  The next commands should be used with extreme care,",
		"  since they can cause the system to hang! The command",
		"  names must be written in upper-case.",
		"",
		"	CMD <cmd>	SCSI command",
		"	CMDF <file>	SCSI commands from file",
		"		command		cdb=byte,byte,..",
		"				rlen=length",
		"				rfile=file",
		"				wdata=byte,byte,...",
		"				wfile=file",
		"		Each command in one line, separated by space or",
		"		';', lines can be continued by a trailing '\\'.",
		"",
		"		The system will hang if you forget RLEN for",
		"		a read-type command or WDATA/WFILE for a",
		"		write-type command!",
		NULL
	};
	char **p;
	for (p = text; *p; ++p)
		fprintf(out, "%s\n", *p);
	exit(1);
}

static int
getstr(char *s, unsigned char *d, int maxlen)
{
	int i;
	for (i = 0; i < maxlen; ) {
		char *t = NULL;
		d[i++] = strtoul(s, &t, 16);
		if (!t || *t != ',')
			break;
		s = t + 1;
	}
	return i;
}

static int
getfile(char *name, unsigned char *buf, int maxlen)
{
	struct stat st;
	FILE *fp;
	if ((fp = fopen(name, "rb")) == NULL || fstat(fileno(fp), &st) != 0) {
		perror(name);
		exit(1);
	}
	if (st.st_size > maxlen) {
		fprintf(out, "file %s too large\n", name);
		st.st_size = maxlen;
	}
	fread(buf, st.st_size, 1, fp);
	fclose(fp);
	return st.st_size;
}

static void
putfile(char *name, unsigned char *buf, int len)
{
	FILE *fp;
	if ((fp = fopen(name, "wb")) == NULL) {
		perror(name);
		exit(1);
	}
	fwrite(buf, len, 1, fp);
	fclose(fp);
}

int
scsicmdln(char *cmd)
{
	/*
		"		command		cdb=byte,byte,..",
		"				rlen=length",
		"				rfile=file",
		"				wdata=byte,byte,...",
		"				wfile=file",
	*/
	int cdblen =  6, rdflag = 0, wrflag = 0, datalen = 0;
	char *rfile = NULL;
	char *p;
	int r;

	for (p = strtok(cmd, " \t;"); p; p = strtok(NULL, " \t;")) {
		while (isspace(*p))
			++p;
		if (!*p)
			continue;
		if (strnicmp(p, "cdb=", 4) == 0) {
			cdblen = getstr(p+4, cdb, 12);
		} else if (strnicmp(p, "rlen=", 5) == 0) {
			rdflag = 1;
			datalen = strtoul(p+5, 0, 10);
		} else if (strnicmp(p, "rfile=", 6) == 0) {
			rfile = p+6;
		} else if (strnicmp(p, "wdata=", 6) == 0) {
			wrflag = 1;
			datalen = getstr(p+6, buffer[0], NBLOCKS * 512);
		} else if (strnicmp(p, "wfile=", 6) == 0) {
			wrflag = 1;
			datalen = getfile(p+6, buffer[0], NBLOCKS * 512);
		} else {
			fprintf(stderr, "unknown control %s\n", p);
			exit(1);
		}
	}
	if (rdflag)
		r = tape_read_cmd(cdb, cdblen, buffer[0], datalen);
	else if (wrflag)
		r = tape_write_cmd(cdb, cdblen, buffer[0], datalen);
	else
		r = tape_cmd(cdb, cdblen);
	if (rdflag)
		if (rfile)
			putfile(rfile, buffer[0], datalen);
		else {
			int i;
			fprintf(out, "data, %d bytes:\n", datalen);
			dump(buffer[0], datalen);
		}
	return r;
}	

int
set_mode(char *arg)
{
	char *p;
	int save = 0;
	int level = scsiLevel;
	int i = 4;
	for (p = strtok(arg, " ,"); p; p = strtok(NULL, " ,")) {
		if (stricmp(p, "save") == 0)
			save = 1;
		else if (stricmp(p, "scsi-1") == 0)
			level = 1;
		else if (stricmp(p, "scsi-2") == 0)
			level = 2;
		else if (isdigit(*p))
			buffer[0][i++] = strtol(p, &p, 16);
		else {
			fprintf(out, "Bad page data: %s\n", arg);
			exit(1);
		}
	}
	cdb[0] = CmdModeSelect;
	cdb[1] = (level >= 2 ? 0x10 : 0x00) | (save ? 0x01 : 0x00);
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = i;
	cdb[5] = 0;
	buffer[0][0] = 0;
	buffer[0][1] = 0;
	buffer[0][2] = 0x10;
	buffer[0][3] = 0;
	return tape_write_cmd(cdb, 6, buffer[0], i);
}

main(int argc, char **argv)
{
	int immed = 0, ignore = 0;
	int unit = -1;
	char *p, devname[20];

	out = stderr;

	for (++argv; argc >= 2; --argc, ++argv) {
		char *cmd = *argv;
		if (eq(cmd, "-wait", 2))
			immed = 0;
		else if (eq(cmd, "-nowait", 2))
			immed = 1;
		else if (eq(cmd, "-ignore", 2))
			ignore = 1;
		else if (eq(cmd, "-abort", 2))
			ignore = 0;
		else if (eq(cmd, "-stdout", 2))
			out = stdout;
		else if (eq(cmd, "-trace", 2))
			tape_trace(++trace);
		else if (cmd[0] == '-' && isdigit(cmd[1]))
			unit = cmd[1] - '0';
		else
			break;
	}

	if (argc <= 1)
		help();

	if (unit < 0) {
		if ((p = getenv("TAPE")) != NULL)
			strncpy(devname, p, 19);
		else
			strcpy(devname, "TAPE$0");
		if ((p = strrchr(devname, '$')) != NULL && isdigit(*(p+1)))
			unit = *p - '0';
	} else
		sprintf(devname, "TAPE$%d", unit);

	tape_name(devname);

	if (unit >= 0)
		tape_target(unit);

	tape_sense(&sense, sizeof sense);

	while (argc-- >= 2) {
		char *cmd = *argv++;
		int r = 0;
		if (eq(cmd, "load", 1)) {
			fprintf(out, "Load\n"),
			r = tape_load(immed, 0);
		} else if (eq(cmd, "retension", 3)) {
			fprintf(out, "Retension\n");
			r = tape_load(immed, 1);
		} else if (eq(cmd, "unload", 1)) {
			fprintf(out, "Unload\n"),
			r = tape_unload(immed, UnloadRewind);
		} else if (eq(cmd, "unloadend", 7)) {
			fprintf(out, "Unload end\n"),
			r = tape_unload(immed, UnloadEndOfTape);
		} else if (eq(cmd, "rewind", 3)) {
			fprintf(out, "Rewind\n");
			r = tape_rewind(immed);
		} else if (eq(cmd, "status", 2)) {
			tape_print_sense(out, tape_ready());
		} else if (eq(cmd, "inquiry", 1)) {
			if ((r = tape_inquiry(inq, sizeof inq)) == 0)
				print_inquiry();
		} else if (eq(cmd, "tell", 2)) {
			long n;
			fprintf(out, "Current block number\n");
			n = tape_tell();
			if (n >= 0)
				fprintf(out, "	%ld\n", n);
			else
				r = n;
		} else if (eq(cmd, "end", 2)) {
			fprintf(out, "Logical end of media\n");
			r = tape_space(SpaceLogEndOfMedia, 0L, NULL);
		} else if (eq(cmd, "erase", 5)) {
			fprintf(out, "Erase\n");
			r = tape_erase();
		} else if (eq(cmd, "read", 2)) {
			char *name = (argc-- >= 2) ? *argv++ : "-";
			if (strcmp(name, "-") == 0) {
				fprintf(out, "Read tape to stdout\n");
				out = stderr;
#if defined(__ZTC__)
				stdout->_flag &= ~_IOTRAN;
#elif !defined(unix)
				setmode(1, O_BINARY);
#endif
				r = read_tape(stdout);
			} else {
				FILE *file = fopen(name, "wb");
				fprintf(out, "Read tape to \"%s\"\n", name);
				if (file == NULL) {
					perror(name);
					exit(1);
				}
				r = read_tape(file);
				fclose(file);
			}
		} else if (eq(cmd, "write", 2)) {
			char *name = (argc-- >= 2) ? *argv++ : "-";
			if (strcmp(name, "-") == 0) {
				fprintf(out, "Write tape from stdin\n");
#if defined(__ZTC__)
				stdin->_flag &= ~_IOTRAN;
#elif !defined(unix)
				setmode(0, O_BINARY);
#endif
				r = write_tape(stdin);
			} else {
				FILE *file = fopen(name, "rb");
				fprintf(out, "Write tape from \"%s\"\n", name);
				if (file == NULL) {
					perror(name);
					exit(1);
				}
				r = write_tape(file);
			}
		} else if (eq(cmd, "reset", 5)) {
			fprintf(out, "SCSI reset\n");
			r = tape_reset(0);
		} else if (eq(cmd, "busreset", 8)) {
			fprintf(out, "SCSI bus reset\n");
			r = tape_reset(1);
		} else if (eq(cmd, "set", 3)) {
			fprintf(out, "Set mode\n");
			r = set_mode((argc-- >= 2) ? *argv++ : "");
		} else if (eq(cmd, "CMD", 3)) {
			if (argc-- < 2) {
				fprintf(out, "Missing command\n");
				exit(1);
			}
			fprintf(out, "SCSI command %s\n", *argv);
			r = scsicmdln(*argv++);
		} else if (eq(cmd, "CMDF", 4)) {
			char cmdln[1000];
			FILE *cmdf;

			if (argc-- < 2) {
				fprintf(out, "Missing file\n");
				exit(1);
			}
			if ((cmdf = fopen(*argv, "r")) == NULL) {
				perror(*argv);
				exit(1);
			}
			fprintf(out, "SCSI command file %s\n", *argv);
			++argv;
			while (!feof(cmdf)) {
				int x = 0;
				while (fgets(cmdln + x, 1000 - x, cmdf)) {
					int len = strlen(cmdln);
					if (len < 2 || cmdln[len - 2] != '\\')
						break;
					x += len - 1;
				}
				for (x = 0; isspace(cmdln[x]); ++x)
					;
				if (cmdln[x] && cmdln[0] != ';')
					if ((r = scsicmdln(cmdln)) == 0)
						goto end;
			}
		end:	fclose(cmdf);
		} else {
			int present = 0;
			long n = 1, actual;
			if (argc >= 2 && strchr("0123456789+-", **argv)) {
				--argc;
				n = strtol(*argv++, NULL, 0);
				present = 1;
			}
			if (eq(cmd, "trace", 5)) {
				scsi_set_trace(present ? n : 0);
			} else if (eq(cmd, "mode", 2)) {
				if (present) {
					if ((r = tape_mode_sense((int)n, mode, sizeof mode)) == 0)
						print_mode((int)n, 0);
				} else {
					for (n = 0; n < 0x3F; ++n)
						if ((r = tape_mode_sense((int)n, mode, sizeof mode)) == 0)
							print_mode((int)n, present++);
					r = 0;
				}
			} else if (eq(cmd, "file", 1)) {
				fprintf(out, "Space over %ld filemark%s\n",
					n, n==1 ? "" : "s");
				r = tape_space(SpaceFileMarks, n, &actual);
				fprintf(out, "	spaced over %ld filemark%s\n",
					actual, actual==1 ? "" : "s");
			} else if (eq(cmd, "block", 1)) {
				fprintf(out, "Space over %ld block%s\n",
					n, n==1 ? "" : "s");
				r = tape_space(SpaceBlocks, n, &actual);
				fprintf(out, "	spaced over %ld block%s\n",
					actual, actual==1 ? "" : "s");
			} else if (eq(cmd, "seek", 2)) {
				fprintf(out, "Seek to block %ld\n", n);
				r = tape_seek(immed, n);
			} else if (eq(cmd, "blocksize", 6)) {
				fprintf(out, "Blocksize %ld\n", n);
				r = tape_set_blocksize(n);
			} else if (eq(cmd, "verify", 3)) {
				long files = 0;
				fprintf(out, "Verify %ld file%s\n",
					n, n==1 ? "" : "s");
				while (--n >= 0) {
					do {
						r = tape_verify(200L * 512, NULL);
					} while (r == 0);
					if (r == SenseKey+BlankCheck
					 || r == EndOfData
					 || r == EndOfTape
					 || r != FileMark && r != 0 && !ignore)
						break;
					++files;
				}
				fprintf(out, "	verified %ld file%s\n",
					files, files==1 ? "" : "s");
			} else if (eq(cmd, "mark", 4)) {
				fprintf(out, "Write %ld filemark%s\n", n, n==1 ? "" : "s");
				r = tape_filemark(immed, n, &actual);
				fprintf(out, "	%ld filemark%s written\n",
					actual, actual==1 ? "" : "s");
			} else if (eq(cmd, "setmark", 4)) {
				fprintf(out, "Write %ld setmark%s\n", n, n==1 ? "" : "s");
				r = tape_setmark(immed, n, &actual);
				fprintf(out, "	%ld setmark%s written\n",
					actual, actual==1 ? "" : "s");
			} else if (eq(cmd, "speed", 2)) {
				fprintf(out, "Set speed %d\n", n);
				r = tape_speed((int)n);
			} else {
				fprintf(out, "Unkown command: %s\n\n", cmd);
				help();
			}
		}
		if (r < 0) {
			tape_print_sense(out, r);
			if (!ignore)
				exit(2);
		} else if (trace && cmd[0] != '-')
			tape_print_sense(out, r);
	}
	tape_term();
	exit(0);
}
