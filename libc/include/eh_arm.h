#ifndef DAVECC_EH_ARM_H
#define DAVECC_EH_ARM_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
  const uint8_t* start;
  const uint8_t* end;
} DaveARMExidxRange;

typedef struct {
  const uint8_t* start;
  const uint8_t* end;
} DaveARMExtabRange;

typedef enum {
  _US_VIRTUAL_UNWIND_FRAME = 0,
  _US_UNWIND_FRAME_STARTING = 1,
  _US_UNWIND_FRAME_RESUME = 2,
  _US_ACTION_MASK = 3,
  _US_FORCE_UNWIND = 8,
} _Unwind_State;

typedef enum {
  _UVRSC_CORE = 0,
  _UVRSC_VFP = 1,
  _UVRSC_WMMXD = 3,
  _UVRSC_PSEUDO = 4,
} _Unwind_VRS_RegClass;

typedef enum {
  _UVRSD_UINT32 = 0,
  _UVRSD_VFPX = 1,
  _UVRSD_UINT64 = 3,
  _UVRSD_FLOAT = 4,
  _UVRSD_DOUBLE = 5,
} _Unwind_VRS_DataRepresentation;

typedef enum {
  _UVRSR_OK = 0,
  _UVRSR_NOT_IMPLEMENTED = 1,
  _UVRSR_FAILED = 2,
} _Unwind_VRS_Result;

#define DAVE_ARM_R_IP 12
#define DAVE_ARM_R_SP 13
#define DAVE_ARM_R_LR 14
#define DAVE_ARM_R_PC 15

typedef struct _Unwind_Control_Block _Unwind_Exception;

typedef struct _Unwind_Control_Block {
  char exception_class[8];
  void (*exception_cleanup)(int, struct _Unwind_Control_Block*);
  struct {
    unsigned long reserved1;
    unsigned long reserved2;
    unsigned long reserved3;
    unsigned long reserved4;
    unsigned long reserved5;
  } unwinder_cache;
  struct {
    unsigned long reserved1;
    unsigned long reserved2;
  } barrier_cache;
  struct {
    unsigned long sp;
    unsigned long bitpattern[5];
  } cleanup_cache;
  struct {
    unsigned long fnstart;
    const uint32_t* ehtp;
    unsigned long additional;
    unsigned long reserved1;
  } pr_cache;
} _Unwind_Control_Block;

typedef struct _Unwind_Context _Unwind_Context;

struct _Unwind_Context {
  uint32_t vrs[16];
  uint64_t vfp_d[16];
  int installed_cleanup;
  uintptr_t resume_scope_start;
  uintptr_t resume_scope_end;
  uintptr_t fnstart;
  uintptr_t fnend;
  const uint8_t* lsda;
  void* personality;
  const uint32_t* ehtp;
  uint32_t eht_flags;
};

typedef struct {
  uintptr_t pc_begin;
  uintptr_t pc_end;
  void* personality;
  const uint8_t* lsda;
  const uint32_t* ehtp;
  uint32_t eht_flags;
  int is_compact_inline;
} DaveARMUnwindInfo;

int DaveARMExidxGetRange(DaveARMExidxRange* range);
int DaveARMExtabGetRange(DaveARMExtabRange* range);
int DaveARMExidxCountEntries(void);
int DaveARMFindUnwindInfo(uintptr_t pc, uintptr_t* pc_begin,
                          uintptr_t* pc_end, const uint8_t** lsda);
int DaveARMFindUnwindInfoInRange(uintptr_t pc, const uint8_t* exidx_start,
                                 const uint8_t* exidx_end,
                                 uintptr_t* pc_begin, uintptr_t* pc_end,
                                 const uint8_t** lsda);
int DaveARMLookupUnwindInfo(uintptr_t pc, DaveARMUnwindInfo* out);

#define DaveARMCanonicalGuestPC(pc)                                       \
  (((pc) != 0 && ((uintptr_t)(pc) >> 28) == 0) ? ((uintptr_t)(pc) | 0x40000000u) \
                                               : (uintptr_t)(pc))

uint64_t DaveARMExceptionClass(const _Unwind_Exception* exc);
void DaveARMSetExceptionClass(_Unwind_Exception* exc, uint64_t value);

int _Unwind_VRS_Get(const _Unwind_Context* context,
                    _Unwind_VRS_RegClass regclass,
                    uint32_t regno,
                    _Unwind_VRS_DataRepresentation representation,
                    void* valuep);
int _Unwind_VRS_Set(_Unwind_Context* context,
                    _Unwind_VRS_RegClass regclass,
                    uint32_t regno,
                    _Unwind_VRS_DataRepresentation representation,
                    void* valuep);
int _Unwind_VRS_Pop(_Unwind_Context* context,
                    _Unwind_VRS_RegClass regclass,
                    uint32_t discriminator,
                    _Unwind_VRS_DataRepresentation representation);

int _Unwind_VRS_Interpret(_Unwind_Context* context,
                          const uint32_t* data,
                          size_t offset, size_t len);

int DaveARMUnwindStep(_Unwind_Context* context);

void DaveARMInitContext(_Unwind_Context* context);
void DaveARMInitContextFromHardware(_Unwind_Context* context);

int DaveARMRaiseException(_Unwind_Exception* exc);
void DaveARMResume(_Unwind_Exception* exc);

typedef int (*DaveARMPersonalityFn)(int state, _Unwind_Exception* ucb,
                                    _Unwind_Context* context);

int __aeabi_unwind_cpp_pr0(int state, _Unwind_Exception* ucb,
                           _Unwind_Context* context);
int __aeabi_unwind_cpp_pr1(int state, _Unwind_Exception* ucb,
                           _Unwind_Context* context);
int __aeabi_unwind_cpp_pr2(int state, _Unwind_Exception* ucb,
                           _Unwind_Context* context);

#endif /* DAVECC_EH_ARM_H */
