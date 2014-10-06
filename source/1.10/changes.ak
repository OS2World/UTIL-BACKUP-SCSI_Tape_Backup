------------------------------------------------------------------------
      23-24.7.92 -- moved several modifications of 1.06 into 1.10.

-- Some OS/2 include-file fixes. Indented the tower of <dirent/ndir>
includes. I was not able to read and fix it unindented.
   [`grep dirent *.c`]

-- Tape interface added. #ifdef TAPE_IO, uses remote tape interface.
   [rmt.h, buffer.c, ak_tape.c]

-- OS/2 system+hidden files supported. Use -p to enable it. Packed into
group and world execute bits.
   [create.c, extract.c, update.c, port.*]

-- OS/2 extended attributes added. Use -p to enable it. EAs are
appended as a separate entry, marked with the previously unused 'A'
tag. This tagging is partially emabled even when not compiling for
OS/2, since EAs written in OS/2 should not be restored on other
systems.
   [create.c, extract.c, list.c, diffarch.c, port.h]

-- OS/2 supports compressed operation. Added child_open for OS/2. Moved
wait() stuff from close_archive into child_close since this is
different in OS/2.
   [buffer.c]

-- OS/2 only: -z is plain compression, -Z is buffered compression
(f_buffered). -Z enables the OS/2 buffered pipe option (my own BUFFER
program). Buffered compression is enabled by default for char device as
archive.
   [tar.h, tar.c, buffer.c]

-- Added -H +host for special host backup mode (f_host). Intended for
multivolume archives. Makes archives separate files and sends them to
the host.
   [tar,c, buffer.c]

-- Added a more sophisticated intconv(). Allowed to append 'B' for
multiples of the block size, 'K' for KB (the default), 'R' for records
(512 bytes) and 'M' for MB.
   [tar.c]

-- Added +tape-directory (f_map_file, map_*). Creates a tape directory
containing the tape block number for each tape entry. On extract, a
device which supports random positioning commands can be positiond
directly to the desired file. Saves much time when restoring single
files.
   [tar.c, ak_dir.c]

-- Fixed a bug which dumped core. In some OS/2 libraries, ctime()
returns NULL from when passed a wierd date value.
   [list.c]

-- Added -Y +no-recursion flag (f_no_recursion), to optionally disable
automatic recursion on directories (create only). Only files specified
on the command line are archived.

-- Stripped drive and directory from program name.
   [tar.c]

-- Added OS/2 EMX wildcard stuff.
   [tar.c]

-- Changed lots of __MSDOS__ to MSDOS. Left __MSDOS__ only for stuff
specific to Microsoft-C.

-- Checked for gnu_dumpfile been non-NULL before trying to write to it.
Caused a core dump at end of backup when -G was specified.
   {gnu.c]

------------------------------------------------------------------------


