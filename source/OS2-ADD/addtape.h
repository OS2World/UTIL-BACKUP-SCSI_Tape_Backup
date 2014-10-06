/*****************************************************************************
 * $Id: addtape.h,v 1.2 1992/09/29 09:35:26 ak Exp $
 *****************************************************************************
 * $Log: addtape.h,v $
 * Revision 1.2  1992/09/29  09:35:26  ak
 * Must allow multiple DrvOpen calls. dup() seems to reopen the driver.
 *
 * Revision 1.1.1.1  1992/09/02  18:57:44  ak
 * Initial checkin of OS/2 ADD based SCSI tape device driver.
 *
 * Revision 1.1  1992/09/02  18:57:43  ak
 * Initial revision
 *
 *****************************************************************************/

#include <os2def.h>
#include <dd.h>
#include <tapedrvr.h>
#include "iorb.h"
#include "scsi.h"

#pragma ZTC align 1
#pragma pack(1)
/* pure magic - analyzed from OS2SCSI.DMD */
typedef struct Status {
	byte	unknown_00 [0x11-0x00];	/* 00 */
	word	sense_len;		/* 11 */
	farptr	sense_ptr;		/* 13 */
} Status;
#define SizeofStatus 23
#pragma ZTC align
#pragma pack()

DevClassTable _far *dctbl;			/* device class table */
byte		devtab_data [4096];		/* devtab data */
DEVICETABLE *	devtab;
byte		iorb1[MAX_IORB_SIZE];		/* iorb for SCSI command */
byte		iorb2[MAX_IORB_SIZE];		/* iorb for Abort */
paddr		paddr_iorb1;			/* phys addr of iorb1 */
Status		status;				/* ADD status */
byte		sense[32];			/* SCSI sense bytes */
SCATGATENTRY	sglist[1];			/* scatter/gather list */
paddr		paddr_sglist;
byte		cdb[12];			/* rw CDB */

UNITINFO *	unitinfo;			/* ptr into devtab */
void		(_far * callADD) (PIORBH);	/* ADD entry point */

ReqPtr		reqpkt;				/* active request packet */

int		trace;				/* trace level */
dword		blocksize;			/* fixed blocksize, variable if 0 */
int		open_count;			/* no of active DrvOpen's */

int		waiting;	/* someone is waiting for iorb */
int		senseMode;	/* sense data mode */
int		senseValid;	/* sense data valid */
int		stickyEOF;	/* return EOF until reopened */

extern word	startup (void);
