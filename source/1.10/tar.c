/*****************************************************************************
 * $Id: tar.c,v 1.20 1993/01/27 08:17:02 ak Exp $
 *****************************************************************************
 * $Log: tar.c,v $
 * Revision 1.20  1993/01/27  08:17:02  ak
 * --archive statt --modified.
 *
 * Revision 1.19  1993/01/14  21:50:32  ak
 * EMX 0.8f fixes and enhancements by Kai Uwe Rommel.
 *
 * Revision 1.18  1993/01/10  11:19:40  ak
 * J.R.: Bugfix: f_gnudump='f' sicherte doppelt.
 *
 * Revision 1.17  1992/12/13  09:57:13  ak
 * *** empty log message ***
 *
 * Revision 1.16  1992/12/13  09:49:58  ak
 * *** empty log message ***
 *
 * Revision 1.15  1992/12/13  09:39:33  ak
 * K.U.R: f_archive/f_reset_archive added to support the "archived" bit.
 *
 * Revision 1.14  1992/10/31  17:42:01  ak
 * *** empty log message ***
 *
 * Revision 1.13  1992/10/31  08:16:12  ak
 * Added long option --all-files as alias for --same-perm. Rearranged the
 * corresponding usage text.
 *
 * Revision 1.12  1992/10/31  06:55:04  ak
 * Modified -G (again) to specify the generation of increment.
 *
 * Revision 1.11  1992/10/29  09:59:58  ak
 * -G for incremental backups like -g, but w/o writing the dumpfile.
 *
 * Revision 1.10  1992/10/25  10:20:41  ak
 * Don't archive/extract ctime+atime if a single -p or --same-perm is
 * specified. It make trouble on incremental backups and file transfers.
 * Added an option --all-timestamps or twice -p to archive/extract
 * ctime+atime.
 *
 * Revision 1.9  1992/09/29  09:56:13  ak
 * K.U.R., once again :-)
 * - removed a dup() in buffer.c
 * - EMX opendir with hidden/system test in gnu.c (...dotdot)
 *
 * Revision 1.8  1992/09/26  08:31:59  ak
 * *** empty log message ***
 *
 * Revision 1.7  1992/09/20  07:46:56  ak
 * Fixes from Kai Uwe Rommel
 *   --checkpoints instead of --semi-verbose (1.11)
 *   -g filenames
 *
 * Revision 1.6  1992/09/16  20:59:57  ak
 * EMX 0.8e
 *
 * Revision 1.5  1992/09/12  17:06:03  ak
 * Added usage of -E --semi-verbose
 *
 * Revision 1.4  1992/09/12  16:07:10  ak
 * Added entry for --fat to usage
 *
 * Revision 1.3  1992/09/12  15:57:18  ak
 * - Usenet patches for GNU TAR 1.10
 * - Bugfixes and patches of Kai Uwe Rommel:
 *         filename conversion for FAT
 *         EMX 0.8e
 *         -0..1 alias for a: b:
 *         -2..7 alias for +++TAPE$x
 *
 * Revision 1.2  1992/09/02  20:08:47  ak
 * Version AK200
 * - Tape access
 * - Quick file access
 * - OS/2 extended attributes
 * - Some OS/2 fixes
 * - Some fixes of Kai Uwe Rommel
 *
 * Revision 1.1.1.1  1992/09/02  19:22:47  ak
 * Original GNU Tar 1.10 with some filenames changed for FAT compatibility.
 *
 * Revision 1.1  1992/09/02  19:22:45  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: tar.c,v 1.20 1993/01/27 08:17:02 ak Exp $";

/*
 * Modified by Andreas Kaiser July 92.
 * See CHANGES.AK for info.
 */

/* Tar -- a tape archiver.

	Copyright (C) 1988 Free Software Foundation

GNU tar is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY.  No author or distributor accepts responsibility to anyone
for the consequences of using it or for whether it serves any
particular purpose or works at all, unless he says so in writing.
Refer to the GNU tar General Public License for full details.

Everyone is granted permission to copy, modify and redistribute GNU tar,
but only under the conditions described in the GNU tar General Public
License.  A copy of this license is supposed to have been given to you
along with GNU tar so you can know your rights and responsibilities.  It
should be in a file named COPYING.  Among other things, the copyright
notice and this notice must be preserved on all copies.

In other words, go ahead and share GNU tar, but don't try to stop
anyone else from sharing it farther.  Help stamp out software hoarding!
*/

/*
 * A tar (tape archiver) program.
 *
 * Written by John Gilmore, ihnp4!hoptoad!gnu, starting 25 Aug 85.
 *
 * @(#)tar.c 1.34 11/6/87 - gnu
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>		/* Needed for typedefs in tar.h */
#include <sys/stat.h>		/* JF */
#include "getopt.h"
#include "regex.h"

#ifdef USG
#define rindex strrchr
#endif

#ifdef BSD42
 #include <sys/dir.h>
#else
 #ifdef __MSDOS__
  #include "msd_dir.h"
 #else
  #ifdef USG
   #ifdef NDIR
    #include <ndir.h>
   #else
    #include <dirent.h>
   #endif
   #ifndef DIRECT
    #define direct dirent
   #endif
   #define DP_NAMELEN(x) strlen((x)->d_name)
  #else
   /*
    * FIXME: On other systems there is no standard place for the header file
    * for the portable directory access routines.  Change the #include line
    * below to bring it in from wherever it is.
    */
   #ifdef OS2
    #include "dirent.h"
    #define direct dirent
    #define DP_NAMELEN(x) strlen((x)->d_name)
   #else
    #include "ndir.h"
   #endif
  #endif
 #endif
#endif

#ifndef DP_NAMELEN
#define DP_NAMELEN(x)	(x)->d_namlen
#endif

extern char 	*malloc();
extern char 	*getenv();
extern char	*strncpy();
extern char	*index();
extern char	*strcpy();	/* JF */
extern char	*strcat();	/* JF */

extern char	*optarg;	/* Pointer to argument */
extern int	optind;		/* Global argv index from getopt */

