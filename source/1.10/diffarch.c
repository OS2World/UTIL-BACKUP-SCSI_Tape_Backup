/*****************************************************************************
 * $Id: diffarch.c,v 1.7 1993/01/14 21:50:37 ak Exp $
 *****************************************************************************
 * $Log: diffarch.c,v $
 * Revision 1.7  1993/01/14  21:50:37  ak
 * EMX 0.8f fixes and enhancements by Kai Uwe Rommel.
 *
 * Revision 1.6  1993/01/12  18:24:55  ak
 * setnbuf for TTY output only.
 *
 * Revision 1.5  1993/01/10  11:54:43  ak
 * diff_init sets output stream to non-buffered.
 *
 * Revision 1.4  1992/12/19  22:22:24  ak
 * Eberhard Mattes: Trapped when ea_load returned NULL.
 *
 * Revision 1.3  1992/09/12  15:57:23  ak
 * - Usenet patches for GNU TAR 1.10
 * - Bugfixes and patches of Kai Uwe Rommel:
 *         filename conversion for FAT
 *         EMX 0.8e
 *         -0..1 alias for a: b:
 *         -2..7 alias for +++TAPE$x
 *
 * Revision 1.2  1992/09/02  20:07:55  ak
 * Version AK200
 * - Tape access
 * - Quick file access
 * - OS/2 extended attributes
 * - Some OS/2 fixes
 * - Some fixes of Kai Uwe Rommel
 *
 * Revision 1.1.1.1  1992/09/02  19:21:14  ak
 * Original GNU Tar 1.10 with some filenames changed for FAT compatibility.
 *
 * Revision 1.1  1992/09/02  19:21:12  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: diffarch.c,v 1.7 1993/01/14 21:50:37 ak Exp $";

/*
 * Modified by Andreas Kaiser July 92.
 * See CHANGES.AK for info.
 */

/* Modified so that the verify option works even if some filenames got
   mangled because of excessive length.  Note that this also required
   adding an inverse_find_mangled operation to mangle.c.  While I was
   at it I also fixed the handling of absolute pathnames and also
   three buggy error messages in the handling of links that used the wrong
   names.  I didn't make the verification of mangled names or absolute
   pathnames work for dump directories.
     -Max Hailperin <max@nic.gac.edu> 8/1/91 */

/* Further modified so that diff won't complain about uid or gid differing
   if the numerically stored id matches, even if the name doesn't tally.
   This prevents error messages verifying an archive with files owned by
   non-existant users or groups.
     -Max Hailperin <max@nic.gac.edu> 8/2/91 */

/* Fixed two bugs in the changes of 8/1/91.
    1) For some reason linkabspath was one byte two small, even though
       abspath was right.
    2) The filename and linkname variables were initialized to point into
       a header record in the buffer which might go away in the course of
       diffing the file contents, leading to garbage filenames in data
       differs error message.  The saverec mechamism only protects head,
       not other pointers into that record.
   -Max Hailperin <max@nic.gac.edu> 8/11/91 */

/* Diff files from a tar archive.
   Copyright (C) 1988 Free Software Foundation

This file is part of GNU Tar.

GNU Tar is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GNU Tar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Tar; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/*
 * Diff files from a tar archive.
 *
 * Written 30 April 1987 by John Gilmore, ihnp4!hoptoad!gnu.
 *
 * @(#) diffarch.c 1.10 87/11/11 - gnu
 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef V7
#include <fcntl.h>
#endif

#ifdef BSD42
#include <sys/file.h>
#endif

#ifndef MSDOS
#include <sys/ioctl.h>
#if !defined(USG) || defined(HAVE_MTIO)
#include <sys/mtio.h>
#endif
#endif

#ifdef USG
#include <fcntl.h>
#endif

/* Some systems don't have these #define's -- we fake it here. */
#ifndef O_RDONLY
#define	O_RDONLY	0
#endif
#ifndef	O_NDELAY
#define	O_NDELAY	0
#endif

/*
 * Most people don't have a #define for this.
 */
#ifndef	O_BINARY
#define	O_BINARY	0
#endif

#ifndef S_IFLNK
#define lstat stat
#endif

extern int errno;			/* From libc.a */
extern char *valloc();			/* From libc.a */

