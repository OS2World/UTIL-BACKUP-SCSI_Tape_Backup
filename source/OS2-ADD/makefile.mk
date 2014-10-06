###############################################################################
# $Id: makefile.mk,v 1.2 1992/09/02 19:00:30 ak Exp $
###############################################################################
# $Log: makefile.mk,v $
# Revision 1.2  1992/09/02  19:00:30  ak
# *** empty log message ***
#
# Revision 1.1.1.1  1992/09/02  18:57:41  ak
# Initial checkin of OS/2 ADD based SCSI tape device driver.
#
# Revision 1.1  1992/09/02  18:57:39  ak
# Initial revision
#
#
###############################################################################

DEST1 = F:/OS2
DEST2 = G:/OS2

CC = ccz
OPTIM = -O
DEBUG = -DDEBUG -C1Aw
M = s
MODEL = $(M)w
CFLAGS += -a1 -I$(ROOTDIR)/usr/inc -I../tape $(DEBUG) $(OPTIM) $(BUGS) -DBASEDEV
ASFLAGS += -I$(ROOTDIR)\usr\inc
LDFLAGS	+= -gm -BL -KNOE -KNOD -f -KMAP
LDLIBS += -lx dd.lib os2.lib

.SOURCE.inc :	$(ROOTDIR)/usr/inc
.SOURCE.h :	$(ROOTDIR)/usr/inc
.SOURCE.lib :	$(ROOTDIR)/usr/lib/ztc

Includes = addtape.h iorb.h dd.h
CFiles =   main.c init.c trace.c
AsmFiles = header.asm
AuxFile =  makefile.mk README scsitape.def
Files =    scsitape.h iorb.h $(CFiles) $(AsmFiles) $(AuxFile)
Objects	=  header.obj main.obj init.obj

driver : scsitape.dmd
shar : scsitape.shr
lzh: scsitape.lzh

scsitape.dmd: $(Objects) makefile.mk dd.lib
	$(CC) $(LDFLAGS) $(Objects) $(LDLIBS) -ox $@
	cp $@ $(DEST1)
	cmd /c if exist $(DEST2) cp $@ $(DEST2)

scsitape.shr: $(Files)
	shar $(Files) > $@

scsitape.lzh: $(Files)
	lh2 a $@ $(Files)

header.obj : dd-head.inc dd-segs.inc

init.obj : init.c $(Includes)
	$(CC) -CNTITEXT $(CFLAGS) init.c -m $@

main.obj init.obj : $(Includes)
header.asm : dd-head.inc dd-segs.inc
