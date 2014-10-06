SCSI_LVL = 0
# SCSI_LVL = 1	# conforms to SCSI 1 (IBM)

DEBUG   = /DDEBUG
VERSION = /DJR=0

ASM = masm
LINK = link

ASMFLAGS = /Mx /T /P $(DEBUG) $(VERSION) /DSCSI_LVL=$(SCSI_LVL)
LINKFLAGS = /nod /noi /map
LINKLIBS = c:\os2\doscalls.lib

OBJECTS = st_tape.obj st_io.obj debug.obj

ZIPFILES = debug.inc defs.inc devhdr.inc devhlp.inc devsym.inc dosmac.inc \
	error.inc sysinfo.inc \
	debug.asm st_io.asm st_tape.asm \
	st_tape.def \
	st01.doc st_tape.doc \
	sense.c reset.c trace.c \
	makefile.mk st01fix.txt \
	st_tape.sys trace.exe

driver : st_tape.sys
zip : st01tape.lzh

st_tape.sys : $(OBJECTS)
	$(LINK) $(LINKFLAGS) $(OBJECTS),$@,st_tape,$(LINKLIBS), st_tape.def;
	cp st_tape.sys c:/sysos2

$(OBJECTS) : defs.inc devsym.inc

st01tape.lzh : $(ZIPFILES)
	lh2 a $@ $(ZIPFILES)

%.obj : %.asm
	masm $(ASMFLAGS) $<, $*, $*, nul

trace.exe : trace.c
	ccm $(CFLAGS) trace.c -o $@
