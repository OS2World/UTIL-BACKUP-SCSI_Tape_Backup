@echo off
rem ###########################################################################
rem $Id: msc.cmd,v 1.1.1.1 1992/09/02 18:57:48 ak Exp $
rem ###########################################################################
rem $Log: msc.cmd,v $
rem Revision 1.1.1.1  1992/09/02  18:57:48  ak
rem Initial checkin of OS/2 ADD based SCSI tape device driver.
rem
rem Revision 1.1  1992/09/02  18:57:46  ak
rem Initial revision
rem
rem ###########################################################################
echo on
cl -Ic:/usr/inc -I../tape -DDEBUG -DBASEDEV -nologo -DMICROSOFT -Ox -Zp1 -ASw -Gs -G2 -Fomain.obj -c main.c 
cl -Ic:/usr/inc -I../tape -DDEBUG -DBASEDEV -nologo -DMICROSOFT -Ox -Zp1 -ASw -Gs -G2 -Foinit.obj -c init.c 
link /NOE /NOD /MAP /li /b/nod/noign/nologo/se:256 /st:8096 header main init, scsitape.dmd, scsitape.map, dd.lib slibcep.lib, scsitape.def
copy scsitape.dmd F:\OS2\scsitape.dmd
