/*****************************************************************************
 * $Id: strategy.c,v 1.4 1992/07/24 11:37:08 ak Exp $
 *****************************************************************************
 * $Log: strategy.c,v $
 * Revision 1.4  1992/07/24  11:37:08  ak
 * OS/2 2.0
 * BASEDEV drivers
 * VGA debugging
 *
 * Revision 1.3  1992/01/06  22:37:29  ak
 * MSC does't call _STI_debug automatically.
 *
 * Revision 1.2  1992/01/06  20:09:57  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1992/01/06  19:55:11  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  19:55:10  ak
 * Initial revision
 *
 *****************************************************************************/

#include "dd.h"
#include "dd_defs.h"

extern void
strategy(ReqPtr p)
{
	word rc, dseg;

	switch (p->cmd) {

	case DevInit:
		devhlp = p->u.ini.devhlp;

		dseg = ctors();

		if ((rc = DrvInit(p->u.ini.cmdline)) & ERROR) {
			p->u.ino.nunits = 0;
			p->u.ino.endCode = 0;
			p->u.ino.endData = 0;
		} else {
			p->u.ino.endCode = Offset(&end_code);
			p->u.ino.endData = dseg;
		}
		p->u.ino.bpb = 0;
		break;

	case DevBaseInit:
		devhlp = p->u.ini.devhlp;

		dseg = ctors();

		if ((rc = DrvInitBase(p->u.ini.cmdline)) & ERROR) {
			p->u.ino.nunits = 0;
			p->u.ino.endCode = 0;
			p->u.ino.endData = 0;
		} else {
			p->u.ino.endCode = Offset(&end_code);
			p->u.ino.endData = dseg;
			p->u.ini.cmdline = 0;
		}
		p->u.ino.bpb = 0;
		break;

	case DevRead:
		PUTS(2, "DevRead file=");
		PUTD(2, p->u.rw.fileno);
		PUTS(2, ", count=");
		PUTD(2, p->u.rw.count);
		PUTC(2, '\n');
		rc = DrvRead(p);
		break;
	case DevWrite:
		PUTS(2, "DevWrite file=");
		PUTD(2, p->u.rw.fileno);
		PUTS(2, ", count=");
		PUTD(2, p->u.rw.count);
		PUTC(2, '\n');
		rc = DrvWrite(p, 0);
		break;
	case DevWriteVerify:
		PUTS(2, "DevWriteVerify file=");
		PUTD(2, p->u.rw.fileno);
		PUTS(2, ", count=");
		PUTD(2, p->u.rw.count);
		PUTC(2, '\n');
		rc = DrvWrite(p, 1);
		break;
	case DevPeek:
		PUTS(2, "DevPeek\n");
		rc = DrvPeek(p);
		break;
	case DevInputStatus:
		PUTS(2, "DevInputStatus\n");
		rc = DrvInputStatus();
		break;
	case DevInputFlush:
		PUTS(2, "DevInputFlush\n");
		rc = DrvInputFlush();
		break;
	case DevOutputStatus:
		PUTS(2, "DevOutputStatus\n");
		rc = DrvOutputStatus();
		break;
	case DevOutputFlush:
		PUTS(2, "DevOutputFlush\n");
		rc = DrvOutputFlush();
		break;
	case DevOpen:
		PUTS(2, "DevOpen file=");
		PUTD(2, p->u.oc.fileno);
		PUTC(2, '\n');
		rc = DrvOpen(p);
		break;
	case DevClose:
		PUTS(2, "DevClose file=");
		PUTD(2, p->u.oc.fileno);
		PUTC(2, '\n');
		rc = DrvClose(p);
		break;
	case DevIOCtl:
		PUTS(2, "DevIOCtl cat=");
		PUTX(2, p->u.ioc.cat);
		PUTS(2, ", fcn=");
		PUTX(2, p->u.ioc.fcn);
		PUTS(2, ", plen=");
		PUTD(2, p->u.ioc.plen);
		PUTS(2, ", dlen=");
		PUTD(2, p->u.ioc.dlen);
		PUTC(2, '\n');
		rc = DrvIOCtl(p);
		break;
	default:
		rc = ERROR + DONE + InvalidCommand;
	}

	PUTS(2, "Status=");
	PUTX(2, rc);
	PUTC(2, '\n');
	p->status = rc;
}
