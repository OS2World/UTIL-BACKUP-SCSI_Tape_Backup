/*****************************************************************************
 * $Id: port.h,v 1.4 1992/09/29 09:55:58 ak Exp $
 *****************************************************************************
 * $Log: port.h,v $
 * Revision 1.4  1992/09/29  09:55:58  ak
 * K.U.R., once again :-)
 * - removed a dup() in buffer.c
 * - EMX opendir with hidden/system test in gnu.c (...dotdot)
 *
 * Revision 1.3  1992/09/12  15:57:00  ak
 * - Usenet patches for GNU TAR 1.10
 * - Bugfixes and patches of Kai Uwe Rommel:
 *         filename conversion for FAT
 *         EMX 0.8e
 *         -0..1 alias for a: b:
 *         -2..7 alias for +++TAPE$x
 *
 * Revision 1.2  1992/09/02  20:08:34  ak
 * Version AK200
 * - Tape access
 * - Quick file access
 * - OS/2 extended attributes
 * - Some OS/2 fixes
 * - Some fixes of Kai Uwe Rommel
 *
 * Revision 1.1.1.1  1992/09/02  19:22:19  ak
 * Original GNU Tar 1.10 with some filenames changed for FAT compatibility.
 *
 * Revision 1.1  1992/09/02  19:22:17  ak
 * Initial revision
 *
 *****************************************************************************/

/*
 * Modified by Andreas Kaiser July 92.
 * See CHANGES.AK for info.
 */

/* Portability declarations.
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
 * Portability declarations for tar.
 *
 * @(#)port.h 1.3	87/11/11	by John Gilmore, 1986
 */

/*
 * Everybody does wait() differently.  There seem to be no definitions
 * for this in V7 (e.g. you are supposed to shift and mask things out
 * using constant shifts and masks.)  So fuck 'em all -- my own non
 * standard but portable macros.  Don't change to a "union wait"
 * based approach -- the ordering of the elements of the struct 
 * depends on the byte-sex of the machine.  Foo!
 */
#define	TERM_SIGNAL(status)	((status) & 0x7F)
#define TERM_COREDUMP(status)	(((status) & 0x80) != 0)
#define TERM_VALUE(status)	((status) >> 8)

#ifdef	MSDOS
/* missing things from sys/stat.h */
#define	S_ISUID		0
#define	S_ISGID		0
#define	S_ISVTX		0

/* device stuff */
#define	makedev(ma, mi)		((ma << 8) | mi)
#define	major(dev)		(dev)
#define	minor(dev)		(dev)
#endif	/* MSDOS */

/*------------------------------------------------------------------------*/

#ifdef OS2

#define FILENAME_MAX 260

#include <os2eattr.h>
pEABuf		eabuf;		/* extended attribute buffer */
long		ealen;		/* length of .. */
#ifdef NAMSIZ
char		eaname[NAMSIZ];	/* file name of current EA */
#endif

#define chdir _chdir2

#endif
