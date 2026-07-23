#ifndef DAVECC_EH_ARM_H
#define DAVECC_EH_ARM_H

#include <stdint.h>

typedef struct {
  const uint8_t* start;
  const uint8_t* end;
} DaveARMExidxRange;

typedef struct {
  const uint8_t* start;
  const uint8_t* end;
} DaveARMExtabRange;

typedef enum {
  _URC_OK = 0,
  _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
  _URC_FATAL_PHASE2_ERROR = 2,
  _URC_FATAL_PHASE1_ERROR = 3,
  _URC_NORMAL_STOP = 4,
  _URC_END_OF_STACK = 5,
  _URC_HANDLER_FOUND = 6,
  _URC_INSTALL_CONTEXT = 7,
  _URC_CONTINUE_UNWIND = 8,
} _Unwind_Reason_Code;

typedef enum {
  _UA_SEARCH_PHASE = 1,
  _UA_CLEANUP_PHASE = 2,
  _UA_HANDLER_FRAME = 4,
  _UA_FORCE_UNWIND = 8,
} _Unwind_Action;

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

#define UNWINDER_PRIVATE_DATA_SIZE 20

typedef struct {
  uint32_t vrs[16];
  uint32_t padding[UNWINDER_PRIVATE_DATA_SIZE - 16];
} _Unwind_Context;

int DaveARMExidxGetRange(DaveARMExidxRange* range);
int DaveARMExtabGetRange(DaveARMExtabRange* range);
int DaveARMExidxCountEntries(void);

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

_Unwind_Reason_Code __davecc_arm_personality(int state,
                                             _Unwind_Action action,
                                             uint64_t exception_class,
                                             void* exception_object,
                                             _Unwind_Context* context);

#endif /* DAVECC_EH_ARM_H */
