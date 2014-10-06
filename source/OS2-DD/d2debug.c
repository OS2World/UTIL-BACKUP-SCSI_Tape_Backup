/*****************************************************************************
 * $Id: d2debug.c,v 1.3 1993/01/17 10:46:59 ak Exp $
 *****************************************************************************
 * $Log: d2debug.c,v $
 * Revision 1.3  1993/01/17  10:46:59  ak
 * *** empty log message ***
 *
 * Revision 1.2  1992/10/14  16:48:54  ak
 * _fmemcpy as macro
 *
 * Revision 1.1  1992/07/24  11:33:10  ak
 * Initial revision
 *
 * Revision 1.2  1992/01/06  20:09:26  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1992/01/06  19:54:09  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:54:07  ak
 * Initial revision
 *
 *****************************************************************************/

#include "dd.h"
#define INCL_DOS
#include <os2.h>
#include <string.h>
#ifdef __ZTC__
# define _fmemcpy(d,s,n) movedata(Segment(s), Offset(s), Segment(d), Offset(d), n)
#else
# include <memory.h>
#endif

#define DefaultAddress 	0x0B8000L
#define NCols		80
#define NRows		25
#define Attrib		0x0700

static int	row, col;
dword		video_address;

void _cdecl
_STI_d2debug()
{
	if (video_address == 0)
		video_address = DefaultAddress;
	row = col = 0;
}

void
put2c(byte c)
{
	if (c == '\t') {
		do put2c(' '); while (col & 7);
	} else {
		word _far *scr = PhysToVirt(video_address, NRows * NCols * 2);
		if (scr) {
			if (c == '\n' || col == 80) {
				col = 0;
				row += 1;
			}
			if (row == NRows) {
				int i;
				_fmemmove(scr, scr + NCols, (NRows - 1) * NCols * 2);
				for (i = 0; i < NCols; ++i)
					scr[(NRows - 1) * NCols + i] = ' ' + Attrib;
				row = NRows - 1;
			}
			if (c != '\n')
				scr[row * NCols + col++] = c | Attrib;
			UnPhysToVirt();
		}
	}
}

void
put2s(char _far *s)
{
	while (*s)
		put2c(*s++);
}

void
put2x(dword x)
{
	int i = 32 - 4;
	for (; i != 0; i -= 4)
		if (x >= 1L << i)
			break;
	for (; i >= 0; i -= 4)
		put2c("0123456789ABCDEF"[x >> i & 0x0F]);
}

void
put2d(long x)
{
	if (x == -0x80000000L) {
		put2s("-2147483648");
		return;
	}
	if (x < 0) {
		put2c('-');
		x = -x;
	}
	if (x >= 10) {
		put2d(x / 10);
		x %= 10;
	}
	put2c("0123456789"[x]);
}