extern char 	*ck_malloc();
extern char 	*ck_realloc();
/*
 * The following causes "tar.h" to produce definitions of all the
 * global variables, rather than just "extern" declarations of them.
 */
#define TAR_EXTERN /**/
#include "tar.h"
#include "port.h"
#include "rmt.h"

#if 0
/*
 * We should use a conversion routine that does reasonable error
 * checking -- atoi doesn't.  For now, punt.  FIXME.
 */
#define intconv	atoi
#else
/*
 * AK920724 -- I did add the error checking. And a bit more.
 * Allowed to append 'B' for multiples of the block size, 'K'
 * for KB (the default), 'R' for records (512 bytes) and 'M'
 * for MB.
 *
 * Second argument is the default factor. The result is divided
 * by this number.
 */
long intconv();
#endif

extern int	getoldopt();
extern void	read_and();
extern void	list_archive();
extern void	extract_archive();
extern void	diff_archive();
extern void	create_archive();
extern void	update_archive();
extern void	junk_archive();

/* JF */
extern time_t	get_date();

time_t new_time;

static FILE	*namef;		/* File to read names from */
static char	**n_argv;	/* Argv used by name routines */
static int	n_argc;		/* Argc used by name routines */
static char	**n_ind;	/* Store an array of names */
static int	n_indalloc;	/* How big is the array? */
static int	n_indused;	/* How many entries does it have? */
static int	n_indscan;	/* How many of the entries have we scanned? */


extern FILE *msg_file;

FILE	*map_file;
int	map_read;

void	describe();
void	options();

#ifndef S_IFLNK
#define lstat stat
#endif

#ifndef DEFBLOCKING
#define DEFBLOCKING 20
#endif

#ifndef DEF_AR_FILE
#if defined(OS2) && defined(TAPE_IO)
#define DEF_AR_FILE (getenv("TAPE") ? getenv("TAPE") : "tape.out")
#else
#define DEF_AR_FILE "tar.out"
#endif
#endif

/* For long options that unconditionally set a single flag, we have getopt
   do it.  For the others, we share the code for the equivalent short
   named option, the name of which is stored in the otherwise-unused `val'
   field of the `struct option'; for long options that have no equivalent
   short option, we use nongraphic characters as pseudo short option
   characters, starting (for no particular reason) with character 10. */

struct option long_options[] =
{
	{"create",		0,	0,			'c'},
	{"append",		0,	0,			'r'},
	{"extract",		0,	0,			'x'},
	{"get",			0,	0,			'x'},
	{"list",		0,	0,			't'},
	{"update",		0,	0,			'u'},
	{"catenate",		0,	0,			'A'},
	{"concatenate",		0,	0,			'A'},
	{"compare",		0,	0,			'd'},
	{"diff",		0,	0,			'd'},
	{"delete",		0,	0,			14},
	{"help",		0,	0,			12},

	{"directory",		1,	0,			'C'},
	{"record-number",	0,	&f_sayblock,		1},
	{"files-from",		1,	0,			'T'},
	{"label",		1,	0,			'V'},
	{"exclude-from",	1,	0,			'X'},
	{"exclude",		1,	0,			15},
	{"show-omitted-dirs",	0,	0,			16},
	{"file",		1,	0,			'f'},
	{"block-size",		1,	0,			'b'},
	{"version",		0,	0,			11},
	{"verbose", 		0,	0,			'v'},
	{"checkpoints",		0,	0,			'E'},
	{"totals",		0,	&f_totals,		1},
	  
	{"read-full-blocks",	0,	&f_reblock,		1},
	{"starting-file",	1,	0,			'K'},
	{"to-stdout",		0,	&f_exstdout,		1},
	{"ignore-zeros",	0,	&f_ignorez,		1},
	{"keep-old-files",	0,	0,			'k'},
	{"uncompress",		0,	&f_compress,		1},
	{"same-permissions",	0,	&f_use_protection,	1},
	{"preserve-permissions",0,	&f_use_protection,	1},
	{"modification-time",	0,	&f_modified,		1},
	{"preserve",		0,	0,			10},
	{"same-order",		0,	&f_sorted_names,	1},
	{"same-owner",		0,	&f_do_chown,		1},
	{"preserve-order",	0,	&f_sorted_names,	1},

	{"newer",		1,	0,			'N'},
	{"after-date",		1,	0,			'N'},
	{"newer-mtime",		1,	0,			13},
	{"generation",		1,	0,			'G'},
	{"incremental",		1,	0,			'g'},
	{"multi-volume",	0,	&f_multivol,		1},
	{"info-script",		1,	0,			'F'},
	{"absolute-paths",	0,	&f_absolute_paths,	1},
	{"interactive",		0,	&f_confirm,		1},
	{"confirmation",	0,	&f_confirm,		1},

	{"verify",		0,	&f_verify,		1},
	{"dereference",		0,	&f_follow_links,	1},
	{"one-file-system",	0,	&f_local_filesys, 	1},
	{"old-archive",		0,	0,			'o'},
	{"portability",		0,	0,			'o'},
	{"compress",		0,	&f_compress,		1},
	{"compress-block",	0,	&f_compress,		2},
	{"sparse",		0,	&f_sparse_files,	1},
	{"tape-length",		1,	0,			'L'},

#ifdef HOST
	{"host",		0,	&f_host,		1},
#endif
	{"tape-directory",	1,	0,			'D'},
	{"no-recursion",	0,	&f_no_recursion,	'Y'},
#ifdef MSDOS
	{"fat", 		0,	&f_fat, 		1},
	{"all-files",		0,	&f_use_protection,	1},
	{"all-timestamps",	0,	&f_all_timestamps,	1},
 	{"archive", 		0,	&f_archive, 		1},
 	{"reset-archive",	0,	&f_reset_archive,	1},
#endif

	{0, 0, 0, 0}
};

/*
 * Main routine for tar.
 */
