//
//  x86_machine.h
//  c_compiler
//
//  Shared x86 integer register numbers (ModR/M encoding) and compile-time
//  upper bounds for the unified i386/AMD64 backend.
//

#ifndef x86_machine_h
#define x86_machine_h

#define X86_REG_EAX 0
#define X86_REG_ECX 1
#define X86_REG_EDX 2
#define X86_REG_EBX 3
#define X86_REG_ESP 4
#define X86_REG_EBP 5
#define X86_REG_ESI 6
#define X86_REG_EDI 7
#define X86_REG_RAX X86_REG_EAX
#define X86_REG_RCX X86_REG_ECX
#define X86_REG_RDX X86_REG_EDX
#define X86_REG_RBX X86_REG_EBX
#define X86_REG_RSP X86_REG_ESP
#define X86_REG_RBP X86_REG_EBP
#define X86_REG_RSI X86_REG_ESI
#define X86_REG_RDI X86_REG_EDI
#define X86_REG_R8 8
#define X86_REG_R9 9
#define X86_REG_R10 10
#define X86_REG_R11 11
#define X86_REG_R12 12
#define X86_REG_R13 13
#define X86_REG_R14 14
#define X86_REG_R15 15
#define X86_MAX_INT_REGS 32
#define X86_MAX_FLOAT_REGS 32
#define X86_MAX_INT_ARGS 6
#define X86_MAX_FP_ARGS 8
#define X86_MAX_ALLOCATION_DEPTH 4

#endif /* x86_machine_h */
