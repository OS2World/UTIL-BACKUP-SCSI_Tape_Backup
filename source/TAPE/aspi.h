/*****************************************************************************
 * $Id: aspi.h,v 1.2 1992/09/02 19:05:27 ak Exp $
 *****************************************************************************
 * $Log: aspi.h,v $
 * Revision 1.2  1992/09/02  19:05:27  ak
 * Version 2.0
 * - EMX version
 * - AIX version
 * - SCSI-2 commands
 * - ADD Driver
 * - blocksize support
 *
 * Revision 1.1.1.1  1992/01/06  20:27:18  ak
 * Interface now based on ST01 and ASPI.
 * AHA_DRVR no longer supported.
 * Files reorganized.
 *
 * Revision 1.1  1992/01/06  20:27:17  ak
 * Initial revision
 *
 *****************************************************************************/

#pragma ZTC align 1
#pragma pack(1)		/* MSC */

	/* SRB command */
#define SRB_Inquiry	0x00
#define SRB_Device	0x01
#define SRB_Command	0x02
#define SRB_Abort	0x03
#define SRB_Reset	0x04
#define SRB_Param	0x05

	/* SRB status */
#define SRB_Busy	0x00	/* SCSI request in progress */
#define SRB_Done	0x01	/* SCSI request completed without error */
#define SRB_Aborted	0x02	/* SCSI aborted by host */
#define SRB_BadAbort	0x03	/* Unable to abort SCSI request */
#define SRB_Error	0x04	/* SCSI request completed with error */
#define SRB_BusyPost	0x10	/* SCSI request in progress with POST - Nokia */
#define SRB_InvalidCmd	0x80	/* Invalid SCSI request */
#define SRB_InvalidHA	0x81	/* Invalid Hhost adapter number */
#define SRB_BadDevice	0x82	/* SCSI device not installed */

	/* SRB flags */
#define SRB_Post	0x01	/* Post vector valid */
#define SRB_Link	0x02	/* Link vector valid */
#define SRB_SG		0x04	/* Nokia: scatter/gather */
				/* S/G: n * (4 bytes length, 4 bytes addr) */
				/* No of s/g items not limited by HA spec. */
#define SRB_NoCheck	0x00	/* determined by command, not checked  */
#define SRB_Read	0x08	/* target to host, length checked  */
#define SRB_Write	0x10	/* host to target, length checked  */
#define SRB_NoTransfer	0x18	/* no data transfer  */
#define SRB_DirMask	0x18	/* bit mask */

	/* SRB host adapter status */
#define SRB_NoError	0x00	/* No host adapter detected error */
#define SRB_Timeout	0x11	/* Selection timeout */
#define SRB_DataLength	0x12	/* Data over/underrun */
#define SRB_BusFree	0x13	/* Unexpected bus free */
#define SRB_BusSequence	0x14	/* Target bus sequence failure */

	/* SRB target status field */
#define SRB_NoStatus	0x00	/* No target status */
#define SRB_CheckStatus	0x02	/* Check status (sense data valid) */
#define SRB_LUN_Busy	0x08	/* Specified LUN is busy */
#define SRB_Reserved	0x18	/* Reservation conflict */

#define MaxCDBStatus	64	/* max size of CDB + status */

typedef struct SRB SRB;
struct SRB {
	unsigned char	cmd,				/* 00 */
			status,				/* 01 */
			ha_num,				/* 02 */
			flags;				/* 03 */
	unsigned long	res_04_07;			/* 04..07 */
	union {						/* 08 */

	/* SRB_Inquiry */
		struct {
			unsigned char	num_ha,		/* 08 */
					ha_target,	/* 09 */
					aspimgr_id[16],	/* 0A..19 */
					host_id[16],	/* 1A..29 */
					unique_id[16];	/* 2A..39 */
		} inq;

	/* SRB_Device */
		struct {
			unsigned char	target,		/* 08 */
					lun,		/* 09 */
					devtype;	/* 0A */
		} dev;

	/* SRB_Command */
		struct {
			unsigned char	target,		/* 08 */
					lun;		/* 09 */
			unsigned long	data_len;	/* 0A..0D */
			unsigned char	sense_len;	/* 0E */
		    #ifdef OS2
			unsigned long	data_ptr;	/* 0F..12 */
			unsigned long	link_ptr;	/* 13..16 */
		    #else
			void _far *	data_ptr;	/* 0F..12 */
			void _far *	link_ptr;	/* 13..16 */
		    #endif
			unsigned char	cdb_len,	/* 17 */
					ha_status,	/* 18 */
					target_status;	/* 19 */
		    #ifdef OS2
			void (_far * realpost) (unsigned, unsigned long);
			unsigned short	realDS;			/* 1A..1F */
			void (_far * protpost) (unsigned, unsigned long);
			unsigned short	protDS;			/* 20..25 */
			unsigned long	phys_addr_srb;	/* 26..29 */
		    #else
			void	(_far *	post) (SRB *);	/* 1A..1D */
			unsigned char	res_1E_29[12];	/* 1E..29 */
		    #endif
			unsigned char	res_2A_3F[22];	/* 2A..3F */
			unsigned char	cdb_st[64];	/* 40..7F CDB+status */
			unsigned char	res_80_BF[64];	/* 80..BF */
		} cmd;

	/* SRB_Abort */
		struct {
		    #ifdef OS2
			unsigned long	phys_addr_srb;	/* 08..0B */
		    #else
			void _far *	srb;		/* 08..0B */
		    #endif
		} abt;

	/* SRB_Reset */
		struct {
			unsigned char	target,		/* 08 */
					lun,		/* 09 */
					res_0A_17[14],	/* 0A..17 */
					ha_status,	/* 18 */
					target_status;	/* 19 */
		} res;

	/* SRB_Param - unused by ASPI4OS2 */
		struct {
			unsigned char	unique[16];	/* 08..17 */
		} par;

	} u;
};

#pragma ZTC align
#pragma pack()