main(argc, argv)
	int	argc;
	char	**argv;
{
	extern char version_string[];

#if defined(__EMX__)
	_response(&argc, &argv);
	_wildcard(&argc, &argv);
	tzset();
#endif

	tar = argv[0];		/* JF: was "tar" Set program name */
#ifdef MSDOS
	{	char *p; int i;

		for (i = 0; i < argc; ++i) {
			p = argv[i];
			if (*p != '\'' && *p != '"')
				for (; *p; ++p)
					if (*p == '\\')
						*p = '/';
		}
		for (p = tar + strlen(tar); --p != tar; )
			if (*p == ':' || *p == '\\') {
				tar = p + 1;
				break;
			}
		for (p = tar; *p && *p != '.'; ++p)
			;
		*p = 0;
		strlwr(tar);

	}
#endif
	errors = 0;

	options(argc, argv);

	if (f_map_file) {
		switch(cmd_mode) {
		default:
			map_file = fopen(f_map_file, "a");
			break;
		case CMD_EXTRACT:
		case CMD_DIFF:
			map_read = 1;
			map_file = fopen(f_map_file, "r");
		}
		if (map_file == NULL) {
			fprintf(stderr, "Cannot open map file %s\n", f_map_file);
			exit(EX_ARGSBAD);
		}
	}

	if(!n_argv)
		name_init(argc, argv);

	switch(cmd_mode) {
	case CMD_CAT:
	case CMD_UPDATE:
	case CMD_APPEND:
		update_archive();
		break;
	case CMD_DELETE:
		junk_archive();
		break;
	case CMD_CREATE:
		create_archive();
		if (f_totals)
			fprintf (stderr, "Total bytes written: %d\n", tot_written);
		break;
	case CMD_EXTRACT:
#ifdef REGEX
		if (f_volhdr) {
			char *err;
			label_pattern = (struct re_pattern_buffer *)
			  ck_malloc (sizeof *label_pattern);
		 	err = re_compile_pattern (f_volhdr, strlen (f_volhdr),
						  label_pattern);
			if (err) {
				fprintf (stderr,"Bad regular expression: %s\n",
					 err);
				errors++;
				break;
			}
		   
		}		  
#endif
		extr_init();
		if (f_map_file)
			seek_and(extract_archive);
		else
			read_and(extract_archive);
		break;
	case CMD_LIST:
#ifdef REGEX
		if (f_volhdr) {
			char *err;
			label_pattern = (struct re_pattern_buffer *)
			  ck_malloc (sizeof *label_pattern);
		 	err = re_compile_pattern (f_volhdr, strlen (f_volhdr),
						  label_pattern);
			if (err) {
				fprintf (stderr,"Bad regular expression: %s\n",
					 err);
				errors++;
				break;
			}
		}
#endif
		read_and(list_archive);
#if 0
		if (!errors)
			errors = different;
#endif
		break;
	case CMD_DIFF:
		diff_init();
		if (f_map_file)
			seek_and(diff_archive);
		else
			read_and(diff_archive);
		break;
	case CMD_VERSION:
		fprintf(stderr,"%s\n",version_string);
		break;
	case CMD_NONE:
		fprintf(stderr,"\nThis is %s, a tape archiving program.\n\n",
			version_string);
		fprintf(stderr,"You must specify exactly one of the r, c, t, x, or d options.\n");
 		fprintf(stderr,"For more information, type ``%s --help''.\n",tar);
		exit(EX_ARGSBAD);
	}
	if (f_map_file)
		fclose(map_file);
	exit(errors);
	/* NOTREACHED */
}


/*
 * Parse the options for tar.
 */
