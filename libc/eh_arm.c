#include <unwind.h>

#include <lsda.h>
#include <stdlib.h>
#include <string.h>

#include "eh_cxa_internal.h"

#if defined(__arm__)

typedef struct {
  uintptr_t eh_frame_start;
  uintptr_t eh_frame_end;
  uintptr_t gcc_except_table_start;
  uintptr_t gcc_except_table_end;
  uintptr_t arm_exidx_start;
  uintptr_t arm_exidx_end;
} DaveEHModuleRange;

extern char __exidx_start[];
extern char __exidx_end[];
extern char __extab_start[];
extern char __extab_end[];

extern void __davecc_arm_capture_vrs(uint32_t* vrs);
extern void __davecc_arm_install_from_vrs(uint32_t* vrs, uint32_t sp, uint32_t fp,
                                          uint32_t landing_pad);

#if defined(__risc_v__)
#define DAVECC_ARM_UNWIND_TLS __thread
#else
#define DAVECC_ARM_UNWIND_TLS
#endif

static DAVECC_ARM_UNWIND_TLS _Unwind_Context g_arm_unwind_context;
static DAVECC_ARM_UNWIND_TLS _Unwind_Context g_arm_resume_context;
static DAVECC_ARM_UNWIND_TLS _Unwind_Exception* g_arm_resume_exception;

extern uint32_t g_davecc_arm_transfer_vrs[16];

int DaveARMExidxGetRange(DaveARMExidxRange* range) {
  if (range == 0) {
    return 0;
  }
  range->start = (const uint8_t*)DaveARMCanonicalGuestPC((uintptr_t)__exidx_start);
  range->end = (const uint8_t*)DaveARMCanonicalGuestPC((uintptr_t)__exidx_end);
  return range->start < range->end;
}

int DaveARMExtabGetRange(DaveARMExtabRange* range) {
  if (range == 0) {
    return 0;
  }
  range->start = (const uint8_t*)DaveARMCanonicalGuestPC((uintptr_t)__extab_start);
  range->end = (const uint8_t*)DaveARMCanonicalGuestPC((uintptr_t)__extab_end);
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
  return DaveARMCanonicalGuestPC((uintptr_t)place + (intptr_t)offset);
}

static void* PersonalityFromIndex(unsigned index) {
  switch (index) {
    case 0:
      return (void*)__aeabi_unwind_cpp_pr0;
    case 1:
      return (void*)__aeabi_unwind_cpp_pr1;
    case 2:
      return (void*)__aeabi_unwind_cpp_pr2;
    default:
      return 0;
  }
}

static void DecodeEHTEntry(const uint32_t* ehtp, const uint32_t** out_data,
                           size_t* out_offset, size_t* out_len,
                           void** out_personality, int* out_compact_inline) {
  const uint32_t* data = ehtp;
  size_t offset = 0;
  size_t len = 0;
  void* personality = 0;
  int compact_inline = 0;

  if (ehtp == 0) {
    goto done;
  }

  if ((*ehtp & 0x80000000u) == 0) {
    if (*ehtp == 0) {
      personality = PersonalityFromIndex(0);
    } else {
      personality = (void*)DecodePrel31(ehtp);
      if (personality == 0) {
        personality = PersonalityFromIndex(1);
      }
    }
    data = ehtp + 1;
    offset = 1;
    len = ((((size_t)data[0] >> 24) & 0xffu) + 1u) * 4u;
  } else {
    unsigned personality_index = (unsigned)((*ehtp >> 24) & 0x0fu);
    compact_inline = 1;
    personality = PersonalityFromIndex(personality_index);
    switch (personality_index) {
      case 0:
        len = 4;
        offset = 1;
        break;
      case 1:
      case 2:
        len = 4u + 4u * (((*ehtp >> 16) & 0xffu));
        offset = 2;
        break;
      default:
        data = 0;
        len = 0;
        goto done;
    }
  }

done:
  if (out_data != 0) {
    *out_data = data;
  }
  if (out_offset != 0) {
    *out_offset = offset;
  }
  if (out_len != 0) {
    *out_len = len;
  }
  if (out_personality != 0) {
    *out_personality = personality;
  }
  if (out_compact_inline != 0) {
    *out_compact_inline = compact_inline;
  }
}

