/*****************************************************************************
 * $Id: tape.h,v 1.3 1992/09/12 18:10:57 ak Exp $
 *****************************************************************************
 * $Log: tape.h,v $
 * Revision 1.3  1992/09/12  18:10:57  ak
 * Added scsi_name
 * Added device name support to tctl.c
 *
 * Revision 1.2  1992/09/02  19:05:25  ak
 * Version 2.0
 * - EMX version
 * - AIX version
 * - SCSI-2 commands
 * - ADD Driver
 * - blocksize support
 *
 * Revision 1.1.1.1  1992/01/06  20:27:43  ak
 * Interface now based on ST01 and ASPI.
 * AHA_DRVR no longer supported.
 * Files reorganized.
 *
 * Revision 1.1  1992/01/06  20:27:41  ak
 * Initial revision
 *
 *****************************************************************************/

#include <limits.h>

/*
 * Tape control.
 */

#define TapeUndefLength	LONG_MIN	/* undefined value of actual count */

extern void	tape_init ();
extern void	tape_name (char *name);
extern void	tape_file (int fd);
extern void	tape_term ();

extern int	tape_reset (int bus);

extern void	tape_trace(int);

extern int	tape_target (int);
extern void	tape_sense (void *data, int len);	/* >= 8, NULL resets */

extern char *	tape_error (int);

extern int	tape_cmd (void *cdb, int cdb_len);
extern int	tape_read_cmd (void *cdb, int cdb_len,
			       void _far *data, long data_len);
extern int	tape_write_cmd (void *cdb, int cdb_len,
			        void _far *data, long data_len);

extern int	tape_inquiry (void *data, int len);
extern int	tape_mode_sense (int page, void *data, int len);
extern int	tape_mode_select (int page, void *data, int len, int SMP);

extern int	tape_rewind (int imed);
extern int	tape_space (int mode, long nitems, long *actual);
extern long	tape_tell (void);   			/* Tandberg */
extern int	tape_seek (int imed, long blkno);	/* Tandberg */

extern int	tape_read (void _far *data, long length, long *actual);
extern int	tape_compare (void _far *data, long length, long *actual);
extern int	tape_verify (long length, long *actual);
extern int	tape_write (void _far *data, long length, long *actual);
extern int	tape_filemark (int imed, long nblocks, long *actual);
extern int	tape_setmark (int imed, long nblocks, long *actual);
extern int	tape_erase (void);

extern int	tape_buffered_write (void _far *data, long length);
extern int	tape_buffered_wait (long *actual);

extern long	tape_get_blocksize (void);
extern int	tape_set_blocksize (long size);
extern int	tape_speed (int);
extern int	tape_ready (void);
extern int	tape_load (int imed, int retension);
extern int	tape_unload (int imed, int mode);

extern void	tape_print_sense (FILE *file, int retcode);

/* Space mode: */
enum SpaceMode {
	SpaceBlocks		= 00,
	SpaceFileMarks		= 01,
	SpaceSequentalFilemarks	= 02,
	SpaceLogEndOfMedia	= 03,
	SpaceSetmarks		= 04,
	SpaceSequentalSetmarks	= 05,
};

/* Unload mode */
enum UnloadMode {
	UnloadRewind		= 00,
	UnloadRetension		= 02,
	UnloadEndOfTape		= 04,
};

/* additional error codes, -> scsi.h */
#define EndOfData	TapeError+1
#define EndOfTape	TapeError+2
#define FileMark	TapeError+3
#define NoCommand	TapeError+4	/* buffered_wait */