void
options(argc, argv)
	int	argc;
	char	**argv;
{
	register int	c;		/* Option letter */
	int		ind = -1;

	/* Set default option values */
	blocking = DEFBLOCKING;		/* From Makefile */
	ar_file = getenv("TAPE");	/* From environment, or */
	if (ar_file == 0)
		ar_file = DEF_AR_FILE;	/* From Makefile */

	/* Parse options */
	while ((c = getoldopt(argc, argv,
			      "-01234567Ab:BcC:dD:Ef:F:g:G:hHikK:lL:mMN:oOpPrRsStT:uvV:wWxX:YzZ",
			      long_options, &ind)) != EOF) {
		switch (c) {
		case 0:		/* long options that set a single flag */
		  	break;
		case 1:
			/* File name or non-parsed option */
			name_add(optarg);
			break;
		case 'C':
			name_add("-C");
			name_add(optarg);
			break;
		case 10:	/* preserve */
			f_use_protection = f_sorted_names = 1;
			break;
		case 11:
			if(cmd_mode!=CMD_NONE)
				goto badopt;
			cmd_mode=CMD_VERSION;
			break;
		case 12:	/* help */
			fprintf(stderr,"This is GNU tar, the tape archiving program.\n");
			describe();
			exit(1);
		case 13:
			f_new_files++;
			goto get_newer;

		case 14:			/* Delete in the archive */
			if(cmd_mode!=CMD_NONE)
				goto badopt;
			cmd_mode=CMD_DELETE;
			break;

		case 15:
			f_exclude++;
			add_exclude(optarg);
			break;

		case 16:	/* show-omitted-dirs */
                	f_show_omitted_dirs++;
                        break;
                        
		case '0':
		case '1':
#ifdef MSDOS
			{
                          static char buf[8];
                          sprintf(buf,"%c:",c-'0'+'A');
                          ar_file=buf;
                        }
                        break;
#endif

		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			{
				/* JF this'll have to be modified for other
				   systems, of course! */
				int d,add;
				static char buf[50];

#ifdef MSDOS
				sprintf(buf, "+++TAPE$%c", c);
#else
				d=getoldopt(argc,argv,"lmh");
#ifdef MAYBEDEF
				sprintf(buf,"/dev/rmt/%d%c",c,d);
#else
#ifndef LOW_NUM
#define LOW_NUM 0
#define MID_NUM 8
#define HGH_NUM 16
#endif
				if(d=='l') add=LOW_NUM;
				else if(d=='m') add=MID_NUM;
				else if(d=='h') add=HGH_NUM;
				else goto badopt;

				sprintf(buf,"/dev/rmt%d",add+c-'0');
#endif
#endif
				ar_file=buf;
			}
			break;

		case 'A':			/* Arguments are tar files,
						   just cat them onto the end
						   of the archive.  */
			if(cmd_mode!=CMD_NONE)
				goto badopt;
			cmd_mode=CMD_CAT;
			break;

		case 'b':			/* Set blocking factor */
			blocking = intconv(optarg, RECORDSIZE);
			break;

		case 'B':			/* Try to reblock input */
			f_reblock++;		/* For reading 4.2BSD pipes */
			break;

		case 'c':			/* Create an archive */
			if(cmd_mode!=CMD_NONE)
				goto badopt;
			cmd_mode=CMD_CREATE;
			break;

/*		case 'C':
			if(chdir(optarg)<0)
				msg_perror("Can't change directory to %d",optarg);
			break; */

		case 'd':			/* Find difference tape/disk */
			if(cmd_mode!=CMD_NONE)
				goto badopt;
			cmd_mode=CMD_DIFF;
			break;

#ifdef TAPE_IO
		case 'D':
			f_map_file = optarg;	/* Maintain/use map file */
			break;
#endif

		case 'f':			/* Use ar_file for the archive */
			ar_file = optarg;
			break;

		case 'F':
			/* Since -F is only useful with -M , make it implied */
			f_run_script_at_end++;  /* run this script at the end */
			info_script = optarg;	/* of each tape */
			f_multivol++;
			break;

		case 'g':			/* We are making a GNU dump; save
						   directories at the beginning of
						   the archive, and include in each
						   directory its contents */
			if(f_oldarch)
				goto badopt;
			f_gnudump = 'i';
#ifdef MSDOS
			f_use_protection = 1;
#endif
			gnu_dumpfile = optarg;
			break;

		case 'G':			/* We are making a GNU dump; save
						   directories at the beginning of
						   the archive, and include in each
						   directory its contents */
			if(f_oldarch)
				goto badopt;
			c = strlen(optarg);
			if (strncmp(optarg, "first", c)
			 && strncmp(optarg, "second", c)
			 && strncmp(optarg, "incremental", c)) {
				msg("don't understand generation option %s", optarg);
				exit(EX_ARGSBAD);
			}
			f_gnudump = optarg[0];
			break;

		case 'h':
			f_follow_links++;	/* follow symbolic links */
			break;

		case 'i':
			f_ignorez++;		/* Ignore zero records (eofs) */
			/*
			 * This can't be the default, because Unix tar
			 * writes two records of zeros, then pads out the
			 * block with garbage.
			 */
			break;

		case 'k':			/* Don't overwrite files */
#ifdef NO_OPEN3
			msg("can't do -k option on this system");
			exit(EX_ARGSBAD);
#else
			f_keep++;
#endif
			break;

		case 'K':
			f_startfile++;
			addname(optarg);
			break;

		case 'l':			/* When dumping directories, don't
						   dump files/subdirectories that are
						   on other filesystems. */
			f_local_filesys++;
			break;

		case 'L':
			tape_length = intconv (optarg, 1024);
			f_multivol++;
			break;
		case 'm':
			f_modified++;
			break;

		case 'M':			/* Make Multivolume archive:
						   When we can't write any more
						   into the archive, re-open it,
						   and continue writing */
			f_multivol++;
			break;

		case 'N':			/* Only write files newer than X */
		get_newer:
			f_new_files++;
			new_time=get_date(optarg,(struct timeb *)0);
			break;

		case 'o':			/* Generate old archive */
			if(f_gnudump /* || f_dironly */)
				goto badopt;
			f_oldarch++;
			break;

		case 'O':
			f_exstdout++;
			break;

		case 'p':
			if (++f_use_protection >= 2)
				++f_all_timestamps;
			break;

		case 'P':
			f_absolute_paths++;
			break;

		case 'r':			/* Append files to the archive */
			if(cmd_mode!=CMD_NONE)
				goto badopt;
			cmd_mode=CMD_APPEND;
			break;

		case 'R':
			f_sayblock++;		/* Print block #s for debug */
			break;			/* of bad tar archives */

		case 's':
			f_sorted_names++;	/* Names to extr are sorted */
			break;

		case 'S':			/* deal with sparse files */
			f_sparse_files++;
			break;
		case 't':
			if(cmd_mode!=CMD_NONE)
				goto badopt;
			cmd_mode=CMD_LIST;
			f_verbose++;		/* "t" output == "cv" or "xv" */
			break;

		case 'T':
			name_file = optarg;
			f_namefile++;
			break;

		case 'u':			/* Append files to the archive that
						   aren't there, or are newer than the
						   copy in the archive */
			if(cmd_mode!=CMD_NONE)
				goto badopt;
			cmd_mode=CMD_UPDATE;
			break;

		case 'E' :
		        f_checkpoints = 1;
		case 'v':
			f_verbose++;
			break;

		case 'V':
			f_volhdr=optarg;
			break;

		case 'w':
			f_confirm++;
			break;

		case 'W':
			f_verify++;
			break;

		case 'x':			/* Extract files from the archive */
			if(cmd_mode!=CMD_NONE)
				goto badopt;
			cmd_mode=CMD_EXTRACT;
			break;

		case 'X':
			f_exclude++;
			add_exclude_file(optarg);
			break;

		case 'Z':		/* Like the filename extension .Z */
			f_buffered++;
		case 'z':		/* Easy to type */
			f_compress++;
			break;

		case '?':
		badopt:
			msg("Unknown option.  Use '%s --help' for a complete list of options.", tar);
			exit(EX_ARGSBAD);

		}
	}

	blocksize = blocking * RECORDSIZE;

	if (f_gnudump && !gnu_dumpfile) {
		msg("you have not specified a dump file (-g)");
		exit(EX_ARGSBAD);
	}
#ifdef HOST
	if (f_host)
		host_init(ar_file);
#endif
#ifdef S_IFCHR
	{ struct stat st;
	  is_device = stat(ar_file, &st) == 0
		   && (st.st_mode & S_IFMT) == S_IFCHR;
	}
#endif
#ifdef TAPE_IO
# ifndef NO_REMOTE
	is_device |= _remdev(ar_file);
# endif
#endif
}