static const uint8_t* LSDAFromGenericExtab(const uint32_t* extab) {
  const uint32_t* data;
  size_t offset;
  size_t len;
  if (extab == 0 || extab[0] == 0) {
    return 0;
  }
  DecodeEHTEntry(extab, &data, &offset, &len, 0, 0);
  if (data == 0 || len < 4) {
    return 0;
  }
  return (const uint8_t*)(extab + 1 + (len / 4));
}

static int LookupInExidxRange(uintptr_t pc, const uint8_t* exidx_start,
                              const uint8_t* exidx_end, DaveARMUnwindInfo* out) {
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

  if (out != 0) {
    out->pc_begin = start;
    out->pc_end = end;
    out->ehtp = selected + 1;
    out->eht_flags = selected[1];
    out->is_compact_inline = (selected[1] & 0x80000000u) != 0;
    out->personality = 0;
    out->lsda = 0;
    if (out->is_compact_inline) {
      out->personality =
          PersonalityFromIndex((unsigned)((selected[1] >> 24) & 0x0fu));
    } else {
      const uint32_t* extab = (const uint32_t*)DecodePrel31(selected + 1);
      out->ehtp = extab;
      if (extab != 0 && extab[0] != 0) {
        out->personality = (void*)DecodePrel31(extab);
      }
      out->lsda = LSDAFromGenericExtab(extab);
    }
  }
  return 1;
}