#include "tar.h"
#include "port.h"
#include "rmt.h"

extern union record *head;		/* Points to current tape header */
extern struct stat hstat;		/* Stat struct corresponding */
extern int head_standard;		/* Tape header is in ANSI format */

extern void print_header();
extern void skip_file();
extern void skip_extended_headers();

extern FILE *msg_file;

int now_verifying = 0;		/* Are we verifying at the moment? */

char	*diff_name;		/* head->header.name */

int	diff_fd;		/* Descriptor of file we're diffing */

char	*diff_buf = 0;		/* Pointer to area for reading
					   file contents into */

char	*diff_dir;		/* Directory contents for LF_DUMPDIR */

#ifdef OS2
char *eaptr;			/* EA pointer */
#endif

int different = 0;

#define FILENAME (filename?filename:head->header.name)
#define LINKNAME (linkname?linkname:head->header.linkname)

static char *filename, *linkname; /* NULL means as in header */
extern char *inverse_find_mangled();
static char abspath[FILENAME_MAX+2] = "/", linkabspath[FILENAME_MAX+2] = "/";

/*struct sp_array *sparsearray;
int 		sp_ar_size = 10;*/
/*
 * Initialize for a diff operation
 */
diff_init()
{

	/*NOSTRICT*/
	diff_buf = (char *) valloc((unsigned)blocksize);
	if (!diff_buf) {
		msg("could not allocate memory for diff buffer of %d bytes",
			blocksize);
		exit(EX_ARGSBAD);
	}
	if (isatty(fileno(msg_file)))
		setvbuf(msg_file, NULL, _IONBF, 0);
}

/*
 * Diff a file against the archive.
 */
