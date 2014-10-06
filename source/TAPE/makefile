###############################################################################
# $Id: makefile,v 1.1 1992/09/02 19:05:12 ak Exp $
###############################################################################
# $Log: makefile,v $
# Revision 1.1  1992/09/02  19:05:12  ak
# Initial revision
#
###############################################################################

CC   = cc
IFC  = -aix

DEBUG =
OPTIM = -O
CFLAGS = $(DEBUG) $(OPTIM) -Dunix
LDFLAGS = $(DEBUG) $(OPTIM)

INTERFACE =	scsi$(IFC).o

tape : tctl.o tape.o errtab.o $(INTERFACE)
	$(CC) $(LDFLAGS) tctl.o tape.o errtab.o $(INTERFACE) $(LDLIBS) -o $@

scsiaspi.o : aspi.h scsi.h
scsitape.o : aspi.h scsi.h tapedrvr.h
scsi-aix.o : scsi.h tapedrvr.h
tape.o : tape.h scsi.h tapedrvr.h

install :
	/usr/ucb/install -c -s tape /usr/local/bin