/*
 * Print as much help as the user's gonna get.
 *
 * We have to sprinkle in the KLUDGE lines because too many compilers
 * cannot handle character strings longer than about 512 bytes.  Yuk!
 * In particular, MSDOS and Xenix MSC and PDP-11 V7 Unix have this
 * problem.
 */
void
describe()
{
	msg("Choose one of the following:");
	fprintf(stderr,
"-A	--catenate,\n"
"	--concatenate		append tar files to an archive\n"
"-c	--create		create a new archive\n"
"-d	--diff,\n"
"	--compare		find differences between archive and file system\n"
"	--delete		delete from the archive (not for use on mag tapes!)\n"
"-r	--append		append files to the end of an archive\n"
"-t	--list			list the contents of an archive\n"
"-u	--update		only append files that are newer than copy in archive\n"
"-x	--extract,\n"
"	--get			extract files from an archive\n"
"\n"
	);
	fprintf(stderr,
"Other options:\n"
"-b	--block-size N		block size of Nx512 bytes (default N=%d)\n"
"-B	--read-full-blocks	reblock as we read (for reading 4.2BSD pipes)\n"
"-C	--directory DIR		change to directory DIR\n"
"-D	--tape-directory F	use/maintain tape directory in file F\n"
"-E	--checkpoints		list directories as they are processed\n"
"-f	--file [HOSTNAME:]F	use archive file/device F (default %s)\n"
"-F	--info-script F		run script at end of each tape (implies -M)\n"
"-g	--incremental F 	create/list/extract new GNU-format incremental backup\n"
"-G	--generation O		specifies the incremental backup generation\n"
"			O=f[irst]	first full backup, F erased before use\n"
"			O=s[econd]	second backup, F used but not written\n"
"			O=i[ncremental]	true incremental: F used and written\n"
"				if -G is not given, -g assumes O=i\n"
"-h	--dereference		don't dump symlinks; dump the files they point to\n"
#ifdef HOST
"-H	--host			send/receive archive to/from VM host\n"
#endif
"-i	--ignore-zeros		ignore blocks of zeros in archive (normally mean EOF)\n"
"-k	--keep-old-files	keep existing files; don't overwrite them from archive\n"
"-K	--starting-file FILE	begin at FILE in the archive\n"
"-l	--one-file-system	stay in local file system creating an archive\n"
"-L	--tape-length LENGTH	change tapes after writing LENGTH\n"
"-m	--modification-time	don't extract file modified time\n"
"-M	--multi-volume		create/list/extract multi-volume archive\n"
"-N	--after-date DATE,\n"
"	--newer DATE		only store files newer than DATE\n"
"-o	--old-archive,\n"
"	--portability		write a V7 rather than ANSI format archive\n"
"-O	--to-stdout		extract files to standard output\n"
	);
	fprintf(stderr,
#ifdef MSDOS
"-p	--all-files		archive/extract extended attributes,\n"
"				hidden and system files\n"
"-pp	--all-timestamps	archive/extract access and creation times too\n"
#else
"-p	--same-permissions,\n"
"	--preserve-permissions	extract all protection information\n"
#endif
"-P	--absolute-paths	don't strip leading `/'s from file names\n"
"	--preserve		like -p -s\n"
"-R	--record-number		show record number within archive with each message\n"
"-s	--same-order,\n"
"	--preserve-order	list of names to extract is sorted to match archive\n"
"	--same-order		create extracted files with the same ownership \n"
"-S	--sparse		handle sparse files efficiently\n"
"-T	--files-from F		get names to extract or create from file F\n"
"	--totals		print total bytes written with --create\n"
"-v	--verbose		verbosely list files processed\n"
"-V	--label NAME		create archive with volume name NAME\n"
"	--version		print tar program version number\n"
"-w	--interactive,\n"
"	--confirmation		ask for confirmation for every action\n"
"-W	--verify		attempt to verify the archive after writing it\n"
"	--exclude FILE		exclude file FILE\n"
"-X	--exclude-from FILE	exclude files listed in FILE\n"
"-Y	--no-recursion		archive only files specified in command line\n"
#ifdef MSDOS
"	--fat			convert filenames to FAT 8.3 format\n"
"	--archive		incremental backup using the 'archive' bit\n"
"	--reset-archive		reset 'archive' bit in files archived\n"
#endif
"-z -Z	--compress,\n"
"	--uncompress      	filter the archive through compress\n"
"-[0-7][lmh]			specify drive and density\n"
		, DEFBLOCKING, DEF_AR_FILE);
}

name_add(name)
char *name;
{
	if(n_indalloc==n_indused) {
		n_indalloc+=10;
		n_ind=(char **)(n_indused ? ck_realloc(n_ind,n_indalloc*sizeof(char *)) : ck_malloc(n_indalloc*sizeof(char *)));
	}
	n_ind[n_indused++]=name;
}
		
/*
 * Set up to gather file names for tar.
 *
 * They can either come from stdin or from argv.
 */
name_init(argc, argv)
	int	argc;
	char	**argv;
{

	if (f_namefile) {
		if (optind < argc) {
			msg("too many args with -T option");
			exit(EX_ARGSBAD);
		}
		if (!strcmp(name_file, "-")) {
			namef = stdin;
		} else {
			namef = fopen(name_file, "r");
			if (namef == NULL) {
				msg_perror("can't open file %s",name_file);
				exit(EX_BADFILE);
			}
		}
	} else {
		/* Get file names from argv, after options. */
		n_argc = argc;
		n_argv = argv;
	}
}

/*
 * Get the next name from argv or the name file.
 *
 * Result is in static storage and can't be relied upon across two calls.
 */