void
diff_archive()
{
	register char *data;
	int check, namelen;
	int err;
	long offset;
	struct stat filestat;
	int compare_chunk();
	int compare_dir();
#ifdef OS2
	int compare_eattr();
#endif
	int no_op();
#ifndef MSDOS
	dev_t	dev;
	ino_t	ino;
#endif
	char *get_dir_contents();
	long from_oct();
	long lseek();
        char *new_name;
        long numerical_uid, numerical_gid;

	errno = EPIPE;			/* FIXME, remove perrors */

	saverec(&head);			/* Make sure it sticks around */
	userec(head);			/* And go past it in the archive */
	decode_header(head, &hstat, &head_standard, 1);	/* Snarf fields */
	numerical_uid = from_oct(8,  head->header.uid);
	numerical_gid = from_oct(8,  head->header.gid);

        filename = NULL;
        linkname = NULL;

	/* Print the record from 'head' and 'hstat' */
	if (f_verbose) {
		if(now_verifying)
			fprintf(msg_file,"Verify ");
		print_header();
	}

	diff_name = head->header.name;
	switch (head->header.linkflag) {

	default:
		msg("Unknown file type '%c' for %s, diffed as normal file",
			head->header.linkflag, FILENAME);

		/* FALL THRU */

	case LF_OLDNORMAL:
	case LF_NORMAL:
	case LF_SPARSE:
	case LF_CONTIG:
		/*
		 * Appears to be a file.
		 * See if it's really a directory.
		 */
		namelen = strlen(FILENAME)-1;
		if (FILENAME[namelen] == '/')
			goto really_dir;

		
		if(do_stat(&filestat)) {
			if (head->header.isextended)
				skip_extended_headers();
			skip_file((long)hstat.st_size);
			different++;
			goto quit;
		}

		if ((filestat.st_mode & S_IFMT) != S_IFREG) {
			fprintf(msg_file, "%s: not a regular file\n",
				FILENAME);
			skip_file((long)hstat.st_size);
			different++;
			goto quit;
		}

		filestat.st_mode &= ~S_IFMT;
		if (filestat.st_mode != hstat.st_mode)
			sigh("mode");
#ifndef OS2
		if (filestat.st_uid  != hstat.st_uid &&
                    filestat.st_uid  != numerical_uid)
			sigh("uid");
		if (filestat.st_gid  != hstat.st_gid &&
                    filestat.st_gid  != numerical_gid)
			sigh("gid");
#endif
		if (filestat.st_mtime != hstat.st_mtime)
			sigh("mod time");
		if (head->header.linkflag != LF_SPARSE &&
				filestat.st_size != hstat.st_size) {
			sigh("size");
			skip_file((long)hstat.st_size);
			goto quit;
		}

		diff_fd = open(FILENAME, O_NDELAY|O_RDONLY|O_BINARY);

		if (diff_fd < 0) {
			msg_perror("cannot open %s",FILENAME);
			if (head->header.isextended)
				skip_extended_headers();
			skip_file((long)hstat.st_size);
			different++;
			goto quit;
		}
		/*
		 * Need to treat sparse files completely differently here.
		 */
		if (head->header.linkflag == LF_SPARSE)
			diff_sparse_files(hstat.st_size);
		else 
			wantbytes((long)(hstat.st_size),compare_chunk);

		check = close(diff_fd);
		if (check < 0)
			msg_perror("Error while closing %s",FILENAME);

	quit:
		break;

        case LF_NAMES:
                skip_file((long)hstat.st_size);
                break;

#ifndef MSDOS
	case LF_LINK:
		if(do_stat(&filestat))
			break;
		dev = filestat.st_dev;
		ino = filestat.st_ino;
              retry_linkstat:
		err = stat(LINKNAME, &filestat);
		if (err < 0) {
			if (errno==ENOENT) {
                          if(now_verifying &&
                             (new_name = inverse_find_mangled(LINKNAME))
                             != NULL){
                            linkname = new_name;
                            goto retry_linkstat;
                          }
                          else if(!f_absolute_paths && *LINKNAME != '/'){
                            strcpy(linkabspath+1,LINKNAME);
                            linkname = linkabspath;
                            goto retry_linkstat;
                          }
                          else{
                            if(!f_absolute_paths && linkname != NULL &&
                               *linkname == '/')
                              linkname++;
                            fprintf(msg_file, "%s: does not exist\n",LINKNAME);
                          }
			} else {
				msg_perror("cannot stat file %s",LINKNAME);
			}
			different++;
			break;
		}
		if(filestat.st_dev!=dev || filestat.st_ino!=ino) {
			fprintf(msg_file, "%s not linked to %s\n",FILENAME,LINKNAME);
			break;
		}
		break;
#endif

#ifdef S_IFLNK
	case LF_SYMLINK:
	{
		char linkbuf[FILENAME_MAX+1];
              retry_readlink:
		check = readlink(FILENAME, linkbuf,
				 (sizeof linkbuf)-1);
		
		if (check < 0) {
			if (errno == ENOENT) {
                          if(now_verifying &&
                             (new_name = inverse_find_mangled(FILENAME))
                             != NULL){
                            filename = new_name;
                            goto retry_readlink;
                          }
                          else if(!f_absolute_paths && *FILENAME != '/'){
                            strcpy(abspath+1, FILENAME);
                            filename = abspath;
                            goto retry_readlink;
                          }
                          else{
                            if(!f_absolute_paths && filename != NULL &&
                               *filename == '/')
                              filename++;
				fprintf(msg_file,
					"%s: no such file or directory\n",
					FILENAME);
                          }
			} else {
				msg_perror("cannot read link %s",FILENAME);
			}
			different++;
			break;
		}

		linkbuf[check] = '\0';	/* Null-terminate it */
              retry_symlink_check:
		if (strncmp(LINKNAME, linkbuf, check) != 0) {
                  if(now_verifying &&
                     (new_name = inverse_find_mangled(LINKNAME)) != NULL){
                    linkname = new_name;
                    goto retry_symlink_check;
                  }
                  else if(!f_absolute_paths && *LINKNAME != '/'){
                    strcpy(linkabspath+1, LINKNAME);
                    linkname = linkabspath;
                    goto retry_symlink_check;
                  }
                  else{
                    if(!f_absolute_paths && linkname != NULL &&
                       *linkname == '/')
                      linkname++;
			fprintf(msg_file, "%s: symlink differs\n",
				FILENAME);
			different++;
		}
	}
	}
		break;
#endif

	case LF_CHR:
		hstat.st_mode |= S_IFCHR;
		goto check_node;

#ifdef S_IFBLK
	/* If local system doesn't support block devices, use default case */
	case LF_BLK:
		hstat.st_mode |= S_IFBLK;
		goto check_node;
#endif

#ifdef S_IFIFO
	/* If local system doesn't support FIFOs, use default case */
	case LF_FIFO:
		hstat.st_mode |= S_IFIFO;
		hstat.st_rdev = 0;		/* FIXME, do we need this? */
		goto check_node;
#endif

	check_node:
		/* FIXME, deal with umask */
		if(do_stat(&filestat))
			break;
		if(hstat.st_rdev != filestat.st_rdev) {
			fprintf(msg_file, "%s: device numbers changed\n", FILENAME);
			different++;
			break;
		}
		if(hstat.st_mode != filestat.st_mode) {
			fprintf(msg_file, "%s: mode or device-type changed\n", FILENAME);
			different++;
			break;
		}
		break;

	case LF_DUMPDIR:
		data=diff_dir=get_dir_contents(FILENAME,0);
		if(data != NULL && *data) {
		wantbytes((long)(hstat.st_size),compare_dir);
		free(data);
		}
		else wantbytes((long)(hstat.st_size),no_op);
		/* FALL THROUGH */

	case LF_DIR:
		/* Check for trailing / */
		namelen = strlen(FILENAME)-1;
	really_dir:
		while (namelen && FILENAME[namelen] == '/')
			FILENAME[namelen--] = '\0';	/* Zap / */

		if(do_stat(&filestat))
			break;
		if((filestat.st_mode&S_IFMT)!=S_IFDIR) {
			fprintf(msg_file, "%s is no longer a directory\n",FILENAME);
			different++;
			break;
		}
		if((filestat.st_mode&~S_IFMT) != hstat.st_mode)
			sigh("mode");
		break;

	case LF_VOLHDR:
		break;

	case LF_MULTIVOL:
		namelen = strlen(FILENAME)-1;
		if (FILENAME[namelen] == '/')
			goto really_dir;

		if(do_stat(&filestat))
			break;

		if ((filestat.st_mode & S_IFMT) != S_IFREG) {
			fprintf(msg_file, "%s: not a regular file\n",
				FILENAME);
			skip_file((long)hstat.st_size);
			different++;
			break;
		}

		filestat.st_mode &= ~S_IFMT;
		offset = from_oct(1+12, head->header.offset);
		if (filestat.st_size != hstat.st_size + offset) {
			sigh("size");
			skip_file((long)hstat.st_size);
			different++;
			break;
		}

		diff_fd = open(FILENAME, O_NDELAY|O_RDONLY|O_BINARY);

		if (diff_fd < 0) {
                        msg_perror("cannot open file %s",FILENAME);
			skip_file((long)hstat.st_size);
			different++;
			break;
		}
		err = lseek(diff_fd, offset, 0);
		if(err!=offset) {
			msg_perror("cannot seek to %ld in file %s",offset,FILENAME);
			different++;
			break;
		}

		wantbytes((long)(hstat.st_size),compare_chunk);

		check = close(diff_fd);
		if (check < 0) {
			msg_perror("Error while closing %s",FILENAME);
		}
		break;

#ifdef OS2
	case LF_EATTR:
		if (eabuf)
			ea_free(eabuf);
		eabuf = ea_load(diff_name);
		if (hstat.st_size != (eabuf ? EALength(eabuf) : 0)) {
			sigh("EA size");
			goto quit;
		}
		if (hstat.st_size != 0) {
			eaptr = (char _far *)eabuf;
			wantbytes((long)(hstat.st_size),compare_eattr);
		}
		break;
#endif
	}

	/* We don't need to save it any longer. */
	saverec((union record **) 0);	/* Unsave it */
}

