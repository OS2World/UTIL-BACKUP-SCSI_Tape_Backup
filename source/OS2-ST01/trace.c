#include <stdio.h>
#include <stdlib.h>
#define INCL_DOSFILEMGR
#define INCL_DOSDEVICES
#include <os2.h>

main(int argc, char **argv)
{
	HFILE	hdev;
	USHORT	action, rc;
	BYTE	level = 1;

	if (argc > 1)
		level = atoi(argv[1]);
	rc = DosOpen("TAPE$4", &hdev, &action, 0L, 0,
		FILE_OPEN, OPEN_ACCESS_READWRITE+OPEN_SHARE_DENYNONE, 0L);
	if (rc) {
		printf("open: %d\n", rc);
		exit(1);
	}
	rc = DosDevIOCtl2(NULL, 0, &level, 1, 0x04, 0x80, hdev);
	if (rc) {
		printf("ioctl sense: %d\n", rc);
		exit(1);
	}
	DosClose(hdev);
}

