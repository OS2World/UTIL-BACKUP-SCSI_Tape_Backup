/*****************************************************************************
 * $Id: tape.c,v 1.3 1992/09/12 18:10:55 ak Exp $
 *****************************************************************************
 * $Log: tape.c,v $
 * Revision 1.3  1992/09/12  18:10:55  ak
 * Added scsi_name
 * Added device name support to tctl.c
 *
 * Revision 1.2  1992/09/02  19:05:22  ak
 * Version 2.0
 * - EMX version
 * - AIX version
 * - SCSI-2 commands
 * - ADD Driver
 * - blocksize support
 *
 * Revision 1.1.1.1  1992/01/06  20:27:39  ak
 * Interface now based on ST01 and ASPI.
 * AHA_DRVR no longer supported.
 * Files reorganized.
 *
 * Revision 1.1  1992/01/06  20:27:37  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: tape.c,v 1.3 1992/09/12 18:10:55 ak Exp $";


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#ifdef __EMX__
#ifndef _far
#define _far
#endif
#endif

#include "scsi.h"
#include "tape.h"
#include "scsitape.h"

#define SenseLength	16
#define SenseAlloc	16

static int		target = 4;
static unsigned char	cdb [10];
static unsigned char	sense_buf [SenseAlloc];
static unsigned char *	sense_ptr = sense_buf;
static int		sense_len = SenseLength;
static int		active;
static long		active_nb;
static void *		ctrl;
int			scsiLevel = 1;
static long		blocksize = 512;
static long		valid_blocksize = 0;

#define int16(x)	((unsigned)(x)[0] << 8 | (x)[1])
#define int24(x)	((unsigned long)int16(x) << 8 | (x)[2])
#define int32(x)	((unsigned long)int24(x) << 8 | (x)[3])

int
tape_cmd(void *cdb, int cdb_len)
{
	return scsi_cmd(target, 0, cdb, cdb_len, sense_ptr, sense_len, (void *)0, 0L, 0);
}

int
tape_read_cmd(void *cdb, int cdb_len, void _far *data, long length)
{
	return scsi_cmd(target, 0, cdb, cdb_len, sense_ptr, sense_len, data, length, 1);
}

int
tape_write_cmd(void *cdb, int cdb_len, void _far *data, long length)
{
	return scsi_cmd(target, 0, cdb, cdb_len, sense_ptr, sense_len, data, length, 0);
}

static void
init()
{
	char *env;
	long size;

	sense_ptr = sense_buf;
	sense_len = SenseLength;
	if ((env = getenv("TAPEMODE")) != NULL)
		scsiLevel = atoi(env);
	else
		scsiLevel = senseMode;
}

void
tape_init(void)
{
	scsi_init();
	init();
}

void
tape_name(char *name)
{
	scsi_name(name);
	init();
}

void
tape_file(int fd)
{
	scsi_file(fd);
	init();
}

void
tape_term(void)
{
	scsi_term();
}

int
tape_reset(int bus)
{
	return scsi_reset(target, 0, bus);
}

void
tape_trace(int level)
{
	scsi_trace(level);
}

int
tape_target(int no)
{
	int old = target;
	target = no;
	return old;
}

void
tape_sense(void *data, int len)
{
	if (len >= 8 && data) {
		sense_ptr = data;
		sense_len = len;
	} else {
		sense_ptr = sense_buf;
		sense_len = SenseLength;
	}
}

char *
tape_error(int code)
{
	switch (code) {
	case EndOfData: return "End of data";
	case EndOfTape: return "End of tape";
	case FileMark:  return "File mark";
	}
	return scsi_error(code);
}

int
tape_inquiry(void *data, int len)
{
	cdb[0] = CmdInquiry;
	cdb[1] = 0;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = len;
	cdb[5] = 0;
	return tape_read_cmd(cdb, 6, data, len);
}	

int
tape_mode_sense(int page, void *data, int len)
{
	cdb[0] = CmdModeSense;
	cdb[1] = 0;
	cdb[2] = page;
	cdb[3] = 0;
	cdb[4] = len;
	cdb[5] = 0;
	return tape_read_cmd(cdb, 6, data, len);
}	

int
tape_mode_select(int page, void *data, int len, int SMP)
{
	cdb[0] = CmdModeSelect;
	cdb[1] = 0;
	cdb[2] = page;
	cdb[3] = 0;
	cdb[4] = len;
	cdb[5] = SMP ? 0x40 : 0x00;
	return tape_write_cmd(cdb, 6, data, len);
}	

int
tape_rewind(int imed)
{
	cdb[0] = SeqRewind;
	cdb[1] = imed ? 1 : 0;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = 0;
	cdb[5] = 0;
	return tape_cmd(cdb, 6);
}

long
tape_tell(void)
{
	int rc;

	if (scsiLevel >= 2) {
		unsigned char addr [20];
		cdb[0] = SeqReadPosition;
		cdb[1] = 1;
		cdb[2] = 0;
		cdb[3] = 0;
		cdb[4] = 0;
		cdb[5] = 0;
		cdb[6] = 0;
		cdb[7] = 0;
		cdb[8] = 0;
		cdb[9] = 0;
		if ((rc = tape_read_cmd(cdb, 10, addr, 20)) != 0)
			return rc;
		return (long)addr[4] << 24 | (long)addr[5] << 16
		     | (long)addr[6] << 8 | addr[7];
	} else {
		unsigned char addr [3];
		cdb[0] = SeqRequestBlockAddress;
		cdb[1] = 0;
		cdb[2] = 0;
		cdb[3] = 0;
		cdb[4] = 3;
		cdb[5] = 0;
		if ((rc = tape_read_cmd(cdb, 6, addr, 3)) != 0)
			return rc;
		return (long)addr[0] << 16 | (long)addr[1] << 8 | addr[2];
	}
}

static int
num_result(int ret, long count, long *actual)
{
	if (actual)
		*actual = TapeUndefLength;
	if (ret == MappedError+0)
		return FileMark;
	if (ErrorClass(ret) == SenseKey) {
		if (actual && sense_ptr[0] & VADD)
			*actual = count - int32(sense_ptr+3);
		if (ret == SenseKey+NoSense) {
			if (sense_ptr[2] & EOM)
				return EndOfTape;
			if (sense_ptr[2] & FM)
				return FileMark;
		}
	} else if (actual && ret == 0)
		*actual = count;
	return ret;
}

int
tape_space(int mode, long nitems, long *actual)
{
	cdb[0] = SeqSpace;
	cdb[1] = mode;
	cdb[2] = nitems >> 16;
	cdb[3] = nitems >> 8;
	cdb[4] = nitems;
	cdb[5] = 0;
	return num_result(tape_cmd(cdb, 6), nitems, actual);
}

int
tape_seek(int imed, long blkno)
{
	if (scsiLevel >= 2) {
		cdb[0] = SeqLocate;
		cdb[1] = imed ? 5 : 4;
		cdb[2] = 0;
		cdb[3] = blkno >> 24;
		cdb[4] = blkno >> 16;
		cdb[5] = blkno >> 8;
		cdb[6] = blkno;
		cdb[7] = 0;
		cdb[8] = 0;
		cdb[9] = 0;
		return tape_cmd(cdb, 10);
	} else {
		cdb[0] = SeqSeek;
		cdb[1] = imed ? 1 : 0;
		cdb[2] = blkno >> 16;
		cdb[3] = blkno >> 8;
		cdb[4] = blkno;
		cdb[5] = 0;
		return tape_cmd(cdb, 6);
	}
}

static int
rw_result(int ret, long nb, long *actual)
{
	if (actual)
		*actual = TapeUndefLength;
	if (ret == MappedError+0)
		return FileMark;
	if (ErrorClass(ret) == SenseKey) {
		if (actual && sense_ptr[0] & VADD)
			*actual = (blocksize ? blocksize : 1)
				* (nb - int32(sense_ptr+3));
		if (ret == SenseKey+NoSense) {
			if (sense_ptr[2] & EOM)
				return EndOfTape;
			if (sense_ptr[2] & FM)
				return FileMark;
		}
	} else if (actual && ret == 0)
		*actual = (blocksize ? blocksize : 1) * nb;
	return ret;
}

int
tape_read(void _far *data, long length, long *actual)
{
	long nb;
	if (!valid_blocksize)
		tape_get_blocksize();
	nb = blocksize ? length / blocksize : length;
	cdb[0] = SeqRead;
	cdb[1] = blocksize ? 1 : 0;
	cdb[2] = nb >> 16;
	cdb[3] = nb >> 8;
	cdb[4] = nb;
	cdb[5] = 0;
	return rw_result(tape_read_cmd(cdb, 6, data, length), nb, actual);
}

int
tape_compare(void _far *data, long length, long *actual)
{
	int ret;
	long nb;
	if (!valid_blocksize)
		tape_get_blocksize();
	nb = blocksize ? length / blocksize : length;
	cdb[0] = SeqVerify;
	cdb[1] = blocksize ? 3 : 2;
	cdb[2] = nb >> 16;
	cdb[3] = nb >> 8;
	cdb[4] = nb;
	cdb[5] = 0;
	return rw_result(tape_write_cmd(cdb, 6, data, length), nb, actual);
}	

int
tape_verify(long length, long *actual)
{
	int ret;
	long nb;
	if (!valid_blocksize)
		tape_get_blocksize();
	nb = blocksize ? length / blocksize : length;
	cdb[0] = SeqVerify;
	cdb[1] = blocksize ? 1 : 0;
	cdb[2] = nb >> 16;
	cdb[3] = nb >> 8;
	cdb[4] = nb;
	cdb[5] = 0;
	return rw_result(tape_cmd(cdb, 6), nb, actual);
}	

int
tape_write(void _far *data, long length, long *actual)
{
	int ret;
	long nb;
	if (!valid_blocksize)
		tape_get_blocksize();
	nb = blocksize ? length / blocksize : length;
	cdb[0] = SeqWrite;
	cdb[1] = blocksize ? 1 : 0;
	cdb[2] = nb >> 16;
	cdb[3] = nb >> 8;
	cdb[4] = nb;
	cdb[5] = 0;
	return rw_result(tape_write_cmd(cdb, 6, data, length), nb, actual);
}

static void
cleanup(void)
{
	if (active) {
		while (scsi_wait(&ctrl, sense_ptr, 1) == ComeAgain)
			;
		active = 0;
	}
}

int
tape_buffered_write(void _far *data, long length)
{
	int ret;

	if (!valid_blocksize)
		tape_get_blocksize();

	if (ctrl == NULL) {
		atexit(cleanup);
		ctrl = scsi_alloc();
	}

	active_nb = blocksize ? length / blocksize : length;

	cdb[0] = SeqWrite;
	cdb[1] = blocksize ? 1 : 0;
	cdb[2] = active_nb >> 16;
	cdb[3] = active_nb >> 8;
	cdb[4] = active_nb;
	cdb[5] = 0;

	ret = scsi_start(ctrl, target, 0, cdb, 6, sense_len, data, length, 0);
	if (ret == ComeAgain)
		active = 1;
	return ret;
}

int
tape_buffered_wait(long *actual)
{
	int ret;

	if (!active)
		return NoCommand;
	ret = scsi_wait(ctrl, sense_ptr, 1);
	active = 0;
	return rw_result(ret, active_nb, actual);
}

int
tape_filemark(int imed, long count, long *actual)
{
	cdb[0] = SeqWriteFilemarks;
	cdb[1] = imed ? 1 : 0;
	cdb[2] = count >> 16;
	cdb[3] = count >> 8;
	cdb[4] = count;
	cdb[5] = 0;
	return num_result(tape_cmd(cdb, 6), count, actual);
}

int
tape_setmark(int imed, long count, long *actual)
{
	cdb[0] = SeqWriteFilemarks;
	cdb[1] = imed ? 3 : 2;
	cdb[2] = count >> 16;
	cdb[3] = count >> 8;
	cdb[4] = count;
	cdb[5] = 0;
	return num_result(tape_cmd(cdb, 6), count, actual);
}

int
tape_erase(void)
{
	cdb[0] = SeqErase;
	cdb[1] = 1;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = 0;
	cdb[5] = 0;
	return tape_cmd(cdb, 6);
}

long
tape_get_blocksize(void)
{
	unsigned char data [12];
	int rc;

	cdb[0] = CmdModeSense;
	cdb[1] = 0;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = 12;		/* length of parameter list */
	cdb[5] = 0;

	if ((rc = tape_read_cmd(cdb, 6, data, 12)) != 0)
		return rc;
	blocksize = (long)data[9] << 16 | (long)data[10] << 8 | data[11];
	valid_blocksize = 1;
	scsi_set_blocksize(target, 0, blocksize);
	return blocksize;
}

