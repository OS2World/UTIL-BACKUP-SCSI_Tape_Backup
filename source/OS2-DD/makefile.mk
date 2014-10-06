###############################################################################
# $Id: makefile.mk,v 1.4 1993/01/15 19:34:50 AK Exp $
###############################################################################
# $Log: makefile.mk,v $
# Revision 1.4  1993/01/15  19:34:50  AK
# inc -> inc2
#
# Revision 1.3  1992/07/24  11:36:57  ak
# OS/2 2.0
# BASEDEV drivers
# VGA debugging
#
# Revision 1.2  1992/01/06  20:10:07  ak
# *** empty log message ***
#
# Revision 1.1.1.1  1992/01/06  19:55:04  ak
# Alpha version.
#
# Revision 1.1  1992/01/06  19:55:03  ak
# Initial revision
#
###############################################################################

DESTLIB = $(ROOTDIR)/usr/lib/ztc
DESTINC = $(ROOTDIR)/usr/inc2

CC = ccz
OPTIM = -O
DEBUG =
MODEL = sw
CFLAGS += -a1 $(OPTIM) $(DEBUG) -DOS2=2

CSrc	= def_init.c def_base.c def_open.c def_clos.c def_read.c def_wrt.c \
	  def_ioc.c def_ifl.c def_ist.c def_ofl.c def_ost.c def_peek.c \
	  strategy.c ddebug.c d2debug.c
AsmSrc =  lowlevel.asm
Headers = dd-head.inc dd-segs.inc macros.inc dd.h
AuxFile = makefile.mk README demo.def
Files =   $(CSrc) $(AsmSrc) $(Headers) $(AuxFile)
CObj =    {$(CSrc:b)}.obj
Objects	= {$(CSrc:b) $(AsmSrc:b)}.obj

lib:	dd.lib
shar:	dd.shr
lzh:    dd.lzh

dd.lib .LIBRARY : $(Objects)
	dolib lib $@ $?

install: dd.lib
	-cp -t dd.lib $(DESTLIB)
	-cp -t dd.h dd-head.inc dd-segs.inc $(DESTINC)

dd.shr : $(Files)
	shar $(Files) > $@

dd.lzh : $(Files)
	lh2 a $@ $(Files)

strategy.obj : dd.h dd_defs.h
lowlevel.obj : macros.inc dd-segs.inc
ddebug.obj : dd.h
# $(CObj) : dd.h
