/*
 * $Id: os2ea_ld.c,v 1.13 1993/01/15 12:29:27 AK Exp $
 *
 * Extended Attribute Management for OS/2.
 * - Load/Store EAs.
 *
 * A.Kaiser	Mai 91
 *
 * $Log: os2ea_ld.c,v $
 * Revision 1.13  1993/01/15  12:29:27  AK
 * - Better handling of empty EA list.
 * - Clear EA buffer before use to clear alignment gaps.
 *
 * Revision 1.12  1992/11/23  14:02:49  AK
 * OS/2 2.0 SP Bug
 *
 * Revision 1.11  1992/10/26  12:28:38  ak
 * Bugfix. Some IFSs return a EA cbList value > of 64K even if no EAs are
 * present. Now cnt is checked for 0 after DosEnumAttribute.
 *
 * Revision 1.10  1992/07/24  11:49:38  ak
 * Fixed memory leak: lists were not freed when load_ea returned 0.
 *
 * OS/2 2.0 ea buffer not contains a magic number field.
 * Type of EA buffer changed from FEAList to EABuf.
 *
 * Revision 1.9  1992/04/21  12:32:49  ak
 * EMX 0.8c.
 *
 * Revision 1.8  1992/04/07  17:23:24  ak
 * OS/2 2.0 bugfix.
 *
 * Revision 1.7  1992/03/05  20:28:26  ak
 * Bugfix.
 *
 * Revision 1.6  1992/02/26  20:53:40  ak
 * OS/2 2.0.
 *
 * Revision 1.5  1992/02/14  18:40:33  ak
 * *** empty log message ***
 *
 * Revision 1.4  1992/01/03  14:37:19  ak
 * Header -> Id
 *
 * Revision 1.3  1992/01/03  14:03:45  ak
 * Zortech fixes & updates.
 *
 * Revision 1.2  1991/12/12  20:21:14  ak
 * RCSID fixes.
 * mk(s)temp fixed.
 *
 * Revision 1.1.1.1  1991/12/12  17:15:22  ak
 * Initial checkin of server source, modified to contain RCS IDs.
 *
 * Revision 1.1  1991/12/12  17:15:17  ak
 * Initial revision
 *
 */
#ifdef OS2

static char *rcsid = "$Id: os2ea_ld.c,v 1.13 1993/01/15 12:29:27 AK Exp $";

/* do not use QueryPathInfo for EA size, cbList is invalid in ServicePack */
#define OS2BUG_InvalidPathInfo	1
#define AcceptZeroMagic		1	/* for GTAK 2.10 bug */

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#define INCL_DOSFILEMGR
#include <os2.h>
#include <errno.h>
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