int
tape_set_blocksize(long size)
{
	unsigned char data [12];

	blocksize = size;
	scsi_set_blocksize(target, 0, size);

	cdb[0] = CmdModeSelect;
	cdb[1] = 0;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = 12;		/* length of parameter list */
	cdb[5] = 0;

	data[0]  = 0;
	data[1]  = 0;
	data[2]  = 0x10;	/* buffered mode */
	data[3]  = 8;		/* block descriptor length */

	data[4]  = 0;		/* density is default */
	data[5]  = 0;
	data[6]  = 0;
	data[7]  = 0;
	data[8]  = 0;
	data[9]  = size >> 16;
	data[10] = size >> 8;
	data[11] = size;

	return tape_write_cmd(cdb, 6, data, 12);
}

int
tape_speed(int code)
{
	unsigned char data [4];

	cdb[0] = CmdModeSelect;
	cdb[1] = 0;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = 4;		/* length of parameter list */
	cdb[5] = 0;

	data[0]  = 0;
	data[1]  = 0;
	data[2]  = 0x10 | code;	/* buffered mode */
	data[3]  = 0;		/* block descriptor length */

	return tape_write_cmd(cdb, 6, data, 4);
}

int
tape_ready(void)
{
	cdb[0] = CmdTestUnitReady;
	cdb[1] = 0;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = 0;
	cdb[5] = 0;
	return tape_cmd(cdb, 6);
}