/* C is non-zero if we should deal with -C */
char *
name_next(c)
{
	static char	*buffer;	/* Holding pattern */
	static buffer_siz;
	register char	*p;
	register char	*q = 0;
	register char	*q2 = 0;
	extern char *un_quote_string();

	if(buffer_siz==0) {
		buffer=ck_malloc(NAMSIZ+2);
		buffer_siz=NAMSIZ;
	}
 tryagain:
	if (namef == NULL) {
		if(n_indscan<n_indused)
			p=n_ind[n_indscan++];
		else if (optind < n_argc)		
			/* Names come from argv, after options */
			p=n_argv[optind++];
		else {
			if(q)
				msg("Missing filename after -C");
			return NULL;
		}

		/* JF trivial support for -C option.  I don't know if
		   chdir'ing at this point is dangerous or not.
		   It seems to work, which is all I ask. */
		if(c && !q && p[0]=='-' && p[1]=='C' && p[2]=='\0') {
			q=p;
			goto tryagain;
		}
		if(q) {
			if(chdir(p)<0)
				msg_perror("Can't chdir to %s",p);
			q=0;
			goto tryagain;
		}
		/* End of JF quick -C hack */

		if(f_exclude && check_exclude(p))
			goto tryagain;
		return un_quote_string(p);
	}
	while(p = fgets(buffer, buffer_siz+1 /*nl*/, namef)) {
		q = p+strlen(p)-1;		/* Find the newline */
		if (q <= p)			/* Ignore empty lines */
			continue;
		while(q==p+buffer_siz && *q!='\n') {
			buffer=ck_realloc(buffer,buffer_siz+NAMSIZ+2);
			p=buffer;
			q=buffer+buffer_siz;
			buffer_siz+=NAMSIZ;
			fgets(q+1,NAMSIZ,namef);
			q=p+strlen(p)-1;
		}
		*q-- = '\0';			/* Zap the newline */
		while (q > p && *q == '/')	/* Zap trailing /s */
			*q-- = '\0';
		if (c && !q2 && p[0] == '-' && p[1] == 'C' && p[2] == '\0') {
			q2 = p;
			goto tryagain;
		}
		if (q2) {
			if (chdir (p) < 0)
				msg_perror ("Can't chdir to %s", p);
			q2 = 0;
			goto tryagain;
		}
		if(f_exclude && check_exclude(p))
			goto tryagain;
		return un_quote_string(p);
	}
	return NULL;
}


/*
 * Close the name file, if any.
 */
name_close()
{

	if (namef != NULL && namef != stdin) fclose(namef);
}


/*
 * Gather names in a list for scanning.
 * Could hash them later if we really care.
 *
 * If the names are already sorted to match the archive, we just
 * read them one by one.  name_gather reads the first one, and it
 * is called by name_match as appropriate to read the next ones.
 * At EOF, the last name read is just left in the buffer.
 * This option lets users of small machines extract an arbitrary
 * number of files by doing "tar t" and editing down the list of files.
 */
name_gather()
{
	register char *p;
	static struct name *namebuf;	/* One-name buffer */
	static namelen;
	static char *chdir_name;

	if (f_sorted_names) {
		if(!namelen) {
			namelen=NAMSIZ;
			namebuf=(struct name *)ck_malloc(sizeof(struct name)+NAMSIZ);
		}
		p = name_next(0);
		if (p) {
			if(*p=='-' && p[1]=='C' && p[2]=='\0') {
				chdir_name=name_next(0);
				p=name_next(0);
				if(!p) {
					msg("Missing file name after -C");
					exit(EX_ARGSBAD);
				}
				namebuf->change_dir=chdir_name;
			}
			namebuf->length = strlen(p);
			if (namebuf->length >= namelen) {
				namebuf=(struct name *)ck_realloc(namebuf,sizeof(struct name)+namebuf->length);
				namelen=namebuf->length;
			}
			strncpy(namebuf->name, p, namebuf->length);
			namebuf->name[ namebuf->length ] = 0;
			namebuf->next = (struct name *)NULL;
			namebuf->found = 0;
			namelist = namebuf;
			namelast = namelist;
		}
		return;
	}

	/* Non sorted names -- read them all in */
	while (p = name_next(0))
		addname(p);
}

/*
 * Add a name to the namelist.
 */
addname(name)
	char	*name;			/* pointer to name */
{
	register int	i;		/* Length of string */
	register struct name	*p;	/* Current struct pointer */
	static char *chdir_name;
	char *new_name();
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

	if(name[0]=='-' && name[1]=='C' && name[2]=='\0') {
		chdir_name=name_next(0);
		name=name_next(0);
		if(!chdir_name) {
			msg("Missing file name after -C");
			exit(EX_ARGSBAD);
		}
#ifdef MSDOS
		if(chdir_name[0]!='/' && (!isalpha(chdir_name[0]) || 
					  chdir_name[1]!=':' || 
					  chdir_name[2]!='/')) {
			char path[MAXPATHLEN];
		  	_fullpath(path,chdir_name,MAXPATHLEN);
			chdir_name=new_name(path,"");
		}
#else
		if(chdir_name[0]!='/') {
			char path[MAXPATHLEN];
#ifdef USG
			int getcwd();
			if(!getcwd(path,MAXPATHLEN))
				msg("Couldn't get current directory.");
				exit(EX_SYSTEM);
#else
			char *getwd();

			if(!getwd(path)) {
				msg("Couldn't get current directory: %s",path);
				exit(EX_SYSTEM);
			}
#endif
			chdir_name=new_name(path,chdir_name);
		}
#endif
	}

	if (name)
	  {
	    i = strlen(name);
	    /*NOSTRICT*/
	    p = (struct name *)malloc((unsigned)(sizeof(struct name) + i));
	  }
	else
	  p = (struct name *)malloc ((unsigned)(sizeof (struct name)));
	if (!p) {
	  if (name)
	    msg("cannot allocate mem for name '%s'.",name);
	  else
	    msg("cannot allocate mem for chdir record.");
	  exit(EX_SYSTEM);
	}
	p->next = (struct name *)NULL;
	if (name)
	  {
	    p->fake = 0;
	    p->length = i;
	    strncpy(p->name, name, i);
	    p->name[i] = '\0';	/* Null term */
	  }
	else
	  p->fake = 1;
	p->found = 0;
	p->regexp = 0;		/* Assume not a regular expression */
	p->firstch = 1;		/* Assume first char is literal */
	p->change_dir=chdir_name;
	p->dir_contents = 0;	/* JF */
	if (name)
	  {
	    if (index(name, '*') || index(name, '[') || index(name, '?')) {
	      p->regexp = 1;	/* No, it's a regexp */
	      if (name[0] == '*' || name[0] == '[' || name[0] == '?')
		p->firstch = 0;		/* Not even 1st char literal */
	    }
	  }

	if (namelast) namelast->next = p;
	namelast = p;
	if (!namelist) namelist = p;
}
/*
 * Match a name from an archive, p, with a name from the namelist.
 */
