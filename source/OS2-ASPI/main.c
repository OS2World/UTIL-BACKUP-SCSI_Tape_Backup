/*****************************************************************************
 * $Id: main.c,v 2.1 1992/11/14 21:00:20 ak Exp $
 *****************************************************************************
 * $Log: main.c,v $
 * Revision 2.1  1992/11/14  21:00:20  ak
 * OS/2 2.00.1 ASPI
 *
 * Revision 1.1.1.1  1992/01/06  20:16:27  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  20:16:25  ak
 * Initial revision
 *
 *****************************************************************************/

static char _far rcsid[] = "$Id: main.c,v 2.1 1992/11/14 21:00:20 ak Exp $";

#include "aspitape.h"
#include "tapedrvr.h"
#include <libx.h>
#include <ztc.h>
#define INCL_DOSDEVICES
#include <os2.h>

typedef void (_far *FcnPtr) (word, SRB _far *);

extern void _far postEntry (unsigned, unsigned long);

#define min(a,b) ((a) < (b) ? (a) : (b))

void
aspi(SRB *p)
{
	if (aspiHandle)
		DosDevIOCtl((PBYTE)0, (PBYTE)p, 0x40, 0x80, aspiHandle);
	else if (inProtMode())
		(*(FcnPtr)aspiEntry.prot)(aspiEntry.protDS, p);
	else
		(*(FcnPtr)aspiEntry.real)(aspiEntry.realDS, p);
}

void
post(dword paddrSRB)
{
	PUTS(4, "	POST\n");
	Run((dword)reqpkt);
}

/*
 * Issue Abort SRB command.
 */
static void
abort(dword paddr)
{
	if (srb2.status == SRB_Busy)
		return;
	srb2.cmd = SRB_Abort;
	srb2.ha_num = adapter;
	srb2.flags = 0;
	srb2.u.abt.phys_addr_srb = paddr;
	aspi(&srb2);
}

/*
 * Call ASPI. Wait until done.
 */
void
aspiWait(SRB *p)
{
	srb.flags |= SRB_Post;
	srb.u.cmd.realpost = Pointer(header.realCS, Offset((dword)postEntry));
	srb.u.cmd.realDS = header.realDS;
	srb.u.cmd.protpost = Pointer(header.protCS, Offset((dword)postEntry));
	srb.u.cmd.protDS = header.protDS;
	aspi(p);
	int_off();
	while (p->status == SRB_Busy || p->status == SRB_BusyPost) {
		PUTS(4, "	Waiting for POST\n");
		if (Block((dword)reqpkt, Infinite, 1)) {
			PUTS(1, "Abort\n");
			abort(paddrSRB);
			continue;
		}
		int_off();
	}
	int_on();
}

/*
 * Get SRB. Wait until available.
 * -> errcode if interrupted.
 */
word
getSRB(ReqPtr p)
{
	int_off();
	while (reqpkt) {
		PUTS(4, "	Waiting for SRB\n");
		++waiting;
		if (Block(paddrSRB, Infinite, 1))
			return ERROR+DONE+Interrupted;
		int_off();
	}
	int_on();
	reqpkt = p;
	return 0;
}

/*
 * Release SRB.
 * Wakeup threads waiting.
 */
void
relSRB(void)
{
	reqpkt = 0;
	if (waiting) {
		waiting = 0;
		Run(paddrSRB);
	}
}

#ifdef DEBUG

static void
dump(byte _far *p, int n)
{
	int i;

	for (i = 0; i < n; ++i) {
		putc(' ');
		putx(p[i]);
	}
}

static void
status(SRB *p)
{
	puts("	ASPI status: ");
	putx(p->status);
	puts(", HA status: ");
	putx(p->u.cmd.ha_status);
	puts(", target status: ");
	putx(p->u.cmd.target_status);
	if (p->u.cmd.target_status == CheckStatus) {
		puts(", sense data:\n\t");
		dump(p->u.cmd.cdb_st + p->u.cmd.cdb_len,
			min(p->u.cmd.sense_len, 16));
	}
	putc('\n');
}

#endif

