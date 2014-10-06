/*****************************************************************************
 * $Id: def_clos.c,v 1.3 1992/07/24 11:34:30 ak Exp $
 *****************************************************************************
 * $Log: def_clos.c,v $
 * Revision 1.3  1992/07/24  11:34:30  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
 *
 * Revision 1.2  1992/01/06  20:09:29  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1992/01/06  19:54:12  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:54:11  ak
 * Initial revision
 *
 *****************************************************************************/

#include "dd.h"

extern word	DrvClose(ReqPtr rp) { return ERROR+DONE+InvalidCommand; }