name_match(p)
	register char *p;
{
	register struct name	*nlp;
	register int		len;

again:
	if (0 == (nlp = namelist))	/* Empty namelist is easy */
		return 1;
	if (nlp->fake)
	  {
	    if (nlp->change_dir && chdir (nlp->change_dir))
	      msg_perror ("Can't change to directory %d", nlp->change_dir);
	    namelist = 0;
	    return 1;
	  }
	len = strlen(p);
	for (; nlp != 0; nlp = nlp->next) {
		/* If first chars don't match, quick skip */
		if (nlp->firstch && nlp->name[0] != p[0])
			continue;

		/* Regular expressions */
		if (nlp->regexp) {
			if (wildmat(p, nlp->name)) {
				nlp->found = 1;	/* Remember it matched */
				if(f_startfile) {
					free((void *)namelist);
					namelist=0;
				}
				if(nlp->change_dir && chdir(nlp->change_dir))
					msg_perror("Can't change to directory %s",nlp->change_dir);
				return 1;	/* We got a match */
			}
			continue;
		}

		/* Plain Old Strings */
		if (nlp->length <= len		/* Archive len >= specified */
		 && (p[nlp->length] == '\0' || p[nlp->length] == '/')
						/* Full match on file/dirname */
		 && strncmp(p, nlp->name, nlp->length) == 0) /* Name compare */
		{
			nlp->found = 1;		/* Remember it matched */
			if(f_startfile) {
				free((void *)namelist);
				namelist = 0;
			}
			if(nlp->change_dir && chdir(nlp->change_dir))
				msg_perror("Can't change to directory %s",nlp->change_dir);
			return 1;		/* We got a match */
		}
	}

	/*
	 * Filename from archive not found in namelist.
	 * If we have the whole namelist here, just return 0.
	 * Otherwise, read the next name in and compare it.
	 * If this was the last name, namelist->found will remain on.
	 * If not, we loop to compare the newly read name.
	 */
	if (f_sorted_names && namelist->found) {
		name_gather();		/* Read one more */
		if (!namelist->found) goto again;
	}
	return 0;
}


/*
 * Print the names of things in the namelist that were not matched.
 */
names_notfound()
{
	register struct name	*nlp,*next;
	register char		*p;

	for (nlp = namelist; nlp != 0; nlp = next) {
		next=nlp->next;
		if (!nlp->found)
			msg("%s not found in archive",nlp->name);

		/*
		 * We could free() the list, but the process is about
		 * to die anyway, so save some CPU time.  Amigas and
		 * other similarly broken software will need to waste
		 * the time, though.
		 */
/*	THIS IS JUST PLAIN WRONG, N. Becker */
#if 0
#ifndef unix
		if (!f_sorted_names)
			free(nlp);
#endif
#endif
	}
	namelist = (struct name *)NULL;
	namelast = (struct name *)NULL;

	if (f_sorted_names) {
		while (0 != (p = name_next(1)))
			msg("%s not found in archive", p);
	}
}

/* These next routines were created by JF */

name_expand()
{
;
}

/* This is like name_match(), except that it returns a pointer to the name
   it matched, and doesn't set ->found  The caller will have to do that
   if it wants to.  Oh, and if the namelist is empty, it returns 0, unlike
   name_match(), which returns TRUE */

struct name *
name_scan(p)
register char *p;
{
	register struct name	*nlp;
	register int		len;

again:
	if (0 == (nlp = namelist))	/* Empty namelist is easy */
		return 0;
	len = strlen(p);
	for (; nlp != 0; nlp = nlp->next) {
		/* If first chars don't match, quick skip */
		if (nlp->firstch && nlp->name[0] != p[0])
			continue;

		/* Regular expressions */
		if (nlp->regexp) {
			if (wildmat(p, nlp->name))
				return nlp;	/* We got a match */
			continue;
		}

		/* Plain Old Strings */
		if (nlp->length <= len		/* Archive len >= specified */
		 && (p[nlp->length] == '\0' || p[nlp->length] == '/')
						/* Full match on file/dirname */
		 && strncmp(p, nlp->name, nlp->length) == 0) /* Name compare */
			return nlp;		/* We got a match */
	}

	/*
	 * Filename from archive not found in namelist.
	 * If we have the whole namelist here, just return 0.
	 * Otherwise, read the next name in and compare it.
	 * If this was the last name, namelist->found will remain on.
	 * If not, we loop to compare the newly read name.
	 */
	if (f_sorted_names && namelist->found) {
		name_gather();		/* Read one more */
		if (!namelist->found) goto again;
	}
	return (struct name *) 0;
}

/* This returns a name from the namelist which doesn't have ->found set.
   It sets ->found before returning, so successive calls will find and return
   all the non-found names in the namelist */

struct name *gnu_list_name;

