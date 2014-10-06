/*****************************************************************************
 * $Id: init.c,v 1.4 1992/09/29 09:35:20 ak Exp $
 *****************************************************************************
 * $Log: init.c,v $
 * Revision 1.4  1992/09/29  09:35:20  ak
 * Must allow multiple DrvOpen calls. dup() seems to reopen the driver.
 *
 * Revision 1.3  1992/09/26  19:30:17  ak
 * Initialize <trace> - doppelt genaeht halt besser.
 *
 * Revision 1.2  1992/09/18  15:59:16  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1992/09/02  18:58:19  ak
 * Initial checkin of OS/2 ADD based SCSI tape device driver.
 *
 * Revision 1.1  1992/09/02  18:58:17  ak
 * Initial revision
 *
 *****************************************************************************/

static char _far rcsid[] = "$Id: init.c,v 1.4 1992/09/29 09:35:20 ak Exp $";

#include "addtape.h"
#include <string.h>

#define TraceDEVTAB	3

static int	adapter;
static int	target;
static int	type_code = UIB_TYPE_TAPE;

#ifdef DEBUG

static char *	devbus[] = {
	/* 0000 */	"?",
	/* 0001 */	"ST506 CAM-I",
	/* 0002 */	"ST506 CAM-II",
	/* 0003 */	"ESDI",
	/* 0004 */	"Floppy",
	/* 0005 */	"SCSI-1",
	/* 0006 */	"SCSI-2",
	/* 0007 */	"SCSI-3",
};
static char *	hostbus[] = {
	/* 0000 */	"?",
	/* 0001 */	"ISA",
	/* 0002 */	"EISA",
	/* 0003 */	"MCA",
};
static char *	buswidth[] = {
	/* 0000 */	"-?",
	/* 0010 */	"-8",
	/* 0020 */	"-16",
	/* 0030 */	"-32",
	/* 0040 */	"-64",
	/* 0050 */	"-?",
	/* 0060 */	"-?",
	/* 0070 */	"-?",
};
static char *	unittype[] = {
	/* 0000 */	"disk",
	/* 0001 */	"tape",
	/* 0002 */	"printer",
	/* 0003 */	"processor",
	/* 0004 */	"WORM",
	/* 0005 */	"CDROM",
	/* 0006 */	"scanner",
	/* 0007 */	"optical",
	/* 0008 */	"changed",
	/* 0009 */	"comm",
	/* 000A */	"?",
	/* 000B */	"?",
	/* 000C */	"?",
	/* 000D */	"?",
	/* 000E */	"?",
	/* 000F */	"?",
};

#endif

/*
 * Handle device command line.
 * Line is terminated by 0.
 */
int
cmdline(char _far *line)
{
	byte *name;

	/*
	 * Skip driver name up to first argument.
	 */
	for (; *line && *line != ' '; ++line) ;
	for (; *line == ' '; ++line) ;

	/*
	 * First argument is device name.
	 */
	name = header.name;
	for (; *line && *line != ' '; ++line)
		if (name < header.name+8)
			*name++ = *line;
	while (name < header.name+8)
		*name++ = ' ';

	/*
	 * Handle arguments.
	 */
	for (;;++line) {
		switch (*line) {
		default:
			return 1;
		case 0:
			return 0;
		case ' ':
			continue;
		case 'D': case 'd':			/* debug level */
			if (*++line < '0' || *line > '9')
				return 1;
			trace = *line - '0';
			continue;
		case 'A': case 'a':			/* adapter number */
			if (*++line < '0' || *line > '9')
				return 1;
			adapter = *line - '0';
			continue;
		case 'T': case 't':			/* target ID */
			if (*++line < '0' || *line > '9')
				return 1;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			target = *line - '0';
			continue;
		case 'S': case 's':			/* sense data mode */
			if (*++line < '0' || *line > '9')
				return 1;
			senseMode = *line - '0';
			continue;
		case 'V':
			video_address = 0xB8000;	/* VGA output */
			continue;
		case 'C':				/* type code */
			if (*++line < '0' || *line > '9')
				return 1;
			type_code = *line - '0';
			continue;
		}
	}		
}


