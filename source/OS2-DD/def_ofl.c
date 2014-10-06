/*****************************************************************************
 * $Id: def_ofl.c,v 1.3 1992/07/24 11:35:19 ak Exp $
 *****************************************************************************
 * $Log: def_ofl.c,v $
 * Revision 1.3  1992/07/24  11:35:19  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
 *
 * Revision 1.2  1992/01/06  20:09:42  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1992/01/06  19:54:26  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:54:24  ak
 * Initial revision
 *
 *****************************************************************************/

#include "dd.h"

extern word	DrvOutputFlush (void) { return ERROR+DONE+InvalidCommand; }
