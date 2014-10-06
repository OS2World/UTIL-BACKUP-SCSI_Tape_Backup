/*****************************************************************************
 * $Id: def_init.c,v 1.3 1992/07/24 11:34:49 ak Exp $
 *****************************************************************************
 * $Log: def_init.c,v $
 * Revision 1.3  1992/07/24  11:34:49  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
 *
 * Revision 1.2  1992/01/06  20:09:34  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1992/01/06  19:54:17  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:54:16  ak
 * Initial revision
 *
 *****************************************************************************/

#include "dd.h"

extern word	DrvInit(char _far *line) { return ERROR+DONE+InvalidCommand; }
