#ifdef __ZTC__
#if defined(__LARGE__) || defined(__COMPACT__)

#define _fmalloc	malloc
#define _frealloc	realloc
#define _ffree		free
#define _fmemcpy	memcpy
#define _fmemcmp	memcmp
#define _fmemmove	memmove

#else

#include <dos.h>
#define _fmalloc	farmalloc
#define _frealloc	farrealloc
#define _ffree		farfree
extern void _far *	_fmemcpy(void _far *, const void _far *, unsigned);
extern int		_fmemcmp(const void _far *, const void _far *, unsigned);
extern void _far *	_fmemmove(void _far *, const void _far *, unsigned);

#endif
#endif

