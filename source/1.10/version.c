/*****************************************************************************
 * $Id: version.c,v 1.8 1993/02/21 14:43:58 ak Exp $
 *****************************************************************************
 * $Log: version.c,v $
 * Revision 1.8  1993/02/21  14:43:58  ak
 * mkdir() changed in 0.8f.
 *
 * Revision 1.7  1993/02/15  22:58:53  ak
 * *** empty log message ***
 *
 * Revision 1.6  1992/12/13  10:18:32  ak
 * 2.10
 *
 * Revision 1.5  1992/10/29  10:34:29  ak
 * *** empty log message ***
 *
 * Revision 1.4  1992/09/26  09:07:24  ak
 * Version 2.01
 *
 * Revision 1.3  1992/09/12  15:57:41  ak
 * - Usenet patches for GNU TAR 1.10
 * - Bugfixes and patches of Kai Uwe Rommel:
 *         filename conversion for FAT
 *         EMX 0.8e
 *         -0..1 alias for a: b:
 *         -2..7 alias for +++TAPE$x
 *
 * Revision 1.2  1992/09/02  20:08:59  ak
 * Version AK200
 * - Tape access
 * - Quick file access
 * - OS/2 extended attributes
 * - Some OS/2 fixes
 * - Some fixes of Kai Uwe Rommel
 *
 * Revision 1.1.1.1  1992/09/02  19:23:08  ak
 * Original GNU Tar 1.10 with some filenames changed for FAT compatibility.
 *
 * Revision 1.1  1992/09/02  19:23:06  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: version.c,v 1.8 1993/02/21 14:43:58 ak Exp $";

/*
 * Modified by Andreas Kaiser July 92.
 * See CHANGES.AK for info.
 */

/* Version info for tar.
   Copyright (C) 1989, Free Software Foundation.

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

#if 1
char version_string[] = "GNU tar version 1.10 (GTAK 2.12)";
#else   
char version_string[] = "GNU tar version 1.10";
#endif