static pEABuf
load_ea(char *name, int file)
{
#if OS2 >= 2
	FILESTATUS4	finfo;
#else
	FILESTATUS2	finfo;
#endif
	pEABuf		pea = 0;
	int		rc;
	ULONG		cnt;
	pFEAList	feaList;
	pFEA		pfea;
	unsigned	length;
	pGEA		pgea;
	pGEAList	geaList = 0;
	EAOPData	eaop;
	pENA		pena;
	PBYTE		pbyte;

	/*
	 * Get size of EA list.
	 */
#if OS2 >= 2
	rc = name ? DosQueryPathInfo((PSZ)name, FIL_QUERYEASIZE, (PBYTE)&finfo, sizeof finfo)
		  : DosQueryFileInfo(file, FIL_QUERYEASIZE, (PBYTE)&finfo, sizeof finfo);
#else
	rc = name ? DosQPathInfo((PSZ)name, FIL_QUERYEASIZE, (PBYTE)&finfo, sizeof finfo, 0L)
		  : DosQFileInfo(file, FIL_QUERYEASIZE, (PBYTE)&finfo, sizeof finfo);
#endif
	if (rc) goto error;
#if OS2BUG_InvalidPathInfo
	if (finfo.cbList < 65535)
		finfo.cbList = 65535;
#endif

	/*
	 * Allocate buffers.
	 */
#if OS2 >= 2
	if (finfo.cbList == 0) return 0;
	length = 2 * finfo.cbList;
	if ((geaList = _fmalloc(length)) == 0) goto nomem;
	if ((pea = _fmalloc(sizeof(long) + length)) == 0) goto nomem;
	pea->magic = EAMagic2;
	feaList = &pea->ea;
#else
	if (finfo.cbList == 0 || finfo.cbList >= UINT_MAX) return 0;
	length = finfo.cbList;
	if ((geaList = _fmalloc(length)) == 0) goto nomem;
	if ((pea = _fmalloc(length)) == 0) goto nomem;
	feaList = &pea->ea;
#endif

	/*
	 * Get EA names to buffer <feaList>.
	 */
	cnt = -1L;
#if OS2 >= 2
	if (name) rc = DosEnumAttribute(1, (PVOID)name,
			1L, feaList, length, &cnt, 1);
	else      rc = DosEnumAttribute(0, (PVOID)&file,
			1L, feaList, length, &cnt, 1);
#else
	if (name) rc = DosEnumAttribute(ENUMEA_REFTYPE_PATH,    (PVOID)name,
			1L, feaList, length, &cnt, ENUMEA_LEVEL_NO_VALUE, 0L);
	else      rc = DosEnumAttribute(ENUMEA_REFTYPE_FHANDLE, (PVOID)&file,
			1L, feaList, length, &cnt, ENUMEA_LEVEL_NO_VALUE, 0L);
#endif
	if (rc) goto error;
	if (cnt == 0) goto error;

	/*
	 * Convert EA enumeration list to <geaList>.
	 */
#if OS2 >= 2
	pena = (pENA)feaList;
	pgea = geaList->list;
	while (cnt--) {
		unsigned n = pena->cbName + 1;
		PBYTE p2 = &pgea->szName[n];
		if ((unsigned)p2 & 3)
			p2 += 4 - ((unsigned)p2 & 3);
		pgea->cbName = pena->cbName;
		_fmemcpy(pgea->szName, pena->szName, n);
		pgea->oNextEntryOffset = cnt ? p2 - (PBYTE)pgea : 0;
		pgea = (pGEA) p2;
		pena = (pENA) ((PBYTE)pena + pena->oNextEntryOffset);
	}
	geaList->cbList = (PBYTE)pgea - (PBYTE)geaList;
#else
	pena = (pENA)feaList;
	pbyte = (PBYTE)geaList->list;
	while (cnt--) {
		unsigned n = pena->cbName;
		*pbyte++ = n++;
		_fmemcpy(pbyte, pena->szName, n);
		pbyte += n;
		pena = (pENA)&pena->szName[n];
	}
	geaList->cbList = pbyte - (PBYTE)geaList;
#endif

#if OS2 >= 2
	/*
	 * Initialize alignment gaps with 0.
	 */
	memset(&pea->ea, 0, length);
#endif

	/*
	 * Get EA name & value list to <feaList>.
	 */
	feaList->cbList = length;
#if OS2 >= 2
	eaop.fpGEA2List = geaList;
	eaop.fpFEA2List = feaList;
	if ( name && (rc = DosQueryPathInfo((PSZ)name, FIL_QUERYEASFROMLIST,
		(PBYTE)&eaop, sizeof(EAOPData))) != 0
	 || !name && (rc = DosQueryFileInfo(file,      FIL_QUERYEASFROMLIST,
		(PBYTE)&eaop, sizeof(EAOPData))) != 0)
		 	goto error;
#else
	eaop.fpGEAList = geaList;
	eaop.fpFEAList = feaList;
	if ( name && (rc = DosQPathInfo((PSZ)name, FIL_QUERYEASFROMLIST,
		(PBYTE)&eaop, sizeof(EAOPData), 0L)) != 0
	 || !name && (rc = DosQFileInfo(file,      FIL_QUERYEASFROMLIST,
		(PBYTE)&eaop, sizeof(EAOPData))) != 0)
		 	goto error;
#endif

	_ffree(geaList);

	return pea;

error:	errno = rc;
	if (geaList) _ffree(geaList);
	if (pea) _ffree(pea);
	return 0;

nomem:	errno = ENOMEM;
	if (geaList) _ffree(geaList);
	if (pea) _ffree(pea);
	return 0;
}

pEABuf
ea_load(char *name)
{
	return load_ea(name, 0);
}

pEABuf
ea_fload(unsigned file)
{
	return load_ea(0, file);
}

int
ea_store(pEABuf pea, char *name)
{
	EAOPData	eaop;
	int		rc;
	static GEAList	geal = { 1 };

#if AcceptZeroMagic
	if (!TestEAMagic(pea) && pea->magic != 0) {
#else
	if (!TestEAMagic(pea)) {
#endif
		errno = 255;	/* ERROR_EA_LIST_INCONSISTENT */
		return -1;
	}

#if OS2 >= 2
	eaop.fpGEA2List = &geal;	/* dummy, unused */
	eaop.fpFEA2List = &pea->ea;
	if ((rc = DosSetPathInfo((PSZ)name, FIL_QUERYEASIZE,
			(PBYTE)&eaop, sizeof(EAOPData), 0)) != 0) {
#else
	eaop.fpGEAList = &geal;		/* dummy, unused */
	eaop.fpFEAList = &pea->ea;
	if ((rc = DosSetPathInfo((PSZ)name, FIL_QUERYEASIZE,
			(PBYTE)&eaop, sizeof(EAOPData), 0, 0L)) != 0) {
#endif
		errno = rc;
		return -1;
	}
	return 0;
}

int
ea_fstore(pEABuf pea, unsigned file)
{
	EAOPData	eaop;
	int		rc;
	static GEAList	geal = { 1 };

#if OS2 >= 2
	eaop.fpGEA2List = &geal;	/* dummy, unused */
	eaop.fpFEA2List = &pea->ea;
#else
	eaop.fpGEAList = &geal;		/* dummy, unused */
	eaop.fpFEAList = &pea->ea;
#endif
	if ((rc = DosSetFileInfo(file, FIL_QUERYEASIZE,
			(PBYTE)&eaop, sizeof(EAOPData))) != 0) {
		errno = rc;
		return -1;
	}
	return 0;
}

pEABuf
ea_alloc(unsigned long size)
{
	pEABuf pea;
	if (size < sizeof(ULONG))
		size = sizeof(ULONG);
	if (size != (size_t)size)
		return 0;
#if OS2 >= 2
	if ((pea = _fmalloc((size_t)(sizeof(long) + size))) == 0)
		return 0;
	pea->magic = EAMagic2;
#else
	if ((pea = _fmalloc((size_t)size)) == 0)
		return 0;
#endif
	pea->ea.cbList = size;
	return pea;
}

void
ea_free(pEABuf pea)
{
	_ffree(pea);
}

#endif
