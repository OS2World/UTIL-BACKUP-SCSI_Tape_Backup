/*****************************************************************************
 * $Id: ak_dir.c,v 1.3 1992/10/29 10:26:35 ak Exp $
 *****************************************************************************
 * $Log: ak_dir.c,v $
 * Revision 1.3  1992/10/29  10:26:35  ak
 * *** empty log message ***
 *
 * Revision 1.2  1992/09/12  15:57:09  ak
 * - Usenet patches for GNU TAR 1.10
 * - Bugfixes and patches of Kai Uwe Rommel:
 *         filename conversion for FAT
 *         EMX 0.8e
 *         -0..1 alias for a: b:
 *         -2..7 alias for +++TAPE$x
 *
 * Revision 1.1  1992/09/02  20:07:32  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: ak_dir.c,v 1.3 1992/10/29 10:26:35 ak Exp $";

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tar.h"
#include "port.h"
#include "rmt.h"

extern union record *head;		/* Points to current tape header */
extern struct stat hstat;		/* Stat struct corresponding */
extern int head_standard;		/* Tape header is in ANSI format */

/*
 * Random archive access.
 */
void
seek_exec(do_something, name, vol, voloff, rec, skip)
	void (*do_something)();
	char *name;
	int vol;
	long voloff, rec, skip;
{
	long r;
	static int error_flag;
	extern int volno;
	extern long baserec;
	static long lastseek = - 1, lastend;

	if (f_multivol) {
		if (vol != volno) {
			/* next volume */
			close_archive(1);
			sprintf(ar_file + strlen(ar_file) - 3, "%03d", vol);
			open_archive(1);
			ar_last = ar_record = ar_block;
			baserec = 0;
			volno = vol;
		}
	}

	if (voloff + rec != lastseek) {
		lastseek = voloff + rec;
		msg("Seek to physical block %ld", voloff + rec);
		r = rmtlseek(archive, (voloff + rec) * RECORDSIZE, 
			     _isrmt(archive) ? 3 : 0);
		ar_last = ar_record = ar_block;
		baserec = rec;

		if (r < 0) {
			if (vol)
			  msg_perror("Seek error on %s, volume %d record %ld",
				     ar_file, vol, rec);
			else
			  msg_perror("Seek error on %s, record %ld",
				     ar_file, rec);
			if (++error_flag > 10) {
				msg("Too many errors, quitting.");
				exit(EX_BADARCH);
			}
			return;
		}
	}
	else
	  	skip -= lastend;

	/* skip alignment blocks */
	if (skip) {
		long record_skip = skip % blocking;
		long block_skip = skip - record_skip;
  	        /* msg("Skip over %ld logical blocks", skip); */
		if (block_skip) {
			r = rmtlseek(archive, block_skip * RECORDSIZE, 1);
			baserec += block_skip;
		}
		while (record_skip--)
			userec(findrec());
	}

	if (read_header() == 1) {
		/* We should decode next field (mode) first... */
		/* Ensure incoming names are null terminated. */
		head->header.name[NAMSIZ-1] = '\0';

		/* Handle the archive member */
		(*do_something)();
		lastend = baserec - rec + ar_record - ar_block;
	} else {
		if (vol)
			msg("File %s volume %d record %ld has invalid header, wrong map file?",
				name, vol, rec);
		else
			msg("File %s record %ld has invalid header, wrong map file?",
				name, rec);
		if (++error_flag > 10) {
			msg("Too many errors, quitting.");
			exit(EX_BADARCH);
		}
	}
}

/*
 * Main loop for random access.
 */
void
seek_and(do_something)
	void (*do_something)();
{
	extern FILE *map_file;
	struct name *nlp;
	char	line[60 + FILENAME_MAX];
	long	voloff, offset, skip;
	char	*p;
	int	len, volume;
#if 0
	struct Volmap {
		long		base;	/* physical base record no */
		short		ctrl;	/* no of inserted control records */
		short		num;	/* volume no */
		struct Volmap	*prev;	/* previous volume */
	} *volmap, *q;
#endif
	name_gather();			/* Gather all the names */
	open_archive(1);		/* Open for reading */

	volume = voloff = 0;
	rewind(map_file);
	while (fgets(line, 60 + FILENAME_MAX, map_file)) {
		char *name = line + strlen(line);
		char *key  = strtok(line, " \t");
		char *arg  = key ? strtok(NULL, ": \t\r\n") : NULL;
		char *mode = arg ? strtok(NULL, " \t") : NULL;
		char *q;
		offset = arg ? strtol(arg, &q, 0) : 0;
		skip = (q && *q == '+') ? strtol(q+1, &q, 0) : 0;
		if (strncmp(key, "vol", 3) == 0) {
			/* base address of tape file */
			voloff = offset;
			continue;
		}
		if (key[0] == 'V') {
			/* tape number */
			volume = atoi(key+1);
			continue;
		}
		if (strcmp(key, "blk") == 0) {
			extern long tapeblock;
			voloff = 0;
			offset = offset * tapeblock / RECORDSIZE;
		} else if (strncmp(key, "rec", 3) != 0)
			continue;
		if (mode[0] == 'M')
			continue;

		/* get file name from record line -- skip "(..)" descr */
		while (isspace(*(name-1)))
			--name;
		if (mode[0] == 'A' && *(name-1) == ')')
			while (*--name != '(')
				;
		while (isspace(*(name-1)))
			--name;
		*name = '\0';
		while (!isspace(*(name-1)))
			--name;
		len = strlen(name);

		/* look if name is mentioned in command line */
		for (nlp = namelist; nlp; nlp = nlp->next) {
			/* fast check for first character */
			if (nlp->firstch && nlp->name[0] != name[0])
				continue;

			/* regular expression? */
			if (nlp->regexp) {
				if (wildmat(name, nlp->name))
					goto match;
				continue;
			}

			/* plain name */
			if (nlp->length <= len
			 && (name[nlp->length] == '\0'
			  || name[nlp->length] == '/')
#ifdef MSDOS
			 && strnicmp(name, nlp->name, nlp->length) == 0)
#else
			 && strncmp(name, nlp->name, nlp->length) == 0)
#endif
			 	goto match;
			continue;

		match:	seek_exec(do_something, name, volume, voloff, offset, skip);
			if (mode[0] != 'A')
				nlp->found = 1;
		}
	}

	close_archive(1);
	names_notfound();		/* Print names not found */
}


