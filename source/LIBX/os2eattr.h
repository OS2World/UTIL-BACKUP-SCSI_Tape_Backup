#ifndef __OS2EATTR_H__
#define __OS2EATTR_H__

/*
 * $Id: os2eattr.h,v 1.10 1992/07/24 11:43:24 ak Exp $
 *
 * Extended Attribute Management for OS/2.
 *
 * A.Kaiser	Mai 91
 *
 * $Log: os2eattr.h,v $
 * Revision 1.10  1992/07/24  11:43:24  ak
 * 2.0 is incompatible with 1.3 -- added magic number
 * mechanism to allow checking of versions.
 *
 * Revision 1.8  1992/04/21  12:34:46  ak
 * EMX 0.8c.
 *
 * Revision 1.7  1992/03/05  20:27:36  ak
 * Bugfix.
 *
 * Revision 1.6  1992/02/26  21:23:16  ak
 * *** empty log message ***
 *
 * Revision 1.5  1992/02/26  20:54:20  ak
 * OS/2 2.0.
 *
 * Revision 1.4  1992/02/14  18:11:04  ak
 * *** empty log message ***
 *
 * Revision 1.3  1992/01/03  14:24:21  ak
 * $Header: h:/SRC.SSB/inc/os2eattr.h,v 1.10 1992/07/24 11:43:24 ak Exp $Id:
 *
 * Revision 1.2  1992/01/03  13:47:14  ak
 * Zortech fixes.
 *
 * Revision 1.1.1.1  1991/12/12  16:12:37  ak
 * Initial checkin of server source, modified to contain RCS IDs.
 *
 * Revision 1.1  1991/12/12  16:12:33  ak
 * Initial revision
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * OS/2 2.2: EA representation is different.
 * Added a ULONG magic number before the internal EA data.
 * Changed external data type from pFEAList to pEABuf.
 */

#if defined(__GNUC__)
  #ifndef _far
    #define _far
  #endif
#endif
#if OS2 >= 2
  #ifndef _far
    #define _far
  #endif
  /*
   * DosEnumAttrib:	DENAx = DENA2 = FEA2 mit cbValue != 0,
   *			die Values sind jedoch nicht(!) drin.
   */
  #define GEAData	struct _GEA2
  #define FEAData	struct _FEA2
  #define GEAList	struct _GEA2LIST
  #define FEAList	struct _FEA2LIST
  #define ENAData	struct _FEA2
  #define EAOPData	struct _EAOP2
#else
  #define GEAData	struct _GEA
  #define FEAData	struct _FEA
  #define GEAList	struct _GEALIST
  #define FEAList	struct _FEALIST
  #define ENAData	struct _DENA1
  #define EAOPData	struct _EAOP
#endif
typedef GEAData _far *	pGEA;
typedef GEAList _far *	pGEAList;
typedef FEAData _far *	pFEA;
typedef FEAList _far *	pFEAList;
typedef ENAData _far *	pENA;
typedef struct EABuf _far * pEABuf;

#if OS2 >= 2

# define SizeFEA(p)   ((p)->oNextEntryOffset)
# define NextFEA(p)   ((pFEA)((unsigned char _far *)p + (p)->oNextEntryOffset))
# define szNameFEA(p) ((p)->szName)
# define aValueFEA(p) ((p)->szName + (p)->cbName+1)
# define EndOfEA(p)   ((unsigned char _far *)(p) + (p)->cbList)
# define WalkFEA(p,q) if (q->cbList) { p = q->list; do {
# define WalkFEAEnd(p,q) } while (p->oNextEntryOffset ? (p = (pFEA)((unsigned char _far *)p + p->oNextEntryOffset)) : 0); }

#else

  typedef struct {
	unsigned char	fEA;
	unsigned char	cbName;
	unsigned short	cbValue;
	unsigned char	szName[1];
  } _far *pFEAx;

# define SizeFEA(p)   ((unsigned char _far *)&((pFEAx)(p))->szName[((pFEAx)(p))->cbName+1+((pFEAx)(p))->cbValue] - (unsigned char _far *)(p))
# define NextFEA(p)   ((pFEA)&((pFEAx)(p))->szName[((pFEAx)(p))->cbName+1+((pFEAx)(p))->cbValue])
# define szNameFEA(p) (((pFEAx)(p))->szName)
# define aValueFEA(p) (((pFEAx)(p))->szName + ((pFEAx)(p))->cbName+1)
# define EndOfEA(p)   ((unsigned char _far *)(p) + (p)->cbList)
# define WalkFEA(p,q) for (p = q->list; p < (pFEA) EndOfEA(q); p = NextFEA(p)) {
# define WalkFEAEnd(p,q) }

#endif

/*
 * The macro EALength accessed cbList without having
 * to include the whole os2.h headers.
 *
 * Use TestEAMagic to check if EAs were written with
 * the same EA buffer layout. 1.3 EAs have a positive
 * count at the position where 2.0 EAs have a negative
 * magic number.
 */
#if OS2 >= 2
# define EAMagic2	-2
# define ptrFEAList(p)	((pFEAList)((long *)(p) + 1))
# define EALength(p)	(sizeof(long) + ((long *)(p))[1])
# define TestEAMagic(p)	(((long *)(p))[0] == EAMagic2)
#else
# define ptrFEAList(p)	((pFEAList)(p))
# define EALength(p)	(*(long _far *)(p))
# define TestEAMagic(p)	(EALength(p) >= 0)
#endif

	/* Load EABuf from file. */
	/* Return 0 if file error (->errno) or not enough memory. */
extern pEABuf	ea_load (char *);
extern pEABuf	ea_fload (unsigned);

	/* Store EAList into file. */
	/* Return -1 if file error (->errno). */
extern int	ea_store (pEABuf, char *);
extern int	ea_fstore (pEABuf, unsigned);

	/* Alloc/free EA data. */
extern pEABuf	ea_alloc (unsigned long size);
extern void	ea_free (pEABuf);

	/* UNTESTED: */

	/* Get EA value - return 0x100 + flag byte, -1 if not enough space */
extern int	ea_get (pEABuf, char *, void *, int *);

#if OS2 < 2
	/* Set EA value - pEABuf reallocated if necessary - 0 if no memory */
extern pEABuf	ea_set (pEABuf, char *, void *, unsigned, unsigned /*flag*/);

	/* Remove EA value - return 0 if out of memory */
extern pEABuf	ea_remove (pEABuf, char *);
#endif

#ifdef __cplusplus
}
#endif

#endif

