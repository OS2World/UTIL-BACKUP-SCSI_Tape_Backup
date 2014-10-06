/*****************************************************************************
 * $Id: ak_tape.c,v 1.4 1993/01/27 08:04:16 ak Exp $
 *****************************************************************************
 * $Log: ak_tape.c,v $
 * Revision 1.4  1993/01/27  08:04:16  ak
 * Fix for variable length tape blocks (HP DAT).
 *
 * Revision 1.3  1992/09/26  09:25:11  ak
 * *** empty log message ***
 *
 * Revision 1.2  1992/09/12  15:57:11  ak
 * - Usenet patches for GNU TAR 1.10
 * - Bugfixes and patches of Kai Uwe Rommel:
 *         filename conversion for FAT
 *         EMX 0.8e
 *         -0..1 alias for a: b:
 *         -2..7 alias for +++TAPE$x
 *
 * Revision 1.1  1992/09/02  20:07:43  ak
 * Initial revision
 *
 * Revision 1.1.1.1  1992/01/06  20:41:35  ak
 * -Y = don't recurse: new.
 * -X = exclude list: for extract.
 * Use BUFFER for OS/2 tape compression.
 * No own tape buffering for OS/2.
 * Support for SYSTEM and HIDDEN files.
 *
 * Revision 1.1  1992/01/06  20:41:34  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: ak_tape.c,v 1.4 1993/01/27 08:04:16 ak Exp $";

/*
 *	tape_io.c
 *
 * Tape interface for GNU tar.
 * Use "Remote Tape Interface".
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <ctype.h>
#if defined(OS2)
# define INCL_DOSFILEMGR
# define INCL_DOSDEVICES
# define INCL_DOSDEVIOCTL
# define _Packed
# include <os2.h>
#elif defined(MSDOS)
# include <dos.h>
#else
  typedef unsigned char BYTE;
  typedef unsigned long ULONG;
#endif

#include "tar.h"
#include "port.h"
#include "rmt.h"

#define _far
#include "scsi.h"
#include "tape.h"

extern FILE *msg_file;
char *__rmt_path;

static int	errcode;
static int	wrflag;
static int	tape_no = 0;
long		tapeblock = 512;

#ifdef OS2
static HFILE	handle;
static ULONG	disk = 0;
#endif

static int
error(int code)
{
	errcode = code;
	errno = EIO;
	return -1;
}

#undef perror

void
my_perror(const char *msg)
{
	if (errno >= 2000) {
		if (msg)
			fprintf(msg_file, "%s: ", msg);
		switch (errno) {
		case EINVAL:
			fprintf(msg_file, "invalid operation\n");
			break;
		case ENOSPC:
			fprintf(msg_file, "reached end of tape\n");
			break;
		case EIO:
			fprintf(msg_file, "tape error\n");
			if (errcode)
				tape_print_sense(msg_file, errcode);
			break;
		}
	} else
		perror(msg);
}

int
__rmt_open(char *name, int mode, int prot, int bias)
{
	int r, unit;
	char *cp;
	extern int map_read;

#ifdef OS2
	ULONG	action;

	if (disk = _isdisk(name)) {
		disk += ((mode & 3) != O_RDONLY);
	} else
		while (*name == '+')
			++name;
	r = DosOpen((PSZ)name, &handle, &action, 0L, 0, FILE_OPEN,
		((mode & 3) == O_RDONLY ? OPEN_SHARE_DENYNONE 
		                        : OPEN_SHARE_DENYREADWRITE)
		+ OPEN_FLAGS_FAIL_ON_ERROR
		+ (disk ? OPEN_FLAGS_DASD : 0)
		+ (mode & 3), 0);
	if (r) {
		errno = EINVAL;
		return -1;
	}
	if (disk) {
	        if (disk > 1) {
		        BYTE cmd = 0;
			ULONG ulParmLengthInOut = sizeof(cmd), 
			      ulDataLengthInOut = 0;
			DosDevIOCtl(handle, IOCTL_DISK, DSK_LOCKDRIVE, 
				    &cmd, sizeof(cmd), &ulParmLengthInOut,
				    0, 0, &ulDataLengthInOut);
		}
		return handle;
	}
	bias += handle;
#endif

	unit = -1;
	for (cp = name; *cp; ++cp)
		if (isdigit(*cp))
			unit = *cp - '0';

#ifdef unix
	tape_init();
#else
	tape_file(handle);
	if (unit >= 0)
		tape_target(unit);
#endif
	tape_ready();		/* skip "Cartridge Changed" */
	r = tape_ready();
	if (r != 0) {
		fprintf(msg_file, "Tape not ready\n");
		tape_print_sense(msg_file, r);
		exit(EX_SYSTEM);
	}

	tapeblock = tape_get_blocksize();
	if (tapeblock == 0)
		tapeblock = blocksize;

	if (++tape_no == 1)
		switch (mode & 3) {
		case O_RDONLY:
			break;
		case O_WRONLY:
			wrflag = 1;
			if (tape_space(SpaceLogEndOfMedia, 0L, NULL) != 0)
				fprintf(msg_file, "Seek to end of tape failed\n");
			break;
		case O_RDWR:
			errno = EINVAL;
			return -1;
		}
	else
		tape_rewind(0);

	return bias;
}