int
compare_chunk(bytes,buffer)
long bytes;
char *buffer;
{
	int err;

	err=read(diff_fd,diff_buf,bytes);
	if(err!=bytes) {
		if(err<0) {
			msg_perror("can't read %s",FILENAME);
		} else {
			fprintf(msg_file,"%s: could only read %d of %d bytes\n",FILENAME,err,bytes);
		}
		different++;
		return -1;
	}
	if(bcmp(buffer,diff_buf,bytes)) {
		fprintf(msg_file, "%s: data differs\n",FILENAME);
		different++;
		return -1;
	}
	return 0;
}

int
compare_dir(bytes,buffer)
long bytes;
char *buffer;
{
	if(bcmp(buffer,diff_dir,bytes)) {
		fprintf(msg_file, "%s: data differs\n",FILENAME);
		different++;
		return -1;
	}
	diff_dir+=bytes;
	return 0;
}

int
compare_eattr(bytes,buffer)
int bytes;
char *buffer;
{
#ifdef OS2
	int err = bcmp(buffer, eaptr, bytes);
	eaptr += bytes;
	if (err) {
		fprintf(msg_file, "%s: EA data differs\n",head->header.name);
		different++;
		return -1;
	}
#endif
	return 0;
}

/*
 * Sigh about something that differs.
 */
