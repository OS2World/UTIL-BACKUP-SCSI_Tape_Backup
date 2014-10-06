#ifdef OS2

/* $Id: inherit.c,v 1.6 1992/03/30 21:41:49 ak Exp $ */

static char *rcsid = "$Id: inherit.c,v 1.6 1992/03/30 21:41:49 ak Exp $";

#define INCL_FILEMGR
#include <os2.h>

void
noinherit(int fd)
{
#if OS2 >= 2
	ULONG s;
	DosQueryFHState(fd, &s);
	DosSetFHState(fd, (s & ~0x77) | 0x80);
#else
	USHORT s;
	DosQFHandState(fd, &s);
	DosSetFHandState(fd, (s & ~0x77) | 0x80);
#endif
}

void
inherit(int fd)
{
#if OS2 >= 2
	ULONG s;
	DosQueryFHState(fd, &s);
	DosSetFHState(fd, (s & ~0x77) & ~0x80);
#else
	USHORT s;
	DosQFHandState(fd, &s);
	DosSetFHandState(fd, (s & ~0x77) & ~0x80);
#endif
}

#endif
