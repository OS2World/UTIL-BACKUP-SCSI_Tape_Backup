/*****************************************************************************
 * $Id: tapedrvr.h,v 1.3 1992/10/14 18:35:31 ak Exp $
 *****************************************************************************
 * $Log: tapedrvr.h,v $
 * Revision 1.3  1992/10/14  18:35:31  ak
 * IBM SCSI driver.
 *
 * Revision 1.2  1992/09/02  19:05:38  ak
 * Version 2.0
 * - EMX version
 * - AIX version
 * - SCSI-2 commands
 * - ADD Driver
 * - blocksize support
 *
 * Revision 1.1.1.1  1992/01/06  20:27:46  ak
 * Interface now based on ST01 and ASPI.
 * AHA_DRVR no longer supported.
 * Files reorganized.
 *
 * Revision 1.1  1992/01/06  20:27:45  ak
 * Initial revision
 *
 *****************************************************************************/

/*
 * Tape driver interface.
 */

#define IOCtlCategory	0x80	/* GIOCtl category of tape driver */

/*
 * GIOCtl2 function codes:
 */

 	/* no Level (ST01): */

#define IOCtlSlow	0	/* Fast data phase command */
				/* Param = CDB, Data = data */
#define IOCtlFast	1	/* Slow data phase command */
				/* Param = CDB, Data = data */
#define IOCtlBusReset	2	/* SCSI bus or device reset */
#define IOCtlDevReset	3	/* SCSI device reset */
#define IOCtlTrace	4	/* Set trace level */
				/* Param -> trace level byte */
				/* Data  <- previous trace level */

 	/* Level 2 (ASPI): */

#define IOCtlLevel	5	/* Return driver level */
				/* Data[0] -> driver level byte */
				/* Data[1] -> sense mode byte */
#define IOCtlRead	6	/* SCSI command, "read" type data transfer */
	/* slow: 6, fast: 7 */	/* Param = CDB, Data = data */
#define IOCtlWrite	8	/* SCSI command, "write" type data transfer */
	/* slow: 8, fast: 9 */	/* Param = CDB, Data = data */
#define IOCtlSense	10	/* Return sense data of last command or issue */
				/* the given CDB if no sense data available */
				/* Param = sense CDB, Data = sense data */

 	/* Level 3 (ADD): -- codes 0..1 no longer supported */

#define IOCtlBlocksize	12	/* Set r/w blocksize */
				/* Param -> blocksize (dword) */
				/* Data  <- previous blocksize */

/*
 * IOCtl return codes, levels 0..2, 0xFF00..0xFFFF:
 */
#define ErrSource	0xE0
#define ErrMask		0x1F
#define ErrTargetStatus	0x00	/* SCSI target status byte */
#define ErrST01Driver	0x20	/* ST01 driver error code */
#define ErrASPIDriver1	0x40	/* ASPI driver error codes 00-1F */
#define ErrASPIDriver2	0x60	/* ASPI driver error codes 80-9F */
#define ErrHostAdapter	0x80	/* AHA154x host adapter status */
#define ErrTapeDriver	0xA0	/* Tape driver error */
/*
 * IOCtl return codes, level 3
 *	00..9F:	ADD ErrorCode 000..90F.
 *	A0..FF:	ErrTapeDriver
 *
 * IOCtl return codes, level 4
 *	00..1F: standard driver status codes
 *	40..4F:	TSB device status codes >> 1
 *	50..7F:	TSB device error codes
 *	80..9F:	OS2SCSI.DMD special status codes
 *	A0..FF:	ErrTapeDriver
 */

/*
 * Driver level
 */
#define ST01driver	0	/* physical ST01 device driver */
#define ASPIdriver	2	/* ASPITAPE.SYS based on ASPI */
#define ADDdriver	3	/* SCSITAPE.DMD based on *.ADD */
#define SCSIdriver	4	/* SCSITAPE.SYS based on OS2SCSI.DMD */

/*
 * Tape driver:
 */
 	/* ASPITAPE: */
#define TapeInvalidFcn	(ErrTapeDriver+0)	/* Invalid cat/fcn code */
#define TapeInvalidParm	(ErrTapeDriver+1)	/* Invalid parm pointer/length */
#define TapeInvalidData	(ErrTapeDriver+2)	/* Invalid data pointer/length */
#define TapeNoSenseData	(ErrTapeDriver+3)	/* No sense data available */
