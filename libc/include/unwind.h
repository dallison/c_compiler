#ifndef DAVECC_UNWIND_H
#define DAVECC_UNWIND_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <eh_frame.h>

#if !defined(__arm__)
typedef struct _Unwind_Exception _Unwind_Exception;
typedef struct _Unwind_Context _Unwind_Context;
#endif

typedef unsigned long long _Unwind_Exception_Class;
typedef unsigned long _Unwind_Word;
typedef long _Unwind_Sword;

typedef enum {
  _URC_NO_REASON = 0,
  _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
  _URC_FATAL_PHASE2_ERROR = 2,
  _URC_FATAL_PHASE1_ERROR = 3,
  _URC_NORMAL_STOP = 4,
  _URC_END_OF_STACK = 5,
  _URC_HANDLER_FOUND = 6,
  _URC_INSTALL_CONTEXT = 7,
  _URC_CONTINUE_UNWIND = 8
} _Unwind_Reason_Code;

typedef enum {
  _UA_SEARCH_PHASE = 1,
  _UA_CLEANUP_PHASE = 2,
  _UA_HANDLER_FRAME = 4,
  _UA_FORCE_UNWIND = 8
} _Unwind_Action;

#if defined(__arm__)
#include <eh_arm.h>
#endif

typedef _Unwind_Reason_Code (*_Unwind_Personality_Fn)(int version,
                                                      _Unwind_Action actions,
                                                      _Unwind_Exception_Class
                                                          exception_class,
                                                      _Unwind_Exception* exc,
                                                      _Unwind_Context* ctx);

typedef void (*_Unwind_Exception_Cleanup_Fn)(
    _Unwind_Reason_Code reason, _Unwind_Exception* exc);

typedef unsigned long _Unwind_Ptr;

#if defined(__x86_64__) || defined(__aarch64__)
#define DAVE_EH_REG_EXCEPTION 0
#define DAVE_EH_REG_SELECTOR 1
#elif defined(__riscv) || defined(__risc_v__)
#define DAVE_EH_REG_EXCEPTION 10
#define DAVE_EH_REG_SELECTOR 11
#else
#define DAVE_EH_REG_EXCEPTION 0
#define DAVE_EH_REG_SELECTOR 1
#endif

#if defined(__arm__)
int DaveUnwindEHExceptionReg(void);
int DaveUnwindEHSelectorReg(void);
#else
#define DaveUnwindEHExceptionReg() DAVE_EH_REG_EXCEPTION
#define DaveUnwindEHSelectorReg() DAVE_EH_REG_SELECTOR
#endif

#if !defined(__arm__)
struct _Unwind_Exception {
  _Unwind_Exception_Class exception_class;
  _Unwind_Exception_Cleanup_Fn exception_cleanup;
  _Unwind_Word private_1;
  _Unwind_Word private_2;
};
#endif

_Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Exception* exc);
void _Unwind_Resume(_Unwind_Exception* exc);
void _Unwind_DeleteException(_Unwind_Exception* exc);
uintptr_t _Unwind_GetIP(_Unwind_Context* ctx);
void _Unwind_SetIP(_Unwind_Context* ctx, uintptr_t ip);
uintptr_t _Unwind_GetIPInfo(_Unwind_Context* ctx, int* ip_before_insn);
_Unwind_Word _Unwind_GetGR(_Unwind_Context* ctx, int index);
void _Unwind_SetGR(_Unwind_Context* ctx, int index, _Unwind_Word value);
uintptr_t _Unwind_GetCFA(_Unwind_Context* ctx);
uintptr_t _Unwind_GetLanguageSpecificData(_Unwind_Context* ctx);
uintptr_t _Unwind_GetRegionStart(_Unwind_Context* ctx);

void DaveUnwindSetInstalledCleanup(_Unwind_Context* ctx, int is_cleanup);
void DaveUnwindSetContext(uintptr_t pc, uintptr_t rsp, uintptr_t rbp);
void DaveUnwindSetRegisters(const DaveEHFrameRegisters* regs);

#if defined(__arm__)
int __gxx_personality_v0(int state, _Unwind_Exception* exc,
                         _Unwind_Context* ctx);
#else
_Unwind_Reason_Code __gxx_personality_v0(int version, _Unwind_Action actions,
                                         _Unwind_Exception_Class exception_class,
                                         _Unwind_Exception* exc,
                                         _Unwind_Context* ctx);
#endif

#ifdef __cplusplus
}
#endif

#endif /* DAVECC_UNWIND_H */
