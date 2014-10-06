/*****************************************************************************
 * $Id: def_open.c,v 1.3 1992/07/24 11:35:29 ak Exp $
 *****************************************************************************
 * $Log: def_open.c,v $
 * Revision 1.3  1992/07/24  11:35:29  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
 *
 * Revision 1.2  1992/01/06  20:09:44  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1992/01/06  19:54:28  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:54:27  ak
 * Initial revision
 *
 *****************************************************************************/

#include "dd.h"

extern word	DrvOpen (ReqPtr rp) { return ERROR+DONE+InvalidCommand; }
