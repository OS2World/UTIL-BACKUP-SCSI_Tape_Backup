SysInfo	struc
  secs_1970	dd	?	; seconds since 1.1.1970 00:00 (like unix)
  msec_timer	dd	?	; free running msec timer
  time_hour	db	?	; 0-23
  time_min	db	?	; 0-59
  time_sec	db	?	; 0-59
  time_hsec	db	?	; 0-99
  time_zone	dw	?	; timezone, minutes from GMT
  tick_time	dw	?	; length of timer tick in 1/10 msec
  date_day	db	?	; 1-31
  date_month	db	?	; 1-12
  date_year	dw	?	; 1980-2079

  os2_major	db	?	; major version letter
  os2_minor	db	?	; minor version letter
  os2_revision	db	?	; revision letter

  fg_scrn_grp	db	?	; foreground screen group
  max_scrn_grps	db	?	; max no of screen groups

  huge_shift	db	?	; shift count for huge segments
  prot_only	db	?	; protected-mode only flag
  last_pid	dw	?	; process ID of last call of KbdCharIn

  dyn_sched	db	?	; dynamic scheduling enabled
  max_wait	db	?	; max wait (seconds)
  min_sched	dw	?	; min timeslice (msec)
  max_sched	dw	?	; max timeslice (msec)

  boot_drive	db	?	; boot drive (1=A)
  drive_map	db	32 dup(?)
SysInfo	ends
