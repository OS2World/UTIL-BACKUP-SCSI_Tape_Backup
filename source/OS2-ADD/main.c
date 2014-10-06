/*****************************************************************************
 * $Id: main.c,v 1.4 1992/12/02 23:58:56 ak Exp $
 *****************************************************************************
 * $Log: main.c,v $
 * Revision 1.4  1992/12/02  23:58:56  ak
 * Display trace info when unit is locked by other driver.
 *
 * Revision 1.3  1992/09/29  09:35:14  ak
 * Must allow multiple DrvOpen calls. dup() seems to reopen the driver.
 *
 * Revision 1.2  1992/09/26  19:29:12  ak
 * IOCtlTrace fixed.
 *
 * Revision 1.1.1.1  1992/09/02  18:58:27  ak
 * Initial checkin of OS/2 ADD based SCSI tape device driver.
 *
 * Revision 1.1  1992/09/02  18:58:25  ak
 * Initial revision
 *
 *****************************************************************************/

static char _far rcsid[] = "$Id: main.c,v 1.4 1992/12/02 23:58:56 ak Exp $";

#include "addtape.h"
#include "tapedrvr.h"
#ifdef __ZTC__
#include <ztc.h>
#else
#include <memory.h>
#endif
#include <string.h>

#define packError(x) ((x) >> 4 & 0xF0 | (x) & 0x0F)

farptr _far _loadds
post(farptr iorb)
{
	PUTS(4, "	POST\n");
	Run((dword)reqpkt);
	return iorb;
}

/*
 * Abort current operation.
 */
void
abort(void)
{
	IORB_DEVICE_CONTROL *iorb = (IORB_DEVICE_CONTROL *)iorb2;
	if (!(iorb->iorbh.Status & IORB_DONE))
		return;
	iorb->iorbh.Length = sizeof(IORB_DEVICE_CONTROL);
	iorb->iorbh.UnitHandle = unitinfo->UnitHandle;
	iorb->iorbh.CommandCode = IOCC_DEVICE_CONTROL;
	iorb->iorbh.CommandModifier = IOCM_ABORT;
	iorb->iorbh.RequestControl = IORB_DISABLE_RETRY;
	iorb->iorbh.Status = 0;
	iorb->iorbh.Timeout = -1;
	iorb->Flags = 0;
	callADD(&iorb->iorbh);
}

/*
 * Call ADD. Wait until done.
 */
void
callADDwait(IORBH *iorb)
{
	iorb->RequestControl |= IORB_ASYNC_POST;
	iorb->NotifyAddress = post;
	callADD(iorb);
	int_off();
	while (!(iorb->Status & IORB_DONE)) {
		PUTS(4, "	Waiting for POST\n");
		if (Block((dword)reqpkt, Infinite, 1)) {
			PUTS(1, "Abort\n");
			abort();
			continue;
		}
		int_off();
	}
	int_on();
}

/*
 * Get IORB. Wait until available.
 * -> errcode if interrupted.
 */
word
getIORB(ReqPtr p)
{
	int_off();
	while (reqpkt) {
		PUTS(4, "	Waiting for IORB\n");
		++waiting;
		if (Block(paddr_iorb1, Infinite, 1))
			return ERROR+DONE+Interrupted;
		int_off();
	}
	int_on();
	reqpkt = p;
	return 0;
}

/*
 * Release IORB.
 * Wakeup threads waiting.
 */
void
relIORB(void)
{
	reqpkt = 0;
	if (waiting) {
		waiting = 0;
		Run(paddr_iorb1);
	}
}

#ifdef DEBUG

void
dump(byte _far *p, int n)
{
	int i;

	for (i = 0; i < n; ++i) {
		putc(' ');
		putx(p[i]);
	}
}

void
result(IORBH *iorb)
{
	puts("\tIORB Status ");
	putx(iorb->Status);
	puts(", ErrorCode ");
	putx(iorb->ErrorCode);
	if (iorb->Status & IORB_STATUSBLOCK_AVAIL) {
		puts("\n\t  status:");
		dump((farptr)&status, SizeofStatus);
		if (status.sense_len && status.sense_ptr) {
			puts("\n\t  sense:");
			dump(status.sense_ptr, 18);
		}
	}
	putc('\n');
}

#endif