sigh(what)
	char *what;
{

	fprintf(msg_file, "%s: %s differs\n",
		FILENAME, what);
}

verify_volume()
{
	int status;
#ifdef MTIOCTOP
	struct mtop t;
	int er;
#endif

	if(!diff_buf)
		diff_init();
#ifdef MTIOCTOP
	t.mt_op = MTBSF;
	t.mt_count = 1;
	if((er=rmtioctl(archive,MTIOCTOP,&t))<0) {
		if(errno!=EIO || (er=rmtioctl(archive,MTIOCTOP,&t))<0) {
#endif
			if(rmtlseek(archive,0L,0)!=0) {
				/* Lseek failed.  Try a different method */
				msg_perror("Couldn't rewind archive file for verify");
				return;
			}
#ifdef MTIOCTOP
		}
	}
#endif
	ar_reading=1;
	now_verifying = 1;
	fl_read();
	for(;;) {
		status = read_header();
		if(status==0) {
			unsigned n;

			n=0;
			do {
				n++;
				status=read_header();
			} while(status==0);
			msg("VERIFY FAILURE: %d invalid header%s detected!",n,n==1?"":"s");
		}
		if(status==2 || status==EOF)
			break;
		diff_archive();
	}
	ar_reading=0;
	now_verifying = 0;

}

int do_stat(statp)
struct stat *statp;
{
	int err;
        char *new_name;

      retry:
	err = f_follow_links ? stat(FILENAME, statp) : lstat(FILENAME, statp);
	if (err < 0) {
		if (errno==ENOENT) {
                  if(now_verifying &&
                     (new_name = inverse_find_mangled(FILENAME)) != NULL){
                    filename = new_name;
                    goto retry;
                  }
                  else if(!f_absolute_paths && *FILENAME != '/'){
                    strcpy(abspath+1, FILENAME);
                    filename = abspath;
                    goto retry;
                  }
                  else{
                    if(!f_absolute_paths && filename != NULL &&
                       *filename == '/')
                      filename++;
                    fprintf(msg_file, "%s: does not exist\n",FILENAME);
                  }
		} else
			msg_perror("can't stat file %s",FILENAME);
/*		skip_file((long)hstat.st_size);
		different++;*/
		return 1;
	}
#ifdef MSDOS
	get_fileattr(diff_name, statp);
#endif
	return 0;
}

/*
 * JK
 * Diff'ing a sparse file with its counterpart on the tar file is a 
 * bit of a different story than a normal file.  First, we must know
 * what areas of the file to skip through, i.e., we need to contruct
 * a sparsearray, which will hold all the information we need.  We must
 * compare small amounts of data at a time as we find it.  
 */

diff_sparse_files(filesize)
int	filesize;

