/*****************************************************************************
 * $Id: ddebug.c,v 1.4 1992/10/14 16:50:28 ak Exp $
 *****************************************************************************
 * $Log: ddebug.c,v $
 * Revision 1.4  1992/10/14  16:50:28  ak
 * Allow either PhysToVirt or GDT selector to access video.
 * Note that PhysToVirt cannot be used in interrupt time.
 *
 * Revision 1.3  1992/07/24  11:34:11  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
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

#define UseSel		0	/* GDT selector or PhysToVirt? */
		/* note that PhysToVirt cannot be used at interrupt time */

#define DefaultAddress 	0x0B8000L
#define NCols		80
#define NRows		25
#define LineSize	(NCols * sizeof(word))
#define ScreenSize	(NRows * LineSize)
#define Attrib		0x0700

static int	row, col;
dword		video_address;
#if UseSel
static word	video_sel;
static word	_far * screen;
static dword	prev_address;
#endif

void _cdecl
_STI_ddebug()
{
	if (video_address == 0)
		video_address = DefaultAddress;
#if UseSel
	prev_address = video_address;
	if (AllocGDTSelector(&video_sel, 1))
		video_sel = 0;
	if (PhysToGDTSelector(video_address, ScreenSize, video_sel))
		screen = 0;
	else
		screen = Pointer(video_sel, 0);
#endif
	row = col = 0;
}

void
putc(byte c)
{
	if (inInitPhase()) {
		USHORT nw;
		static byte cr = 0x0D;
		if (c == '\n')
			DosWrite(1, &cr, 1, &nw);
		DosWrite(1, &c, 1, &nw);
	} else if (c == '\t') {
		do putc(' '); while (col & 7);
	} else {
		word _far *scr;
		if (inProtMode()) {
#if UseSel
			if (video_address != prev_address) {
				video_address = prev_address;
				if (video_sel
				 && PhysToGDTSelector(video_address, ScreenSize,
							video_sel) == 0)
					screen = Pointer(video_sel, 0);
			}
			scr = screen;
#else
			scr = PhysToVirt(video_address, NRows * NCols * 2);
#endif
		} else
			scr = (word _far *)(video_address << 12);
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
#if !UseSel
			UnPhysToVirt();
#endif
		}
	}
}

void
puts(char _far *s)
{
	while (*s)
		putc(*s++);
}

void
putx(dword x)
{
	int i = 32 - 4;
	for (; i != 0; i -= 4)
		if (x >= 1L << i)
			break;
	for (; i >= 0; i -= 4)
		putc("0123456789ABCDEF"[x >> i & 0x0F]);
}

void
putd(long x)
{
	if (x == -0x80000000L) {
		puts("-2147483648");
		return;
	}
	if (x < 0) {
		putc('-');
		x = -x;
	}
	if (x >= 10) {
		putd(x / 10);
		x %= 10;
	}
	putc("0123456789"[x]);
}