word
mapIORBerror(word errcode)
{
	switch (errcode & ~IOERR_RETRY) {
	case IOERR_CMD_NOT_SUPPORTED:
	case IOERR_CMD_SYNTAX:
		return ERROR+DONE+InvalidCommand;
	case IOERR_CMD_ABORTED:
		return ERROR+DONE+Interrupted;
	case IOERR_UNIT_NOT_ALLOCATED:
	case IOERR_UNIT_NOT_READY:
	case IOERR_UNIT_PWR_OFF:
	case IOERR_ADAPTER_TIMEOUT:
	case IOERR_ADAPTER_DEVICE_TIMEOUT:
	case IOERR_MEDIA_NOT_PRESENT:
		return ERROR+DONE+DeviceNotReady;
	case IOERR_RBA_LIMIT:
		return ERROR+DONE+WriteFault;
	case IOERR_RBA_CRC_ERROR:
		return ERROR+DONE+CRCError;
	case IOERR_MEDIA_WRITE_PROTECT:
		return ERROR+DONE+WriteProtected;
	case IOERR_MEDIA_NOT_FORMATTED:
	case IOERR_MEDIA_NOT_SUPPORTED:
		return ERROR+DONE+UnknownMedia;
	case IOERR_MEDIA_CHANGED:
		return ERROR+DONE+ChangeDisk;
	default:
		return ERROR+DONE+GeneralFailure;
	}
}


