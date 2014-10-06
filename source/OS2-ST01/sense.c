#include <stdio.h>
#define INCL_DOSFILEMGR
#define INCL_DOSDEVICES
#include <os2.h>

unsigned char	sense[6]   = { 0x03, 0, 0, 0, 0, 0 };
unsigned char	status[4];

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
	rc = DosDevIOCtl2(status, 4, sense, 6, 0x00, 0x80, hdev);
	if (rc) {
		printf("ioctl sense: %d\n", rc);
		exit(1);
	}
	printf("%02X %02X %02X %02X\n",
		status[0], status[1], status[2], status[3]);
	DosClose(hdev);
}