static word
rw(ReqPtr p, byte flags)
{
	word rc;
	dword count, nb;

	count = p->u.rw.count;
	p->u.rw.count = 0;

	if (stickyEOF) {
		PUTS(4, "	still EOF\n");
		return DONE;
	}

	if (blocksize) {
		dword rem;
		switch (blocksize) {
		case 512:
			nb = count >> 9;
			rem = count & 0x1FF;
			break;
		case 1024:
			nb = count >> 10;
			rem = count & 0x3FF;
			break;
		default:
			nb = count / blocksize;
			rem = count % blocksize;
		}
		if (rem)
			return ERROR+DONE+InvalidParam;
	} else
		nb = count;

	if ((rc = getSRB(p)) != 0)
		return rc;

	srb.cmd = SRB_Command;
	srb.ha_num = adapter;
	srb.flags = flags;
	srb.u.cmd.target = target;
	srb.u.cmd.lun = 0;
	srb.u.cmd.data_len = count;
	srb.u.cmd.sense_len = 20;
	srb.u.cmd.data_ptr = p->u.rw.addr;
	srb.u.cmd.phys_addr_srb = paddrSRB;
	srb.u.cmd.cdb_len = 6;
	srb.u.cmd.cdb_st[0] = flags & SRB_Read ? SeqRead : SeqWrite;
	srb.u.cmd.cdb_st[1] = blocksize ? 1 : 0;
	srb.u.cmd.cdb_st[2] = nb >> 16;
	srb.u.cmd.cdb_st[3] = nb >> 8;
	srb.u.cmd.cdb_st[4] = nb;
	srb.u.cmd.cdb_st[5] = 0x00;

#ifdef DEBUG
	if (trace >= 3) {
		puts("	R/W CDB:");
		dump(srb.u.cmd.cdb_st, 6);
		putc('\n');
	}
#endif

	aspiWait(&srb);

#ifdef DEBUG
	if (trace >= 3)
		status(&srb);
#endif

	relSRB();

	switch (srb.status) {
	case SRB_Done:
		p->u.rw.count = count;
		return DONE;
	case SRB_Aborted:
		return ERROR+DONE+Interrupted;
	case SRB_Error:
		break;
	case SRB_InvalidHA:
	case SRB_BadDevice:
		return ERROR+DONE+DeviceNotReady;
	default:
		return ERROR+DONE+GeneralFailure;
	}

	switch (srb.u.cmd.ha_status) {
	case 0x00:	/* No host adapter detected error */
		break;
	case 0x11:	/* Selection timeout */
		return ERROR+DONE+DeviceNotReady;
	default:
		return ERROR+DONE+GeneralFailure;
	}

	switch (srb.u.cmd.target_status) {
	case SRB_NoStatus:
		p->u.rw.count = count;
		return DONE;
	case SRB_CheckStatus:
		senseValid = 20;
		break;
	case SRB_LUN_Busy:
	case SRB_Reserved:
		return ERROR+DONE+DeviceNotReady;
	default:
		return ERROR+DONE+GeneralFailure;
	}

#ifdef DEBUG
	if (trace && trace < 3)
		status(&srb);
#endif

	if (srb.u.cmd.cdb_st[6+2] & VADD) {
		dword info = 0;
		for (rc = 6+3; rc <= 6+6; ++rc) {
			info <<= 8;
			info |= srb.u.cmd.cdb_st[rc];
		}
		switch (blocksize) {
		case 512:
			info <<= 9;
			break;
		case 1024:
			info <<= 10;
			break;
		default:
			info *= blocksize;
		case 0:	;
		}
		p->u.rw.count = count - (info << 9);

		PUTS(4, "	result count=");
		PUTD(4, p->u.rw.count);
		PUTC(4, '\n');
	} else
		p->u.rw.count = count;

	if (srb.u.cmd.cdb_st[6+2] & (FM|EOM)) {
		PUTS(4, "	EOF\n");
		stickyEOF = 1;
	}
	if (senseMode == Sensekey) {
		switch (srb.u.cmd.cdb_st[6+2] & 0x0F) {
		case NoSense:
		case BlankCheck:
			return DONE;
		case RecoveredData:
			return ERROR+DONE+CRCError;
		case NotReady:
		case Hardware:
		case UnitAttention:
			return ERROR+DONE+DeviceNotReady;
		case MediaError:
			return ERROR+DONE+ReadFault;
		case WriteProtected:
			return ERROR+DONE+WriteProtect;
		case MediaOverflow:
			return ERROR+DONE+NoPaper;
		case AbortedCommand:
			return ERROR+DONE+Interrupted;
		default:
			return ERROR+DONE+GeneralFailure;
		}
	}
	if ((srb.u.cmd.cdb_st[6+0] & ~0x81) != 0x70)
		rc = srb.u.cmd.cdb_st[6+0];
	else
		switch (senseMode) {
		case TDC3600:
			rc = srb.u.cmd.cdb_st[6+14];
			break;
		default:
			rc = srb.u.cmd.cdb_st[6+12];
		}
	switch (senseMode) {
	case TDC3600:
		switch (rc) {
		case 0x00:		/* ok */
			return DONE;
		case 0x02:		/* hardware error */
		case 0x04:		/* not ready */
		case 0x09:		/* not loaded */
			return ERROR+DONE+DeviceNotReady;
		case 0x0A:		/* insufficient capacity */
			return ERROR+DONE+NoPaper;
		case 0x11:		/* uncorrectable data error */
			return ERROR+DONE+ReadFault;
		case 0x14:		/* empty cartridge */
		case 0x1C:		/* file mark detected */
		case 0x34:		/* read EOM */
			PUTS(4, "	EOF\n");
			stickyEOF = 1;
			return DONE;
		case 0x17:		/* write protected */
			return ERROR+DONE+WriteProtect;
		case 0x19:		/* bad block found */
			return ERROR+DONE+WriteFault;
		case 0x30:
			return ERROR+DONE+ChangeDisk;
		case 0x33:		/* append error */
			return ERROR+DONE+SeekError;
		default:
			return ERROR+DONE+GeneralFailure;
		}
	case SCSI2:
		switch (rc) {
		case 0x00:	/* end-of-data */
		case 0x14:	/* end-of-data not found / filemark */
			PUTS(4, "	EOF\n");
			stickyEOF = 1;
			return DONE;
		case 0x30:	/* cannot read medium */
			return ERROR+DONE+UnknownMedia;
		case 0x52:	/* cartridge fault */
		case 0x05:	/* selection failure */
		case 0x40:	/* device failure */
		case 0x04:	/* not ready */
		case 0x25:	/* LUN not supported */
		case 0x3A:	/* medium not present */
			return ERROR+DONE+DeviceNotReady;
		case 0x03:	/* excessive write errors */
		case 0x0C:	/* write error */
			return ERROR+DONE+WriteFault;
		case 0x21:	/* block address out of range */
		case 0x15:	/* positioning error */
		case 0x50:	/* write append error */
			return ERROR+DONE+SeekError;
		case 0x53:	/* media load failed */
		case 0x29:	/* power-up or reset */
			return ERROR+DONE+ChangeDisk;
		case 0x31:	/* media format corrupted */
		case 0x11:	/* read retries exhausted */
			return ERROR+DONE+ReadFault;
		case 0x17:	/* recovered error */
			return ERROR+DONE+CRCError;
		case 0x27:	/* write protected */
			return ERROR+DONE+WriteProtect;
		default:
			return ERROR+DONE+GeneralFailure;
		}
	default:
		return ERROR+DONE+GeneralFailure;
	}
}

