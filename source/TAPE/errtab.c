/*****************************************************************************
 * $Id: errtab.c,v 1.2 1992/09/02 19:05:46 ak Exp $
 *****************************************************************************
 * $Log: errtab.c,v $
 * Revision 1.2  1992/09/02  19:05:46  ak
 * Version 2.0
 * - EMX version
 * - AIX version
 * - SCSI-2 commands
 * - ADD Driver
 * - blocksize support
 *
 * Revision 1.1.1.1  1992/01/06  20:27:22  ak
 * Interface now based on ST01 and ASPI.
 * AHA_DRVR no longer supported.
 * Files reorganized.
 *
 * Revision 1.1  1992/01/06  20:27:20  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: errtab.c,v 1.2 1992/09/02 19:05:46 ak Exp $";

#include "scsi.h"

char	*senseTab[] = {
	"No error",
	"Recovered data",
	"Not ready",
	"Media error",
	"Hardware error",
	"Illegal request",
	"Unit attention",
	"Write protected",
	"At end of logical media",
	"Vendor sense key",
	"Copy aborted",
	"Aborted",
	"Search equal",
	"Media overflow",
	"Verify failed",
	"Reserved"
};

ErrorTable targetStatusTab[] = {
	GoodStatus,		"Good status",
	CheckStatus,		"Check status",
	ConditionMet,		"Condition met",
	BusyStatus,		"Device busy",
	Intermediate,		"Linked good status",
	IntermediateCondMet,	"Linked good status, condition met",
	ReservationConflict,	"Reservation conflict",
	CommandTerminated,	"Command terminated",
	QueueFull,		"Queue full",
	-1
};

ErrorTable tdc3600ercd[] = {
#if !defined(__SMALL__) && !defined(__MEDIUM__)
	0x00,	"No error",
	0x02,	"Hardware error",
	0x04,	"Media not loaded",
	0x09,	"Media not loaded",
	0x0A,	"Insufficient capacity",
	0x11,	"Uncorrectable data error",
	0x14,	"No record found",
	0x17,	"Write protected",
	0x19,	"Bad block found",
	0x1C,	"File mark detected",
	0x1D,	"Compare error",
	0x20,	"Invalid command",
	0x30,	"Unit attention",
	0x33,	"Append error",
	0x34,	"Read End-Of-Media",
#endif
	-1
};

ErrorTable tdc3600xercd[] = {
#if !defined(__SMALL__) && !defined(__MEDIUM__)
	0x00,	"No error",
	0x01,	"Append error",
	0x02,	"Bad command block",
	0x03,	"Bad parameter block",
	0x04,	"Bus parity error",
	0x05,	"Busy",
	0x06,	"Capstan servo error",
	0x07,	"Cartridge removed",
	0x08,	"Compare error",
	0x09,	"Copy data error",
	0x0A,	"Copy management error",
	0x0B,	"File mark error",
	0x0C,	"Head servo error",
	0x0D,	"Illegal command",
	0x0E,	"Illegal copy",
	0x0F,	"Illegal length",
	0x10,	"Inappropriate request",
	0x11,	"Latch error",
	0x12,	"No cartridge",
	0x13,	"Not loaded",
	0x14,	"Power-on request",
	0x15,	"QIC. No data detected",
	0x16,	"Read after write error",
	0x17,	"Read EOM logical",
	0x18,	"Read EOM physical",
	0x19,	"Reservation conflict",
	0x1A,	"Sensor error",
	0x1B,	"Tape runout",
	0x1C,	"Unit attention",
	0x1D,	"Write EOM warning",
	0x1E,	"Write EOM",
	0x1F,	"Catridge write protected",
	0x20,	"16 rewrite errors",
	0x21,	"24 rereads, block found",
	0x22,	"24 rereads, block not found",
	0x23,	"Illegal copy function",
	0x24,	"Illegal header",
	0x25,	"No header",
	0x26,	"Too large address",
	0x27,	"Bad ID or LUN",
	0x28,	"Partial description",
	0x29,	"Bad target status",
	0x2A,	"Check condition",
	0x2B,	"Data transfer error",
	0x2C,	"Selection failure",
	0x2D,	"Sequence error",
	0x2E,	"Illegal block size",
#endif
	-1
};

ErrorTable scsi2asc[] = {
#if !defined(__SMALL__) && !defined(__MEDIUM__)
	0x0000,	"No additional sense info",
	0x0001,	"Filemark detected",
	0x0002,	"End of partition/medium detected",
	0x0003,	"Setmark detected",
	0x0004,	"Beginning of partition/medium detected",
	0x0005,	"End of data detected",
	0x0006,	"I/O process terminated",
	0x0300,	"Peripheral device write fault",
	0x0301,	"No write current",
	0x0302,	"Excessive write errors",
	0x0400,	"Logical unit not ready",
	0x0401,	"Logical unit is in process of becoming ready",
	0x0402,	"Logical unit not ready, initializing command required",
	0x0403,	"Logical unit not ready, manual intervention required",
	0x0404,	"Logical unit not ready, format in progress",
	0x0500,	"Logical unit does not respond to selection",
	0x0700,	"Multiple peripheral devices selected",
	0x0800,	"Logical unit communication failure",
	0x0801,	"Logical unit communication time-out",
	0x0802,	"Logical unit communication parity error",
	0x0900,	"Track following error",
	0x0A00,	"Error log overflow",
	0x0C00,	"Write error",
	0x1100,	"Unrecovered read error",
	0x1101,	"Read retries exhausted",
	0x1102,	"Error too long to correct",
	0x1103,	"Multiple read errors",
	0x1108,	"Incomplete block read",
	0x1109,	"No gap found",
	0x110A,	"Miscorrected error",
	0x1400,	"Recorded entity not found",
	0x1401,	"Record not found",
	0x1500,	"Random positioning error",
	0x1501,	"Mechanical positioning error",
	0x1502,	"Positioning error detected  by read of medium",
	0x1700,	"Recovered data with no error correction applied",
	0x1701,	"Recovered data with retries",
	0x1702,	"Recovered data with positive head offset",
	0x1703,	"Recovered data with negative head offset",
	0x1800,	"Recovered data with error correction applied",
	0x1A00,	"Parameter list length error",
	0x1B00,	"Synchronous data transfer error",
	0x2000,	"Invalid command operation code",
	0x2400,	"Invalid field in CDB",
	0x2500,	"Logical unit not supported",
	0x2600,	"Invalid field in parameter list",
	0x2601,	"Parameter not supported",
	0x2602,	"Parameter value invalid",
	0x2603,	"Threshold parameters not supported",
	0x2700,	"Write protected",
	0x2800,	"Not ready to ready transition (medium may have changed)",
	0x2900,	"Power on, reset or bus device reset occurred",
	0x2A00,	"Parameters changed",
	0x2A01,	"Mode parameters changed",
	0x2A02,	"Log parameters changed",
	0x2B00,	"Copy cannot execute since host cannot disconnect",
	0x2C00,	"Command sequence error",
	0x2D00,	"Overwrite error on update in place",
	0x2F00,	"Commands cleared by another initiator",
	0x3000,	"Incompatible medium installed",
	0x3001,	"Cannot read medium - unknown format",
	0x3002,	"Cannot read medium - incompatible format",
	0x3003,	"Cleaning cartridge installed",
	0x3100,	"Medium format corrupted",
	0x3300,	"Tape length error",
	0x3700,	"Rounded parameter",
	0x3900,	"Saving parameters not supported",
	0x3A00,	"Medium not present",
	0x3B00,	"Sequntial positioning error",
	0x3B01,	"Tape position error at beginning of medium",
	0x3B02,	"Tape position error at end of medium",
	0x3B08,	"Reposition error",
	0x3D00,	"Invalid bits in identify message",
	0x3E00,	"Logical unit has not self-configured yet",
	0x3F00,	"Target operating conditions have changed",
	0x3F01,	"Microcode has been changed",
	0x3F02,	"Changed operatin definition",
	0x3F03,	"Inquiry data has changed",
	0x4300,	"Message error",
	0x4400,	"Internal target failure",
	0x4500,	"Select or reselect failure",
	0x4600,	"Unsuccessful soft reset",
	0x4700,	"SCSI parity error",
	0x4800,	"Initiator detected error message received",
	0x4900,	"Invalid message error",
	0x4A00,	"Command phase error",
	0x4B00,	"Data phase error",
	0x4C00,	"Logical unit failed self-configuration",
	0x5000,	"Write append error",
	0x5001,	"Write append position error",
	0x5002,	"Position error related to timing",
	0x5100,	"Erase failure",
	0x5200,	"Cartridge fault",
	0x5300,	"Media load or eject failed",
	0x5301,	"Unload tape failure",
	0x5302,	"Medium removal prevented",
	0x5A00,	"Operator request or state change input (unspecified)",
	0x5A01,	"Operator medium removal request",
	0x5A02,	"Operator selected write protect",
	0x5A03,	"Operator selected write permit",
	0x5B00,	"Log exception",
	0x5B01,	"Treshold condition met",
	0x5B02,	"Log counter at maximum",
	0x5B03,	"Log list codes exhausted",
#endif
	-1
};

ErrorTable addErrorTab[] = {
	0x11,	"Command not supported",
	0x12,	"Command syntax",
	0x13,	"Bad scatter/gather list",
	0x14,	"Out of ressources",
	0x15,	"Aborted",
	0x16,	"ADD software failure",
	0x17,	"OS software failure",
	0x21,	"Unit not allocated",
	0x22,	"Unit busy",
	0x23,	"Not ready",
	0x24,	"Powered off",
	0x31,	"Addressing error",
	0x32,	"Volume overflow",
	0x33,	"CRC error",
	0x41,	"Not formatted",
	0x42,	"Media not supported",
	0x43,	"Write protected",
	0x44,	"Media changed",
	0x45,	"Media not present",
	0x51,	"Host bus error",
	0x52,	"Adapter SCSI bus error",
	0x53,	"Adapter overrun",
	0x54,	"Adapter underrun",
	0x55,	"Adapter diagnose failure",
	0x56,	"Adapter timeout",
	0x57,	"Device timeout",
	0x58,	"Adapter operation not supported",
	0x59,	"Refer to status",
	0x5A,	"Nonspecific adapter error",
	0x61,	"Device SCSI bus error",
	0x62,	"Device operation not supported",
	0x63,	"Device diagnose failure",
	0x64,	"Device busy",
	0x65,	"Device overrun",
	0x66,	"Device underrun",
	0x67,	"Device reset",
	0x68,	"Nonspecific device error",
	-1
};

char *
find_error(ErrorTable *p, unsigned code)
{
	static char other[20];

	for (; p->code != (unsigned)-1; ++p)
		if (p->code == code)
			return p->text;
	sprintf(other, "Unknown error code: %02X", code);
	return other;
}