int
tape_load(int imed, int retension)
{
	cdb[0] = SeqLoad;
	cdb[1] = imed != 0;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = retension ? 03 : 01;
	cdb[5] = 0;
	return tape_cmd(cdb, 6);
}

int
tape_unload(int imed, int mode)
{
	cdb[0] = SeqLoad;
	cdb[1] = imed != 0;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = (scsiLevel >= 2) ? mode & 07 : mode & 03;
	cdb[5] = (scsiLevel >= 2) ? 0 : mode & 04 << 6;
	return tape_cmd(cdb, 6);
}

void
tape_print_sense(FILE *out, int code)
{
	struct Tape_Extended_Sense *p
		= (struct Tape_Extended_Sense *) sense_ptr;

	if (ErrorClass(code) == ErrorClass(MappedError)) {
		fprintf(out, "%s\n", tape_error(code));
		return;
	}
	fprintf(out, "Return code:	%s\n", tape_error(code));
	fprintf(out, "Sense key:	%s\n", senseTab[p->sense_key]);

	if (p->filemark || p->end_of_media || p->incorrect_length) {
		fprintf(out, "    ");
		if (p->filemark)
			fprintf(out, "Filemark ");
		if (p->end_of_media)
			fprintf(out, "End-of-media ");
		if (p->incorrect_length)
			fprintf(out, "Incorrect-length ");
		fprintf(out, "\n");
	}

	switch (senseMode) {
	case TDC3600:
		fprintf(out, "ERCL+ERCD:	%s\n",
			find_error(tdc3600ercd, p->u.tdc.error));
		fprintf(out, "EXERCD:		%s\n",
			find_error(tdc3600xercd, p->u.tdc.xerror));
		fprintf(out, "    blocks=%ld filemarks=%u remaining=%u\n",
			int24(p->u.tdc.no_blocks),
			int16(p->u.tdc.no_filemarks),
			p->u.tdc.no_remaining);
		fprintf(out, "    recovered=%u underruns=%u marginal=%u\n",
			int16(p->u.tdc.no_recovered),
			int16(p->u.tdc.no_underruns),
			p->u.tdc.no_marginal);
		break;
	case SCSI2:
		fprintf(out, "ASC::		%02X\n", p->u.scsi2.asc);
		fprintf(out, "ASCQ:		%02X\n", p->u.scsi2.ascq);
		fprintf(out, "Description:	%s\n",
			find_error(scsi2asc,
				p->u.scsi2.asc << 8 | p->u.scsi2.ascq));
		break;
	}
}