word
DrvOpen(ReqPtr p)
{
	if (!inProtMode())
		return ERROR+DONE+InvalidCommand;
	senseValid = 0;
	stickyEOF = 0;
	return DONE;
}

word
DrvClose(ReqPtr p)
{
	return DONE;
}

word
DrvRead(ReqPtr p)
{
	return rw(p, SRB_Read);
}

word
DrvWrite(ReqPtr p, int vfy)
{
	return rw(p, SRB_Write);
}

word
DrvIOCtl(ReqPtr p)
{
	handle dataLock;
	byte flags;
	word rc;

	if (p->u.ioc.cat < 0x80)
		return ERROR+DONE+InvalidCommand;
	if (p->u.ioc.cat != 0x80)
		return ERROR+DONE+TapeInvalidFcn;

	stickyEOF = 0;

	switch (p->u.ioc.fcn) {

	case IOCtlBusReset:
	case IOCtlDevReset:
		PUTS(3, "IOCtl Reset\n");

		senseValid = 0;

		if ((rc = getSRB(p)) != 0)
			return rc;
		srb.u.res.target = target;
		srb.u.res.lun = 0;
		aspiWait(&srb);
		relSRB();

		if (srb.status == SRB_Done)
			return DONE;
		if (srb.status & 0x80)
			return ERROR+DONE+ErrASPIDriver1+(srb.status & 0x1F);
		return ERROR+DONE+ErrASPIDriver2+(srb.status & 0x1F);

	case IOCtlLevel:
		PUTS(3, "IOCtl Level\n");
		if (VerifyAccess(p->u.ioc.data, 2, VerifyWrite))
			return ERROR+DONE+InvalidParam;
		((byte _far *)p->u.ioc.data)[0] = ASPIdriver;
		((byte _far *)p->u.ioc.data)[1] = senseMode;
		return DONE;

	case IOCtlTrace:
		PUTS(3, "IOCtl Trace\n");
		if (p->u.ioc.dlen >= 1) {
			if (VerifyAccess(p->u.ioc.data, 1, VerifyWrite))
				return ERROR+DONE+InvalidParam;
			*(byte _far *)p->u.ioc.data = trace;
		}
		if (p->u.ioc.plen >= 1) {
			if (VerifyAccess(p->u.ioc.parm, 1, VerifyRead))
				return ERROR+DONE+InvalidParam;
			trace = *(byte _far *)p->u.ioc.parm;
		}
		return DONE;

	case IOCtlBlocksize:
		PUTS(3, "IOCtl Blocksize\n");
		if (p->u.ioc.dlen >= 4) {
			if (VerifyAccess(p->u.ioc.data, 4, VerifyWrite))
				return ERROR+DONE+InvalidParam;
			*(dword _far *)p->u.ioc.data = blocksize;
		}
		if (p->u.ioc.plen >= 4) {
			if (VerifyAccess(p->u.ioc.parm, 4, VerifyRead))
				return ERROR+DONE+InvalidParam;
			blocksize = *(dword _far *)p->u.ioc.data;
		}
		return DONE;

	case IOCtlSlow:
	case IOCtlFast:
		PUTS(3, "IOCtl Unchecked Command\n");
		flags = 0;
		goto unchecked;
	case IOCtlRead+IOCtlSlow:
	case IOCtlRead+IOCtlFast:
		PUTS(3, "IOCtl Read\n");
	read:	flags = SRB_Read;
	unchecked:
		if (VerifyAccess(p->u.ioc.parm, p->u.ioc.plen, VerifyRead))
			return ERROR+DONE+TapeInvalidParm;
		if (p->u.ioc.dlen
		 && VerifyAccess(p->u.ioc.data, p->u.ioc.dlen, VerifyWrite))
			return ERROR+DONE+TapeInvalidData;
		goto checked;
	case IOCtlWrite+IOCtlSlow:
	case IOCtlWrite+IOCtlFast:
		PUTS(3, "IOCtl Write\n");
		flags = SRB_Write;
		if (VerifyAccess(p->u.ioc.parm, p->u.ioc.plen, VerifyRead))
			return ERROR+DONE+TapeInvalidParm;
		if (p->u.ioc.dlen
		 && VerifyAccess(p->u.ioc.data, p->u.ioc.dlen, VerifyRead))
			return ERROR+DONE+TapeInvalidData;
	checked:
		if (p->u.ioc.plen < 6 || p->u.ioc.plen > 12)
			return ERROR+DONE+TapeInvalidParm;
		if ((rc = getSRB(p)) != 0)
			return rc;
		dataLock = Lock(Segment(p->u.ioc.data), LockShort);
		senseValid = 0;
		
		srb.cmd = SRB_Command;
		srb.status = 0;
		srb.ha_num = adapter;
		srb.flags = flags;
		srb.u.cmd.target = target;
		srb.u.cmd.lun = 0;
		srb.u.cmd.data_len = p->u.ioc.dlen;
#if SenseLength
		srb.u.cmd.sense_len = SenseLength;
#else
		srb.u.cmd.sense_len = MaxCDBStatus - p->u.ioc.plen;
#endif
		if (p->u.ioc.dlen)
			srb.u.cmd.data_ptr = VirtToPhys(p->u.ioc.data);
		srb.u.cmd.phys_addr_srb = paddrSRB;
		srb.u.cmd.cdb_len = p->u.ioc.plen;
		_fmemcpy(srb.u.cmd.cdb_st, p->u.ioc.parm, p->u.ioc.plen);

#ifdef DEBUG
		if (trace >= 3) {
			puts("	CDB:");
			dump(srb.u.cmd.cdb_st, srb.u.cmd.cdb_len);
			putc('\n');
		}
#endif

		aspiWait(&srb);

#ifdef DEBUG
		if (trace >= 3)
			status(&srb);
#endif

		relSRB();
		Unlock(dataLock);

		if (srb.u.cmd.target_status == CheckStatus)
			senseValid = srb.u.cmd.sense_len;

		if (srb.status != SRB_Done && srb.status != SRB_Error)
			return ERROR+DONE
				+(srb.status & 0x80 ? ErrASPIDriver2 : ErrASPIDriver1)
				+(srb.status & 0x1F);
		if (srb.u.cmd.ha_status)
			return ERROR+DONE+ErrHostAdapter
				+(srb.u.cmd.ha_status & 0x1F);
		if (srb.u.cmd.target_status)
			return ERROR+DONE+ErrTargetStatus
				+(srb.u.cmd.target_status & 0x1F);
		return DONE;

	case IOCtlSense:
		PUTS(3, "IOCtl Sense\n");
		if (!senseValid) {
			PUTS(3, "	No sense data\n");
			return ERROR+DONE+TapeNoSenseData;
		}
		PUTS(3, "	Valid sense data\n");
		if (p->u.ioc.dlen > senseValid)
			return ERROR+DONE+TapeInvalidData;
		_fmemcpy((farptr)p->u.ioc.data,
			srb.u.cmd.cdb_st + srb.u.cmd.cdb_len,
			p->u.ioc.dlen);
		return DONE;

	default:
		return ERROR+DONE+TapeInvalidFcn;
	}
}
