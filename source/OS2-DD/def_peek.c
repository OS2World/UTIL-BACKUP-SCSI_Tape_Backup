/*****************************************************************************
 * $Id: def_peek.c,v 1.3 1992/07/24 11:35:49 ak Exp $
 *****************************************************************************
 * $Log: def_peek.c,v $
 * Revision 1.3  1992/07/24  11:35:49  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
 *
 * Revision 1.2  1992/01/06  20:09:49  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1992/01/06  19:54:34  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:54:32  ak
 * Initial revision
 *
 *****************************************************************************/

#include "dd.h"

extern word	DrvPeek (ReqPtr rp) { return ERROR+DONE+InvalidCommand; }
