/*****************************************************************************
 * $Id: aspitape.h,v 2.1 1992/11/14 21:00:20 ak Exp $
 *****************************************************************************
 * $Log: aspitape.h,v $
 * Revision 2.1  1992/11/14  21:00:20  ak
 * OS/2 2.00.1 ASPI
 *
 * Revision 1.1.1.1  1992/01/06  20:16:21  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  20:16:20  ak
 * Initial revision
 *
 *****************************************************************************/

#include <dd.h>
#include "aspi.h"
#include "scsi.h"

SRB		srb;		/* ASPI request block */
paddr		paddrSRB;	/* physical address of srb */
int		waiting;	/* someone is waiting for srb */
int		senseValid;	/* SRB sense data valid (no of bytes) */
int		stickyEOF;	/* return EOF until reopened */

SRB		srb2;		/* ASPI request block for ABORT */

IDCEntry	aspiEntry;	/* ASPI entry point */
word		aspiHandle;	/* ASPI handle in init phase */

ReqPtr		reqpkt;		/* active request packet */

int		trace;		/* trace level */
dword		blocksize;	/* fixed blocksize, variable if 0 */
byte		adapter;	/* HA number */
byte		target;		/* tape target ID */

enum SenseMode	senseMode;	/* sense data mode */

extern void	aspi (SRB *);
