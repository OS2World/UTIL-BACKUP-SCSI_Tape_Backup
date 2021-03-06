;
; Copyright (c) Microsoft Corporation,  1988
;
;

;**     Device Table Record
;
;       Devices are described by a chain of these records

SysDev  STRUC
SDevNext        DD      ?       ; Pointer to next device header
SDevAtt         DW      ?       ; Attributes of the device
SDevStrat       DW      ?       ; Strategy entry point
SDevInt         DW      ?       ; Interrupt entry point
SDevName        DB      '        ' ; Name of device (only first byte used for block)
SDevProtCS      DW      ?       ; Protect-mode CS selecter of strategy entry pt
SDevProtDS      DW      ?       ; Protect-mode DS selecter
SDevRealCS      DW      ?       ; Real-mode CS segment of strategy entry pt
SDevRealDS      DW      ?       ; Real-mode DS segment
SysDev  ENDS

;*      Field aliases for use in loading block drivers

SDevCDPBs       equ     byte ptr SDevName+1
SDevDPBseg      equ     word ptr SDevProtCS
SDevBPBvec      equ     word ptr SDevProtDS



;**     Device Driver Type defininitions
;

DEV_CIN         EQU 0000000000000001B ;  0  Device is console in
DEV_COUT        EQU 0000000000000010B ;  1  Device is console out
DEV_NULL        EQU 0000000000000100B ;  2  Device is the Null device
DEV_CLOCK       EQU 0000000000001000B ;  3  Device is the clock device
DEV_SPEC        EQU 0000000000010000B ;  4  Devices can support INT 29h
DEV_ADD_ON      EQU 0000000000100000B ;  5  Device is add-on driver (BWS)
DEV_GIOCTL      EQU 0000000001000000B ;  6  Device supports generic ioctl
DEV_FCNLEV      EQU 0000001110000000B ; 9-7 Device function level
;                   0000010000000000B ; 10
DEV_OPEN        EQU 0000100000000000B ; 11  Accepts Open/Close (char)
DEV_REMOVABLE   EQU 0000100000000000B ; 11  Removable Media (block)
DEV_PROTECT     EQU 0001000000000000B ; 12  Let OS/2 handle sharing
DEV_TIL_BUSY    EQU 0010000000000000B ; 13  Supports output until busy
DEV_NON_IBM     EQU 0010000000000000B ; 13  Is a non IBM device (block)
DEV_IDC         EQU 0100000000000000B ; 14  Device accepts IDC request
DEV_CHAR_DEV    EQU 1000000000000000B ; 15  Device is a character device

;       Level definitions for devices

;                   0000001110000000B   ; Device function level
DEVLEV_0    EQU     0000000000000000B   ; DOS 3.0 and before 
DEVLEV_1    EQU     0000000010000000B   ; OS/2
DEVLEV_2    EQU     0000000100000000B	; OS/2, supports ioctl2
DEVLEV_3    EQU     0000000110000000B
DEVLEV_4    EQU     0000001000000000B
DEVLEV_5    EQU     0000001010000000B
DEVLEV_6    EQU     0000001100000000B
DEVLEV_7    EQU     0000001110000000B


