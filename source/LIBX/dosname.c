/*****************************************************************************
 * $Id: dosname.c,v 1.4 1993/01/16 17:05:58 ak Exp $
 *****************************************************************************
 * Unix/HPFS filename translation for FAT file systems
 * Author: Kai Uwe Rommel
 *
 * $Log: dosname.c,v $
 * Revision 1.4  1993/01/16  17:05:58  ak
 * *** empty log message ***
 *
 * Revision 1.3  1992/10/12  12:16:32  ak
 * *** empty log message ***
 *
 * Revision 1.2  1992/09/21  15:20:30  ak
 * *** empty log message ***
 *
 * Revision 1.1  1992/09/14  12:24:08  ak
 * Initial revision
 *
 *****************************************************************************/

#if defined(OS2) && OS2 >= 2

static char *rcsid = "$Id: dosname.c,v 1.4 1993/01/16 17:05:58 ak Exp $";



#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#include <os2.h>


void ChangeNameForFAT(char *name)
{
  char *src, *dst, *next, *ptr, *dot, *start;
  static char invalid[] = ":;,=+\"[]<>| \t";

  if ( isalpha(name[0]) && (name[1] == ':') )
    start = name + 2;
  else
    start = name;

  src = dst = start;
  if ( (*src == '/') || (*src == '\\') )
    src++, dst++;

  while ( *src )
  {
    for ( next = src; *next && (*next != '/') && (*next != '\\'); next++ );

    for ( ptr = src, dot = NULL; ptr < next; ptr++ )
      if ( *ptr == '.' )
      {
        dot = ptr; /* remember last dot */
        *ptr = '_';
      }

    if ( dot == NULL )
      for ( ptr = src; ptr < next; ptr++ )
        if ( *ptr == '_' )
          dot = ptr; /* remember last _ as if it were a dot */

    if ( dot && (dot > src) &&
         ((next - dot <= 4) ||
          ((next - src > 8) && (dot - src > 3))) )
    {
      if ( dot )
        *dot = '.';

      for ( ptr = src; (ptr < dot) && ((ptr - src) < 8); ptr++ )
        *dst++ = *ptr;

      for ( ptr = dot; (ptr < next) && ((ptr - dot) < 4); ptr++ )
        *dst++ = *ptr;
    }
    else
    {
      if ( dot && (next - src == 1) )
        *dot = '.';           /* special case: "." as a path component */

      for ( ptr = src; (ptr < next) && ((ptr - src) < 8); ptr++ )
        *dst++ = *ptr;
    }

    *dst++ = *next; /* either '/' or 0 */

    if ( *next )
    {
      src = next + 1;

      if ( *src == 0 ) /* handle trailing '/' on dirs ! */
        *dst = 0;
    }
    else
      break;
  }

  for ( src = start; *src != 0; ++src )
    if ( strchr(invalid, *src) != NULL )
        *src = '_';
}


int IsFileNameValid(char *name)
{
  HFILE hf;
  ULONG uAction;

  switch( DosOpen(name, &hf, &uAction, 0, 0, FILE_OPEN,
                  OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE, 0) )
  {
  case ERROR_INVALID_NAME:
  case ERROR_FILENAME_EXCED_RANGE:
    return FALSE;
  case NO_ERROR:
    DosClose(hf);
  default:
    return TRUE;
  }
}


typedef struct
{
  ULONG cbList;               /* length of value + 22 */
  ULONG oNext;
  BYTE fEA;                   /* 0 */
  BYTE cbName;                /* length of ".LONGNAME" = 9 */
  USHORT cbValue;             /* length of value + 4 */
  BYTE szName[10];            /* ".LONGNAME" */
  USHORT eaType;              /* 0xFFFD for length-preceded ASCII */
  USHORT eaSize;              /* length of value */
  BYTE szValue[CCHMAXPATH];
}
FEALST;

typedef struct
{
  ULONG cbList;
  ULONG oNext;
  BYTE cbName;
  BYTE szName[10];            /* ".LONGNAME" */
}
GEALST;


int SetLongNameEA(char *name, char *longname)
{
  EAOP eaop;
  FEALST fealst;

  eaop.fpFEAList = (PFEALIST) &fealst;
  eaop.fpGEAList = NULL;
  eaop.oError = 0;

  strcpy(fealst.szName, ".LONGNAME");
  strcpy(fealst.szValue, longname);

  fealst.cbList  = sizeof(fealst) - CCHMAXPATH + strlen(fealst.szValue);
  fealst.cbName  = (BYTE) strlen(fealst.szName);
  fealst.cbValue = sizeof(USHORT) * 2 + strlen(fealst.szValue);

  fealst.oNext   = 0;
  fealst.fEA     = 0;
  fealst.eaType  = 0xFFFD;
  fealst.eaSize  = strlen(fealst.szValue);

  return DosSetPathInfo(name, FIL_QUERYEASIZE, (PBYTE) &eaop, sizeof(eaop), 0);
}


char *GetLongNameEA(char *name)
{
  EAOP eaop;
  GEALST gealst;
  FEALST fealst;

  eaop.fpGEAList = (PGEALIST) &gealst;
  eaop.fpFEAList = (PFEALIST) &fealst;
  eaop.oError = 0;

  strcpy(gealst.szName, ".LONGNAME");
  gealst.cbName  = (BYTE) strlen(gealst.szName);

  gealst.cbList  = sizeof(gealst);
  fealst.cbList  = sizeof(fealst);

#if OS2 >= 2
  if ( DosQueryPathInfo(name, FIL_QUERYEASFROMLIST, (PBYTE) &eaop, sizeof(eaop)) )
#else
  if ( DosQPathInfo(name, FIL_QUERYEASFROMLIST, (PBYTE) &eaop, sizeof(eaop)) )
#endif
    return NULL;

  if ( fealst.cbValue > 4 && fealst.eaType == 0xFFFD )
  {
    fealst.szValue[fealst.eaSize] = 0;
    return fealst.szValue;
  }

  return NULL;
}


char *GetLongPathEA(char *name)
{
  static char nbuf[CCHMAXPATH + 1];
  char *comp, *next, *ea, sep;
  BOOL bFound = FALSE;

  nbuf[0] = 0;
  next = name;

  while ( *next )
  {
    comp = next;

    while ( *next != '\\' && *next != '/' && *next != 0 )
      next++;

    sep = *next;
    *next = 0;

    ea = GetLongNameEA(name);
    strcat(nbuf, ea ? ea : comp);
    bFound = bFound || (ea != NULL);

    *next = sep;

    if ( *next )
    {
      strcat(nbuf, "\\");
      next++;
    }
  }

  return nbuf[0] && bFound ? nbuf : NULL;
}

#endif
