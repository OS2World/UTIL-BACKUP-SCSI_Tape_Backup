/*
 * $Id: os2ea_op.c,v 1.8 1992/07/24 11:49:52 ak Exp $
 *
 * Extended Attribute Management for OS/2.
 * - Operations on EAs.
 *
 * A.Kaiser	Mai 91
 *
 * $Log: os2ea_op.c,v $
 * Revision 1.8  1992/07/24  11:49:52  ak
 * Fixed memory leak: lists were not freed when load_ea returned 0.
 *
 * OS/2 2.0 ea buffer not contains a magic number field.
 * Type of EA buffer changed from FEAList to EABuf.
 *
 * Revision 1.7  1992/03/05  20:28:38  ak
 * Bugfix.
 *
 * Revision 1.6  1992/02/26  20:53:31  ak
 * OS/2 2.0.
 *
 * Revision 1.5  1992/02/14  18:40:44  ak
 * *** empty log message ***
 *
 * Revision 1.4  1992/01/03  14:37:29  ak
 * $Header: h:/SRC.SSB/lib/libx/os2ea_op.c,v 1.8 1992/07/24 11:49:52 ak Exp $Id:
 *
 * Revision 1.3  1992/01/03  14:03:53  ak
 * Zortech fixes & updates.
 *
 * Revision 1.2  1991/12/12  20:21:20  ak
 * RCSID fixes.
 * mk(s)temp fixed.
 *
 * Revision 1.1.1.1  1991/12/12  17:15:31  ak
 * Initial checkin of server source, modified to contain RCS IDs.
 *
 * Revision 1.1  1991/12/12  17:15:26  ak
 * Initial revision
 *
 */
#ifdef OS2

static char *rcsid = "$Id: os2ea_op.c,v 1.8 1992/07/24 11:49:52 ak Exp $";

#include <string.h>
#include <stdlib.h>
#define INCL_DOSFILEMGR
#include <os2.h>
#include <os2eattr.h>

#if OS2 >= 2
 #ifndef _fmalloc
  #define _fmalloc	malloc
  #define _frealloc	realloc
  #define _ffree	free
  #define _fmemcpy	memcpy
  #define _fmemcmp	memcmp
 #endif
#elif defined(__ZTC__)
 #include <ztc.h>
#else
 #include <malloc.h>
 #include <memory.h>
#endif

#if OS2 >= 2
  struct EABuf {
  	long	magic;
  	FEAList	ea;
  };
#else
  struct EABuf {
  	FEAList	ea;
  };
#endif

static pFEA
find_ea(pFEAList feaList, char *name)
{
	register pFEA	p;
	pFEA		end;
	register int	i, len;

	len = strlen(name);
	WalkFEA(p, feaList)
		if (p->cbName == len) {
			if (_fmemcmp(szNameFEA(p), name, len) != 0)
				continue;
			return p;
		}
	WalkFEAEnd(p, feaList)
	return 0;
}

int
ea_get(pEABuf pea, char *name, void *value, int *length)
{
	pFEA p;

	if ((p = find_ea(&pea->ea, name)) == 0)
		return 0;
	if (p->cbName > *length)
		return -1;
	*length = p->cbValue;
	_fmemcpy(value, aValueFEA(p), *length);
	return 0x100 + p->fEA;
}

#if OS2 < 2	/* too complicated in OS/2 2.0 */

pEABuf
ea_set(pEABuf pea, char *name, void *value, unsigned length, unsigned flag)
{
	pFEA	p, q;
	int	size, namlen, diff;

	namlen = strlen(name) + 1;
	size = sizeof(FEAData) + namlen + length;
	if (pea == 0) {
		if ((pea = _fmalloc((size_t)(size + sizeof(ULONG)))) == 0)
			return 0;
		p = pea->ea.list;
	} else {
		if ((p = find_ea(&pea->ea, name)) == 0) {
			diff = size;
			p = (pFEA) EndOfEA(&pea->ea);
		} else {
			diff = length - p->cbValue;
			if (diff == 0) {
				p->fEA = flag;
				_fmemcpy(aValueFEA(p), value, length);
				return pea;
			}
		}
		if ((pea = _frealloc(pea, (size_t)(pea->ea.cbList + diff))) == 0)
			return 0;
		_fmemmove((PBYTE)p + diff, p, (PBYTE) EndOfEA(&pea->ea) - (PBYTE)p);
	}
	p->fEA = flag;
	p->cbName = namlen;
	p->cbValue = length;
	_fmemcpy(szNameFEA(p), name, namlen);
	_fmemcpy(aValueFEA(p), value, length);
	pea->ea.cbList += diff;
	return pea;
}

pEABuf
ea_remove(pEABuf pea, char *name)
{
	pFEA	p, q;
	int	size;

	if ((p = find_ea(&pea->ea, name)) == 0)
		return pea;
	size = SizeFEA(p);
	if ((pea = _frealloc(pea, (size_t)(pea->ea.cbList - size))) == 0)
		return 0;
	q = NextFEA(p);
	if (q > p)
		_fmemmove(p, q, (size_t)((PBYTE)EndOfEA(&pea->ea) - (PBYTE)q));
	pea->ea.cbList -= size;
	return pea;
}

#endif	/* 2.0 */
#endif

