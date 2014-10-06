###############################################################################
# $Id: makefile.mk,v 2.1 1992/11/14 21:00:20 ak Exp $
###############################################################################
# $Log: makefile.mk,v $
# Revision 2.1  1992/11/14  21:00:20  ak
# OS/2 2.00.1 ASPI
#
# Revision 1.1.1.1  1992/01/06  20:15:57  ak
# Alpha version.
#
# Revision 1.1  1992/01/06  20:15:56  ak
# Initial revision
#
###############################################################################

CC = ccz
LK = link
OPTIM = -O
DEBUG = -DDEBUG
M = s
MODEL = $(M)w
CFLAGS += -a1 -I$(ROOTDIR)/usr/inc -I../tape $(DEBUG) $(OPTIM) $(BUGS)
ASFLAGS += -I$(ROOTDIR)\usr\inc -I..\tape
LDFLAGS	+= -gm -BL -KNOE -KNOD -f
LDLIBS += -lx dd.lib os2.lib

.SOURCE.inc :	$(ROOTDIR)/usr/inc
.SOURCE.h :	$(ROOTDIR)/usr/inc ../tape

Includes = aspitape.h aspi.h dd.h
CFiles =   main.c init.c trace.c
AsmFiles = header.asm post.asm
AuxFile =  makefile.mk README aspitape.def
Files =    aspitape.h aspi.h $(CFiles) $(AsmFiles) $(AuxFile)
Objects	=  header.obj main.obj post.obj init.obj

driver : aspitape.sys
shar : aspitape.shr
lzh: aspitape.lzh

aspitape.sys : $(Objects) makefile.mk
	$(CC) $(LDFLAGS) $(Objects) $(LDLIBS) -o $@
	cp $@ c:\sysos2

aspitape.shr: $(Files)
	shar $(Files) > $@

aspitape.lzh: $(Files)
	lh2 a $@ $(Files)

header.obj : dd-head.inc dd-segs.inc

init.obj : init.c $(Includes)
	$(CC) -CNTITEXT $(CFLAGS) init.c -m $@