word
rw(ReqPtr p, word flags)
{
	word rc;
	dword count, nb;
	IORB_ADAPTER_PASSTHRU *iorb = (IORB_ADAPTER_PASSTHRU *)iorb1;

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

	if ((rc = getIORB(p)) != 0)
		return rc;

	cdb[0] = (flags & PT_DIRECTION_IN) ? SeqRead : SeqWrite;
	cdb[1] = blocksize ? 1 : 0;
	cdb[2] = nb >> 16;
	cdb[3] = nb >> 8;
	cdb[4] = nb;
	cdb[5] = 0;

	sglist[0].ppXferBuf = p->u.rw.addr;
	sglist[0].XferBufLen = count;

	memset(&status, 0, SizeofStatus);
	status.sense_len = sizeof sense;
	status.sense_ptr = sense;

	iorb->iorbh.Length = sizeof(IORB_ADAPTER_PASSTHRU);
	iorb->iorbh.UnitHandle = unitinfo->UnitHandle;
	iorb->iorbh.CommandCode = IOCC_ADAPTER_PASSTHRU;
	iorb->iorbh.CommandModifier = IOCM_EXECUTE_CDB;
	iorb->iorbh.RequestControl = IORB_DISABLE_RETRY+IORB_REQ_STATUSBLOCK;
	iorb->iorbh.StatusBlockLen = SizeofStatus;
	iorb->iorbh.pStatusBlock = (nearptr)&status;
	iorb->iorbh.Status = 0;
	iorb->iorbh.Timeout = -1;
	iorb->cSGList = 1;
	iorb->pSGList = sglist;
	iorb->ppSGList = paddr_sglist;
	iorb->ControllerCmdLen = 6;
	iorb->pControllerCmd = cdb;
	iorb->Flags = flags;

#ifdef DEBUG
	if (trace >= 3) {
		puts("	R/W CDB:");
		dump(cdb, 6);
		putc('\n');
	}
#endif

	callADDwait(&iorb->iorbh);

#ifdef DEBUG
	if (trace && iorb->iorbh.Status & ERROR || trace >= 3)
		result(&iorb->iorbh);
#endif

	relIORB();

	p->u.rw.count = blocksize ? nb * blocksize : nb;

	if (iorb->iorbh.Status & IORB_STATUSBLOCK_AVAIL
	 && status.sense_len && status.sense_ptr) {
		if (sense[2] & (FM|EOM)) {
			PUTS(4, "	EOF\n");
			stickyEOF = 1;
		}
		if (sense[0] & VADD) {
			dword info = 0;
			for (rc = 3; rc <= 6; ++rc) {
				info <<= 8;
				info |= sense[rc];
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
			p->u.rw.count -= info;
			PUTS(4, "	result count=");
			PUTD(4, p->u.rw.count);
			PUTC(4, '\n');
		}
		rc = 1;
	} else
		rc = 0;

	if (!(iorb->iorbh.Status & IORB_ERROR))
		return DONE;
	if (!rc)
		return mapIORBerror(iorb->iorbh.ErrorCode);

	if (senseMode == Sensekey) {
		switch (sense[2] & 0x0F) {
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
	if ((sense[0] & ~0x81) != 0x70)
		rc = sense[0];
	else
		switch (senseMode) {
		case TDC3600:
			rc = sense[14];
			break;
		default:
			rc = sense[12];
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
	IORB_UNIT_CONTROL *iorb = (IORB_UNIT_CONTROL *)iorb1;

	if (!callADD) {
		word rc = startup();
		if (rc != DONE)
			return rc;
	}

	PUTS(3, "Open\n");
	if (open_count) {
		++open_count;
		return DONE;
	}

	iorb->iorbh.Length = sizeof(IORB_UNIT_CONTROL);
	iorb->iorbh.UnitHandle = unitinfo->UnitHandle;
	iorb->iorbh.CommandCode = IOCC_UNIT_CONTROL;
	iorb->iorbh.CommandModifier = IOCM_ALLOCATE_UNIT;
	iorb->iorbh.RequestControl = IORB_DISABLE_RETRY;
	iorb->iorbh.Status = 0;
	iorb->iorbh.Timeout = -1;
	iorb->Flags = 0;
	callADDwait(&iorb->iorbh);

#ifdef DEBUG
	if (trace && iorb->iorbh.Status & ERROR || trace >= 3)
		result(&iorb->iorbh);
#endif

	stickyEOF = 0;

	if (iorb->iorbh.Status & IORB_ERROR) {
		if (iorb->iorbh.ErrorCode != IOERR_UNIT_ALLOCATED)
			return mapIORBerror(iorb->iorbh.ErrorCode);
		PUTS(3, "Unit allocated by another driver\n");
		return ERROR+DONE+DeviceNotReady;
	}
	PUTS(3, "Unit allocated\n");
	open_count = 1;
	return DONE;
}

word
DrvClose(ReqPtr p)
{
	IORB_UNIT_CONTROL *iorb = (IORB_UNIT_CONTROL *)iorb1;

	PUTS(3, "Close\n");

	if (--open_count)
		return DONE;

	iorb->iorbh.Length = sizeof(IORB_UNIT_CONTROL);
	iorb->iorbh.UnitHandle = unitinfo->UnitHandle;
	iorb->iorbh.CommandCode = IOCC_UNIT_CONTROL;
	iorb->iorbh.CommandModifier = IOCM_DEALLOCATE_UNIT;
	iorb->iorbh.RequestControl = IORB_DISABLE_RETRY;
	iorb->iorbh.Status = 0;
	iorb->iorbh.Timeout = -1;
	iorb->Flags = 0;
	callADDwait(&iorb->iorbh);

#ifdef DEBUG
	if (trace && iorb->iorbh.Status & ERROR || trace >= 3)
		result(&iorb->iorbh);
#endif

	if (iorb->iorbh.Status & ERROR)
		return mapIORBerror(iorb->iorbh.ErrorCode);

	PUTS(3, "Unit deallocated\n");
	return DONE;
}

word
DrvRead(ReqPtr p)
{
	PUTS(3, "Read\n");
	return rw(p, PT_DIRECTION_IN);
}

word
DrvWrite(ReqPtr p, int vfy)
{
	PUTS(3, "Write\n");
	return rw(p, 0);
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
	    {
		IORB_DEVICE_CONTROL *iorb = (IORB_DEVICE_CONTROL *)iorb1;

		PUTS(3, "IOCtl Reset\n");

		senseValid = 0;

		if ((rc = getIORB(p)) != 0)
			return rc;

		iorb->iorbh.Length = sizeof(IORB_DEVICE_CONTROL);
		iorb->iorbh.UnitHandle = unitinfo->UnitHandle;
		iorb->iorbh.CommandCode = IOCC_DEVICE_CONTROL;
		iorb->iorbh.CommandModifier = IOCM_RESET;
		iorb->iorbh.RequestControl = IORB_DISABLE_RETRY;
		iorb->iorbh.Status = 0;
		iorb->iorbh.Timeout = -1;
		iorb->Flags = 0;
		callADDwait(&iorb->iorbh);
		relIORB();

		if (iorb->iorbh.Status & ERROR)
			return ERROR+DONE+packError(iorb->iorbh.ErrorCode);
		return DONE;
	    }

	case IOCtlLevel:
		PUTS(3, "IOCtl Level\n");
		if (VerifyAccess(p->u.ioc.data, 2, VerifyWrite))
			return ERROR+DONE+InvalidParam;
		((byte _far *)p->u.ioc.data)[0] = 3;
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
#ifdef DEBUG
		if (trace >= 1) {
			puts("Trace level ");
			putd(trace);
			putc('\n');
		}
#endif
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

	case IOCtlRead+IOCtlSlow:
	case IOCtlRead+IOCtlFast:
		PUTS(3, "IOCtl Read\n");
	read:	flags = PT_DIRECTION_IN;
		if (VerifyAccess(p->u.ioc.parm, p->u.ioc.plen, VerifyRead))
			return ERROR+DONE+TapeInvalidParm;
		if (p->u.ioc.dlen
		 && VerifyAccess(p->u.ioc.data, p->u.ioc.dlen, VerifyWrite))
			return ERROR+DONE+TapeInvalidData;
		goto checked;
	case IOCtlWrite+IOCtlSlow:
	case IOCtlWrite+IOCtlFast:
		PUTS(3, "IOCtl Write\n");
		flags = 0;
		if (VerifyAccess(p->u.ioc.parm, p->u.ioc.plen, VerifyRead))
			return ERROR+DONE+TapeInvalidParm;
		if (p->u.ioc.dlen
		 && VerifyAccess(p->u.ioc.data, p->u.ioc.dlen, VerifyRead))
			return ERROR+DONE+TapeInvalidData;
	checked:
	    {	IORB_ADAPTER_PASSTHRU *iorb = (IORB_ADAPTER_PASSTHRU *)iorb1;

		if (p->u.ioc.plen < 6 || p->u.ioc.plen > 12)
			return ERROR+DONE+TapeInvalidParm;
		if ((rc = getIORB(p)) != 0)
			return rc;
		_fmemcpy(cdb, p->u.ioc.parm, p->u.ioc.plen);

		dataLock = Lock(Segment(p->u.ioc.data), LockShort);
		sglist[0].ppXferBuf = VirtToPhys(p->u.ioc.data);
		sglist[0].XferBufLen = p->u.ioc.dlen;

		senseValid = 0;
		memset(&status, 0, SizeofStatus);
		status.sense_len = sizeof sense;
		status.sense_ptr = sense;

		iorb->iorbh.Length = sizeof(IORB_ADAPTER_PASSTHRU);
		iorb->iorbh.UnitHandle = unitinfo->UnitHandle;
		iorb->iorbh.CommandCode = IOCC_ADAPTER_PASSTHRU;
		iorb->iorbh.CommandModifier = IOCM_EXECUTE_CDB;
		iorb->iorbh.RequestControl = IORB_REQ_STATUSBLOCK + IORB_DISABLE_RETRY;
		iorb->iorbh.StatusBlockLen = SizeofStatus;
		iorb->iorbh.pStatusBlock = (nearptr)&status;
		iorb->iorbh.Status = 0;
		iorb->iorbh.Timeout = -1;
		iorb->cSGList = 1;
		iorb->pSGList = sglist;
		iorb->ppSGList = paddr_sglist;
		iorb->ControllerCmdLen = p->u.ioc.plen;
		iorb->pControllerCmd = cdb;
		iorb->Flags = flags;

#ifdef DEBUG
		if (trace >= 3) {
			puts("	CDB:");
			dump(cdb, p->u.ioc.plen);
			putc('\n');
		}
#endif

		callADDwait(&iorb->iorbh);

#ifdef DEBUG
		if (trace && iorb->iorbh.Status & ERROR || trace >= 3)
			result(&iorb->iorbh);
#endif

		relIORB();
		Unlock(dataLock);

		if (iorb->iorbh.Status & IORB_STATUSBLOCK_AVAIL)
			senseValid = 1;
		if (iorb->iorbh.Status & IORB_ERROR)
			return ERROR+DONE+packError(iorb->iorbh.ErrorCode);
		return DONE;
	    }

	case IOCtlSense:
		PUTS(3, "IOCtl Sense\n");
		if (!senseValid) {
			PUTS(3, "	No sense data\n");
			return ERROR+DONE+TapeNoSenseData;
		}
		PUTS(3, "	Valid sense data\n");
		if (p->u.ioc.dlen > sizeof sense)
			return ERROR+DONE+TapeInvalidData;
		_fmemcpy((farptr)p->u.ioc.data, sense, p->u.ioc.dlen);
		return DONE;

	default:
		return ERROR+DONE+TapeInvalidFcn;
	}
}
