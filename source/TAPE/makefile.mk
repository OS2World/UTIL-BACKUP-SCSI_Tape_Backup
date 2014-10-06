###############################################################################
# $Id: makefile.mk,v 1.2 1992/09/02 19:05:20 ak Exp $
###############################################################################
# $Log: makefile.mk,v $
# Revision 1.2  1992/09/02  19:05:20  ak
# Version 2.0
# - EMX version
# - AIX version
# - SCSI-2 commands
# - ADD Driver
# - blocksize support
#
# Revision 1.1.1.1  1992/01/06  20:27:25  ak
# Interface now based on ST01 and ASPI.
# AHA_DRVR no longer supported.
# Files reorganized.
#
# Revision 1.1  1992/01/06  20:27:23  ak
# Initial revision
#
###############################################################################

# MODE=r/p

CC   = ccz
MODE = p

DEBUG = -g
OPTIM =
MODEL = l
CFLAGS += $(DEBUG) $(OPTIM) -b$(MODE)
ASFLAGS += $(DEBUG)
LDFLAGS += $(DEBUG) $(OPTIM) -b$(MODE) -lx

.IF $(MODE) == p
  MODE = p
  IFC = tape
  CFLAGS += -DOS2=1
.ELSE
  MODE = r
  IFC = aspi
.END

DRIVER =	c:/system/aha_drvr.sys

INTERFACE =	scsi$(IFC).obj

ZIP =	aha1540.h aha_drvr.def aha_drvr.s $(DRIVER) aha_drvr.doc driver.h \
	aspi.h scsi*.h scsi*c tape.h tdc3600.h tape.c tctl.c \
	copying makefile.mk readme.ak tape.exe tapep.exe 

.SOURCE$O: obj$(MODE)

tape :	 tape$(MODE).exe
driver : $(DRIVER)
zip :	 tape.zip
t1 :	 t1.exe $(DRIVER)
mode2 :	 mode2.exe $(DRIVER)
vfy :	 vfy.exe $(DRIVER)

install : tape$(MODE).exe
	cp -t tape$(MODE).exe $(ROOTDIR)/usr/bin$(MODE)/tape.exe

tape.zip : $(ZIP)
	pkzip -u $@ @<+$(ZIP:t"\n")\n+> 

$(DRIVER) : aha_drvr.s aha_drvr.def aha1540.h
	cpp -DDriver=1 aha_drvr.s t:aha_drvr.t
	as t:aha_drvr.t -o t:aha_drvr.m
	iconv t:aha_drvr.m $@
	@rm -c - t:aha_drvr.t t:aha_drvr.m

tape$(MODE).exe : tctl.obj tape.obj errtab.obj $(INTERFACE)
	$(CC) $(LDFLAGS) $& -o $@

scsiaspi.obj : aspi.h scsi.h
scsitape.obj : aspi.h scsi.h tapedrvr.h

vfy.exe : vfy.obj tape.obj $(INTERFACE)
	$(CC) $(LDFLAGS) $& -o $@

t1.obj vfy.obj : tape.h
tape.obj : tape.h scsi.h tapedrvr.h