{
	int		sparse_ind = 0;
	char		*buf;
	int		buf_size = RECORDSIZE;
	union record 	*datarec;	
	int		err;
	long		numbytes;
	int		amt_read = 0;
	int		size = filesize;

	buf = (char *) malloc(buf_size * sizeof (char));
	
	fill_in_sparse_array();
	

	while (size > 0) {
		datarec = findrec();
		if (!sparsearray[sparse_ind].numbytes)
			break;

		/*
		 * 'numbytes' is nicer to write than
		 * 'sparsearray[sparse_ind].numbytes' all the time ...
		 */
		numbytes = sparsearray[sparse_ind].numbytes;
		
		lseek(diff_fd, sparsearray[sparse_ind].offset, 0);
		/*
		 * take care to not run out of room in our buffer
		 */
		while (buf_size < numbytes) {
			buf = (char *) realloc(buf, buf_size * 2 * sizeof(char));
			buf_size *= 2;
		}
		while (numbytes > RECORDSIZE) {
			if ((err = read(diff_fd, buf, RECORDSIZE)) != RECORDSIZE) {
	 			if (err < 0) 
					msg_perror("can't read %s", FILENAME);
				else
					fprintf(msg_file, "%s: could only read %d of %d bytes\n", 
						err, numbytes);
				break;
			}
			if (bcmp(buf, datarec->charptr, RECORDSIZE)) {
				different++;
				break;
			}
			numbytes -= err;
			size -= err;
			userec(datarec);
			datarec = findrec();
		}
		if ((err = read(diff_fd, buf, numbytes)) != numbytes) {
 			if (err < 0) 
				msg_perror("can't read %s", FILENAME);
			else
				fprintf(msg_file, "%s: could only read %d of %d bytes\n", 
						err, numbytes);
			break;
		}

		if (bcmp(buf, datarec->charptr, numbytes)) {
			different++;
			break;
		}
/*		amt_read += numbytes;
		if (amt_read >= RECORDSIZE) {
			amt_read = 0;
			userec(datarec);
			datarec = findrec();
		}*/
		userec(datarec);
		sparse_ind++;
		size -= numbytes;
	}
	/* 
	 * if the number of bytes read isn't the
	 * number of bytes supposedly in the file,
	 * they're different
	 */
/*	if (amt_read != filesize)
		different++;*/
	userec(datarec);
	free(sparsearray);
	if (different)
		fprintf(msg_file, "%s: data differs\n", FILENAME);

}

/*
 * JK
 * This routine should be used more often than it is ... look into
 * that.  Anyhow, what it does is translate the sparse information
 * on the header, and in any subsequent extended headers, into an
 * array of structures with true numbers, as opposed to character
 * strings.  It simply makes our life much easier, doing so many
 * comparisong and such.
 */
fill_in_sparse_array()
{
	int 	ind;
	long from_oct();

	/*
	 * allocate space for our scratch space; it's initially
	 * 10 elements long, but can change in this routine if
	 * necessary
	 */
	sp_array_size = 10;
	sparsearray = (struct sp_array *) malloc(sp_array_size * sizeof(struct sp_array));

	/*
	 * there are at most five of these structures in the header
	 * itself; read these in first
	 */
	for (ind = 0; ind < SPARSE_IN_HDR; ind++) {
		if (!head->header.sp[ind].numbytes)
			break;
		sparsearray[ind].offset =
			from_oct(1+12, head->header.sp[ind].offset);
		sparsearray[ind].numbytes =
			from_oct(1+12, head->header.sp[ind].numbytes);
	}
	/*
	 * if the header's extended, we gotta read in exhdr's till
	 * we're done
	 */
	if (head->header.isextended) {
 	    /* how far into the sparsearray we are 'so far' */
	    static int so_far_ind = SPARSE_IN_HDR;	
	    union record *exhdr;
   	    
	    for (;;) {
		exhdr = findrec();
		for (ind = 0; ind < SPARSE_EXT_HDR; ind++) {
			if (ind+so_far_ind > sp_array_size-1) {
				/*
 				 * we just ran out of room in our
				 *  scratch area - realloc it
 				 */
				sparsearray = (struct sp_array *)
					realloc(sparsearray, 
						sp_array_size*2*sizeof(struct sp_array));
				sp_array_size *= 2;
			}
			/*
			 * convert the character strings into longs
			 */
			sparsearray[ind+so_far_ind].offset = 
			    from_oct(1+12, exhdr->ext_hdr.sp[ind].offset);
			sparsearray[ind+so_far_ind].numbytes =
			    from_oct(1+12, exhdr->ext_hdr.sp[ind].numbytes);
		}
		/* 
		 * if this is the last extended header for this
		 * file, we can stop
		 */
		if (!exhdr->ext_hdr.isextended)
			break;
		else {
			so_far_ind += SPARSE_EXT_HDR;
			userec(exhdr);
		}
	    }
	    /* be sure to skip past the last one  */
	    userec(exhdr);
	}
}