int
__rmt_read(int fd, void *p, unsigned len)
{
  	long actual = 0, chunk, real;
  	int r = 0;
  	char *cp = p;
  	
	while (len) {
		chunk = (len > 32768) ? 32768 : len;
		r = tape_read(cp, chunk, &real);
		if (actual != TapeUndefLength)
			actual += real;
		if (r < 0 || real != chunk)
	  		break;
		cp  += chunk;
		len -= chunk;
	}
	if (r <= 0) {
		if (r == SenseKey+MediaOverflow || r == MappedError+0x32) {
			errcode = r;
			errno = ENOSPC;
			return -1;
		}
		if (r < 0 && r != EndOfTape)
			return error(r);
	}
	return r ? r : actual;
}

int
__rmt_write(int fd, void *p, unsigned len)
{
	long actual = 0, chunk, real;
	int r = 0;
	char *cp = p;

	while (len) {
		chunk = (len > 32768) ? 32768 : len;
		r = tape_write(cp, chunk, &real);
		if (actual != TapeUndefLength)
			actual += real;
		if (r < 0 || real != chunk)
	 		break;
		cp  += chunk;
		len -= chunk;
	}
	if (r <= 0) {
		if (r == SenseKey+MediaOverflow || r == MappedError+0x32) {
			errcode = r;
			errno = ENOSPC;
			return -1;
		}
		if (r < 0)
			return error(r);
	}
	return r ? r : actual;
}

long
__rmt_lseek(int fd, long offs, int mode)
{
	/* used:	offs, 0/1/2	-> standard lseek, return code 0
			offs, 3		-> physical block, return code 0
			0, 4		-> return current phys block
	*/
	long r, actual;

	offs /= tapeblock;

	switch (mode) {
	case 4:
		r = tape_tell();
		if (r < 0)
			return error(r);
		return r;
	case 3:
		/* physical block, direct seek required */
		r = tape_seek(0, offs);
		if (r < 0)
			return error(r);
		return 0;
	case 2:
		r = tape_space(SpaceFileMarks, 1L, &actual);
		tape_space(SpaceFileMarks, -1L, &actual);
		break;
	case 1:
		r = 0;
		break;
	case 0:
		r = tape_space(SpaceFileMarks, -1L, &actual);
		tape_space(SpaceFileMarks, 1L, &actual);
		break;
	default:
		errno = EINVAL;
		return -1;
	}
	if (r != 0 && r != EndOfTape)
		return error(r);
	if (offs) {
		r = tape_space(SpaceBlocks, offs, &actual);
		if (r)
			return error(r);
	}
	return 0;
}

int
__rmt_close(int fd)
{
	int r = 0;

#ifdef OS2
	if (disk > 1) {
		BYTE cmd = 0;
		ULONG ulParmLengthInOut = sizeof(cmd), 
		      ulDataLengthInOut = 0;
		DosDevIOCtl(handle, IOCTL_DISK, DSK_UNLOCKDRIVE, 
			    &cmd, sizeof(cmd), &ulParmLengthInOut,
			    0, 0, &ulDataLengthInOut);
	} else if (wrflag && !disk) {
		r = tape_filemark(0, 1L, NULL);
		if (r < 0)
			r = error(r);
	}
#endif
	tape_term();
	wrflag = 0;
	return r;
}
