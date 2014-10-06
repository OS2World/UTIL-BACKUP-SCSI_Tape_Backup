/*****************************************************************************
 * $Id: dd_defs.h,v 1.2 1992/07/24 11:34:21 ak Exp $
 *****************************************************************************
 * $Log: dd_defs.h,v $
 * Revision 1.2  1992/07/24  11:34:21  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
 *
 * Revision 1.1.1.1  1992/01/06  19:54:06  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:54:05  ak
 * Initial revision
 *
 *****************************************************************************/

extern farfcn		devhlp;		/* devhlp vector */
extern byte		end_data;	/* end of resident data */
extern byte		_far end_code;	/* end of resident code */

extern void	strategy (ReqPtr);	/* C strategy routine */
extern void	initialize (ReqPtr);	/* initialization routine */
extern word	ctors (void);		/* call constructors */
