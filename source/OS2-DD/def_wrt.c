/*****************************************************************************
 * $Id: def_wrt.c,v 1.3 1992/07/24 11:36:10 ak Exp $
 *****************************************************************************
 * $Log: def_wrt.c,v $
 * Revision 1.3  1992/07/24  11:36:10  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
 *
 * Revision 1.2  1992/01/06  20:09:54  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1992/01/06  19:54:39  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:54:37  ak
 * Initial revision
 *
 *****************************************************************************/

#include "dd.h"

extern word	DrvWrite(ReqPtr rp, int vfy) { return ERROR+DONE+InvalidCommand; }
