/*****************************************************************************
 * $Id: def_ifl.c,v 1.3 1992/07/24 11:34:40 ak Exp $
 *****************************************************************************
 * $Log: def_ifl.c,v $
 * Revision 1.3  1992/07/24  11:34:40  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
 *
 * Revision 1.2  1992/01/06  20:09:31  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1992/01/06  19:54:15  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:54:13  ak
 * Initial revision
 *
 *****************************************************************************/

#include "dd.h"

extern word	DrvInputFlush(void) { return ERROR+DONE+InvalidCommand; }
