/*****************************************************************************
 * $Id: init.c,v 2.1 1992/11/14 21:00:20 ak Exp $
 *****************************************************************************
 * $Log: init.c,v $
 * Revision 2.1  1992/11/14  21:00:20  ak
 * OS/2 2.00.1 ASPI
 *
 * Revision 1.1.1.1  1992/01/06  20:16:00  ak
 * Alpha version.
 *
 * Revision 1.1  1992/01/06  20:15:58  ak
 * Initial revision
 *
 *****************************************************************************/

static char _far rcsid[] = "$Id: init.c,v 2.1 1992/11/14 21:00:20 ak Exp $";

#include "aspitape.h"
#define INCL_DOS
#include <os2.h>

static char     aspiName[]	= "SCSIMGR$";

/*
 * Put initialization data in a separate segment.
 */
char _far	noAttach[]	= "Cannot attach to SCSIMGR$\n";
char _far	noOpen[]	= "Cannot open SCSIMGR$\n";
char _far	badLine[]	= "ASPITAPE command line error$\n";
char _far	noTape[]	= "No target ID on ASPITAPE command line\n";
char _far	okMsg1[]	= "ASPI tape driver installed"
					  " for adapter ";
char _far	okMsg2[]	= ", target ";
char _far	okMsg3[]	= ", sense mode is ";
char _far	mode0Msg[]	= "Sense Key";
char _far	mode1Msg[]	= "TDC3600";
char _far	mode2Msg[]	= "SCSI-2";
char _far	modeXMsg[]	= "invalid";
char _far * _far modeMsg[]	= { mode0Msg, mode1Msg, mode2Msg };

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
			continue;
		}
	}		
}

word
DrvInit(char _far *line)
{
	word action;

	trace = 0;
	blocksize = 512;
	paddrSRB = VirtToPhys(&srb);

	if (AttachDD(aspiName, &aspiEntry)) {
		puts(noAttach);
		return ERROR+DONE+GeneralFailure;
	}

	if (DosOpen((PSZ)aspiName, &aspiHandle, &action, 0L, FILE_NORMAL,
			OPEN_ACTION_OPEN_IF_EXISTS, OPEN_ACCESS_READWRITE
			+ OPEN_SHARE_DENYNONE + OPEN_FLAGS_FAIL_ON_ERROR, 0L)) {
		puts(noOpen);
		return ERROR+DONE+GeneralFailure;
	}
	adapter = 0;
	target = 0xFF;

	DosClose(aspiHandle);
	aspiHandle = 0;

	if (cmdline(line)) {
		puts(badLine);
		return ERROR+DONE+GeneralFailure;
	}

	if (target == 0xFF) {
		puts(noTape);
		return ERROR+DONE+GeneralFailure;
	}

	puts(okMsg1);
	putd(adapter);
	puts(okMsg2);
	putd(target);
	puts(okMsg3);
	puts(senseMode <= SCSI2 ? modeMsg[senseMode] : modeXMsg);
	putc('\n');
	return DONE;
}