int DaveARMFindUnwindInfoInRange(uintptr_t pc, const uint8_t* exidx_start,
                                 const uint8_t* exidx_end,
                                 uintptr_t* pc_begin, uintptr_t* pc_end,
                                 const uint8_t** lsda) {
  DaveARMUnwindInfo info;
  if (!LookupInExidxRange(pc, exidx_start, exidx_end, &info)) {
    return 0;
  }
  if (pc_begin != 0) {
    *pc_begin = info.pc_begin;
  }
  if (pc_end != 0) {
    *pc_end = info.pc_end;
  }
  if (lsda != 0) {
    *lsda = info.lsda;
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

int DaveARMLookupUnwindInfo(uintptr_t pc, DaveARMUnwindInfo* out) {
  DaveARMExidxRange range;
  size_t count;
  size_t i;

  if (out == 0) {
    return 0;
  }
  if (DaveARMExidxGetRange(&range) &&
      LookupInExidxRange(pc, range.start, range.end, out)) {
    return 1;
  }
  extern size_t __davecc_eh_module_count;
  extern DaveEHModuleRange __davecc_eh_modules[];
  count = __davecc_eh_module_count;
  if (count > 32) {
    count = 32;
  }
  for (i = 0; i < count; i++) {
    if (LookupInExidxRange(pc, (const uint8_t*)__davecc_eh_modules[i].arm_exidx_start,
                           (const uint8_t*)__davecc_eh_modules[i].arm_exidx_end,
                           out)) {
      return 1;
    }
  }
  return 0;
}

uint64_t DaveARMExceptionClass(const _Unwind_Exception* exc) {
  uint64_t value = 0;
  if (exc == 0) {
    return 0;
  }
  memcpy(&value, exc->exception_class, sizeof(value));
  return value;
}

void DaveARMSetExceptionClass(_Unwind_Exception* exc, uint64_t value) {
  if (exc == 0) {
    return;
  }
  memset(exc->exception_class, 0, sizeof(exc->exception_class));
  memcpy(exc->exception_class, &value, sizeof(value));
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
  const _Unwind_Context* ctx = context;
  if (valuep == 0 || ctx == 0) {
    return 0;
  }
  if (regclass == _UVRSC_CORE) {
    const uint32_t* word;
    if (representation != _UVRSD_UINT32 || regno > 15) {
      return 0;
    }
    word = VRSWord((_Unwind_Context*)ctx, regno);
    if (word == 0) {
      return 0;
    }
    *(uint32_t*)valuep = *word;
    return 1;
  }
  if (regclass == _UVRSC_VFP) {
    if (representation != _UVRSD_DOUBLE && representation != _UVRSD_VFPX) {
      return 0;
    }
    if (regno >= 16) {
      return 0;
    }
    *(uint64_t*)valuep = ctx->vfp_d[regno];
    return 1;
  }
  return 0;
}

int _Unwind_VRS_Set(_Unwind_Context* context,
                    _Unwind_VRS_RegClass regclass,
                    uint32_t regno,
                    _Unwind_VRS_DataRepresentation representation,
                    void* valuep) {
  if (valuep == 0 || context == 0) {
    return 0;
  }
  if (regclass == _UVRSC_CORE) {
    uint32_t* word;
    if (representation != _UVRSD_UINT32 || regno > 15) {
      return 0;
    }
    word = VRSWord(context, regno);
    if (word == 0) {
      return 0;
    }
    *word = *(const uint32_t*)valuep;
    return 1;
  }
  if (regclass == _UVRSC_VFP) {
    if (representation != _UVRSD_DOUBLE && representation != _UVRSD_VFPX) {
      return 0;
    }
    if (regno >= 16) {
      return 0;
    }
    context->vfp_d[regno] = *(const uint64_t*)valuep;
    return 1;
  }
  return 0;
}

static uint32_t RegisterMask(uint8_t start, uint8_t count_minus_one) {
  return ((1u << (count_minus_one + 1)) - 1u) << start;
}

static uint32_t RegisterRange(uint8_t start, uint8_t count_minus_one) {
  return ((uint32_t)start << 16) | ((uint32_t)count_minus_one + 1u);
}

int _Unwind_VRS_Pop(_Unwind_Context* context,
                    _Unwind_VRS_RegClass regclass,
                    uint32_t discriminator,
                    _Unwind_VRS_DataRepresentation representation) {
  uint32_t mask;
  uint32_t* vsp;
  if (context == 0) {
    return 0;
  }
  if (regclass == _UVRSC_CORE) {
    uint32_t cursor;
    if (representation != _UVRSD_UINT32) {
      return 0;
    }
    vsp = VRSWord(context, DAVE_ARM_R_SP);
    if (vsp == 0) {
      return 0;
    }
    mask = discriminator & 0xffffu;
    cursor = *vsp;
    for (uint32_t reg = 0; reg < 16; reg++) {
      if ((mask & (1u << reg)) == 0) {
        continue;
      }
      uint32_t* dst = VRSWord(context, reg);
      if (dst == 0) {
        return 0;
      }
      *dst = *(const uint32_t*)(uintptr_t)cursor;
      cursor += 4;
    }
    if ((mask & (1u << DAVE_ARM_R_SP)) == 0) {
      *vsp = cursor;
    }
    return 1;
  }
  if (regclass == _UVRSC_VFP) {
    uint32_t start = discriminator >> 16;
    uint32_t count = discriminator & 0xffffu;
    uint32_t* sp_word = VRSWord(context, DAVE_ARM_R_SP);
    uint32_t* sp;
    if (sp_word == 0) {
      return 0;
    }
    if (representation != _UVRSD_VFPX && representation != _UVRSD_DOUBLE) {
      return 0;
    }
    if (start + count > 16) {
      return 0;
    }
    sp = (uint32_t*)(uintptr_t)*sp_word;
    if (representation == _UVRSD_VFPX) {
      for (uint32_t i = start; i < start + count; i++) {
        context->vfp_d[i] = *(const uint64_t*)sp;
        sp += 2;
      }
      sp++;
    } else {
      for (uint32_t i = start; i < start + count; i++) {
        context->vfp_d[i] = *(const uint64_t*)sp;
        sp += 2;
      }
    }
    *sp_word = (uint32_t)(uintptr_t)sp;
    return 1;
  }
  return 0;
}

static uint8_t EHTByte(const uint32_t* data, size_t offset) {
  const uint8_t* bytes = (const uint8_t*)data;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return bytes[(offset & ~(size_t)0x03u) + (3u - (offset & (size_t)0x03u))];
#else
  return bytes[offset];
#endif
}

int _Unwind_VRS_Interpret(_Unwind_Context* context,
                                          const uint32_t* data,
                                          size_t offset, size_t len) {
  int wrote_pc = 0;
  int finish = 0;

  if (context == 0 || data == 0) {
    return _URC_FATAL_PHASE1_ERROR;
  }

  while (offset < len && !finish) {
    uint8_t byte = EHTByte(data, offset++);
    if ((byte & 0x80u) == 0) {
      uint32_t sp;
      if (!_Unwind_VRS_Get(context, _UVRSC_CORE, DAVE_ARM_R_SP, _UVRSD_UINT32,
                          &sp)) {
        return _URC_FATAL_PHASE1_ERROR;
      }
      if (byte & 0x40u) {
        sp -= (((uint32_t)byte & 0x3fu) << 2) + 4u;
      } else {
        sp += ((uint32_t)byte << 2) + 4u;
      }
      if (!_Unwind_VRS_Set(context, _UVRSC_CORE, DAVE_ARM_R_SP, _UVRSD_UINT32,
                           &sp)) {
        return _URC_FATAL_PHASE1_ERROR;
      }
    } else {
      switch (byte & 0xf0u) {
        case 0x80: {
          if (offset >= len) {
            return _URC_FATAL_PHASE1_ERROR;
          }
          uint32_t registers =
              (((uint32_t)byte & 0x0fu) << 12) |
              (((uint32_t)EHTByte(data, offset++)) << 4);
          if (registers == 0) {
            return _URC_FATAL_PHASE1_ERROR;
          }
          if (registers & (1u << DAVE_ARM_R_PC)) {
            wrote_pc = 1;
          }
          if (!_Unwind_VRS_Pop(context, _UVRSC_CORE, registers,
                               _UVRSD_UINT32)) {
            return _URC_FATAL_PHASE1_ERROR;
          }
          break;
        }
        case 0x90: {
          uint8_t reg = byte & 0x0fu;
          if (reg == DAVE_ARM_R_SP || reg == DAVE_ARM_R_PC) {
            return _URC_FATAL_PHASE1_ERROR;
          }
          uint32_t sp;
          if (!_Unwind_VRS_Get(context, _UVRSC_CORE, reg, _UVRSD_UINT32, &sp) ||
              !_Unwind_VRS_Set(context, _UVRSC_CORE, DAVE_ARM_R_SP,
                               _UVRSD_UINT32, &sp)) {
            return _URC_FATAL_PHASE1_ERROR;
          }
          break;
        }
        case 0xa0: {
          uint32_t registers = RegisterMask(4, byte & 0x07u);
          if (byte & 0x08u) {
            registers |= 1u << DAVE_ARM_R_LR;
          }
          if (!_Unwind_VRS_Pop(context, _UVRSC_CORE, registers,
                               _UVRSD_UINT32)) {
            return _URC_FATAL_PHASE1_ERROR;
          }
          break;
        }
        case 0xb0: {
          switch (byte) {
            case 0xb0:
              finish = 1;
              break;
            case 0xb1: {
              if (offset >= len) {
                return _URC_FATAL_PHASE1_ERROR;
              }
              uint8_t registers = EHTByte(data, offset++);
              if ((registers & 0xf0u) != 0 || registers == 0) {
                return _URC_FATAL_PHASE1_ERROR;
              }
              if (!_Unwind_VRS_Pop(context, _UVRSC_CORE, registers,
                                   _UVRSD_UINT32)) {
                return _URC_FATAL_PHASE1_ERROR;
              }
              break;
            }
            case 0xb2: {
              uint32_t addend = 0;
              uint32_t shift = 0;
              while (1) {
                if (offset >= len) {
                  return _URC_FATAL_PHASE1_ERROR;
                }
                uint32_t v = EHTByte(data, offset++);
                addend |= (v & 0x7fu) << shift;
                if ((v & 0x80u) == 0) {
                  break;
                }
                shift += 7;
              }
              uint32_t sp;
              if (!_Unwind_VRS_Get(context, _UVRSC_CORE, DAVE_ARM_R_SP,
                                   _UVRSD_UINT32, &sp)) {
                return _URC_FATAL_PHASE1_ERROR;
              }
              sp += 0x204u + (addend << 2);
              if (!_Unwind_VRS_Set(context, _UVRSC_CORE, DAVE_ARM_R_SP,
                                   _UVRSD_UINT32, &sp)) {
                return _URC_FATAL_PHASE1_ERROR;
              }
              break;
            }
            case 0xb3:
              if (offset >= len) {
                return _URC_FATAL_PHASE1_ERROR;
              }
              {
                uint8_t v = EHTByte(data, offset++);
                if (!_Unwind_VRS_Pop(context, _UVRSC_VFP,
                                     RegisterRange(v >> 4, v & 0x0fu),
                                     _UVRSD_VFPX)) {
                  return _URC_FATAL_PHASE1_ERROR;
                }
              }
              break;
            case 0xb4:
            case 0xb5:
            case 0xb6:
            case 0xb7:
              return _URC_FATAL_PHASE1_ERROR;
            default:
              if (!_Unwind_VRS_Pop(context, _UVRSC_VFP,
                                   RegisterRange(8, byte & 0x07u),
                                   _UVRSD_VFPX)) {
                return _URC_FATAL_PHASE1_ERROR;
              }
              break;
          }
          break;
        }
        case 0xc0: {
          if (byte == 0xc8 || byte == 0xc9) {
            if (offset >= len) {
              return _URC_FATAL_PHASE1_ERROR;
            }
            uint8_t v = EHTByte(data, offset++);
            uint8_t start =
                (uint8_t)(((byte == 0xc8) ? 16u : 0u) + (v >> 4));
            uint8_t count_minus_one = v & 0x0fu;
            if (start + count_minus_one >= 32) {
              return _URC_FATAL_PHASE1_ERROR;
            }
            if (!_Unwind_VRS_Pop(context, _UVRSC_VFP,
                                 RegisterRange(start, count_minus_one),
                                 _UVRSD_DOUBLE)) {
              return _URC_FATAL_PHASE1_ERROR;
            }
          } else {
            return _URC_FATAL_PHASE1_ERROR;
          }
          break;
        }
        case 0xd0:
          if ((byte & 0x08u) != 0) {
            return _URC_FATAL_PHASE1_ERROR;
          }
          if (!_Unwind_VRS_Pop(context, _UVRSC_VFP,
                               RegisterRange(8, byte & 0x07u), _UVRSD_DOUBLE)) {
            return _URC_FATAL_PHASE1_ERROR;
          }
          break;
        default:
          return _URC_FATAL_PHASE1_ERROR;
      }
    }
  }

  if (!wrote_pc) {
    uint32_t lr;
    if (!_Unwind_VRS_Get(context, _UVRSC_CORE, DAVE_ARM_R_LR, _UVRSD_UINT32,
                         &lr)) {
      return _URC_FATAL_PHASE1_ERROR;
    }
    if (!_Unwind_VRS_Set(context, _UVRSC_CORE, DAVE_ARM_R_IP, _UVRSD_UINT32,
                         &lr)) {
      return _URC_FATAL_PHASE1_ERROR;
    }
    if (!_Unwind_VRS_Set(context, _UVRSC_CORE, DAVE_ARM_R_PC, _UVRSD_UINT32,
                         &lr)) {
      return _URC_FATAL_PHASE1_ERROR;
    }
  }
  return _URC_CONTINUE_UNWIND;
}

void DaveARMInitContext(_Unwind_Context* context) {
  if (context == 0) {
    return;
  }
  memset(context, 0, sizeof(*context));
}

void DaveARMInitContextFromHardware(_Unwind_Context* context) {
  if (context == 0) {
    return;
  }
  DaveARMInitContext(context);
  __davecc_arm_capture_vrs(context->vrs);
  if (context->vrs[DAVE_ARM_R_LR] >= 4) {
    context->vrs[DAVE_ARM_R_PC] = context->vrs[DAVE_ARM_R_LR] - 4;
  }
}

static void BindFrameInfo(_Unwind_Context* context, uintptr_t pc) {
  DaveARMUnwindInfo info;
  if (context == 0) {
    return;
  }
  if (!DaveARMLookupUnwindInfo(pc, &info)) {
    context->fnstart = 0;
    context->fnend = 0;
    context->lsda = 0;
    context->personality = 0;
    context->ehtp = 0;
    context->eht_flags = 0;
    return;
  }
  context->fnstart = DaveARMCanonicalGuestPC(info.pc_begin);
  context->fnend = DaveARMCanonicalGuestPC(info.pc_end);
  context->lsda = (const uint8_t*)DaveARMCanonicalGuestPC((uintptr_t)info.lsda);
  context->personality = info.personality;
  context->ehtp = info.ehtp;
  context->eht_flags = info.eht_flags;
}

int DaveARMUnwindStep(_Unwind_Context* context) {
  const uint32_t* data = 0;
  size_t offset = 0;
  size_t len = 0;
  _Unwind_Reason_Code reason;
  uint32_t pc;

  if (context == 0 || context->ehtp == 0) {
    return 0;
  }

  DecodeEHTEntry(context->ehtp, &data, &offset, &len, 0, 0);
  if (data == 0 || len == 0) {
    return 0;
  }

  reason = _Unwind_VRS_Interpret(context, data, offset, len);
  if (reason != _URC_CONTINUE_UNWIND) {
    return 0;
  }

  pc = context->vrs[DAVE_ARM_R_PC];
  if (pc == 0) {
    pc = context->vrs[DAVE_ARM_R_LR];
    context->vrs[DAVE_ARM_R_PC] = pc;
  }
  if (pc == 0) {
    return 0;
  }
  BindFrameInfo(context, pc > 0 ? pc - 1 : 0);
  return 1;
}

static void SetResumeException(_Unwind_Exception* exc) {
  memcpy((void*)&g_arm_resume_exception, &exc, sizeof(exc));
}

static void InstallLandingContext(_Unwind_Context* cleanup,
                                  _Unwind_Exception* exc) {
  SetResumeException(exc);
  uint32_t landing = cleanup->vrs[DAVE_ARM_R_PC];
  __davecc_arm_install_from_vrs(cleanup->vrs, cleanup->vrs[DAVE_ARM_R_SP],
                              cleanup->vrs[11], landing);
}

static int IsHandlerFrame(const _Unwind_Context* context,
                          _Unwind_Exception* exc) {
  if (exc == 0 || context == 0) {
    return 0;
  }
  return context->vrs[DAVE_ARM_R_PC] == (uint32_t)exc->barrier_cache.reserved1;
}

static int InvokePersonality(_Unwind_Context* context, _Unwind_Exception* exc,
                             _Unwind_Action actions,
                             _Unwind_Reason_Code* out_reason) {
  DaveARMPersonalityFn personality;
  int state;
  if (context == 0 || exc == 0 || out_reason == 0) {
    return 0;
  }
  personality = (DaveARMPersonalityFn)context->personality;
  if (personality == 0) {
    return 0;
  }
  exc->pr_cache.fnstart = context->fnstart;
  exc->pr_cache.ehtp = context->ehtp;
  exc->pr_cache.additional = context->eht_flags;
  if ((actions & _UA_SEARCH_PHASE) != 0) {
    state = _US_VIRTUAL_UNWIND_FRAME;
  } else if ((actions & _UA_HANDLER_FRAME) != 0) {
    state = _US_UNWIND_FRAME_STARTING;
  } else {
    state = _US_UNWIND_FRAME_RESUME;
  }
  *out_reason = personality(state, exc, context);
  return 1;
}

int DaveARMRaiseException(_Unwind_Exception* exc) {
  _Unwind_Context search;
  _Unwind_Context cleanup;
  uintptr_t handler_pc = 0;
  uintptr_t handler_region = 0;
  _Unwind_Reason_Code reason;

  if (exc == 0) {
    return _URC_FATAL_PHASE1_ERROR;
  }

  exc->unwinder_cache.reserved1 = 0;
  DaveARMInitContextFromHardware(&search);
  BindFrameInfo(&search, search.vrs[DAVE_ARM_R_PC]);
  g_arm_unwind_context = search;

  while (1) {
    if (InvokePersonality(&search, exc, _UA_SEARCH_PHASE, &reason)) {
      if (reason == _URC_HANDLER_FOUND) {
        handler_pc = search.vrs[DAVE_ARM_R_PC];
        handler_region = search.fnstart;
        break;
      }
      if (reason != _URC_CONTINUE_UNWIND) {
        return _URC_FATAL_PHASE1_ERROR;
      }
    }
    if (!DaveARMUnwindStep(&search)) {
      break;
    }
  }
  if (handler_pc == 0) {
    return _URC_END_OF_STACK;
  }

  exc->barrier_cache.reserved1 = handler_pc;
  exc->barrier_cache.reserved2 = handler_region;
  cleanup = g_arm_unwind_context;
  while (1) {
    _Unwind_Action actions = _UA_CLEANUP_PHASE;
    uint32_t frame_pc = cleanup.vrs[DAVE_ARM_R_PC];
    if (IsHandlerFrame(&cleanup, exc)) {
      actions = (_Unwind_Action)(actions | _UA_HANDLER_FRAME);
    }
    if (InvokePersonality(&cleanup, exc, actions, &reason)) {
      if (reason == _URC_INSTALL_CONTEXT) {
        g_arm_resume_context = cleanup;
        if (cleanup.installed_cleanup) {
          g_arm_resume_context.vrs[DAVE_ARM_R_PC] = frame_pc;
        } else {
          DaveARMUnwindStep(&g_arm_resume_context);
        }
        InstallLandingContext(&cleanup, exc);
        return _URC_FATAL_PHASE2_ERROR;
      }
      if (reason != _URC_CONTINUE_UNWIND) {
        return _URC_FATAL_PHASE2_ERROR;
      }
    }
    if (!DaveARMUnwindStep(&cleanup)) {
      break;
    }
  }
  return _URC_FATAL_PHASE2_ERROR;
}

void DaveARMResume(_Unwind_Exception* exc) {
  _Unwind_Reason_Code reason;
  _Unwind_Context context;
  if (exc == 0 || exc != g_arm_resume_exception) {
    abort();
  }
  context = g_arm_resume_context;
  while (1) {
    _Unwind_Action actions = _UA_CLEANUP_PHASE;
    uint32_t frame_pc = context.vrs[DAVE_ARM_R_PC];
    if (IsHandlerFrame(&context, exc)) {
      actions = (_Unwind_Action)(actions | _UA_HANDLER_FRAME);
    }
    if (InvokePersonality(&context, exc, actions, &reason)) {
      if (reason == _URC_INSTALL_CONTEXT) {
        g_arm_resume_context = context;
        if (context.installed_cleanup) {
          g_arm_resume_context.vrs[DAVE_ARM_R_PC] = frame_pc;
        } else {
          DaveARMUnwindStep(&g_arm_resume_context);
        }
        InstallLandingContext(&context, exc);
      }
      if (reason != _URC_CONTINUE_UNWIND) {
        abort();
      }
    }
    if (!DaveARMUnwindStep(&context)) {
      break;
    }
  }
  abort();
}

#endif /* __arm__ */
