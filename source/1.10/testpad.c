/*****************************************************************************
 * $Id: testpad.c,v 1.2 1992/09/02 20:08:54 ak Exp $
 *****************************************************************************
 * $Log: testpad.c,v $
 * Revision 1.2  1992/09/02  20:08:54  ak
 * Version AK200
 * - Tape access
 * - Quick file access
 * - OS/2 extended attributes
 * - Some OS/2 fixes
 * - Some fixes of Kai Uwe Rommel
 *
 * Revision 1.1.1.1  1992/09/02  19:23:01  ak
 * Original GNU Tar 1.10 with some filenames changed for FAT compatibility.
 *
 * Revision 1.1  1992/09/02  19:22:59  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: testpad.c,v 1.2 1992/09/02 20:08:54 ak Exp $";

/* Find out if we need the pad field in the header for this machine
   Copyright (C) 1991 Free Software Foundation

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 1, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>

struct inc
{
  char a[20];
  char b[20];
};

struct test1
{
  char a;
  struct inc in[5];
};

struct test2
{
  char a;
  char b;
  struct inc in[5];
};

main ()
{
  struct test1 t1;
  struct test2 t2;
  int t1diff, t2diff;
  
  t1diff = (char *)&t1.in - (char *)&t1;
  t2diff = (char *)&t2.in - (char *)&t2;
  
  if (t2diff == t1diff + 1)
    printf ("#define NEEDPAD\n");
  else if (t1diff != t2diff)
    fprintf (stderr, "Cannot determine padding for tar struct, \n\
will try with none.\n");

  exit (0);
}

      
    
  
