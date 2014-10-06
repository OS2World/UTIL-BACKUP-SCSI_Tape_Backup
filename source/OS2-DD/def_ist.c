/*****************************************************************************
 * $Id: def_ist.c,v 1.3 1992/07/24 11:35:09 ak Exp $
 *****************************************************************************
 * $Log: def_ist.c,v $
 * Revision 1.3  1992/07/24  11:35:09  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
 *
 * Revision 1.2  1992/01/06  20:09:39  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1992/01/06  19:54:23  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:54:21  ak
 * Initial revision
 *
 *****************************************************************************/

#include "dd.h"

extern word	DrvInputStatus(void) { return ERROR+DONE+InvalidCommand; }
