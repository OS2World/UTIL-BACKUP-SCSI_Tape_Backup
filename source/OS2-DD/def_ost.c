/*****************************************************************************
 * $Id: def_ost.c,v 1.3 1992/07/24 11:35:39 ak Exp $
 *****************************************************************************
 * $Log: def_ost.c,v $
 * Revision 1.3  1992/07/24  11:35:39  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
 *
 * Revision 1.2  1992/01/06  20:09:47  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1992/01/06  19:54:31  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:54:30  ak
 * Initial revision
 *
 *****************************************************************************/

#include "dd.h"

extern word	DrvOutputStatus(void) { return ERROR+DONE+InvalidCommand; }