char *
name_from_list()
{
	if(!gnu_list_name)
		gnu_list_name = namelist;
	while(gnu_list_name && gnu_list_name->found)
		gnu_list_name=gnu_list_name->next;
	if(gnu_list_name) {
		gnu_list_name->found++;
		if(gnu_list_name->change_dir)
			if(chdir(gnu_list_name->change_dir)<0)
				msg_perror("can't chdir to %s",gnu_list_name->change_dir);
		return gnu_list_name->name;
	}
	return (char *)0;
}

blank_name_list()
{
	struct name *n;

	gnu_list_name = 0;
	for(n=namelist;n;n=n->next)
		n->found = 0;
}

char *
new_name(path,name)
char *path,*name;
{
	char *path_buf;

	path_buf=(char *)malloc(strlen(path)+strlen(name)+2);
	if(path_buf==0) {
		msg("Can't allocate memory for name '%s/%s",path,name);
		exit(EX_SYSTEM);
	}
	(void) sprintf(path_buf,"%s%s%s",path,(*name?"/":""),name);
	return path_buf;
}

/* returns non-zero if the luser typed 'y' or 'Y', zero otherwise. */

int
confirm(action,file)
char *action, *file;
{
	int	c,nl;
	static FILE *confirm_file = 0;
	extern FILE *msg_file;
	extern char TTY_NAME[];

	fprintf(msg_file,"%s %s?", action, file);
	fflush(msg_file);
	if(!confirm_file) {
		confirm_file = (archive == 0) ? fopen(TTY_NAME, "r") : stdin;
		if(!confirm_file) {
			msg("Can't read confirmation from user");
			exit(EX_SYSTEM);
		}
	}
	c=getc(confirm_file);
	for(nl = c; nl != '\n' && nl != EOF; nl = getc(confirm_file))
		;
	return (c=='y' || c=='Y');
}

char *x_buffer = 0;
int size_x_buffer;
int free_x_buffer;

char **exclude = 0;
int size_exclude = 0;
int free_exclude = 0;

char **re_exclude = 0;
int size_re_exclude = 0;
int free_re_exclude = 0;

add_exclude(name)
char *name;
{
	char *rname;
	char **tmp_ptr;
	int size_buf;
	extern char *un_quote_string();

	un_quote_string(name);
	size_buf = strlen(name);

	if(x_buffer==0) {
		x_buffer = (char *)ck_malloc(size_buf+1024);
		free_x_buffer=1024;
	} else if(free_x_buffer<=size_buf) {
		char *old_x_buffer;
		char **tmp_ptr;

		old_x_buffer = x_buffer;
		x_buffer = (char *)ck_realloc(x_buffer,size_x_buffer+1024);
		free_x_buffer = 1024;
		for(tmp_ptr=exclude;tmp_ptr<exclude+size_exclude;tmp_ptr++)
			*tmp_ptr= x_buffer + ((*tmp_ptr) - old_x_buffer);
		for(tmp_ptr=re_exclude;tmp_ptr<re_exclude+size_re_exclude;tmp_ptr++)
			*tmp_ptr= x_buffer + ((*tmp_ptr) - old_x_buffer);
	}

	if(is_regex(name)) {
		if(free_re_exclude==0) {
			re_exclude= (char **)(re_exclude ? ck_realloc(re_exclude,(size_re_exclude+32)*sizeof(char *)) : ck_malloc(sizeof(char *)*32));
			free_re_exclude+=32;
		}
		re_exclude[size_re_exclude]=x_buffer+size_x_buffer;
		size_re_exclude++;
		free_re_exclude--;
	} else {
		if(free_exclude==0) {
			exclude=(char **)(exclude ? ck_realloc(exclude,(size_exclude+32)*sizeof(char *)) : ck_malloc(sizeof(char *)*32));
			free_exclude+=32;
		}
		exclude[size_exclude]=x_buffer+size_x_buffer;
		size_exclude++;
		free_exclude--;
	}
	strcpy(x_buffer+size_x_buffer,name);
	size_x_buffer+=size_buf+1;
	free_x_buffer-=size_buf+1;
}

add_exclude_file(file)
char *file;
{
	FILE *fp;
	char buf[1024];
	extern char *rindex();

	if(strcmp(file, "-"))
		fp=fopen(file,"r");
	else
		/* Let's hope the person knows what they're doing. */
		/* Using -X - -T - -f - will get you *REALLY* strange
		   results. . . */
		fp=stdin;

	if(!fp) {
		msg_perror("can't open %s",file);
		exit(2);
	}
	while(fgets(buf,1024,fp)) {
		int size_buf;
		char *end_str;

		end_str=rindex(buf,'\n');
		if(end_str)
			*end_str='\0';
		add_exclude(buf);

	}
	fclose(fp);
}

int
is_regex(str)
char *str;
{
	return index(str,'*') || index(str,'[') || index(str,'?');
}

/* Returns non-zero if the file 'name' should not be added/extracted */
int
check_exclude(name)
char *name;
{
	int n;
	char *str;
	extern char *strstr();

	for(n=0;n<size_re_exclude;n++) {
		if(wildmat(name,re_exclude[n]))
			return 1;
	}
	for(n=0;n<size_exclude;n++) {
		/* Accept the output from strstr only if it is the last
		   part of the string.  There is certainly a faster way to
		   do this. . . */
		if(   (str=strstr(name,exclude[n]))
 		   && (str==name || str[-1]=='/')
		   && str[strlen(exclude[n])]=='\0')
			return 1;
	}
	return 0;
}

long
intconv(char *s, long unit)
{
	long value, factor = unit;
	char *q;
	value = strtol(s, &q, 0);
	if (q == s) {
bad:		fprintf(stderr, "Invalid number '%s' specified\n", s);
		exit(EX_ARGSBAD);
	}
	if (q && *q) {
		switch (*q++) {
		case 'b': case 'B':
			factor = blocking * RECORDSIZE;
			break;
		case 'k': case 'K':
			factor = 1024;
			break;
		case 'm': case 'M':
			factor = 1024L * 1024L;
			break;
		case 'r': case 'R':
			factor = RECORDSIZE;
			break;
		default:
			goto bad;
		}
		if (*q)
			goto bad;
	}
	return (value * factor) / unit;
}
