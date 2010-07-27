/*
GPLv3 license. This is a generated file from mkinstall.pl.

This file is part of the Dtrace/Linux distribution.
Generated: 20100728 12:38:34

$Header: Last edited: 28-Jul-2010 1.1 $ 

*/

inline int R_GS = 0;
#pragma D binding "1.0" R_GS
inline int R_FS = 1;
#pragma D binding "1.0" R_FS
inline int R_ES = 2;
#pragma D binding "1.0" R_ES
inline int R_DS = 3;
#pragma D binding "1.0" R_DS

inline int R_EDI = 4;
#pragma D binding "1.0" R_EDI
inline int R_ESI = 5;
#pragma D binding "1.0" R_ESI
inline int R_EBP = 6;
#pragma D binding "1.0" R_EBP
inline int R_ESP = 7;
#pragma D binding "1.0" R_ESP
inline int R_EBX = 8;
#pragma D binding "1.0" R_EBX
inline int R_EDX = 9;
#pragma D binding "1.0" R_EDX
inline int R_ECX = 10;
#pragma D binding "1.0" R_ECX
inline int R_EAX = 11;
#pragma D binding "1.0" R_EAX

inline int R_TRAPNO = 12;
#pragma D binding "1.0" R_TRAPNO
inline int R_ERR = 13;
#pragma D binding "1.0" R_ERR
inline int R_EIP = 14;
#pragma D binding "1.0" R_EIP
inline int R_CS = 15;
#pragma D binding "1.0" R_CS
inline int R_EFL = 16;
#pragma D binding "1.0" R_EFL
inline int R_UESP = 17;
#pragma D binding "1.0" R_UESP
inline int R_SS = 18;
#pragma D binding "1.0" R_SS

inline int R_PC = R_EIP;
#pragma D binding "1.0" R_PC
inline int R_SP = R_UESP;
#pragma D binding "1.0" R_SP
inline int R_PS = R_EFL;
#pragma D binding "1.0" R_PS
inline int R_R0 = R_EAX;
#pragma D binding "1.0" R_R0
inline int R_R1 = R_EBX;
#pragma D binding "1.0" R_R1

/* amd64/64-bit registers. */

inline int R_RDI = 0;
#pragma D binding "1.0" R_RDI
inline int R_RSI = 1;
#pragma D binding "1.0" R_RSI
inline int R_RDX = 2;
#pragma D binding "1.0" R_RDX
inline int R_RCX = 3;
#pragma D binding "1.0" R_RCX
inline int R_R8 = 4;
#pragma D binding "1.0" R_R8
inline int R_R9 = 5;
#pragma D binding "1.0" R_R9
inline int R_RAX = 6;
#pragma D binding "1.0" R_RAX
inline int R_RBX = 7;
#pragma D binding "1.0" R_RBX
inline int R_RBP = 8;
#pragma D binding "1.0" R_RBP
inline int R_R10 = 9;
#pragma D binding "1.0" R_R10
inline int R_R11 = 10;
#pragma D binding "1.0" R_R11
inline int R_R12 = 11;
#pragma D binding "1.0" R_R12
inline int R_R13 = 12;
#pragma D binding "1.0" R_R13
inline int R_R14 = 13;
#pragma D binding "1.0" R_R14
inline int R_R15 = 14;
