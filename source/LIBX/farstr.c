/*****************************************************************************
 * $Id: farstr.c,v 1.5 1992/06/22 14:30:11 ak Exp $
 *****************************************************************************
 * $Log: farstr.c,v $
 * Revision 1.5  1992/06/22  14:30:11  ak
 * *** empty log message ***
 *
 * Revision 1.4  1992/02/14  18:34:41  ak
 * *** empty log message ***
 *
 * Revision 1.3  1992/01/07  14:20:19  ak
 * *** empty log message ***
 *
 * Revision 1.2  1992/01/03  14:32:35  ak
 * $Header: h:/SRC.SSB/lib/libx/farstr.c,v 1.5 1992/06/22 14:30:11 ak Exp $Id:
 *
 * Revision 1.1  1992/01/03  13:57:48  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: farstr.c,v 1.5 1992/06/22 14:30:11 ak Exp $";

#if defined(__ZTC__) && (defined(__SMALL__) || defined(__MEDIUM__)) && !defined(DOS386)

#include <ztc.h>
#include <dos.h>

void _far *
_fmemcpy(void _far *dst, void const _far *src, unsigned len)
{
	movedata(FP_SEG(src), FP_OFF(src), FP_SEG(dst), FP_OFF(dst), len);
	return dst;
}

void _far *
_fmemmove(void _far *dst, void const _far *src, unsigned len)
{
	if (FP_SEG(dst) == FP_SEG(src)) {
		unsigned char _far *d = dst;
		unsigned char const _far *s = src;
		if (dst > src) {
			if (d >= s + len)
				goto move;
			d += len;
			s += len;
			while (len--)
				*--d = *--s;
		} else {
			if (s >= d + len)
				goto move;
			while (len--)
				*d++ = *s++;
		}
	} else
move:		movedata(FP_SEG(src), FP_OFF(src), FP_SEG(dst), FP_OFF(dst), len);
	return dst;
}

int
_fmemcmp(void const _far *dst, void const _far *src, unsigned len)
{
	unsigned char const _far *d = dst, _far *s = src;
	while (len--)
		if (*d != *s)
			return (*d < *s) ? -1 : 1;
	return 0;
}

#endif