word
startup(void)
{
	int driver_no;

	unitinfo = NULL;
	for (driver_no = 0; driver_no < dctbl->count; ++driver_no) {
		DevClassTableEntry _far *dcent = &dctbl->entries[driver_no];
		IORB_CONFIGURATION *iorb = (IORB_CONFIGURATION *)iorb1;
		int adapter_no;

#ifdef DEBUG
		if (trace >= TraceDEVTAB) {
			puts("Driver ");
			putd(driver_no);
			puts(", name '");
			puts(dcent->name);
			putc('\'');
		}
#endif

		iorb->iorbh.Length = sizeof(IORB_CONFIGURATION);
		iorb->iorbh.RequestControl = IORB_DISABLE_RETRY;
		iorb->iorbh.CommandCode = IOCC_CONFIGURATION;
		iorb->iorbh.CommandModifier = IOCM_GET_DEVICE_TABLE;
		iorb->pDeviceTable = devtab;
		iorb->DeviceTableLen = sizeof devtab_data;
		dcent->entry(iorb);

		if (iorb->iorbh.Status != IORB_DONE) {
#ifdef DEBUG
			if (trace >= 1) {
				puts(" -- Error ");
				putx(iorb->iorbh.ErrorCode);
				putc('\n');
				continue;
			}
#endif
		}

#ifdef DEBUG
		if (trace >= TraceDEVTAB) {
			puts(", level ");
			putd(devtab->ADDLevelMajor);
			putc('.');
			putd(devtab->ADDLevelMinor);
			puts(", ");
			putd(devtab->TotalAdapters);
			puts(" adapters\n");
		}
#endif

		for (adapter_no = 0; adapter_no < devtab->TotalAdapters; ++adapter_no) {
			ADAPTERINFO *adap = devtab->pAdapter[adapter_no];
			int unit_no, i;
			int scsi = (adap->AdapterDevBus & 7) >= AI_DEVBUS_SCSI_1;

#ifdef DEBUG
			if (trace >= TraceDEVTAB) {
				puts("   Adapter ");
				putd(adapter_no);
				puts(", name '");
				puts(adap->AdapterName);
				puts("', ");
				putd(adap->AdapterUnits);
				puts(" units\n");

				puts("      ");
				i = adap->AdapterDevBus;
				puts(devbus[i & 7]);
				if (i & AI_DEVBUS_FAST_SCSI)
					puts("-fast");
				if (i & AI_DEVBUS_8BIT)
					puts("-8");
				if (i & AI_DEVBUS_16BIT)
					puts("-16");
				if (i & AI_DEVBUS_32BIT)
					puts("-32");
				i = adap->AdapterIOAccess;
				if (i & AI_IOACCESS_BUS_MASTER)
					puts(", bus-master");
				if (i & AI_IOACCESS_PIO)
					puts(", programmed-i/o");
				if (i & AI_IOACCESS_DMA_SLAVE)
					puts(", DMA-slave");
				if (i & AI_IOACCESS_MEMORY_MAP)
					puts(", memory-mapped");
				puts(", ");
				puts(hostbus[adap->AdapterHostBus & 3]);
				puts(buswidth[adap->AdapterHostBus >> 4 & 15]);
				if (scsi) {
					puts(", ID ");
					putx(adap->AdapterSCSITargetID);
					puts(", LUN ");
					putx(adap->AdapterSCSILUN);
				}
				puts("\n      S/G max ");
				putd(adap->MaxHWSGList);
				if (scsi) {
					puts(", CDB max ");
					putd(adap->MaxCDBTransferLength);
				}
				i = adap->AdapterFlags;
				if (i & AF_16M)
					puts(", > 16MB");
				else
					puts(", max 16MB");
				if (i & AF_IBM_SCB)
					puts(", IBM SCB");
				if (i & AF_HW_SCATGAT)
					puts(", hardware S/G");
				if (i & AF_HW_SCATGAT)
					puts(", CHS addressing");
				putc('\n');
			}
#endif

			for (unit_no = 0; unit_no < adap->AdapterUnits; ++unit_no) {
				UNITINFO *unit = &adap->UnitInfo[unit_no];

#ifdef DEBUG
				if (trace >= TraceDEVTAB) {
					puts("      Unit ");
					putd(unit_no);
					if (scsi) {
						puts(", ID ");
						putd(unit->UnitSCSITargetID);
						puts(", LUN ");
						putd(unit->UnitSCSILUN);
					}
					puts(", ");
					puts(unittype[unit->UnitType]);
					puts(" dev");
					i = unit->UnitFlags;
					if (i & UF_REMOVABLE)
						puts(", removable");
					if (i & UF_REMOVABLE)
						puts(", changeline");
					if (i & UF_PREFETCH)
						puts(", prefetch");
					if (i & UF_A_DRIVE)
						puts(", drive A");
					if (i & UF_B_DRIVE)
						puts(", drive B");
					if (i & UF_NODASD_SUPT)
						puts(", no DASD");
					if (i & UF_NOSCSI_SUPT)
						puts(", no SCSI");
					if (i & UF_DEFECTIVE)
						puts(", defective");
					puts(", queuing ");
					putd(unit->QueuingCount);
					putc('\n');
				}
#endif

				if (scsi
				 && unit->UnitType == type_code
				 && !(unit->UnitFlags & UF_NOSCSI_SUPT)
				 && adapter_no == adapter
				 && unit->UnitSCSITargetID == target) {
				 	PUTS(TraceDEVTAB, "         Matched\n");
				 	callADD = dcent->entry;
					unitinfo = unit;
					if (trace < TraceDEVTAB)
						goto found;
				}
			}
		}
	}

	if (!unitinfo || !callADD) {
		PUTS(1, "No such tape device found\n");
		return ERROR+DONE+GeneralFailure;
	}

found:	return DONE;
}

word
DrvInitBase(char _far *line)
{
	int driver_no;

#ifndef __ZTC__
	_STI_d2debug();
#endif

	trace = 0;
	open_count = 0;

	devtab = (DEVICETABLE *)devtab_data;
	memset(iorb1, 0, MAX_IORB_SIZE);
	memset(iorb2, 0, MAX_IORB_SIZE);
	((IORBH *)iorb2)->Status = DONE;
	paddr_iorb1 = VirtToPhys(iorb1);
	paddr_sglist = VirtToPhys(sglist);
	blocksize = 512;

	if (cmdline(line)) {
		PUTS(1, "SCSITAPE command line error\n");
		return ERROR+DONE+GeneralFailure;
	}

	dctbl = GetDOSVar(DosVar_DeviceClassTable, 1);
	if (dctbl == NULL) {
		PUTS(1, "GetDOSVar failed\n");
		return ERROR+DONE+GeneralFailure;
	}

	startup();

	return DONE;
}
