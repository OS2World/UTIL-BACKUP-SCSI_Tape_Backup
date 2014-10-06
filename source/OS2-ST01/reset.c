#include <stdio.h>
#define INCL_DOSFILEMGR
#define INCL_DOSDEVICES
#include <os2.h>

main()
{
	
	HFILE	hdev;
	USHORT	action, rc;
	BYTE	dummy;

	rc = DosOpen("TAPE$0", &hdev, &action, 0L, 0,
		FILE_OPEN, OPEN_ACCESS_READWRITE+OPEN_SHARE_DENYNONE, 0L);
	if (rc) {
		printf("open: %d\n", rc);
		exit(1);
	}
	rc = DosDevIOCtl(0, 0, 0x02, 0x80, hdev);
/*	rc = DosDevIOCtl2(0, 0, 0, 0, 0x02, 0x80, hdev);	*/
	if (rc) {
		printf("ioctl: %d\n", rc);
		exit(1);
	}
	DosClose(hdev);
}

