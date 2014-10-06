/*****************************************************************************
 * $Id: def_read.c,v 1.3 1992/07/24 11:35:59 ak Exp $
 *****************************************************************************
 * $Log: def_read.c,v $
 * Revision 1.3  1992/07/24  11:35:59  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
 *
 * Revision 1.2  1992/01/06  20:09:52  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1992/01/06  19:54:36  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:54:35  ak
 * Initial revision
 *
 *****************************************************************************/

#include "dd.h"

extern word	DrvRead (ReqPtr rp) { return ERROR+DONE+InvalidCommand; }
