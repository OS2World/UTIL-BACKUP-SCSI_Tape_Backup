/*****************************************************************************
 * $Id: scsitape.h,v 1.2 1992/09/02 19:05:30 ak Exp $
 *****************************************************************************
 * $Log: scsitape.h,v $
 * Revision 1.2  1992/09/02  19:05:30  ak
 * Version 2.0
 * - EMX version
 * - AIX version
 * - SCSI-2 commands
 * - ADD Driver
 * - blocksize support
 *
 * Revision 1.1.1.1  1992/01/06  20:27:35  ak
 * Interface now based on ST01 and ASPI.
 * AHA_DRVR no longer supported.
 * Files reorganized.
 *
 * Revision 1.1  1992/01/06  20:27:34  ak
 * Initial revision
 *
 *****************************************************************************/

/*
 * SCSI Tape Definitions.
 */

#if defined(unix) && !defined(__GNUC__)
#define BitFieldType	unsigned
#else
#define BitFieldType	unsigned char
#endif

struct TDC3600_Mode {
    struct BasicMode {
	unsigned char	mode_data_len;
	unsigned char	media_type;
	BitFieldType	speed_code	: 4;
	BitFieldType	buffered_mode	: 1;
	BitFieldType	reserved_1	: 2;
	BitFieldType	write_protected	: 1;
	unsigned char	ext_data_len;
    } m;
    struct BlockMode {
	unsigned char	density_cod;
	unsigned char	reserved [4];
	unsigned char	block_size [3];
    } b;
    struct VendorMode {
	unsigned char	page_code;
	unsigned char	write_treshold;
	unsigned char	read_treshold;
	unsigned char	buffer_size;
	unsigned char	forced_count [2];
	unsigned char	bus_treshold;
	unsigned char	copy_treshold;
	unsigned char	normal_sense_treshold;
	unsigned char	copy_sense_treshold;
	unsigned char	load_function;
	unsigned char	power_up_delay;
    } v;
};
#define TDC3600ModeDataSize	(8+8+12)

typedef struct Tape_Extended_Sense {
	BitFieldType	page_code	: 7;			/* 0 */
	BitFieldType	valid_address	: 1;
	BitFieldType	segment		: 8;			/* 1 */
	BitFieldType	sense_key	: 4;			/* 2 */
	BitFieldType	_reserved_	: 1;
	BitFieldType	incorrect_length: 1;
	BitFieldType	end_of_media	: 1;
	BitFieldType	filemark	: 1;
	unsigned char	info [4];				/* 3 */
	unsigned char	additional_length;			/* 7 */
	union {
		struct TDC3600_Sense {
			unsigned char	src_sense_ptr;		/* 8 */
			unsigned char	dst_sense_ptr;		/* 9 */
			unsigned char	reserved_1 [2];		/* 10 */
			unsigned char	no_recovered [2];	/* 12 */
			unsigned char	error;			/* 14 */
			unsigned char	xerror;			/* 15 */
			unsigned char	no_blocks [3];		/* 16 */
			unsigned char	no_filemarks [2];	/* 19 */
			unsigned char	no_underruns [2];	/* 21 */
			unsigned char	no_marginal;		/* 23 */
			unsigned char	no_remaining;		/* 24 */
			unsigned char	copy_sense_data [1];	/* 25 */
		} tdc;
		struct SCSI2_Sense {
			unsigned char	cmd_specific [4];	/* 8 */
			unsigned char	asc;			/* 12 */
			unsigned char	ascq;			/* 13 */
			unsigned char	unit_code;		/* 14 */
			unsigned char	sense_key_specific [3];	/* 15 */
			unsigned char	additional [1];		/* 18 */
		} scsi2;
	} u;
} ExtSenseData;
