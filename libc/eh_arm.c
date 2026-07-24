#include <eh_arm.h>
#include <stddef.h>

extern char __exidx_start[];
extern char __exidx_end[];
extern char __extab_start[];
extern char __extab_end[];

int DaveARMExidxGetRange(DaveARMExidxRange* range) {
  if (range == 0) {
    return 0;
  }
  range->start = (const uint8_t*)__exidx_start;
  range->end = (const uint8_t*)__exidx_end;
  return range->start < range->end;
}

int DaveARMExtabGetRange(DaveARMExtabRange* range) {
  if (range == 0) {
    return 0;
  }
  range->start = (const uint8_t*)__extab_start;
  range->end = (const uint8_t*)__extab_end;
  return range->start < range->end;
}

int DaveARMExidxCountEntries(void) {
  DaveARMExidxRange range;
  const uint8_t* entry;
  int count = 0;

  if (!DaveARMExidxGetRange(&range)) {
    return 0;
  }
  for (entry = range.start; entry + 8 <= range.end; entry += 8) {
    count++;
  }
  return count;
}

static uintptr_t DecodePrel31(const uint32_t* place) {
  int32_t offset = (int32_t)(*place << 1) >> 1;
  return (uintptr_t)place + (intptr_t)offset;
}

int DaveARMFindUnwindInfoInRange(uintptr_t pc, const uint8_t* exidx_start,
                                 const uint8_t* exidx_end,
                                 uintptr_t* pc_begin, uintptr_t* pc_end,
                                 const uint8_t** lsda) {
  const uint32_t* selected = 0;
  const uint32_t* entries;
  size_t count;
  if (exidx_start == 0 || exidx_end <= exidx_start) {
    return 0;
  }
  entries = (const uint32_t*)exidx_start;
  count = (size_t)(exidx_end - exidx_start) / 8;
  for (size_t i = 0; i < count; i++) {
    const uint32_t* entry = entries + i * 2;
    uintptr_t start = DecodePrel31(entry);
    if (start <= pc &&
        (selected == 0 || start > DecodePrel31(selected))) {
      selected = entry;
    }
  }
  if (selected == 0 || selected[1] == 1) {
    return 0;
  }
  uintptr_t start = DecodePrel31(selected);
  uintptr_t end = ~(uintptr_t)0;
  for (size_t i = 0; i < count; i++) {
    uintptr_t candidate = DecodePrel31(entries + i * 2);
    if (candidate > start && candidate < end) {
      end = candidate;
    }
  }
  const uint32_t* extab = (const uint32_t*)DecodePrel31(selected + 1);
  const uint8_t* table = 0;
  if (extab[0] != 0) {
    // DaveCC's generic EHABI descriptor contains the personality PREL31 word,
    // one unwind-instruction word, then a PREL31 pointer to the Itanium LSDA.
    table = (const uint8_t*)DecodePrel31(extab + 2);
  }
  if (pc_begin != 0) {
    *pc_begin = start;
  }
  if (pc_end != 0) {
    *pc_end = end;
  }
  if (lsda != 0) {
    *lsda = table;
  }
  return 1;
}

int DaveARMFindUnwindInfo(uintptr_t pc, uintptr_t* pc_begin,
                          uintptr_t* pc_end, const uint8_t** lsda) {
  DaveARMExidxRange range;
  if (!DaveARMExidxGetRange(&range)) {
    return 0;
  }
  return DaveARMFindUnwindInfoInRange(pc, range.start, range.end, pc_begin,
                                      pc_end, lsda);
}

static uint32_t* VRSWord(_Unwind_Context* context, uint32_t regno) {
  if (context == 0 || regno >= 16) {
    return 0;
  }
  return &context->vrs[regno];
}

int _Unwind_VRS_Get(const _Unwind_Context* context,
                    _Unwind_VRS_RegClass regclass,
                    uint32_t regno,
                    _Unwind_VRS_DataRepresentation representation,
                    void* valuep) {
  const uint32_t* word;
  if (valuep == 0 || regclass != _UVRSC_CORE ||
      representation != _UVRSD_UINT32) {
    return 0;
  }
  word = VRSWord((_Unwind_Context*)context, regno);
  if (word == 0) {
    return 0;
  }
  *(uint32_t*)valuep = *word;
  return 1;
}

int _Unwind_VRS_Set(_Unwind_Context* context,
                    _Unwind_VRS_RegClass regclass,
                    uint32_t regno,
                    _Unwind_VRS_DataRepresentation representation,
                    void* valuep) {
  uint32_t* word;
  if (valuep == 0 || regclass != _UVRSC_CORE ||
      representation != _UVRSD_UINT32) {
    return 0;
  }
  word = VRSWord(context, regno);
  if (word == 0) {
    return 0;
  }
  *word = *(const uint32_t*)valuep;
  return 1;
}

int _Unwind_VRS_Pop(_Unwind_Context* context,
                    _Unwind_VRS_RegClass regclass,
                    uint32_t discriminator,
                    _Unwind_VRS_DataRepresentation representation) {
  uint32_t mask;
  uint32_t* vsp;
  if (context == 0 || regclass != _UVRSC_CORE ||
      representation != _UVRSD_UINT32) {
    return 0;
  }
  vsp = VRSWord(context, 13);
  if (vsp == 0) {
    return 0;
  }
  mask = discriminator;
  for (uint32_t reg = 0; reg < 16; reg++) {
    if ((mask & (1u << reg)) == 0) {
      continue;
    }
    uint32_t* dst = VRSWord(context, reg);
    if (dst == 0) {
      return 0;
    }
    *dst = *(const uint32_t*)(uintptr_t)*vsp;
    *vsp += 4;
  }
  return 1;
}

#if 0
_Unwind_Reason_Code __davecc_arm_personality(int state,
                                             _Unwind_Action action,
                                             uint64_t exception_class,
                                             void* exception_object,
                                             _Unwind_Context* context) {
  (void)state;
  (void)action;
  (void)exception_class;
  (void)exception_object;
  (void)context;
  return _URC_CONTINUE_UNWIND;
}
#endif
