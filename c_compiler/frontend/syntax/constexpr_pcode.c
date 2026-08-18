//
//  constexpr_pcode.c
//  c_compiler
//

#include "constexpr_pcode.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "codegen.h"
#include "compiler.h"
#include "errors.h"
#include "member_pointer.h"
#include "p_code_object.h"
#include "p_code_reg_alloc.h"
#include "p_code_target.h"
#include "symbol.h"
#include "p_code_vm.h"
#include "type.h"

typedef struct {
  const char* reason;
  bool ok;
} ValidationState;

const char* ConstexprPCodeFailureReason(ConstEvalContext* ctx) {
  return ctx != NULL && ctx->pcode_failure_reason != NULL
             ? ctx->pcode_failure_reason
             : "constexpr pcode evaluation failed";
}

static bool ConstexprPCodeSuccess(ConstEvalContext* ctx) {
  ctx->pcode_failure_reason = NULL;
  ctx->pcode_failure_kind = kConstexprPCodeFailureUnsupported;
  return true;
}

static bool ConstexprPCodeFailure(ConstEvalContext* ctx,
                                  ConstexprPCodeFailureKind kind,
                                  const char* reason) {
  ctx->pcode_failure_reason =
      reason != NULL ? reason : "constexpr pcode evaluation failed";
  ctx->pcode_failure_kind = kind;
  return false;
}

static ConstexprPCodeFailureKind ConstexprPCodeVMFailureKind(
    PCodeVMStatus status, bool heap_activity) {
  if (!heap_activity) {
    return kConstexprPCodeFailureUnsupported;
  }
  switch (status) {
    case kPCodeVMStatusInvalidRead:
    case kPCodeVMStatusInvalidWrite:
    case kPCodeVMStatusInvalidFree:
      return kConstexprPCodeFailureInvalid;
    default:
      return kConstexprPCodeFailureUnsupported;
  }
}

typedef struct {
  const char* name;
  int64_t base_count;
  const void* bases;
  int64_t object_size;
  int64_t object_is_class;
} ConstexprPCodeTypeInfo;

typedef struct {
  const char* name;
  int64_t offset;
} ConstexprPCodeTypeInfoBase;

typedef struct {
  uint64_t try_start;
  uint64_t try_end;
  uint64_t catch_label;
  uint64_t catch_typeinfo;
} ConstexprPCodeExceptionRange;

typedef struct {
  unsigned char* text;
  size_t text_size;
  uint64_t text_base;
  unsigned char* rodata;
  size_t rodata_size;
  unsigned char* exception_table;
  size_t exception_table_size;
  uint64_t entry;
  bool owns_memory;
} ConstexprPCodeImage;

typedef struct {
  TypeRecord* func;
  unsigned char* text;
  size_t text_size;
  unsigned char* rodata;
  size_t rodata_size;
  unsigned char* exception_table;
  size_t exception_table_size;
  uint64_t entry_offset;
} ConstexprPCodeImageCacheEntry;

// Per-function memoization of relocatable pcode. Lowering a function body to
// pcode (GenerateFunction: IR + SSA + optimization + pcode codegen) is the
// expensive step and is context-independent, so it is cached once per function
// and reused whenever the function appears in another root's closure.
typedef struct {
  TypeRecord* func;
  PCodeObject object;
  Vector referenced;  // TypeRecord* callees referenced by the function body
} ConstexprPCodeObjectCacheEntry;

typedef struct {
  ASTNode* expression;
  TypeRecord* function;
} ConstexprPCodeThunkCacheEntry;

typedef struct {
  String name;
  unsigned char* memory;
  size_t size;
} ConstexprPCodeStaticData;

typedef struct ConstexprPCodeFreeBlock {
  size_t length;
  struct ConstexprPCodeFreeBlock* next;
} ConstexprPCodeFreeBlock;

typedef struct {
  void* memory;
  size_t size;
  size_t allocation_size;
  bool live;
} ConstexprPCodeHeapBlock;

typedef struct {
  unsigned char* memory;
  size_t size;
  TypeRecord* type;
  ConstexprObject* object;
} ConstexprPCodeAddressRegion;

typedef struct {
  uint64_t pc;
  uint64_t ap;
  uint64_t fp;
  uint64_t scope_start;
  uint64_t scope_end;
  bool active;
} ConstexprPCodeExceptionResume;

typedef enum {
  kConstexprPCodeExceptionDirect,
  kConstexprPCodeExceptionI8,
  kConstexprPCodeExceptionF4,
  kConstexprPCodeExceptionF8,
} ConstexprPCodeExceptionKind;

typedef struct {
  uint64_t typeinfo;
  uint64_t destructor;
  uint64_t direct;
  int64_t i8;
  float f4;
  double f8;
  unsigned char* storage;
  size_t storage_size;
  int64_t base_offset;
  ConstexprPCodeExceptionKind kind;
  bool handling;
  int handler_depth;
} ConstexprPCodeSavedException;

typedef struct {
  ConstexprPCodeSavedException exception;
  size_t references;
} ConstexprPCodeExceptionHandle;

typedef enum {
  kConstexprPCodeLifetimeStartAggregate,
  kConstexprPCodeLifetimePlacementConstruction,
  kConstexprPCodeLifetimeEnd,
  kConstexprPCodeUnionMemberAddress,
} ConstexprPCodeLifetimeEventKind;

typedef struct {
  uint64_t address;
  size_t size;
  uint64_t type_token;
  size_t union_member_index_plus_one;
  ConstexprPCodeLifetimeEventKind kind;
} ConstexprPCodeLifetimeEvent;

typedef struct {
  Vector heap_blocks;
  Vector exception_stack;
  Vector exception_handles;
  Vector lifetime_events;
  unsigned char* heap;
  size_t heap_size;
  ConstexprPCodeFreeBlock* free_list;
  size_t source_size_t_size;
  ConstexprPCodeImage* image;
  uint64_t exception_typeinfo;
  uint64_t exception_destructor;
  uint64_t exception_direct;
  int64_t exception_i8;
  float exception_f4;
  double exception_f8;
  unsigned char* exception_storage;
  size_t exception_storage_size;
  int64_t exception_base_offset;
  ConstexprPCodeExceptionKind exception_kind;
  bool has_exception;
  bool handling_exception;
  int exception_handler_depth;
  bool ending_exception;
  bool halt_after_exception_destructor;
  ConstexprPCodeExceptionResume resume;
} ConstexprPCodeRuntime;

static bool ConstexprPCodeCopyLifetimeEvents(
    ConstexprPCodeRuntime* runtime, uint64_t source, uint64_t destination,
    size_t size) {
  size_t original_event_count = runtime->lifetime_events.length;
  for (size_t i = 0; i < original_event_count; i++) {
    ConstexprPCodeLifetimeEvent* event =
        runtime->lifetime_events.value.p[i];
    if (event == NULL || event->address < source) {
      continue;
    }
    uint64_t offset = event->address - source;
    if (offset > size || event->size > size - offset) {
      continue;
    }
    ConstexprPCodeLifetimeEvent* copy = malloc(sizeof(*copy));
    if (copy == NULL) {
      return false;
    }
    *copy = *event;
    copy->address = destination + offset;
    VectorAppend(&runtime->lifetime_events, copy);
  }
  return true;
}

static PCodeVMStatus ConstexprEscape(PCodeVM* vm, int32_t code, void* data);
static void ConstexprPCodeRuntimeInit(ConstexprPCodeRuntime* runtime);
static void ConstexprPCodeRuntimeDestruct(ConstexprPCodeRuntime* runtime);
static bool ConstexprPCodeRuntimeHasLiveHeap(ConstexprPCodeRuntime* runtime);
static bool ConstexprPCodeNormalizeAddress(PCodeVM* vm, uint64_t raw,
                                           size_t size, bool write,
                                           uint64_t* normalized);
static Symbol* PCodeConstexprFunctionDefinition(Symbol* symbol);
static ConstexprValue* PCodeConstexprObjectSlot(ConstexprObject* object,
                                                size_t index);
static ConstexprValue* PCodeConstexprSlotForOffset(TypeRecord* type,
                                                   ConstexprObject* object,
                                                   size_t offset);
static void DeletePCodeConstexprObject(ConstexprObject* object);
static ConstexprObject* NewPCodeConstexprObject(TypeRecord* type);
static bool RegisterConstexprPCodeStaticData(Symbol* symbol);
static bool RegisterConstexprPCodeLiteral(const char* name);
static ConstexprPCodeStaticData* FindConstexprPCodeStaticData(const char* name);
static bool ConstexprPCodeEvaluateConstructorObject(ConstEvalContext* ctx,
                                                    TypeRecord* object_type,
                                                    ASTNode* node,
                                                    ConstexprObject** result);

static Vector pcode_image_cache;
static bool pcode_image_cache_initialized = false;

static Vector pcode_object_cache;
static bool pcode_object_cache_initialized = false;

static Vector pcode_static_data;
static bool pcode_static_data_initialized = false;
static Vector pcode_thunk_cache;
static bool pcode_thunk_cache_initialized = false;

#define CONSTEXPR_PCODE_HEAP_SIZE (1024 * 1024)

enum {
  kConstexprPCodeEscapeMalloc = 100,
  kConstexprPCodeEscapeFree = 101,
  kConstexprPCodeEscapeRealloc = 102,
  kConstexprPCodeEscapePlacementNew = 103,
  kConstexprPCodeEscapeThrow = 104,
  kConstexprPCodeEscapeMemcpy = 105,
  kConstexprPCodeEscapeInvalidConstantOperation = 106,
  kConstexprPCodeEscapeThrowI8 = 107,
  kConstexprPCodeEscapeThrowF4 = 108,
  kConstexprPCodeEscapeThrowF8 = 109,
  kConstexprPCodeEscapeCurrentExceptionInteger = 110,
  kConstexprPCodeEscapeCurrentExceptionF4 = 111,
  kConstexprPCodeEscapeCurrentExceptionF8 = 112,
  kConstexprPCodeEscapeCurrentExceptionPointer = 113,
  kConstexprPCodeEscapeCurrentExceptionAddress = 114,
  kConstexprPCodeEscapeResume = 115,
  kConstexprPCodeEscapeConstexprThrow = 116,
  kConstexprPCodeEscapeEndCatch = 117,
  kConstexprPCodeEscapeEndCatchComplete = 118,
  kConstexprPCodeEscapeExceptionPtrCurrent = 119,
  kConstexprPCodeEscapeExceptionPtrRetain = 120,
  kConstexprPCodeEscapeExceptionPtrRelease = 121,
  kConstexprPCodeEscapeExceptionPtrRethrow = 122,
  kConstexprPCodeEscapeUncaughtExceptions = 123,
  kConstexprPCodeEscapeStartLifetime = 124,
};

static const uint32_t constexpr_pcode_malloc_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeMalloc,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_free_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeFree,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_realloc_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeRealloc,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_placement_new_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapePlacementNew,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_start_lifetime_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeStartLifetime,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_throw_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeThrow,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_throw_i8_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeThrowI8,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_throw_f4_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeThrowF4,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_throw_f8_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeThrowF8,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_current_exception_integer_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeCurrentExceptionInteger,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_current_exception_f4_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeCurrentExceptionF4,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_current_exception_f8_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeCurrentExceptionF8,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_current_exception_pointer_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeCurrentExceptionPointer,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_current_exception_address_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeCurrentExceptionAddress,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_resume_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeResume,
};

static const uint32_t constexpr_pcode_constexpr_throw_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeConstexprThrow,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_end_catch_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeEndCatch,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_end_catch_complete_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeEndCatchComplete,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_exception_ptr_current_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeExceptionPtrCurrent,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_exception_ptr_retain_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeExceptionPtrRetain,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_exception_ptr_release_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeExceptionPtrRelease,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_exception_ptr_rethrow_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeExceptionPtrRethrow,
};

static const uint32_t constexpr_pcode_uncaught_exceptions_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeUncaughtExceptions,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_memcpy_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeMemcpy,
    (PCODE_OP(ret) << 24),
};

static const uint32_t constexpr_pcode_invalid_operation_stub[] = {
    (PCODE_OP(esc) << 24) | kConstexprPCodeEscapeInvalidConstantOperation,
};

struct ConstexprValue {
  bool is_object;
  bool is_address;
  bool is_floating;
  bool lifetime_ended;
  int64_t ivalue;
  double fvalue;
  ConstexprObject* object;
  void* address_binding;
  ConstexprValue* address_slot;
  ConstexprObject* address_object;
  size_t address_index;
  ConstexprHeapBlock* heap_block;
  size_t heap_index;
};

struct ConstexprObject {
  TypeRecord* type;
  Vector slots;
  StructMember* active_union_member;
  bool lifetime_ended;
  ConstexprObject* complete_object;
  size_t complete_offset;
};

// Defined in constexpr.c; dereference an address-valued ConstexprValue and
// read the referred-to scalar.  Declared here (rather than in constexpr.h)
// because passing ConstexprValue by value requires its complete type, which is
// private to these two translation units.
bool ConstexprValueAsInteger(ConstexprValue value, int64_t* result);
bool ConstexprValueAsFloating(ConstexprValue value, double* result);

static bool PCodeConstexprObjectIsUnion(ConstexprObject* object) {
  return object != NULL && TypeIsStructOrUnion(object->type) &&
         object->type->info.struct_info != NULL &&
         object->type->info.struct_info->is_union;
}

static void ConstexprPCodeImageInit(ConstexprPCodeImage* image) {
  image->text = NULL;
  image->text_size = 0;
  image->text_base = 0;
  image->rodata = NULL;
  image->rodata_size = 0;
  image->exception_table = NULL;
  image->exception_table_size = 0;
  image->entry = 0;
  image->owns_memory = true;
}

static void ConstexprPCodeImageDestruct(ConstexprPCodeImage* image) {
  if (image->owns_memory) {
    free(image->text);
    free(image->rodata);
    free(image->exception_table);
  }
  image->text = NULL;
  image->text_size = 0;
  image->text_base = 0;
  image->rodata = NULL;
  image->rodata_size = 0;
  image->exception_table = NULL;
  image->exception_table_size = 0;
  image->entry = 0;
  image->owns_memory = true;
}

static size_t ConstexprPCodeHostBufferSize(size_t object_size) {
  size_t size = object_size == 0 ? 1 : object_size;
  size = (size + 7) & ~(size_t)7;
  return size < 64 ? 64 : size;
}

static bool ConstexprPCodeImageCopyFromCache(
    ConstexprPCodeImageCacheEntry* entry, ConstexprPCodeImage* image) {
  image->text = entry->text;
  image->text_size = entry->text_size;
  image->text_base = (uint64_t)(uintptr_t)image->text;
  image->rodata = entry->rodata;
  image->rodata_size = entry->rodata_size;
  image->exception_table = entry->exception_table;
  image->exception_table_size = entry->exception_table_size;
  image->entry = image->text_base + entry->entry_offset;
  image->owns_memory = false;
  return true;
}

static ConstexprPCodeImageCacheEntry* ConstexprPCodeFindCachedImage(
    TypeRecord* func) {
  if (!pcode_image_cache_initialized) {
    return NULL;
  }
  for (size_t i = 0; i < pcode_image_cache.length; i++) {
    ConstexprPCodeImageCacheEntry* entry = pcode_image_cache.value.p[i];
    if (entry != NULL && entry->func == func) {
      return entry;
    }
  }
  return NULL;
}

static void ConstexprPCodeCacheImage(TypeRecord* func,
                                     ConstexprPCodeImage* image) {
  if (!pcode_image_cache_initialized) {
    VectorInit(&pcode_image_cache);
    pcode_image_cache_initialized = true;
  }
  ConstexprPCodeImageCacheEntry* entry =
      malloc(sizeof(ConstexprPCodeImageCacheEntry));
  if (entry == NULL) {
    return;
  }
  entry->func = func;
  entry->text = image->text;
  entry->text_size = image->text_size;
  entry->rodata = image->rodata;
  entry->rodata_size = image->rodata_size;
  entry->exception_table = image->exception_table;
  entry->exception_table_size = image->exception_table_size;
  entry->entry_offset = image->entry - image->text_base;
  image->owns_memory = false;
  VectorAppend(&pcode_image_cache, entry);
}

static ConstexprPCodeObjectCacheEntry* ConstexprPCodeFindCachedObject(
    TypeRecord* func) {
  if (!pcode_object_cache_initialized) {
    return NULL;
  }
  for (size_t i = 0; i < pcode_object_cache.length; i++) {
    ConstexprPCodeObjectCacheEntry* entry = pcode_object_cache.value.p[i];
    if (entry != NULL && entry->func == func) {
      return entry;
    }
  }
  return NULL;
}

static void ConstexprPCodeCacheObject(TypeRecord* func, PCodeObject* object,
                                      Vector* referenced) {
  if (!pcode_object_cache_initialized) {
    VectorInit(&pcode_object_cache);
    pcode_object_cache_initialized = true;
  }
  ConstexprPCodeObjectCacheEntry* entry = malloc(sizeof(*entry));
  if (entry == NULL) {
    return;
  }
  entry->func = func;
  if (!PCodeObjectCopy(&entry->object, object)) {
    free(entry);
    return;
  }
  VectorInit(&entry->referenced);
  for (size_t i = 0; i < referenced->length; i++) {
    VectorAppend(&entry->referenced, referenced->value.p[i]);
  }
  VectorAppend(&pcode_object_cache, entry);
}

void ConstexprPCodeClearImageCache(void) {
  if (pcode_image_cache_initialized) {
    for (size_t i = 0; i < pcode_image_cache.length; i++) {
      ConstexprPCodeImageCacheEntry* entry = pcode_image_cache.value.p[i];
      if (entry != NULL) {
        free(entry->text);
        free(entry->rodata);
        free(entry->exception_table);
        free(entry);
      }
    }
    VectorDestruct(&pcode_image_cache);
    pcode_image_cache_initialized = false;
  }
  if (pcode_object_cache_initialized) {
    for (size_t i = 0; i < pcode_object_cache.length; i++) {
      ConstexprPCodeObjectCacheEntry* entry =
          pcode_object_cache.value.p[i];
      if (entry != NULL) {
        PCodeObjectDestruct(&entry->object);
        VectorDestruct(&entry->referenced);
        free(entry);
      }
    }
    VectorDestruct(&pcode_object_cache);
    pcode_object_cache_initialized = false;
  }
  if (pcode_static_data_initialized) {
    for (size_t i = 0; i < pcode_static_data.length; i++) {
      ConstexprPCodeStaticData* entry = pcode_static_data.value.p[i];
      if (entry != NULL) {
        StringDestruct(&entry->name);
        free(entry->memory);
        free(entry);
      }
    }
    VectorDestruct(&pcode_static_data);
    pcode_static_data_initialized = false;
  }
  if (pcode_thunk_cache_initialized) {
    VectorDestructWithContents(&pcode_thunk_cache, NULL, /*free_element=*/true);
    pcode_thunk_cache_initialized = false;
  }
}

static bool VectorContainsPointer(Vector* vector, void* value) {
  for (size_t i = 0; i < vector->length; i++) {
    if (vector->value.p[i] == value) {
      return true;
    }
  }
  return false;
}

static Symbol* CollectReferencedConstexprSymbol(Symbol* symbol,
                                                Vector* referenced) {
  Symbol* callee = PCodeConstexprFunctionDefinition(symbol);
  if (callee == NULL || callee->type == NULL ||
      !callee->type->info.function.is_constexpr ||
      VectorContainsPointer(referenced, callee->type)) {
    return callee;
  }
  VectorAppend(referenced, callee->type);
  return callee;
}

static InitializedStaticVariable* FindConstexprInitializedStatic(
    Symbol* symbol) {
  if (symbol == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        compiler->initialized_static_variables.value.p[i];
    if (var != NULL && var->symbol == symbol) {
      return var;
    }
  }
  return NULL;
}

static void CollectConstexprStaticFunctionReferences(Symbol* symbol,
                                                     Vector* referenced) {
  InitializedStaticVariable* var = FindConstexprInitializedStatic(symbol);
  if (var == NULL) {
    return;
  }
  for (size_t i = 0; i < var->initializers.length; i++) {
    Initializer* init = var->initializers.value.p[i];
    if (init != NULL && init->type == kInitTypeSymbol) {
      (void)CollectReferencedConstexprSymbol(init->value.symbol, referenced);
    }
  }
}

static void CollectReferencedConstexprFunctions(PCodeGenerator* pcode,
                                                Vector* referenced) {
  for (TargetInstruction* inst = TargetFirstInstruction(&pcode->base);
       inst != NULL; inst = TargetNext(inst)) {
    if (inst->opcode == (TargetOpcode)P_OP(symbol)) {
      TargetSymbol* symbol_inst = (TargetSymbol*)inst;
      Symbol* callee =
          CollectReferencedConstexprSymbol(symbol_inst->symbol, referenced);
      if (callee != NULL && callee->type != NULL &&
          callee->type->info.function.is_constexpr) {
        symbol_inst->symbol = callee;
      } else {
        (void)RegisterConstexprPCodeStaticData(symbol_inst->symbol);
        CollectConstexprStaticFunctionReferences(symbol_inst->symbol,
                                                 referenced);
      }
    }
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      TargetInstruction* operand = inst->operand[i];
      if (operand != NULL && operand->opcode == (TargetOpcode)P_OP(symbol)) {
        TargetSymbol* symbol_operand = (TargetSymbol*)operand;
        Symbol* callee = CollectReferencedConstexprSymbol(
            symbol_operand->symbol, referenced);
        if (callee != NULL && callee->type != NULL &&
            callee->type->info.function.is_constexpr) {
          symbol_operand->symbol = callee;
        } else {
          (void)RegisterConstexprPCodeStaticData(symbol_operand->symbol);
          CollectConstexprStaticFunctionReferences(symbol_operand->symbol,
                                                   referenced);
        }
      }
    }
  }
}

static bool CompileFunctionToPCodeObject(TypeRecord* func, PCodeObject* object,
                                         Vector* referenced,
                                         const char** reason) {
  if (func == NULL || func->info.function.symbol == NULL) {
    *reason = "constexpr function has no symbol";
    return false;
  }
  // A function whose body is still being semantically analyzed has untyped
  // nodes and cannot be lowered.  This is the recursive/mutually-recursive
  // constexpr case: fail the fold cleanly here rather than relying on the
  // code-generation recovery longjmp below to catch every fatal path.
  if (VectorContainsPointer(&compiler->functions_being_analyzed, func)) {
    *reason = "callee body is still being analyzed";
    return false;
  }
  if (func->info.function.body == NULL ||
      (func->info.function.body->flags & kASTAnalyzed) == 0) {
    *reason = "callee body has not finished semantic analysis";
    return false;
  }
  Generator gen;
  GeneratorInit(&gen, &compiler->syntax, func);
  // This IR is only interpreted for constant evaluation, so suppress runtime-
  // only constructs like the noexcept terminate guard.
  gen.for_constant_evaluation = true;

  CompilerTarget* saved_target = compiler->target;
  CompilerTarget* pcode_target = NewPCodeTarget();
  compiler->target = pcode_target;
  TypeRecord* saved_current_function = compiler->current_function;
  compiler->current_function = func;

  // Constant evaluation is speculative and may reach a callee whose inline body
  // has not yet been semantically analyzed (untyped nodes).  Lowering such a
  // body would otherwise hit an assertion deep in code generation; instead we
  // arm a recovery point so those paths longjmp back here and we fail the fold
  // gracefully.  Save/restore the previous state to support nested compilation.
  bool saved_recover = compiler->constexpr_codegen_recover;
  jmp_buf saved_abort;
  memcpy(saved_abort, compiler->constexpr_codegen_abort, sizeof(jmp_buf));
  compiler->constexpr_codegen_recover = true;
  PCodeGenerator* pcode = NULL;
  if (setjmp(compiler->constexpr_codegen_abort) == 0) {
    pcode = GenerateFunction(&gen);
  } else {
    pcode = NULL;  // recovered from an otherwise-fatal code generation path
  }
  compiler->constexpr_codegen_recover = saved_recover;
  memcpy(compiler->constexpr_codegen_abort, saved_abort, sizeof(jmp_buf));

  if (pcode == NULL) {
    compiler->current_function = saved_current_function;
    compiler->target = saved_target;
    pcode_target->cleanup = NULL;
    free(pcode_target);
    GeneratorDestruct(&gen);
    *reason = "pcode lowering failed";
    return false;
  }
  if (referenced != NULL) {
    CollectReferencedConstexprFunctions(pcode, referenced);
  }
  bool built = PCodeObjectBuildFunction(object, pcode, reason);

  compiler->current_function = saved_current_function;
  compiler->target = saved_target;
  pcode_target->cleanup(pcode);
  pcode_target->cleanup = NULL;
  free(pcode_target);
  GeneratorDestruct(&gen);
  return built;
}

typedef struct {
  const char* name;
  const uint32_t* stub;
} ConstexprPCodeRuntimeSymbol;

// Keep this table sorted by name for the binary search below.  Mangled names
// are matched in full: a prefix identifies an overload set, not a particular
// function ABI.
static const ConstexprPCodeRuntimeSymbol constexpr_pcode_runtime_symbols[] = {
    {"_Z4freePv", constexpr_pcode_free_stub},
    {"_Z6mallocj", constexpr_pcode_malloc_stub},
    {"_Z6mallocm", constexpr_pcode_malloc_stub},
    {"_Z6mallocy", constexpr_pcode_malloc_stub},
    {"_Z7reallocPvj", constexpr_pcode_realloc_stub},
    {"_Z7reallocPvm", constexpr_pcode_realloc_stub},
    {"_Z7reallocPvy", constexpr_pcode_realloc_stub},
    {"_ZdaPv", constexpr_pcode_free_stub},
    {"_ZdaPvj", constexpr_pcode_free_stub},
    {"_ZdaPvm", constexpr_pcode_free_stub},
    {"_ZdaPvy", constexpr_pcode_free_stub},
    {"_ZdlPv", constexpr_pcode_free_stub},
    {"_ZdlPvj", constexpr_pcode_free_stub},
    {"_ZdlPvm", constexpr_pcode_free_stub},
    {"_ZdlPvy", constexpr_pcode_free_stub},
    {"_Znaj", constexpr_pcode_malloc_stub},
    {"_ZnajPv", constexpr_pcode_placement_new_stub},
    {"_Znam", constexpr_pcode_malloc_stub},
    {"_ZnamPv", constexpr_pcode_placement_new_stub},
    {"_Znay", constexpr_pcode_malloc_stub},
    {"_ZnayPv", constexpr_pcode_placement_new_stub},
    {"_Znwj", constexpr_pcode_malloc_stub},
    {"_ZnwjPv", constexpr_pcode_placement_new_stub},
    {"_Znwm", constexpr_pcode_malloc_stub},
    {"_ZnwmPv", constexpr_pcode_placement_new_stub},
    {"_Znwy", constexpr_pcode_malloc_stub},
    {"_ZnwyPv", constexpr_pcode_placement_new_stub},
    {"__davecc_constexpr_end_catch", constexpr_pcode_end_catch_stub},
    {"__davecc_constexpr_invalid_throw",
     constexpr_pcode_invalid_operation_stub},
    {"__davecc_constexpr_throw", constexpr_pcode_constexpr_throw_stub},
    {"__davecc_current_exception_addr",
     constexpr_pcode_current_exception_address_stub},
    {"__davecc_current_exception_f4",
     constexpr_pcode_current_exception_f4_stub},
    {"__davecc_current_exception_f8",
     constexpr_pcode_current_exception_f8_stub},
    {"__davecc_current_exception_i1",
     constexpr_pcode_current_exception_integer_stub},
    {"__davecc_current_exception_i2",
     constexpr_pcode_current_exception_integer_stub},
    {"__davecc_current_exception_i4",
     constexpr_pcode_current_exception_integer_stub},
    {"__davecc_current_exception_i8",
     constexpr_pcode_current_exception_integer_stub},
    {"__davecc_current_exception_int",
     constexpr_pcode_current_exception_integer_stub},
    {"__davecc_current_exception_object",
     constexpr_pcode_current_exception_pointer_stub},
    {"__davecc_current_exception_ptr",
     constexpr_pcode_current_exception_pointer_stub},
    {"__davecc_exception_ptr_current",
     constexpr_pcode_exception_ptr_current_stub},
    {"__davecc_exception_ptr_release",
     constexpr_pcode_exception_ptr_release_stub},
    {"__davecc_exception_ptr_retain",
     constexpr_pcode_exception_ptr_retain_stub},
    {"__davecc_exception_ptr_rethrow",
     constexpr_pcode_exception_ptr_rethrow_stub},
    {"__davecc_resume", constexpr_pcode_resume_stub},
    {"__davecc_start_lifetime", constexpr_pcode_start_lifetime_stub},
    {"__davecc_throw", constexpr_pcode_throw_stub},
    {"__davecc_throw_f4", constexpr_pcode_throw_f4_stub},
    {"__davecc_throw_f8", constexpr_pcode_throw_f8_stub},
    {"__davecc_throw_i8", constexpr_pcode_throw_i8_stub},
    {"__davecc_uncaught_exceptions",
     constexpr_pcode_uncaught_exceptions_stub},
    {"abort", constexpr_pcode_invalid_operation_stub},
    {"free", constexpr_pcode_free_stub},
    {"malloc", constexpr_pcode_malloc_stub},
    {"memcpy", constexpr_pcode_memcpy_stub},
    {"operator delete", constexpr_pcode_free_stub},
    {"operator delete[]", constexpr_pcode_free_stub},
    {"operator new", constexpr_pcode_malloc_stub},
    {"operator new[]", constexpr_pcode_malloc_stub},
    {"realloc", constexpr_pcode_realloc_stub},
};

static bool ConstexprPCodeRuntimeSymbolAddress(const char* symbol,
                                               uint64_t* address) {
  if (symbol == NULL || address == NULL) {
    return false;
  }
  size_t begin = 0;
  size_t end = sizeof(constexpr_pcode_runtime_symbols) /
               sizeof(constexpr_pcode_runtime_symbols[0]);
  while (begin < end) {
    size_t middle = begin + (end - begin) / 2;
    const ConstexprPCodeRuntimeSymbol* candidate =
        &constexpr_pcode_runtime_symbols[middle];
    int comparison = strcmp(symbol, candidate->name);
    if (comparison < 0) {
      end = middle;
    } else if (comparison > 0) {
      begin = middle + 1;
    } else {
      *address = (uint64_t)(uintptr_t)candidate->stub;
      return true;
    }
  }
  return false;
}

static bool IsConstexprPCodeRuntimeCallSymbol(Symbol* symbol) {
  if (symbol == NULL || symbol->name.value == NULL) {
    return false;
  }
  // A source definition takes precedence over a runtime hook with the same
  // external name.
  if (PCodeConstexprFunctionDefinition(symbol) != NULL) {
    return false;
  }
  const char* runtime_name = symbol->asm_name.length != 0
                                 ? symbol->asm_name.value
                                 : symbol->name.value;
  uint64_t address;
  return ConstexprPCodeRuntimeSymbolAddress(runtime_name, &address);
}

static InitializedStaticVariable* FindConstexprInitializedStaticByName(
    const char* name) {
  if (name == NULL) {
    return NULL;
  }
  char namebuf[1024];
  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        compiler->initialized_static_variables.value.p[i];
    if (var != NULL && var->symbol != NULL &&
        strcmp(TargetSymbolName(var->symbol, namebuf, sizeof(namebuf)), name) ==
            0) {
      return var;
    }
  }
  return NULL;
}

static unsigned char* DirectImageSectionMemory(
    ConstexprPCodeImage* image, PCodeObjectSection section, size_t* size) {
  switch (section) {
    case kPCodeObjectText:
      if (size != NULL) *size = image->text_size;
      return image->text;
    case kPCodeObjectROData:
      if (size != NULL) *size = image->rodata_size;
      return image->rodata;
    case kPCodeObjectExceptionTable:
      if (size != NULL) *size = image->exception_table_size;
      return image->exception_table;
  }
  if (size != NULL) *size = 0;
  return NULL;
}

static uint64_t DirectSymbolRuntimeAddress(PCodeObject* object,
                                           ConstexprPCodeImage* image,
                                           const char* name, bool* ok);

static bool StoreDirectConstexprPCodeInitializer(
    PCodeObject* object, ConstexprPCodeImage* image,
    ConstexprPCodeStaticData* entry, Initializer* init) {
  if (entry == NULL || init == NULL || init->offset < 0 ||
      (size_t)init->offset >= entry->size) {
    return false;
  }
  unsigned char* dest = entry->memory + init->offset;
  size_t available = entry->size - (size_t)init->offset;
  switch (init->type) {
    case kInitTypeByte:
      if (available < sizeof(init->value.byte)) return false;
      memcpy(dest, &init->value.byte, sizeof(init->value.byte));
      return true;
    case kInitTypeHalf:
      if (available < sizeof(init->value.half)) return false;
      memcpy(dest, &init->value.half, sizeof(init->value.half));
      return true;
    case kInitTypeWord:
      if (available < sizeof(init->value.word)) return false;
      memcpy(dest, &init->value.word, sizeof(init->value.word));
      return true;
    case kInitTypeLong:
      if (available < sizeof(init->value._long)) return false;
      memcpy(dest, &init->value._long, sizeof(init->value._long));
      return true;
    case kInitTypeSymbol: {
      char symbol_name[1024];
      const char* target_name =
          TargetSymbolName(init->value.symbol, symbol_name, sizeof(symbol_name));
      bool address_ok = true;
      uint64_t address = DirectSymbolRuntimeAddress(
          object, image, target_name, &address_ok);
      if (!address_ok) {
        address = 0;
      }
      size_t pointer_size = (size_t)SizeofPointer();
      if (available < pointer_size) return false;
      memcpy(dest, &address, pointer_size);
      return true;
    }
    case kInitTypeString:
    case kInitTypeMemory:
      return false;
  }
  return false;
}

static bool RegisterDirectConstexprPCodeGeneratedStatic(
    PCodeObject* object, ConstexprPCodeImage* image, const char* name) {
  InitializedStaticVariable* var =
      FindConstexprInitializedStaticByName(name);
  if (var == NULL) {
    return false;
  }
  size_t size = var->size == 0 ? 1 : var->size;
  ConstexprPCodeStaticData* entry = malloc(sizeof(*entry));
  unsigned char* memory = calloc(1, size);
  if (entry == NULL || memory == NULL) {
    free(entry);
    free(memory);
    return false;
  }
  StringInit(&entry->name, name);
  entry->memory = memory;
  entry->size = size;
  if (!pcode_static_data_initialized) {
    VectorInit(&pcode_static_data);
    pcode_static_data_initialized = true;
  }
  VectorAppend(&pcode_static_data, entry);
  for (size_t i = 0; i < var->initializers.length; i++) {
    if (!StoreDirectConstexprPCodeInitializer(
            object, image, entry, var->initializers.value.p[i])) {
      pcode_static_data.length--;
      StringDestruct(&entry->name);
      free(entry->memory);
      free(entry);
      return false;
    }
  }
  return true;
}

static uint64_t DirectSymbolRuntimeAddress(PCodeObject* object,
                                           ConstexprPCodeImage* image,
                                           const char* name, bool* ok) {
  PCodeObjectSymbol* object_symbol = PCodeObjectFindSymbol(object, name);
  if (object_symbol != NULL && object_symbol->defined) {
    unsigned char* memory =
        DirectImageSectionMemory(image, object_symbol->section, NULL);
    if (memory == NULL) {
      *ok = false;
      return 0;
    }
    return (uint64_t)(uintptr_t)memory + object_symbol->offset;
  }
  uint64_t runtime_address = 0;
  if (ConstexprPCodeRuntimeSymbolAddress(name, &runtime_address)) {
    return runtime_address;
  }
  ConstexprPCodeStaticData* static_data =
      FindConstexprPCodeStaticData(name);
  if (static_data == NULL && RegisterConstexprPCodeLiteral(name)) {
    static_data = FindConstexprPCodeStaticData(name);
  }
  if (static_data == NULL &&
      RegisterDirectConstexprPCodeGeneratedStatic(object, image, name)) {
    static_data = FindConstexprPCodeStaticData(name);
  }
  if (static_data != NULL) {
    return (uint64_t)(uintptr_t)static_data->memory;
  }
  PCodeObjectSymbol* symbol = PCodeObjectFindSymbol(object, name);
  if (symbol == NULL || !symbol->defined) {
    *ok = false;
    return 0;
  }
  unsigned char* memory =
      DirectImageSectionMemory(image, symbol->section, NULL);
  if (memory == NULL) {
    *ok = false;
    return 0;
  }
  return (uint64_t)(uintptr_t)memory + symbol->offset;
}

static bool CopyDirectPCodeSection(const String* section,
                                   unsigned char** memory, size_t* size,
                                   const char** reason) {
  *size = section->length;
  *memory = malloc(*size == 0 ? 1 : *size);
  if (*memory == NULL) {
    *reason = "could not allocate direct constexpr pcode section";
    return false;
  }
  memcpy(*memory, section->value, *size);
  return true;
}

static bool ApplyDirectPCodeFixup(PCodeObject* object,
                                  ConstexprPCodeImage* image,
                                  PCodeObjectFixup* fixup,
                                  const char** reason) {
  size_t section_size = 0;
  unsigned char* section = DirectImageSectionMemory(
      image, fixup->section, &section_size);
  size_t write_size =
      fixup->kind == kPCodeFixupData32 ? sizeof(uint32_t) : sizeof(uint64_t);
  if (section == NULL || fixup->offset > section_size ||
      write_size > section_size - fixup->offset) {
    *reason = "bad direct constexpr pcode fixup";
    return false;
  }
  bool ok = true;
  uint64_t symbol = DirectSymbolRuntimeAddress(
      object, image, fixup->symbol_name.value, &ok);
  if (!ok &&
      (compiler->constexpr_eval_mode == kConstexprEvalPCode ||
       compiler->constexpr_eval_mode == kConstexprEvalAudit) &&
      (compiler->current_function == NULL ||
       compiler->constant_evaluation_required_depth > 0)) {
    if (fixup->kind == kPCodeFixupCall) {
      symbol = (uint64_t)(uintptr_t)constexpr_pcode_invalid_operation_stub;
    } else if (fixup->kind == kPCodeFixupAbsolute ||
               fixup->kind == kPCodeFixupAddress ||
               fixup->kind == kPCodeFixupData64 ||
               fixup->kind == kPCodeFixupData32) {
      symbol = 1;
    } else {
      *reason = "unresolved direct constexpr pcode symbol";
      return false;
    }
  } else if (!ok) {
    *reason = "unresolved direct constexpr pcode symbol";
    return false;
  }
  unsigned char* target = section + fixup->offset;
  uint64_t value = symbol + fixup->addend;
  switch (fixup->kind) {
    case kPCodeFixupBranch:
      value -= (uint64_t)(uintptr_t)target + 8;
      memcpy(target + 4, &value, sizeof(uint32_t));
      return true;
    case kPCodeFixupAddress:
    case kPCodeFixupPCRelative:
    case kPCodeFixupCall:
    case kPCodeFixupJump:
      value -= (uint64_t)(uintptr_t)target + 12;
      memcpy(target + 4, &value, sizeof(value));
      return true;
    case kPCodeFixupAbsolute:
      memcpy(target + 4, &value, sizeof(value));
      return true;
    case kPCodeFixupData64:
      memcpy(target, &value, sizeof(value));
      return true;
    case kPCodeFixupData32: {
      uint32_t value32 = (uint32_t)value;
      memcpy(target, &value32, sizeof(value32));
      return true;
    }
  }
  *reason = "unknown direct constexpr pcode fixup";
  return false;
}

static bool FinalizeDirectPCodeImage(PCodeObject* object,
                                     const char* entry_name,
                                     ConstexprPCodeImage* image,
                                     const char** reason) {
  if (!CopyDirectPCodeSection(&object->text, &image->text,
                              &image->text_size, reason) ||
      !CopyDirectPCodeSection(&object->rodata, &image->rodata,
                              &image->rodata_size, reason) ||
      !CopyDirectPCodeSection(&object->exception_table,
                              &image->exception_table,
                              &image->exception_table_size, reason)) {
    return false;
  }
  image->text_base = (uint64_t)(uintptr_t)image->text;
  for (size_t i = 0; i < object->fixups.length; i++) {
    if (!ApplyDirectPCodeFixup(object, image, object->fixups.value.p[i],
                               reason)) {
      return false;
    }
  }
  bool ok = true;
  image->entry =
      DirectSymbolRuntimeAddress(object, image, entry_name, &ok);
  if (!ok) {
    *reason = "could not resolve direct constexpr pcode entry";
    return false;
  }
  return true;
}

static bool CompileFunctionToPCodeObjectCached(TypeRecord* func,
                                                PCodeObject* object,
                                                Vector* referenced,
                                                const char** reason) {
  ConstexprPCodeObjectCacheEntry* cached =
      ConstexprPCodeFindCachedObject(func);
  if (cached != NULL) {
    if (!PCodeObjectAppend(object, &cached->object, reason)) {
      return false;
    }
    for (size_t i = 0; i < cached->referenced.length; i++) {
      VectorAppend(referenced, cached->referenced.value.p[i]);
    }
    return true;
  }
  PCodeObject part;
  PCodeObjectInit(&part);
  Vector local_referenced;
  VectorInit(&local_referenced);
  bool ok =
      CompileFunctionToPCodeObject(func, &part, &local_referenced, reason);
  if (ok) {
    ConstexprPCodeCacheObject(func, &part, &local_referenced);
    ok = PCodeObjectAppend(object, &part, reason);
  }
  if (ok) {
    for (size_t i = 0; i < local_referenced.length; i++) {
      VectorAppend(referenced, local_referenced.value.p[i]);
    }
  }
  VectorDestruct(&local_referenced);
  PCodeObjectDestruct(&part);
  return ok;
}

static bool CompileConstexprFunctionImage(TypeRecord* func,
                                          ConstexprPCodeImage* image,
                                          const char** reason) {
  ConstexprPCodeImageCacheEntry* cached =
      ConstexprPCodeFindCachedImage(func);
  if (cached != NULL) {
    if (ConstexprPCodeImageCopyFromCache(cached, image)) {
      return true;
    }
    *reason = "could not copy cached constexpr pcode image";
    return false;
  }

  PCodeObject object;
  PCodeObjectInit(&object);
  Vector pending;
  Vector emitted;
  VectorInit(&pending);
  VectorInit(&emitted);
  VectorAppend(&pending, func);
  bool ok = true;
  for (size_t i = 0; ok && i < pending.length; i++) {
    TypeRecord* current = pending.value.p[i];
    if (VectorContainsPointer(&emitted, current)) {
      continue;
    }
    VectorAppend(&emitted, current);
    Vector referenced;
    VectorInit(&referenced);
    ok = CompileFunctionToPCodeObjectCached(current, &object, &referenced,
                                             reason);
    if (ok) {
      for (size_t j = 0; j < referenced.length; j++) {
        TypeRecord* callee = referenced.value.p[j];
        if (!VectorContainsPointer(&emitted, callee) &&
            !VectorContainsPointer(&pending, callee)) {
          VectorAppend(&pending, callee);
        }
      }
    }
    VectorDestruct(&referenced);
  }
  VectorDestruct(&pending);
  VectorDestruct(&emitted);
  if (!ok) {
    PCodeObjectDestruct(&object);
    return false;
  }

  char namebuf[256];
  const char* entry_name =
      TargetSymbolName(func->info.function.symbol, namebuf, sizeof(namebuf));
  ok = FinalizeDirectPCodeImage(&object, entry_name, image, reason);
  PCodeObjectDestruct(&object);
  if (ok) {
    ConstexprPCodeCacheImage(func, image);
  }
  return ok;
}

static size_t ConstexprPCodeArgumentSize(TypeRecord* type) {
  if (type == NULL) {
    return 0;
  }
  TypeRecordCalculateSize(type);
  if (TypeUsesFloat32Representation(type) || TypeIsInt(type) ||
      TypeIsShort(type) ||
      TypeIsCharFamily(type)) {
    return 4;
  }
  if (TypeUsesFloat64Representation(type) || TypeIsLong(type) ||
      TypeIsLongLong(type) || TypeIsPointerOrArray(type) ||
      TypeIsFunction(type)) {
    return 8;
  }
  return type->size < 4 ? 4 : type->size;
}

static bool StoreConstexprScalarBytes(TypeRecord* type, ConstexprValue* value,
                                      unsigned char* dest) {
  if (type == NULL || value == NULL || dest == NULL || value->is_object ||
      value->is_address) {
    return false;
  }
  if (TypeIsFloatingPoint(type)) {
    if (TypeUsesFloat32Representation(type)) {
      float fvalue = (float)value->fvalue;
      memcpy(dest, &fvalue, sizeof(fvalue));
      return true;
    }
    double dvalue = value->is_floating ? value->fvalue : (double)value->ivalue;
    memcpy(dest, &dvalue, sizeof(dvalue));
    return true;
  }
  int64_t ivalue = value->is_floating ? (int64_t)value->fvalue : value->ivalue;
  size_t size = type->size == 0 ? sizeof(int64_t) : type->size;
  if (size == 1) {
    int8_t v = (int8_t)ivalue;
    memcpy(dest, &v, sizeof(v));
  } else if (size == 2) {
    int16_t v = (int16_t)ivalue;
    memcpy(dest, &v, sizeof(v));
  } else if (size == 4) {
    int32_t v = (int32_t)ivalue;
    memcpy(dest, &v, sizeof(v));
  } else {
    memcpy(dest, &ivalue, sizeof(ivalue));
  }
  return true;
}

static bool StoreConstexprObjectBytes(TypeRecord* type, ConstexprObject* object,
                                      unsigned char* dest) {
  if (type == NULL || object == NULL || dest == NULL) {
    return false;
  }
  if (TypeIsFixedArray(type)) {
    size_t elem_size = type->next != NULL ? type->next->size : 0;
    for (size_t i = 0; i < object->slots.length; i++) {
      ConstexprValue* slot = object->slots.value.p[i];
      unsigned char* elem = dest + i * elem_size;
      if (slot != NULL && slot->is_object) {
        if (!StoreConstexprObjectBytes(type->next, slot->object, elem)) {
          return false;
        }
      } else if (!StoreConstexprScalarBytes(type->next, slot, elem)) {
        return false;
      }
    }
    return true;
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* str = type->info.struct_info;
    for (size_t i = 0; i < str->members.length; i++) {
      StructMember* member = str->members.value.p[i];
      if (member == NULL || member->symbol == NULL || member->is_static ||
          member->is_member_function ||
          StorageIs(member->symbol->storage, STO(typedef))) {
        continue;
      }
      if (str->is_union &&
          (object->active_union_member == NULL ||
           object->active_union_member != member)) {
        continue;
      }
      size_t slot_index = str->is_union ? 0 : member->index;
      ConstexprValue* slot = PCodeConstexprObjectSlot(object, slot_index);
      unsigned char* member_dest = dest + member->byte_offset;
      if (slot != NULL && slot->is_object) {
        if (!StoreConstexprObjectBytes(member->symbol->type, slot->object,
                                       member_dest)) {
          return false;
        }
      } else if (!StoreConstexprScalarBytes(member->symbol->type, slot,
                                           member_dest)) {
        return false;
      }
      if (str->is_union) {
        break;
      }
    }
    return true;
  }
  return false;
}

static ConstexprPCodeStaticData* FindConstexprPCodeStaticData(
    const char* name) {
  if (!pcode_static_data_initialized || name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < pcode_static_data.length; i++) {
    ConstexprPCodeStaticData* entry = pcode_static_data.value.p[i];
    if (entry != NULL && entry->name.value != NULL &&
        strcmp(entry->name.value, name) == 0) {
      return entry;
    }
  }
  return NULL;
}

static bool RegisterConstexprPCodeLiteral(const char* name) {
  if (name == NULL || strncmp(name, ".str.", 5) != 0) {
    return false;
  }
  char* end = NULL;
  long id = strtol(name + 5, &end, 10);
  if (end == name + 5 || *end != '\0' || id < 0) {
    return false;
  }
  StringLiteral* literal = CompilerFindStringLiteral((int)id);
  if (literal == NULL) {
    return false;
  }
  size_t size = literal->value.length + (size_t)literal->element_size;
  unsigned char* memory = calloc(1, size == 0 ? 1 : size);
  if (memory == NULL) {
    return false;
  }
  memcpy(memory, literal->value.value, literal->value.length);
  if (!pcode_static_data_initialized) {
    VectorInit(&pcode_static_data);
    pcode_static_data_initialized = true;
  }
  ConstexprPCodeStaticData* entry = malloc(sizeof(*entry));
  if (entry == NULL) {
    free(memory);
    return false;
  }
  StringInit(&entry->name, name);
  entry->memory = memory;
  entry->size = size == 0 ? 1 : size;
  VectorAppend(&pcode_static_data, entry);
  return true;
}

static bool RegisterConstexprPCodeStaticData(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL || !symbol->flags.value_set ||
      (!TypeIsIntegral(symbol->type) && !TypeIsFloatingPoint(symbol->type) &&
       !TypeIsPointer(symbol->type) && !TypeIsStructOrUnion(symbol->type) &&
       !TypeIsFixedArray(symbol->type))) {
    return false;
  }
  char namebuf[1024];
  const char* name = TargetSymbolName(symbol, namebuf, sizeof(namebuf));
  if (FindConstexprPCodeStaticData(name) != NULL) {
    return true;
  }
  size_t size = ConstexprPCodeHostBufferSize(symbol->type->size);
  unsigned char* memory = calloc(1, size);
  if (memory == NULL) {
    return false;
  }
  bool ok = false;
  if (TypeIsStructOrUnion(symbol->type) || TypeIsFixedArray(symbol->type)) {
    ok = symbol->value.other != NULL &&
         StoreConstexprObjectBytes(symbol->type,
                                   (ConstexprObject*)symbol->value.other,
                                   memory);
  } else {
    ConstexprValue value = {
        .is_floating = TypeIsFloatingPoint(symbol->type),
        .ivalue = symbol->value.ivalue,
        .fvalue = symbol->value.fvalue,
    };
    ok = StoreConstexprScalarBytes(symbol->type, &value, memory);
  }
  if (!ok && compiler->constexpr_eval_mode == kConstexprEvalPCode &&
      (compiler->current_function == NULL ||
       compiler->constant_evaluation_required_depth > 0)) {
    free(memory);
    return false;
  }
  if (!pcode_static_data_initialized) {
    VectorInit(&pcode_static_data);
    pcode_static_data_initialized = true;
  }
  ConstexprPCodeStaticData* entry = malloc(sizeof(*entry));
  if (entry == NULL) {
    free(memory);
    return false;
  }
  StringInit(&entry->name, name);
  entry->memory = memory;
  entry->size = size;
  VectorAppend(&pcode_static_data, entry);
  return true;
}

static bool ConstexprPCodeAggregateHasStoredMembers(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return false;
  }
  Struct* str = type->info.struct_info;
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function ||
        StorageIs(member->symbol->storage, STO(typedef))) {
      continue;
    }
    return true;
  }
  return false;
}

static bool LoadConstexprScalarBytes(TypeRecord* type, unsigned char* src,
                                     ConstexprValue* value) {
  if (type == NULL || src == NULL || value == NULL) {
    return false;
  }
  *value = (ConstexprValue){0};
  if (TypeIsFloatingPoint(type)) {
    if (TypeUsesFloat32Representation(type)) {
      float fvalue;
      memcpy(&fvalue, src, sizeof(fvalue));
      value->is_floating = true;
      value->fvalue = fvalue;
      value->ivalue = (int64_t)fvalue;
      return true;
    }
    double dvalue;
    memcpy(&dvalue, src, sizeof(dvalue));
    value->is_floating = true;
    value->fvalue = dvalue;
    value->ivalue = (int64_t)dvalue;
    return true;
  }
  if (!TypeIsIntegral(type) && !TypeIsPointer(type)) {
    return false;
  }
  int64_t ivalue = 0;
  size_t size = type->size == 0 ? sizeof(int64_t) : type->size;
  if (size == 1) {
    int8_t v;
    memcpy(&v, src, sizeof(v));
    ivalue = v;
  } else if (size == 2) {
    int16_t v;
    memcpy(&v, src, sizeof(v));
    ivalue = v;
  } else if (size == 4) {
    int32_t v;
    memcpy(&v, src, sizeof(v));
    ivalue = v;
  } else {
    memcpy(&ivalue, src, sizeof(ivalue));
  }
  value->ivalue = ivalue;
  value->fvalue = (double)ivalue;
  return true;
}

static bool LoadConstexprObjectBytes(TypeRecord* type, unsigned char* src,
                                     ConstexprPCodeRuntime* runtime,
                                     ConstexprObject** result);

static bool LoadConstexprValueBytes(TypeRecord* type, unsigned char* src,
                                    ConstexprPCodeRuntime* runtime,
                                    ConstexprValue* value) {
  if (TypeIsFixedArray(type) || TypeIsStructOrUnion(type)) {
    ConstexprObject* object = NULL;
    if (!LoadConstexprObjectBytes(type, src, runtime, &object)) {
      return false;
    }
    *value = (ConstexprValue){
        .is_object = true,
        .object = object,
    };
    return true;
  }
  return LoadConstexprScalarBytes(type, src, value);
}

static bool LoadConstexprObjectBytes(TypeRecord* type, unsigned char* src,
                                     ConstexprPCodeRuntime* runtime,
                                     ConstexprObject** result) {
  if (type == NULL || src == NULL || result == NULL) {
    return false;
  }
  ConstexprObject* object = NewPCodeConstexprObject(type);
  if (object == NULL) {
    return false;
  }
  if (TypeIsFixedArray(type)) {
    size_t elem_size = type->next != NULL ? type->next->size : 0;
    bool aggregate_started = false;
    if (runtime != NULL) {
      for (size_t i = 0; i < runtime->lifetime_events.length; i++) {
        ConstexprPCodeLifetimeEvent* event =
            runtime->lifetime_events.value.p[i];
        if (event != NULL &&
            event->kind == kConstexprPCodeLifetimeStartAggregate &&
            event->address == (uint64_t)(uintptr_t)src &&
            (event->type_token == 0 ||
             event->type_token == TypeRecordSemanticIdentityHash(type))) {
          aggregate_started = true;
        }
      }
    }
    for (size_t i = 0; i < object->slots.length; i++) {
      ConstexprValue* slot = PCodeConstexprObjectSlot(object, i);
      if (slot == NULL ||
          !LoadConstexprValueBytes(type->next, src + i * elem_size, runtime,
                                   slot)) {
        DeletePCodeConstexprObject(object);
        return false;
      }
      if (aggregate_started) {
        slot->lifetime_ended = true;
        uint64_t element_address =
            (uint64_t)(uintptr_t)(src + i * elem_size);
        for (size_t j = 0; j < runtime->lifetime_events.length; j++) {
          ConstexprPCodeLifetimeEvent* event =
              runtime->lifetime_events.value.p[j];
          if (event != NULL && event->address == element_address) {
            if (event->kind ==
                kConstexprPCodeLifetimePlacementConstruction) {
              slot->lifetime_ended = false;
            } else if (event->kind == kConstexprPCodeLifetimeEnd) {
              slot->lifetime_ended = true;
            }
          }
        }
      }
    }
    *result = object;
    return true;
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* str = type->info.struct_info;
    uint64_t started_union_member_token = 0;
    size_t started_union_member_index_plus_one = 0;
    bool started_union_member_ended = false;
    if (str->is_union && runtime != NULL) {
      for (size_t i = 0; i < runtime->lifetime_events.length; i++) {
        ConstexprPCodeLifetimeEvent* event =
            runtime->lifetime_events.value.p[i];
        if (event != NULL &&
            (event->kind == kConstexprPCodeLifetimeStartAggregate ||
             event->kind == kConstexprPCodeLifetimePlacementConstruction) &&
            event->address == (uint64_t)(uintptr_t)src) {
          started_union_member_token = event->type_token;
          started_union_member_index_plus_one =
              event->union_member_index_plus_one;
          started_union_member_ended = false;
        } else if (event != NULL &&
                   event->kind == kConstexprPCodeLifetimeEnd &&
                   event->address == (uint64_t)(uintptr_t)src) {
          started_union_member_ended = true;
        }
      }
    }
    if (str->is_union && started_union_member_ended) {
      *result = object;
      return true;
    }
    for (size_t i = 0; i < str->members.length; i++) {
      StructMember* member = str->members.value.p[i];
      if (member == NULL || member->symbol == NULL || member->is_static ||
          member->is_member_function ||
          StorageIs(member->symbol->storage, STO(typedef))) {
        continue;
      }
      if (str->is_union) {
        if (started_union_member_index_plus_one != 0) {
          if (member->index + 1 != started_union_member_index_plus_one) {
            continue;
          }
        } else if (started_union_member_token != 0 &&
                   TypeRecordSemanticIdentityHash(member->symbol->type) !=
                       started_union_member_token) {
          continue;
        }
      }
      size_t slot_index = str->is_union ? 0 : member->index;
      ConstexprValue* slot = PCodeConstexprObjectSlot(object, slot_index);
      if (slot == NULL ||
          !LoadConstexprValueBytes(member->symbol->type,
                                   src + member->byte_offset, runtime, slot)) {
        DeletePCodeConstexprObject(object);
        return false;
      }
      if (str->is_union) {
        object->active_union_member = member;
        break;
      }
    }
    *result = object;
    return true;
  }
  DeletePCodeConstexprObject(object);
  return false;
}

static ConstexprObject* ConstexprObjectArgument(ASTNode* arg) {
  if (arg == NULL) {
    return NULL;
  }
  if (arg->op == AST_OP(expr_init)) {
    return ConstexprObjectArgument(((ExpressionInitializerASTNode*)arg)->expr);
  }
  if (arg->op == AST_OP(cast)) {
    return ConstexprObjectArgument(((CastASTNode*)arg)->expr);
  }
  if (arg->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)arg)->symbol;
    TypeRecord* symbol_type = symbol != NULL ? symbol->type : NULL;
    if (TypeIsReference(symbol_type)) {
      symbol_type = symbol_type->next;
    }
    if (symbol != NULL && symbol->flags.value_set &&
        (TypeIsStructOrUnion(symbol_type) || TypeIsFixedArray(symbol_type)) &&
        symbol->value.other != NULL) {
      return (ConstexprObject*)symbol->value.other;
    }
  }
  if (arg->op == AST_OP(subscript)) {
    BinaryASTNode* subscript = (BinaryASTNode*)arg;
    ConstexprObject* object = ConstexprObjectArgument(subscript->left);
    int64_t index = 0;
    ConstEvalContext ctx;
    ConstEvalContextInit(&ctx);
    bool ok = EvaluateIntegerExpressionInContext(&ctx, subscript->right, &index);
    ConstEvalContextDestruct(&ctx);
    if (ok && index >= 0) {
      ConstexprValue* slot =
          PCodeConstexprObjectSlot(object, (size_t)index);
      return slot != NULL && slot->is_object ? slot->object : NULL;
    }
  }
  if (arg->op == AST_OP(dot) || arg->op == AST_OP(arrow)) {
    BinaryASTNode* member_access = (BinaryASTNode*)arg;
    ConstexprObject* object = ConstexprObjectArgument(member_access->left);
    if (object != NULL && member_access->right != NULL &&
        member_access->right->op == AST_OP(structmember)) {
      StructMember* member =
          ((StructMemberASTNode*)member_access->right)->member;
      if (member != NULL) {
        size_t slot_index = PCodeConstexprObjectIsUnion(object) ? 0 : member->index;
        ConstexprValue* slot = PCodeConstexprObjectSlot(object, slot_index);
        return slot != NULL && slot->is_object ? slot->object : NULL;
      }
    }
  }
  if (arg->op == AST_OP(address)) {
    return ConstexprObjectArgument(((UnaryASTNode*)arg)->sub);
  }
  return NULL;
}

static bool StoreConstexprPCodeObjectPointer(ConstEvalContext* ctx,
                                             PCodeVM* vm,
                                             unsigned char** sp,
                                             TypeRecord* type, ASTNode* arg,
                                             Vector* allocations,
                                             Vector* address_regions,
                                             const char** reason) {
  TypeRecord* object_type = type != NULL ? type->next : NULL;
  ConstexprObject* object = NULL;
  if (ctx != NULL && TypeIsReference(type) && object_type != NULL &&
      TypeIsStructOrUnion(object_type) && arg != NULL) {
    if (!ConstexprMaterializeClassArgument(ctx, arg, object_type, &object)) {
      *reason = "could not materialize constexpr reference argument";
      return false;
    }
  } else {
    object = ConstexprObjectArgument(arg);
    if (object == NULL && ctx != NULL) {
      (void)ConstexprEvaluateObjectAddress(ctx, arg, &object);
    }
  }
  if (object_type == NULL) {
    return false;
  }
  size_t memory_size = ConstexprPCodeHostBufferSize(object_type->size);
  unsigned char* memory = calloc(1, memory_size);
  if (memory == NULL) {
    *reason = "could not allocate constexpr object argument";
    return false;
  }
  if (object != NULL) {
    if (!StoreConstexprObjectBytes(object_type, object, memory)) {
      free(memory);
      return false;
    }
  } else if (!TypeIsReference(type)) {
    // An empty class temporary has no symbolic slots to materialize, but its
    // implicit object parameter still needs a valid address for a member call.
    if (!TypeIsPointer(type) || !TypeIsStructOrUnion(object_type) ||
        ConstexprPCodeAggregateHasStoredMembers(object_type) || arg == NULL ||
        !TypeIsStructOrUnion(arg->type)) {
      free(memory);
      return false;
    }
  } else if (TypeIsReference(type)) {
    if (TypeIsFloatingPoint(object_type)) {
      double value;
      if (!EvaluateFloatingPointExpressionInContext(ctx, arg, &value)) {
        free(memory);
        return false;
      }
      ConstexprValue constexpr_value = {
          .is_floating = true,
          .fvalue = value,
          .ivalue = (int64_t)value,
      };
      if (!StoreConstexprScalarBytes(object_type, &constexpr_value, memory)) {
        free(memory);
        return false;
      }
    } else if (TypeIsIntegral(object_type) || TypeIsPointer(object_type) ||
               TypeIsFunction(object_type)) {
      int64_t value;
      if (!EvaluateIntegerExpressionInContext(ctx, arg, &value)) {
        free(memory);
        return false;
      }
      ConstexprValue constexpr_value = {.ivalue = value};
      if (!StoreConstexprScalarBytes(object_type, &constexpr_value, memory)) {
        free(memory);
        return false;
      }
    } else if (!TypeIsStructOrUnion(object_type) ||
               ConstexprPCodeAggregateHasStoredMembers(object_type)) {
      free(memory);
      return false;
    }
  } else {
    free(memory);
    return false;
  }
  if (!PCodeVMRegisterMemoryRegion(vm, memory, memory_size, true)) {
    free(memory);
    *reason = "could not register constexpr object argument";
    return false;
  }
  VectorAppend(allocations, memory);
  if (address_regions != NULL) {
    ConstexprPCodeAddressRegion* region = malloc(sizeof(*region));
    if (region == NULL) {
      *reason = "could not track constexpr object argument";
      return false;
    }
    *region = (ConstexprPCodeAddressRegion){
        .memory = memory,
        .size = object_type->size,
        .type = object_type,
        .object = object,
    };
    VectorAppend(address_regions, region);
  }
  *sp -= sizeof(uint64_t);
  uint64_t address = (uint64_t)(uintptr_t)memory;
  memcpy(*sp, &address, sizeof(address));
  return true;
}

static SourceLocation PCodeBuiltinSourceLocation(ASTNode* node) {
  ASTNode* current = node;
  while (current != NULL && (current->flags & kASTDefaultArgument) != 0 &&
         current->parent != NULL) {
    current = current->parent;
  }
  return current != NULL ? current->location : SOURCE_LOCATION_MISSING;
}

static bool PCodeSourceBuiltinStringValue(ASTNode* arg, String* value) {
  if (arg == NULL) {
    return false;
  }
  if (arg->op == AST_OP(builtin_source_file)) {
    const char* filename = NULL;
    int lineno = 0;
    int start = 0;
    int end = 0;
    DecodeSourceLocation(PCodeBuiltinSourceLocation(arg), &filename, &lineno,
                         &start, &end);
    (void)lineno;
    (void)start;
    (void)end;
    StringInit(value, filename != NULL ? filename : "<unknown>");
    return true;
  }
  if (arg->op == AST_OP(builtin_source_function)) {
    const char* function_name = "";
    if (compiler->current_function != NULL &&
        compiler->current_function->info.function.symbol != NULL) {
      function_name =
          compiler->current_function->info.function.symbol->name.value;
    }
    StringInit(value, function_name);
    return true;
  }
  if (arg->op == AST_OP(builtin_source_pretty_function)) {
    StringInit(value, NULL);
    TypeRecordFunctionPrettyName(compiler->current_function, value);
    if (value->value == NULL) {
      StringSet(value, "");
    }
    return true;
  }
  return false;
}

static bool StoreConstexprPCodeSourceStringArgument(PCodeVM* vm,
                                                   unsigned char** sp,
                                                   ASTNode* arg,
                                                   Vector* allocations,
                                                   const char** reason) {
  String value;
  ASTNode* expression = arg;
  while (expression != NULL &&
         (expression->op == AST_OP(expr_init) ||
          expression->op == AST_OP(cast))) {
    expression =
        expression->op == AST_OP(expr_init)
            ? ((ExpressionInitializerASTNode*)expression)->expr
            : ((CastASTNode*)expression)->expr;
  }
  if (expression != NULL && expression->op == AST_OP(string)) {
    String* literal = ((ConstantASTNode*)expression)->value.string;
    if (literal == NULL) {
      return false;
    }
    StringInitFromSegment(&value, literal->value, literal->length);
  } else if (!PCodeSourceBuiltinStringValue(arg, &value)) {
    return false;
  }
  size_t size = value.length + 1;
  char* memory = malloc(size);
  if (memory == NULL) {
    StringDestruct(&value);
    *reason = "could not allocate constexpr source string argument";
    return false;
  }
  memcpy(memory, value.value, size);
  StringDestruct(&value);
  if (!PCodeVMRegisterMemoryRegion(vm, memory, size, false)) {
    free(memory);
    *reason = "could not register constexpr source string argument";
    return false;
  }
  // The returned constexpr object may store this pointer, so keep it alive for
  // the remainder of compilation rather than tying it to this VM invocation.
  (void)allocations;
  *sp -= sizeof(uint64_t);
  uint64_t address = (uint64_t)(uintptr_t)memory;
  memcpy(*sp, &address, sizeof(address));
  return true;
}

static bool StoreConstexprPCodeArgument(ConstEvalContext* ctx,
                                        PCodeVM* vm,
                                        unsigned char** sp,
                                        TypeRecord* type, ASTNode* arg,
                                        Vector* allocations,
                                        Vector* address_regions,
                                        const char** reason) {
  size_t size = ConstexprPCodeArgumentSize(type);
  if (size == 0) {
    *reason = "bad constexpr pcode argument type";
    return false;
  }
  if ((TypeIsPointerOrArray(type) || TypeIsReference(type)) &&
      StoreConstexprPCodeObjectPointer(ctx, vm, sp, type, arg, allocations,
                                       address_regions, reason)) {
    return true;
  }
  if (TypeIsPointer(type) &&
      StoreConstexprPCodeSourceStringArgument(vm, sp, arg, allocations,
                                             reason)) {
    return true;
  }
  *sp -= size;
  if (TypeIsMemberDataPointer(type)) {
    MemberPointerValue value;
    if (!MemberPointerTryEvaluateConstant(arg, type, &value)) {
      *reason = "could not evaluate member-pointer constexpr argument";
      return false;
    }
    if (size == 4) {
      int32_t narrowed = (int32_t)value.ptr;
      memcpy(*sp, &narrowed, sizeof(narrowed));
    } else {
      memcpy(*sp, &value.ptr, sizeof(value.ptr));
    }
    return true;
  }
  if (TypeIsStructOrUnion(type)) {
    ConstexprObject* object = ConstexprObjectArgument(arg);
    bool delete_object = false;
    if (object == NULL && ctx != NULL && arg != NULL) {
      // The evaluation context owns successfully materialized arguments.
      (void)ConstexprMaterializeClassArgument(ctx, arg, type, &object);
    }
    if (object == NULL && arg != NULL && arg->op == AST_OP(call) &&
        ConstexprPCodeEvaluateCallObjectResult(ctx, arg, &object)) {
      delete_object = true;
    }
    if (object == NULL && !ConstexprPCodeAggregateHasStoredMembers(type)) {
      memset(*sp, 0, size);
      return true;
    }
    if (object == NULL || !StoreConstexprObjectBytes(type, object, *sp)) {
      if (delete_object) {
        DeletePCodeConstexprObject(object);
      }
      *reason = "could not marshal aggregate constexpr argument";
      return false;
    }
    if (delete_object) {
      DeletePCodeConstexprObject(object);
    }
    return true;
  }
  if (TypeIsFloatingPoint(type)) {
    double value;
    if (!EvaluateFloatingPointExpressionInContext(ctx, arg, &value)) {
      *reason = "could not evaluate floating constexpr argument";
      return false;
    }
    if (TypeUsesFloat32Representation(type)) {
      float fvalue = (float)value;
      memcpy(*sp, &fvalue, sizeof(fvalue));
    } else {
      memcpy(*sp, &value, sizeof(value));
    }
    return true;
  }
  if (TypeIsIntegral(type) || TypeIsPointer(type) || TypeIsFunction(type)) {
    int64_t value;
    if (!EvaluateIntegerExpressionInContext(ctx, arg, &value)) {
      *reason = "could not evaluate integer constexpr argument";
      return false;
    }
    if (size == 4) {
      int32_t narrowed = (int32_t)value;
      memcpy(*sp, &narrowed, sizeof(narrowed));
    } else {
      memcpy(*sp, &value, sizeof(value));
    }
    return true;
  }
  *reason = "aggregate constexpr arguments are not implemented";
  return false;
}

static bool PrepareConstexprPCodeCallStack(ConstEvalContext* ctx,
                                           PCodeVM* vm, TypeRecord* func,
                                           ASTNode* call_node,
                                           uint32_t* halt_instruction,
                                           Vector* allocations,
                                           Vector* address_regions,
                                           unsigned char* struct_return,
                                           const char** reason) {
  if (call_node == NULL || call_node->op != AST_OP(call)) {
    *reason = "constexpr pcode invocation needs a call expression";
    return false;
  }
  VectorASTNode* call = (VectorASTNode*)call_node;
  bool has_receiver =
      call->left != NULL &&
      (call->left->op == AST_OP(dot) || call->left->op == AST_OP(arrow));
  size_t explicit_count = call->children != NULL ? call->children->length : 0;
  size_t expected_count = explicit_count + (has_receiver ? 1 : 0);
  if (call->children == NULL ||
      expected_count != func->info.function.prototype.length) {
    *reason = "constexpr pcode argument count mismatch";
    return false;
  }

  unsigned char* sp = (unsigned char*)vm->stack + vm->stack_size;
  for (size_t i = explicit_count; i > 0; i--) {
    size_t formal_index = i - 1 + (has_receiver ? 1 : 0);
    Symbol* formal = func->info.function.prototype.value.p[formal_index];
    ASTNode* arg = call->children->value.p[i - 1];
    if (formal == NULL ||
        !StoreConstexprPCodeArgument(ctx, vm, &sp, formal->type, arg,
                                     allocations, address_regions, reason)) {
      return false;
    }
  }
  if (has_receiver) {
    Symbol* formal = func->info.function.prototype.value.p[0];
    ASTNode* receiver = ((BinaryASTNode*)call->left)->left;
    if (formal == NULL ||
        !StoreConstexprPCodeArgument(ctx, vm, &sp, formal->type, receiver,
                                     allocations, address_regions, reason)) {
      return false;
    }
  }
  if (struct_return != NULL) {
    sp -= sizeof(uint64_t);
    uint64_t address = (uint64_t)(uintptr_t)struct_return;
    memcpy(sp, &address, sizeof(address));
  }

  *halt_instruction = (uint32_t)(PCODE_OP(esc) << 24 | 4);
  sp -= sizeof(uint64_t);
  uint64_t return_address = (uint64_t)(uintptr_t)halt_instruction;
  memcpy(sp, &return_address, sizeof(return_address));
  vm->iregs[PCODE_SP_REG] = (int64_t)(uintptr_t)sp;
  return true;
}

static void StoreConstexprPCodePointer(unsigned char** sp, void* pointer) {
  *sp -= sizeof(uint64_t);
  uint64_t address = (uint64_t)(uintptr_t)pointer;
  memcpy(*sp, &address, sizeof(address));
}

static bool PrepareConstexprPCodeConstructorStack(ConstEvalContext* ctx,
                                                  PCodeVM* vm,
                                                  TypeRecord* func,
                                                  ASTNode* call_node,
                                                  uint32_t* halt_instruction,
                                                  Vector* allocations,
                                                  unsigned char* object_memory,
                                                  const char** reason) {
  if (call_node == NULL || call_node->op != AST_OP(call) ||
      object_memory == NULL) {
    *reason = "constexpr pcode constructor invocation needs a call expression";
    return false;
  }
  VectorASTNode* call = (VectorASTNode*)call_node;
  size_t child_count = call->children != NULL ? call->children->length : 0;
  size_t argument_offset = 0;
  if (child_count == func->info.function.prototype.length) {
    // Some analyzed constructor calls retain the implicit receiver as their
    // first child. The pcode entry stack supplies its own destination object.
    argument_offset = 1;
  }
  size_t explicit_count = child_count - argument_offset;
  if (explicit_count + 1 != func->info.function.prototype.length) {
    *reason = "constexpr pcode constructor argument count mismatch";
    return false;
  }

  unsigned char* sp = (unsigned char*)vm->stack + vm->stack_size;
  for (size_t i = explicit_count; i > 0; i--) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    ASTNode* arg = call->children->value.p[argument_offset + i - 1];
    if (formal == NULL ||
        !StoreConstexprPCodeArgument(ctx, vm, &sp, formal->type, arg,
                                     allocations, NULL, reason)) {
      return false;
    }
  }
  StoreConstexprPCodePointer(&sp, object_memory);

  *halt_instruction = (uint32_t)(PCODE_OP(esc) << 24 | 4);
  sp -= sizeof(uint64_t);
  uint64_t return_address = (uint64_t)(uintptr_t)halt_instruction;
  memcpy(sp, &return_address, sizeof(return_address));
  vm->iregs[PCODE_SP_REG] = (int64_t)(uintptr_t)sp;
  return true;
}

static bool EnableConstexprPCodeCheckedMemory(PCodeVM* vm,
                                              ConstexprPCodeImage* image,
                                              ConstexprPCodeRuntime* runtime,
                                              uint32_t* halt_instruction,
                                              const char** reason) {
  if (!PCodeVMEnableCheckedMemory(vm) ||
      !PCodeVMRegisterMemoryRegion(vm, image->text, image->text_size,
                                   false) ||
      !PCodeVMRegisterMemoryRegion(vm, image->rodata, image->rodata_size,
                                   false) ||
      !PCodeVMRegisterMemoryRegion(vm, image->exception_table,
                                   image->exception_table_size, false) ||
      !PCodeVMRegisterMemoryRegion(vm, runtime, sizeof(*runtime), true) ||
      !PCodeVMRegisterMemoryRegion(vm, halt_instruction,
                                   sizeof(*halt_instruction), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_malloc_stub,
          sizeof(constexpr_pcode_malloc_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_placement_new_stub,
          sizeof(constexpr_pcode_placement_new_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_start_lifetime_stub,
          sizeof(constexpr_pcode_start_lifetime_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_free_stub, sizeof(constexpr_pcode_free_stub),
          false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_realloc_stub,
          sizeof(constexpr_pcode_realloc_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_throw_stub,
          sizeof(constexpr_pcode_throw_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_throw_i8_stub,
          sizeof(constexpr_pcode_throw_i8_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_throw_f4_stub,
          sizeof(constexpr_pcode_throw_f4_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_throw_f8_stub,
          sizeof(constexpr_pcode_throw_f8_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_current_exception_integer_stub,
          sizeof(constexpr_pcode_current_exception_integer_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_current_exception_f4_stub,
          sizeof(constexpr_pcode_current_exception_f4_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_current_exception_f8_stub,
          sizeof(constexpr_pcode_current_exception_f8_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_current_exception_pointer_stub,
          sizeof(constexpr_pcode_current_exception_pointer_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_current_exception_address_stub,
          sizeof(constexpr_pcode_current_exception_address_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_resume_stub,
          sizeof(constexpr_pcode_resume_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_constexpr_throw_stub,
          sizeof(constexpr_pcode_constexpr_throw_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_end_catch_stub,
          sizeof(constexpr_pcode_end_catch_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_end_catch_complete_stub,
          sizeof(constexpr_pcode_end_catch_complete_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_exception_ptr_current_stub,
          sizeof(constexpr_pcode_exception_ptr_current_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_exception_ptr_retain_stub,
          sizeof(constexpr_pcode_exception_ptr_retain_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_exception_ptr_release_stub,
          sizeof(constexpr_pcode_exception_ptr_release_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_exception_ptr_rethrow_stub,
          sizeof(constexpr_pcode_exception_ptr_rethrow_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_uncaught_exceptions_stub,
          sizeof(constexpr_pcode_uncaught_exceptions_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_memcpy_stub,
          sizeof(constexpr_pcode_memcpy_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_invalid_operation_stub,
          sizeof(constexpr_pcode_invalid_operation_stub), false)) {
    *reason = "could not enable checked constexpr pcode memory";
    return false;
  }
  if (pcode_static_data_initialized) {
    for (size_t i = 0; i < pcode_static_data.length; i++) {
      ConstexprPCodeStaticData* entry = pcode_static_data.value.p[i];
      if (entry != NULL &&
          !PCodeVMRegisterMemoryRegion(vm, entry->memory, entry->size, false)) {
        *reason = "could not register constexpr pcode static data";
        return false;
      }
    }
  }
  if (pcode_image_cache_initialized) {
    for (size_t i = 0; i < pcode_image_cache.length; i++) {
      ConstexprPCodeImageCacheEntry* entry = pcode_image_cache.value.p[i];
      if (entry != NULL) {
        if (entry->text == image->text) {
          continue;
        }
        if (!PCodeVMRegisterMemoryRegion(vm, entry->text, entry->text_size,
                                         false) ||
            !PCodeVMRegisterMemoryRegion(vm, entry->rodata,
                                         entry->rodata_size, false) ||
            !PCodeVMRegisterMemoryRegion(vm, entry->exception_table,
                                         entry->exception_table_size, false)) {
          *reason = "could not register cached constexpr pcode image";
          return false;
        }
      }
    }
  }
  return true;
}

static bool RunRealPCodeCall(ConstEvalContext* ctx, ASTNode* node,
                             TypeRecord* func, int64_t* int_result,
                             double* double_result,
                             ConstexprObject** object_result,
                             ConstexprValue* address_result,
                             ConstexprPCodeFailureKind* failure_kind,
                             const char** reason) {
  *failure_kind = kConstexprPCodeFailureUnsupported;
  ConstexprPCodeImage image;
  ConstexprPCodeImageInit(&image);
  DiagnosticSuppressBegin();
  bool image_ok = CompileConstexprFunctionImage(func, &image, reason);
  DiagnosticSuppressEnd();
  if (!image_ok) {
    ConstexprPCodeImageDestruct(&image);
    return false;
  }

  PCodeVM vm;
  if (!PCodeVMInitWithStack(&vm, P_CODE_VM_DEFAULT_STACK_SIZE)) {
    ConstexprPCodeImageDestruct(&image);
    *reason = "could not allocate constexpr pcode stack";
    return false;
  }
  ConstexprPCodeRuntime runtime;
  ConstexprPCodeRuntimeInit(&runtime);
  runtime.image = &image;
  PCodeVMSetEscapeHandler(&vm, ConstexprEscape, &runtime);
  uint32_t halt_instruction = 0;
  if (!EnableConstexprPCodeCheckedMemory(&vm, &image, &runtime,
                                         &halt_instruction,
                                         reason)) {
    ConstexprPCodeRuntimeDestruct(&runtime);
    PCodeVMDestruct(&vm);
    ConstexprPCodeImageDestruct(&image);
    return false;
  }
  Vector allocations;
  VectorInit(&allocations);
  unsigned char* struct_return = NULL;
  if (object_result != NULL) {
    if (func->next == NULL || !TypeIsStructOrUnion(func->next)) {
      *reason = "constexpr pcode object result requires aggregate return type";
      VectorDestruct(&allocations);
      ConstexprPCodeRuntimeDestruct(&runtime);
      PCodeVMDestruct(&vm);
      ConstexprPCodeImageDestruct(&image);
      return false;
    }
    struct_return =
        calloc(1, ConstexprPCodeHostBufferSize(func->next->size));
    if (struct_return == NULL) {
      *reason = "could not allocate constexpr pcode object result";
      VectorDestruct(&allocations);
      ConstexprPCodeRuntimeDestruct(&runtime);
      PCodeVMDestruct(&vm);
      ConstexprPCodeImageDestruct(&image);
      return false;
    }
    VectorAppend(&allocations, struct_return);
    if (!PCodeVMRegisterMemoryRegion(
            &vm, struct_return, ConstexprPCodeHostBufferSize(func->next->size),
            true)) {
      *reason = "could not register constexpr pcode object result";
      for (size_t i = 0; i < allocations.length; i++) {
        free(allocations.value.p[i]);
      }
      VectorDestruct(&allocations);
      ConstexprPCodeRuntimeDestruct(&runtime);
      PCodeVMDestruct(&vm);
      ConstexprPCodeImageDestruct(&image);
      return false;
    }
  }
  Vector address_regions;
  VectorInit(&address_regions);
  if (!PrepareConstexprPCodeCallStack(ctx, &vm, func, node, &halt_instruction,
                                      &allocations, &address_regions,
                                      struct_return, reason)) {
    for (size_t i = 0; i < allocations.length; i++) {
      free(allocations.value.p[i]);
    }
    VectorDestruct(&allocations);
    VectorDestructWithContents(&address_regions, NULL, /*free_element=*/true);
    ConstexprPCodeRuntimeDestruct(&runtime);
    PCodeVMDestruct(&vm);
    ConstexprPCodeImageDestruct(&image);
    return false;
  }
  PCodeVMSetEntry(&vm, image.entry);
  vm.max_steps = 100000;
  PCodeVMStatus status = PCodeVMRun(&vm);
  if (status == kPCodeVMStatusHalted) {
    if (int_result != NULL) {
      *int_result = vm.iregs[PCODE_INT_RETURN_REG];
    }
    if (double_result != NULL) {
      *double_result = TypeUsesFloat32Representation(func->next)
                           ? (double)vm.fregs[PCODE_FLOAT_RETURN_REG]
                           : vm.dregs[PCODE_DOUBLE_RETURN_REG];
    }
    if (address_result != NULL) {
      uint64_t address = (uint64_t)vm.iregs[PCODE_INT_RETURN_REG];
      if (runtime.source_size_t_size < sizeof(uint64_t) && address != 0) {
        uint64_t normalized_address = 0;
        if (ConstexprPCodeNormalizeAddress(
                &vm, address, 0, false, &normalized_address)) {
          address = normalized_address;
        }
      }
      *address_result = (ConstexprValue){0};
      if (address == 0) {
        address_result->is_address = true;
      } else {
        bool found = false;
        for (size_t i = 0; i < address_regions.length; i++) {
          ConstexprPCodeAddressRegion* region = address_regions.value.p[i];
          if (region == NULL) {
            continue;
          }
          uint64_t start = (uint64_t)(uintptr_t)region->memory;
          uint64_t end = start + region->size;
          if (address >= start && address < end) {
            ConstexprValue* slot = PCodeConstexprSlotForOffset(
                region->type, region->object, (size_t)(address - start));
            if (slot == NULL) {
              *reason = "could not map constexpr pcode returned address";
              for (size_t j = 0; j < allocations.length; j++) {
                free(allocations.value.p[j]);
              }
              VectorDestruct(&allocations);
              VectorDestructWithContents(&address_regions, NULL,
                                         /*free_element=*/true);
              ConstexprPCodeRuntimeDestruct(&runtime);
              PCodeVMDestruct(&vm);
              ConstexprPCodeImageDestruct(&image);
              return false;
            }
            *address_result = (ConstexprValue){
                .is_address = true,
                .address_slot = slot,
            };
            found = true;
            break;
          }
        }
        if (!found) {
          *failure_kind = kConstexprPCodeFailureInvalid;
          *reason = "constexpr pcode returned escaping address";
          for (size_t j = 0; j < allocations.length; j++) {
            free(allocations.value.p[j]);
          }
          VectorDestruct(&allocations);
          VectorDestructWithContents(&address_regions, NULL,
                                     /*free_element=*/true);
          ConstexprPCodeRuntimeDestruct(&runtime);
          PCodeVMDestruct(&vm);
          ConstexprPCodeImageDestruct(&image);
          return false;
        }
      }
    }
    bool returns_exception_ptr =
        func->next != NULL && TypeIsStructOrUnion(func->next) &&
        func->next->info.struct_info != NULL &&
        func->next->info.struct_info->tag_name != NULL &&
        func->next->info.struct_info->tag_name->value != NULL &&
        strcmp(func->next->info.struct_info->tag_name->value,
               "exception_ptr") == 0;
    uint64_t returned_exception_handle = 0;
    if (object_result != NULL && returns_exception_ptr &&
        struct_return != NULL) {
      size_t pointer_size =
          compiler->target != NULL
              ? (size_t)compiler->target->pointer_size
              : sizeof(returned_exception_handle);
      if (pointer_size > sizeof(returned_exception_handle)) {
        pointer_size = sizeof(returned_exception_handle);
      }
      memcpy(&returned_exception_handle, struct_return, pointer_size);
    }
    if (returned_exception_handle != 0) {
      *failure_kind = kConstexprPCodeFailureInvalid;
      *reason = "constexpr pcode returned escaping exception_ptr";
      for (size_t i = 0; i < allocations.length; i++) {
        free(allocations.value.p[i]);
      }
      VectorDestruct(&allocations);
      VectorDestructWithContents(&address_regions, NULL,
                                 /*free_element=*/true);
      ConstexprPCodeRuntimeDestruct(&runtime);
      PCodeVMDestruct(&vm);
      ConstexprPCodeImageDestruct(&image);
      return false;
    }
    if (object_result != NULL && struct_return != NULL &&
        func->next->size != 0) {
      uint64_t return_address = (uint64_t)(uintptr_t)struct_return;
      bool return_has_lifetime_state = false;
      for (size_t i = 0; i < runtime.lifetime_events.length; i++) {
        ConstexprPCodeLifetimeEvent* event =
            runtime.lifetime_events.value.p[i];
        if (event != NULL &&
            event->kind == kConstexprPCodeLifetimeStartAggregate &&
            event->address == return_address) {
          return_has_lifetime_state = true;
          break;
        }
      }
      if (!return_has_lifetime_state) {
        for (size_t i = runtime.lifetime_events.length; i > 0; i--) {
          ConstexprPCodeLifetimeEvent* event =
              runtime.lifetime_events.value.p[i - 1];
          uint64_t source_address = 0;
          if (event == NULL ||
              event->kind != kConstexprPCodeLifetimeStartAggregate ||
              event->size != func->next->size ||
              !ConstexprPCodeNormalizeAddress(
                  &vm, event->address, event->size, false, &source_address) ||
              memcmp((void*)(uintptr_t)source_address, struct_return,
                     event->size) != 0) {
            continue;
          }
          if (!ConstexprPCodeCopyLifetimeEvents(
                  &runtime, source_address, return_address, event->size)) {
            *reason = "could not marshal constexpr pcode lifetime state";
            for (size_t j = 0; j < allocations.length; j++) {
              free(allocations.value.p[j]);
            }
            VectorDestruct(&allocations);
            VectorDestructWithContents(&address_regions, NULL,
                                       /*free_element=*/true);
            ConstexprPCodeRuntimeDestruct(&runtime);
            PCodeVMDestruct(&vm);
            ConstexprPCodeImageDestruct(&image);
            return false;
          }
          break;
        }
      }
    }
    if (object_result != NULL &&
        !LoadConstexprObjectBytes(func->next, struct_return, &runtime,
                                  object_result)) {
      *reason = "could not decode constexpr pcode object result";
      for (size_t i = 0; i < allocations.length; i++) {
        free(allocations.value.p[i]);
      }
      VectorDestruct(&allocations);
      VectorDestructWithContents(&address_regions, NULL,
                                 /*free_element=*/true);
      ConstexprPCodeRuntimeDestruct(&runtime);
      PCodeVMDestruct(&vm);
      ConstexprPCodeImageDestruct(&image);
      return false;
    }
    if (ConstexprPCodeRuntimeHasLiveHeap(&runtime)) {
      *failure_kind = kConstexprPCodeFailureInvalid;
      *reason = "constexpr pcode evaluation leaked allocation";
      for (size_t i = 0; i < allocations.length; i++) {
        free(allocations.value.p[i]);
      }
      VectorDestruct(&allocations);
      VectorDestructWithContents(&address_regions, NULL,
                                 /*free_element=*/true);
      ConstexprPCodeRuntimeDestruct(&runtime);
      PCodeVMDestruct(&vm);
      ConstexprPCodeImageDestruct(&image);
      return false;
    }
  } else {
    *failure_kind = ConstexprPCodeVMFailureKind(
        status, runtime.heap_blocks.length != 0);
    *reason = PCodeVMStatusName(status);
  }
  for (size_t i = 0; i < allocations.length; i++) {
    free(allocations.value.p[i]);
  }
  VectorDestruct(&allocations);
  VectorDestructWithContents(&address_regions, NULL, /*free_element=*/true);
  ConstexprPCodeRuntimeDestruct(&runtime);
  PCodeVMDestruct(&vm);
  ConstexprPCodeImageDestruct(&image);
  return status == kPCodeVMStatusHalted;
}

static bool RunRealPCodeConstructor(ConstEvalContext* ctx, TypeRecord* object_type,
                                    ASTNode* node, TypeRecord* func,
                                    ConstexprObject** object_result,
                                    ConstexprPCodeFailureKind* failure_kind,
                                    const char** reason) {
  *failure_kind = kConstexprPCodeFailureUnsupported;
  ConstexprPCodeImage image;
  ConstexprPCodeImageInit(&image);
  DiagnosticSuppressBegin();
  bool image_ok = CompileConstexprFunctionImage(func, &image, reason);
  DiagnosticSuppressEnd();
  if (!image_ok) {
    ConstexprPCodeImageDestruct(&image);
    return false;
  }

  PCodeVM vm;
  if (!PCodeVMInitWithStack(&vm, P_CODE_VM_DEFAULT_STACK_SIZE)) {
    ConstexprPCodeImageDestruct(&image);
    *reason = "could not allocate constexpr pcode stack";
    return false;
  }
  ConstexprPCodeRuntime runtime;
  ConstexprPCodeRuntimeInit(&runtime);
  runtime.image = &image;
  PCodeVMSetEscapeHandler(&vm, ConstexprEscape, &runtime);
  uint32_t halt_instruction = 0;
  if (!EnableConstexprPCodeCheckedMemory(&vm, &image, &runtime,
                                         &halt_instruction,
                                         reason)) {
    ConstexprPCodeRuntimeDestruct(&runtime);
    PCodeVMDestruct(&vm);
    ConstexprPCodeImageDestruct(&image);
    return false;
  }
  Vector allocations;
  VectorInit(&allocations);
  unsigned char* object_memory =
      calloc(1, ConstexprPCodeHostBufferSize(object_type->size));
  if (object_memory == NULL) {
    *reason = "could not allocate constexpr pcode constructor object";
    VectorDestruct(&allocations);
    ConstexprPCodeRuntimeDestruct(&runtime);
    PCodeVMDestruct(&vm);
    ConstexprPCodeImageDestruct(&image);
    return false;
  }
  VectorAppend(&allocations, object_memory);
  if (!PCodeVMRegisterMemoryRegion(
          &vm, object_memory, ConstexprPCodeHostBufferSize(object_type->size),
          true)) {
    *reason = "could not register constexpr pcode constructor object";
    for (size_t i = 0; i < allocations.length; i++) {
      free(allocations.value.p[i]);
    }
    VectorDestruct(&allocations);
    ConstexprPCodeRuntimeDestruct(&runtime);
    PCodeVMDestruct(&vm);
    ConstexprPCodeImageDestruct(&image);
    return false;
  }

  if (!PrepareConstexprPCodeConstructorStack(
          ctx, &vm, func, node, &halt_instruction, &allocations, object_memory,
          reason)) {
    for (size_t i = 0; i < allocations.length; i++) {
      free(allocations.value.p[i]);
    }
    VectorDestruct(&allocations);
    ConstexprPCodeRuntimeDestruct(&runtime);
    PCodeVMDestruct(&vm);
    ConstexprPCodeImageDestruct(&image);
    return false;
  }
  PCodeVMSetEntry(&vm, image.entry);
  vm.max_steps = 100000;
  PCodeVMStatus status = PCodeVMRun(&vm);
  bool ok = status == kPCodeVMStatusHalted &&
            LoadConstexprObjectBytes(object_type, object_memory, &runtime,
                                     object_result);
  if (status != kPCodeVMStatusHalted) {
    *failure_kind = ConstexprPCodeVMFailureKind(
        status, runtime.heap_blocks.length != 0);
    *reason = PCodeVMStatusName(status);
  } else if (!ok) {
    *reason = "could not decode constexpr pcode constructor object";
  } else if (ConstexprPCodeRuntimeHasLiveHeap(&runtime)) {
    *failure_kind = kConstexprPCodeFailureInvalid;
    *reason = "constexpr pcode evaluation leaked allocation";
    ok = false;
  }
  for (size_t i = 0; i < allocations.length; i++) {
    free(allocations.value.p[i]);
  }
  VectorDestruct(&allocations);
  ConstexprPCodeRuntimeDestruct(&runtime);
  PCodeVMDestruct(&vm);
  ConstexprPCodeImageDestruct(&image);
  return ok;
}

static Symbol* PCodeConstexprFunctionDefinition(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return NULL;
  }
  Symbol* resolved = ConstexprFunctionDefinition(symbol);
  if (resolved != NULL && resolved->type != NULL &&
      TypeIsFunction(resolved->type) &&
      resolved->type->info.function.body != NULL) {
    SymbolSetCXXMangledAsmName(resolved);
    if (symbol->asm_name.length == 0 && resolved->asm_name.length != 0) {
      StringSetString(&symbol->asm_name, &resolved->asm_name);
    }
    return resolved;
  }
  if (symbol->value.func_defn != NULL &&
      symbol->value.func_defn->type != NULL &&
      TypeIsFunction(symbol->value.func_defn->type) &&
      symbol->value.func_defn->type->info.function.body != NULL) {
    SymbolSetCXXMangledAsmName(symbol->value.func_defn);
    if (symbol->asm_name.length == 0 &&
        symbol->value.func_defn->asm_name.length != 0) {
      StringSetString(&symbol->asm_name, &symbol->value.func_defn->asm_name);
    }
    return symbol->value.func_defn;
  }
  if (symbol->type->info.function.body != NULL) {
    SymbolSetCXXMangledAsmName(symbol);
    return symbol;
  }
  return NULL;
}

static Symbol* PCodeConstexprCallSymbol(ASTNode* node) {
  if (node == NULL || node->op != AST_OP(call)) {
    return NULL;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL) {
    return NULL;
  }
  if (call->left->op == AST_OP(identifier)) {
    return ((IdentifierASTNode*)call->left)->symbol;
  }
  if (call->left->op == AST_OP(dot) || call->left->op == AST_OP(arrow)) {
    BinaryASTNode* member_access = (BinaryASTNode*)call->left;
    if (member_access->right != NULL &&
        member_access->right->op == AST_OP(structmember)) {
      StructMember* member =
          ((StructMemberASTNode*)member_access->right)->member;
      if (member != NULL && member->is_member_function) {
        return member->symbol;
      }
    }
  }
  return NULL;
}

static void ValidationReject(ValidationState* state, const char* reason) {
  if (state->ok) {
    state->ok = false;
    state->reason = reason;
  }
}

static bool PointerExpressionUsesFixedArray(ASTNode* node) {
  if (node == NULL) {
    return false;
  }
  if (TypeIsFixedArray(node->type)) {
    return true;
  }
  switch (node->op) {
    case AST_OP(address):
      return PointerExpressionUsesFixedArray(((UnaryASTNode*)node)->sub);
    case AST_OP(subscript):
    case AST_OP(plus):
    case AST_OP(minus):
      return PointerExpressionUsesFixedArray(((BinaryASTNode*)node)->left);
    default:
      return false;
  }
}

static void ValidateASTNode(ASTNode* node, void* data, int child_id,
                            VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  ValidationState* state = data;
  if (!state->ok) {
    return;
  }
  switch (node->op) {
    case AST_OP(goto):
      if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
        ValidationReject(
            state, "goto requires C++23 constexpr evaluation rules");
      }
      return;
    case AST_OP(label):
      if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
        ValidationReject(
            state, "labels require C++23 constexpr evaluation rules");
      }
      return;
    case AST_OP(vardecl): {
      VariableDeclarationASTNode* declaration =
          (VariableDeclarationASTNode*)node;
      if (declaration->symbol != NULL &&
          StorageIs(declaration->symbol->storage, STO(static) | STO(thread)) &&
          !CompilerCXXAtLeast(kLanguageStandardCXX23)) {
        ValidationReject(
            state,
            "static and thread_local variables require AST constexpr evaluation");
      }
      return;
    }
    case AST_OP(call): {
      if (PCodeConstexprCallSymbol(node) == NULL) {
        // Indirect calls are linked through expression thunks. If the selected
        // target cannot be resolved into the pcode image, checked execution
        // rejects the path rather than accepting a host address.
        return;
      }
      if (IsConstexprPCodeRuntimeCallSymbol(PCodeConstexprCallSymbol(node))) {
        return;
      }
      Symbol* callee =
          PCodeConstexprFunctionDefinition(PCodeConstexprCallSymbol(node));
      if (callee == NULL || callee->type == NULL) {
        // The unresolved call is linked to a trap stub. It therefore rejects
        // the expression only when execution actually reaches this path.
        return;
      }
      return;
    }
    case AST_OP(subscript): {
      return;
    }
    case AST_OP(dot):
    case AST_OP(arrow): {
      BinaryASTNode* access = (BinaryASTNode*)node;
      TypeRecord* receiver = access->left != NULL ? access->left->type : NULL;
      if (node->op == AST_OP(arrow) && TypeIsPointer(receiver)) {
        receiver = receiver->next;
      }
      if (TypeIsStructOrUnion(receiver) &&
          receiver->info.struct_info != NULL &&
          receiver->info.struct_info->is_union) {
        // Active-member legality is checked by the semantic overlay before
        // byte-oriented pcode execution.
      }
      return;
    }
    case AST_OP(plus):
    case AST_OP(minus): {
      BinaryASTNode* binary = (BinaryASTNode*)node;
      if (TypeIsPointer(node->type) && binary->left != NULL &&
          PointerExpressionUsesFixedArray(binary->left)) {
        // Formation bounds are checked by the semantic overlay; checked-memory
        // execution still validates any subsequent access.
      }
      return;
    }
    default:
      return;
  }
}

static bool ValidateFunction(TypeRecord* func, const char** reason) {
  if (func == NULL || !TypeIsFunction(func)) {
    *reason = "callee is not a function";
    return false;
  }
  if (!func->info.function.is_constexpr) {
    *reason = "callee is not constexpr";
    return false;
  }
  if (func->info.function.body == NULL) {
    *reason = "callee has no body";
    return false;
  }
  ValidationState state = {.reason = "ok", .ok = true};
  ASTNodeVisit(func->info.function.body, ValidateASTNode, 0, &state);
  if (!state.ok) {
    *reason = state.reason;
    return false;
  }
  return true;
}

bool ConstexprPCodeValidateCall(ASTNode* node, const char** reason) {
  static const char* ok = "ok";
  if (reason == NULL) {
    reason = &ok;
  }
  *reason = ok;
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  if (callee == NULL) {
    *reason = "call is not a direct constexpr function definition";
    return false;
  }
  return ValidateFunction(callee->type, reason);
}

typedef struct {
  bool required;
  bool ast_only;
  int function_depth;
  Vector visited_functions;  // Symbol*
} ConstexprASTOverlayScan;

static void ScanConstexprFunctionCapabilities(
    Symbol* function, ConstexprASTOverlayScan* scan);

static bool ConstexprASTOverlayVisited(ConstexprASTOverlayScan* scan,
                                       Symbol* function) {
  for (size_t i = 0; i < scan->visited_functions.length; ++i) {
    if (scan->visited_functions.value.p[i] == function) {
      return true;
    }
  }
  return false;
}

static void DetectConstexprASTOverlay(ASTNode* node, void* data, int child_id,
                                      VisitorMode mode) {
  (void)child_id;
  ConstexprASTOverlayScan* scan = data;
  if (mode != kVisitPreChildren || node == NULL || scan->ast_only) {
    return;
  }
  bool cxx26 = CompilerCXXAtLeast(kLanguageStandardCXX26);
  bool placement_new =
      (node->flags & kASTCXXPlacementNew) != 0;
  if (!placement_new && node->op == AST_OP(cast) &&
      (node->flags & kASTCXXNewExpression) != 0) {
    ASTNode* allocation = ((CastASTNode*)node)->expr;
    placement_new =
        allocation != NULL && allocation->op == AST_OP(call) &&
        ((VectorASTNode*)allocation)->children != NULL &&
        ((VectorASTNode*)allocation)->children->length > 1;
  }
  if ((node->flags & kASTRequiresASTConstexpr) != 0 ||
      (node->op == AST_OP(identifier) &&
       ((IdentifierASTNode*)node)->symbol != NULL &&
       ((IdentifierASTNode*)node)->symbol->requires_ast_constexpr) ||
      node->op == AST_OP(reflect) ||
      node->op == AST_OP(reflection_constant) ||
      node->op == AST_OP(splice) ||
      node->op == AST_OP(splice_qualified) ||
      node->op == AST_OP(contract_assert) ||
      (node->type != NULL && TypeContainsReflection(node->type))) {
    scan->required = true;
    scan->ast_only = true;
  } else if (cxx26 && placement_new) {
    scan->required = true;
  } else if (node->op == AST_OP(cast)) {
    CastASTNode* cast = (CastASTNode*)node;
    TypeRecord* source =
        cast->expr != NULL && TypeIsPointer(cast->expr->type)
            ? cast->expr->type->next : NULL;
    TypeRecord* target =
        TypeIsPointer(cast->cast_type) ? cast->cast_type->next : NULL;
    if (cxx26 && source != NULL && TypeIsVoid(source) &&
        target != NULL && !TypeIsVoid(target) &&
        (node->flags & kASTCXXNewExpression) == 0) {
      scan->required = true;
    }
  } else if (scan->function_depth <= 1 &&
             (node->op == AST_OP(dot) || node->op == AST_OP(arrow))) {
    BinaryASTNode* access = (BinaryASTNode*)node;
    TypeRecord* receiver = access->left != NULL ? access->left->type : NULL;
    if (node->op == AST_OP(arrow) && TypeIsPointer(receiver)) {
      receiver = receiver->next;
    }
    if (TypeIsStructOrUnion(receiver) &&
        receiver->info.struct_info != NULL &&
        receiver->info.struct_info->is_union) {
      scan->required = true;
    }
  } else if (scan->function_depth <= 1 &&
             (node->op == AST_OP(plus) || node->op == AST_OP(minus))) {
    BinaryASTNode* binary = (BinaryASTNode*)node;
    if (TypeIsPointer(node->type) && binary->left != NULL &&
        PointerExpressionUsesFixedArray(binary->left)) {
      scan->required = true;
    }
  } else if (node->op == AST_OP(call)) {
    Symbol* call_symbol = PCodeConstexprCallSymbol(node);
    VectorASTNode* call = (VectorASTNode*)node;
    if (cxx26 && call_symbol != NULL &&
        (StringEqual(&call_symbol->name, "operator new") ||
         StringEqual(&call_symbol->name, "operator new[]")) &&
        call->children != NULL && call->children->length > 1) {
      scan->required = true;
      return;
    }
    Symbol* callee = PCodeConstexprFunctionDefinition(
        call_symbol);
    ScanConstexprFunctionCapabilities(callee, scan);
  }
}

static void ScanConstexprFunctionCapabilities(
    Symbol* function, ConstexprASTOverlayScan* scan) {
  if (function == NULL || function->type == NULL ||
      !TypeIsFunction(function->type) ||
      ConstexprASTOverlayVisited(scan, function)) {
    return;
  }
  VectorAppend(&scan->visited_functions, function);
  if (function->type->info.function.contract_assertions.length != 0) {
    scan->required = true;
    scan->ast_only = true;
    return;
  }
  if (function->type->info.function.body != NULL) {
    scan->function_depth++;
    ASTNodeVisit(function->type->info.function.body,
                 DetectConstexprASTOverlay, 0, scan);
    scan->function_depth--;
  }
}

ConstexprPCodeCapability ConstexprPCodeCapabilityForExpression(ASTNode* node) {
  ConstexprASTOverlayScan scan = {0};
  VectorInit(&scan.visited_functions);
  ASTNodeVisit(node, DetectConstexprASTOverlay, 0, &scan);
  VectorDestruct(&scan.visited_functions);
  return scan.ast_only
             ? kConstexprPCodeASTOnly
             : scan.required ? kConstexprPCodeRequiresOverlay
                             : kConstexprPCodeEligible;
}

bool ConstexprPCodeRequiresASTOverlay(ASTNode* node) {
  return ConstexprPCodeCapabilityForExpression(node) !=
         kConstexprPCodeEligible;
}

static void ConstexprPCodeRuntimeInit(ConstexprPCodeRuntime* runtime) {
  memset(runtime, 0, sizeof(*runtime));
  VectorInit(&runtime->heap_blocks);
  VectorInit(&runtime->exception_stack);
  VectorInit(&runtime->exception_handles);
  VectorInit(&runtime->lifetime_events);
  runtime->source_size_t_size =
      compiler->target != NULL ? (size_t)compiler->target->pointer_size
                               : sizeof(size_t);
}

static void ConstexprPCodeRuntimeDestruct(ConstexprPCodeRuntime* runtime) {
  for (size_t i = 0; i < runtime->heap_blocks.length; i++) {
    ConstexprPCodeHeapBlock* block = runtime->heap_blocks.value.p[i];
    if (block != NULL) {
      free(block);
    }
  }
  VectorDestruct(&runtime->heap_blocks);
  for (size_t i = 0; i < runtime->exception_stack.length; i++) {
    ConstexprPCodeSavedException* saved =
        runtime->exception_stack.value.p[i];
    if (saved != NULL) {
      free(saved->storage);
      free(saved);
    }
  }
  VectorDestruct(&runtime->exception_stack);
  for (size_t i = 0; i < runtime->exception_handles.length; i++) {
    ConstexprPCodeExceptionHandle* handle =
        runtime->exception_handles.value.p[i];
    if (handle != NULL) {
      free(handle->exception.storage);
      free(handle);
    }
  }
  VectorDestruct(&runtime->exception_handles);
  VectorDestructWithContents(&runtime->lifetime_events, NULL,
                             /*free_element=*/true);
  free(runtime->heap);
  free(runtime->exception_storage);
  runtime->heap = NULL;
  runtime->heap_size = 0;
  runtime->free_list = NULL;
  runtime->exception_storage = NULL;
  runtime->exception_storage_size = 0;
}

static bool ConstexprPCodeRuntimeHasLiveHeap(
    ConstexprPCodeRuntime* runtime) {
  for (size_t i = 0; i < runtime->heap_blocks.length; i++) {
    ConstexprPCodeHeapBlock* block = runtime->heap_blocks.value.p[i];
    if (block != NULL && block->live) {
      return true;
    }
  }
  return false;
}

static ConstexprPCodeHeapBlock* ConstexprPCodeFindHeapBlock(
    ConstexprPCodeRuntime* runtime, void* memory) {
  ConstexprPCodeHeapBlock* dead_match = NULL;
  for (size_t i = 0; i < runtime->heap_blocks.length; i++) {
    ConstexprPCodeHeapBlock* block = runtime->heap_blocks.value.p[i];
    if (block != NULL && block->memory == memory) {
      if (block->live) {
        return block;
      }
      dead_match = block;
    }
  }
  return dead_match;
}

static size_t ConstexprPCodeAlignHeapSize(size_t size) {
  return (size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
}

static bool ConstexprPCodeRuntimeInitHeap(ConstexprPCodeRuntime* runtime) {
  if (runtime->heap != NULL) {
    return true;
  }
  runtime->heap = calloc(1, CONSTEXPR_PCODE_HEAP_SIZE);
  if (runtime->heap == NULL) {
    return false;
  }
  runtime->heap_size = CONSTEXPR_PCODE_HEAP_SIZE;
  runtime->free_list = (ConstexprPCodeFreeBlock*)runtime->heap;
  runtime->free_list->length = runtime->heap_size;
  runtime->free_list->next = NULL;
  return true;
}

static void ConstexprPCodeRemoveFreeBlock(ConstexprPCodeRuntime* runtime,
                                          ConstexprPCodeFreeBlock* previous,
                                          ConstexprPCodeFreeBlock* block) {
  if (previous == NULL) {
    runtime->free_list = block->next;
  } else {
    previous->next = block->next;
  }
}

static void ConstexprPCodeInsertFreeBlock(ConstexprPCodeRuntime* runtime,
                                          ConstexprPCodeFreeBlock* block,
                                          size_t length) {
  block->length = length;
  block->next = NULL;

  ConstexprPCodeFreeBlock* previous = NULL;
  ConstexprPCodeFreeBlock* current = runtime->free_list;
  while (current != NULL && current < block) {
    previous = current;
    current = current->next;
  }

  block->next = current;
  if (previous == NULL) {
    runtime->free_list = block;
  } else {
    previous->next = block;
  }

  if (current != NULL &&
      (unsigned char*)block + block->length == (unsigned char*)current) {
    block->length += current->length;
    block->next = current->next;
  }
  if (previous != NULL &&
      (unsigned char*)previous + previous->length == (unsigned char*)block) {
    previous->length += block->length;
    previous->next = block->next;
  }
}

static void* ConstexprPCodeHeapMalloc(ConstexprPCodeRuntime* runtime,
                                      size_t size, size_t* allocated_size) {
  if (!ConstexprPCodeRuntimeInitHeap(runtime)) {
    return NULL;
  }
  size_t aligned_size = ConstexprPCodeAlignHeapSize(size == 0 ? 1 : size);
  size_t full_size = aligned_size + sizeof(size_t);
  ConstexprPCodeFreeBlock* previous = NULL;
  ConstexprPCodeFreeBlock* block = runtime->free_list;
  while (block != NULL) {
    if (block->length >= full_size) {
      size_t block_length = block->length;
      if (block_length - full_size >= sizeof(ConstexprPCodeFreeBlock)) {
        ConstexprPCodeFreeBlock* remainder =
            (ConstexprPCodeFreeBlock*)((unsigned char*)block + full_size);
        remainder->length = block_length - full_size;
        remainder->next = block->next;
        if (previous == NULL) {
          runtime->free_list = remainder;
        } else {
          previous->next = remainder;
        }
      } else {
        full_size = block_length;
        aligned_size = full_size - sizeof(size_t);
        ConstexprPCodeRemoveFreeBlock(runtime, previous, block);
      }
      *(size_t*)block = aligned_size;
      *allocated_size = aligned_size;
      return (unsigned char*)block + sizeof(size_t);
    }
    previous = block;
    block = block->next;
  }
  return NULL;
}

static void ConstexprPCodeHeapFree(ConstexprPCodeRuntime* runtime,
                                   void* memory, size_t size) {
  ConstexprPCodeFreeBlock* block =
      (ConstexprPCodeFreeBlock*)((unsigned char*)memory - sizeof(size_t));
  ConstexprPCodeInsertFreeBlock(runtime, block, size + sizeof(size_t));
}

static uint64_t ConstexprPCodeStackArgumentAt(PCodeVM* vm, size_t byte_offset,
                                              size_t size) {
  assert(size <= sizeof(uint64_t));
  uint64_t value = 0;
  memcpy(&value,
         (void*)(uintptr_t)(vm->iregs[PCODE_SP_REG] + sizeof(uint64_t) +
                            (int64_t)byte_offset),
         size);
  return value;
}

static uint64_t ConstexprPCodeStackArgument(PCodeVM* vm, size_t index) {
  return ConstexprPCodeStackArgumentAt(
      vm, index * sizeof(uint64_t), sizeof(uint64_t));
}

static PCodeVMStatus ConstexprPCodeAllocateHeapBlock(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime, size_t size) {
  size_t object_size = size == 0 ? 1 : size;
  size_t allocation_size = 0;
  void* memory = ConstexprPCodeHeapMalloc(runtime, size, &allocation_size);
  if (memory == NULL) {
    return kPCodeVMStatusAllocationFailure;
  }
  memset(memory, 0, allocation_size);
  ConstexprPCodeHeapBlock* block = malloc(sizeof(*block));
  if (block == NULL) {
    ConstexprPCodeHeapFree(runtime, memory, allocation_size);
    return kPCodeVMStatusAllocationFailure;
  }
  *block = (ConstexprPCodeHeapBlock){
      .memory = memory,
      .size = object_size,
      .allocation_size = allocation_size,
      .live = true,
  };
  if (!PCodeVMRegisterMemoryRegion(vm, memory, object_size, true)) {
    free(block);
    ConstexprPCodeHeapFree(runtime, memory, allocation_size);
    return kPCodeVMStatusAllocationFailure;
  }
  VectorAppend(&runtime->heap_blocks, block);
  vm->iregs[PCODE_INT_RETURN_REG] = (int64_t)(uintptr_t)memory;
  return kPCodeVMStatusRunning;
}

static PCodeVMStatus ConstexprPCodeEscapeMalloc(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime) {
  size_t size = (size_t)ConstexprPCodeStackArgumentAt(
      vm, 0, runtime->source_size_t_size);
  return ConstexprPCodeAllocateHeapBlock(vm, runtime, size);
}

static PCodeVMStatus ConstexprPCodeEscapeFree(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime) {
  void* memory = (void*)(uintptr_t)ConstexprPCodeStackArgument(vm, 0);
  if (memory == NULL) {
    return kPCodeVMStatusRunning;
  }
  ConstexprPCodeHeapBlock* block =
      ConstexprPCodeFindHeapBlock(runtime, memory);
  if (block == NULL || !block->live) {
    return kPCodeVMStatusInvalidFree;
  }
  PCodeVMUnregisterMemoryRegion(vm, memory);
  ConstexprPCodeHeapFree(runtime, memory, block->allocation_size);
  block->live = false;
  return kPCodeVMStatusRunning;
}

static PCodeVMStatus ConstexprPCodeEscapeRealloc(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime) {
  void* memory = (void*)(uintptr_t)ConstexprPCodeStackArgument(vm, 0);
  size_t size = (size_t)ConstexprPCodeStackArgumentAt(
      vm, sizeof(uint64_t), runtime->source_size_t_size);
  if (memory == NULL) {
    return ConstexprPCodeAllocateHeapBlock(vm, runtime, size);
  }
  if (size == 0) {
    PCodeVMStatus status = ConstexprPCodeEscapeFree(vm, runtime);
    if (status == kPCodeVMStatusRunning) {
      vm->iregs[PCODE_INT_RETURN_REG] = 0;
    }
    return status;
  }
  ConstexprPCodeHeapBlock* block =
      ConstexprPCodeFindHeapBlock(runtime, memory);
  if (block == NULL || !block->live) {
    return kPCodeVMStatusInvalidFree;
  }
  size_t object_size = size == 0 ? 1 : size;
  size_t allocation_size = 0;
  void* new_memory = ConstexprPCodeHeapMalloc(runtime, size, &allocation_size);
  if (new_memory == NULL) {
    return kPCodeVMStatusAllocationFailure;
  }
  memset(new_memory, 0, allocation_size);
  size_t copy_size = block->size < object_size ? block->size : object_size;
  memcpy(new_memory, memory, copy_size);
  if (!PCodeVMRegisterMemoryRegion(vm, new_memory, object_size, true)) {
    ConstexprPCodeHeapFree(runtime, new_memory, allocation_size);
    return kPCodeVMStatusAllocationFailure;
  }
  PCodeVMUnregisterMemoryRegion(vm, memory);
  ConstexprPCodeHeapFree(runtime, memory, block->allocation_size);
  block->memory = new_memory;
  block->size = object_size;
  block->allocation_size = allocation_size;
  block->live = true;
  vm->iregs[PCODE_INT_RETURN_REG] = (int64_t)(uintptr_t)new_memory;
  return kPCodeVMStatusRunning;
}

static PCodeVMStatus ConstexprPCodeEscapePlacementNew(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime) {
  size_t size = (size_t)ConstexprPCodeStackArgumentAt(
      vm, 0, runtime->source_size_t_size);
  uint64_t source_address = ConstexprPCodeStackArgumentAt(
      vm, sizeof(uint64_t), runtime->source_size_t_size);
  uint64_t address = source_address;
  if (runtime->source_size_t_size < sizeof(uint64_t)) {
    uint64_t normalized_address = 0;
    if (!ConstexprPCodeNormalizeAddress(
            vm, address, 0, false, &normalized_address)) {
      return kPCodeVMStatusInvalidRead;
    }
    address = normalized_address;
  }
  ConstexprPCodeLifetimeEvent* event = malloc(sizeof(*event));
  if (event == NULL) {
    return kPCodeVMStatusAllocationFailure;
  }
  size_t union_member_index_plus_one = 0;
  for (size_t i = runtime->lifetime_events.length; i > 0; i--) {
    ConstexprPCodeLifetimeEvent* provenance =
        runtime->lifetime_events.value.p[i - 1];
    if (provenance != NULL &&
        provenance->kind == kConstexprPCodeUnionMemberAddress &&
        provenance->address == address) {
      union_member_index_plus_one = (size_t)provenance->type_token;
      break;
    }
  }
  *event = (ConstexprPCodeLifetimeEvent){
      .address = address,
      .size = size,
      .union_member_index_plus_one = union_member_index_plus_one,
      .kind = kConstexprPCodeLifetimePlacementConstruction,
  };
  VectorAppend(&runtime->lifetime_events, event);
  vm->iregs[PCODE_INT_RETURN_REG] = (int64_t)source_address;
  return kPCodeVMStatusRunning;
}

static PCodeVMStatus ConstexprPCodeEscapeStartLifetime(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime) {
  uint64_t address =
      ConstexprPCodeStackArgumentAt(vm, 0, runtime->source_size_t_size);
  size_t size = (size_t)ConstexprPCodeStackArgument(vm, 1);
  uint64_t type_token = ConstexprPCodeStackArgument(vm, 2);
  if (runtime->source_size_t_size < sizeof(uint64_t)) {
    uint64_t normalized_address = 0;
    if (!ConstexprPCodeNormalizeAddress(
            vm, address, 0, false, &normalized_address)) {
      return kPCodeVMStatusInvalidRead;
    }
    address = normalized_address;
  }
  ConstexprPCodeLifetimeEvent* event = malloc(sizeof(*event));
  if (event == NULL) {
    return kPCodeVMStatusAllocationFailure;
  }
  if (size == CONSTEXPR_PCODE_UNION_MEMBER_ADDRESS_MARKER) {
    *event = (ConstexprPCodeLifetimeEvent){
        .address = address,
        .type_token = type_token,
        .kind = kConstexprPCodeUnionMemberAddress,
    };
    VectorAppend(&runtime->lifetime_events, event);
    return kPCodeVMStatusRunning;
  }
  if (size == CONSTEXPR_PCODE_LIFETIME_END_MARKER) {
    *event = (ConstexprPCodeLifetimeEvent){
        .address = address,
        .kind = kConstexprPCodeLifetimeEnd,
    };
    VectorAppend(&runtime->lifetime_events, event);
    return kPCodeVMStatusRunning;
  }
  if (size == CONSTEXPR_PCODE_LIFETIME_CONSTRUCTION_MARKER) {
    size_t union_member_index_plus_one = 0;
    for (size_t i = runtime->lifetime_events.length; i > 0; i--) {
      ConstexprPCodeLifetimeEvent* provenance =
          runtime->lifetime_events.value.p[i - 1];
      if (provenance != NULL &&
          provenance->kind == kConstexprPCodeUnionMemberAddress &&
          provenance->address == address) {
        union_member_index_plus_one = (size_t)provenance->type_token;
        break;
      }
    }
    *event = (ConstexprPCodeLifetimeEvent){
        .address = address,
        .union_member_index_plus_one = union_member_index_plus_one,
        .kind = kConstexprPCodeLifetimePlacementConstruction,
    };
    VectorAppend(&runtime->lifetime_events, event);
    return kPCodeVMStatusRunning;
  }
  size_t union_member_index_plus_one = 0;
  for (size_t i = runtime->lifetime_events.length; i > 0; i--) {
    ConstexprPCodeLifetimeEvent* provenance =
        runtime->lifetime_events.value.p[i - 1];
    if (provenance != NULL &&
        provenance->kind == kConstexprPCodeUnionMemberAddress &&
        provenance->address == address) {
      union_member_index_plus_one = (size_t)provenance->type_token;
      break;
    }
  }
  *event = (ConstexprPCodeLifetimeEvent){
      .address = address,
      .size = size,
      .type_token = type_token,
      .union_member_index_plus_one = union_member_index_plus_one,
      .kind = kConstexprPCodeLifetimeStartAggregate,
  };
  VectorAppend(&runtime->lifetime_events, event);
  return kPCodeVMStatusRunning;
}

static bool ConstexprPCodeRegionContains(uint64_t start, size_t region_size,
                                         uint64_t address, size_t size) {
  if (size == 0) {
    return address >= start && address <= start + region_size;
  }
  if (size > UINT64_MAX - address || size > UINT64_MAX - start) {
    return false;
  }
  return address >= start && address + size <= start + region_size;
}

static bool ConstexprPCodeNormalizeAddress(PCodeVM* vm, uint64_t raw,
                                           size_t size, bool write,
                                           uint64_t* normalized) {
  if (vm->stack != NULL && vm->stack_size != 0) {
    uint64_t stack_start = (uint64_t)(uintptr_t)vm->stack;
    uint64_t stack_end = stack_start + vm->stack_size;
    if (raw >= stack_start && raw + size <= stack_end) {
      if (raw >= (uint64_t)vm->iregs[PCODE_SP_REG]) {
        *normalized = raw;
        return true;
      }
    }
    for (size_t i = vm->memory_region_count; i > 0; --i) {
      PCodeVMMemoryRegion* region = &vm->memory_regions[i - 1];
      if ((!write || region->writable) &&
          ConstexprPCodeRegionContains(region->start, region->size, raw,
                                       size)) {
        *normalized = raw;
        return true;
      }
    }
    uint64_t stack_address =
        (stack_start & ~UINT64_C(0xffffffff)) | (raw & UINT64_C(0xffffffff));
    if (stack_address + size <= stack_end &&
        stack_address >= (uint64_t)vm->iregs[PCODE_SP_REG]) {
      *normalized = stack_address;
      return true;
    }
  }
  for (size_t i = vm->memory_region_count; i > 0; --i) {
    PCodeVMMemoryRegion* region = &vm->memory_regions[i - 1];
    if (write && !region->writable) {
      continue;
    }
    uint64_t region_address =
        (region->start & ~UINT64_C(0xffffffff)) | (raw & UINT64_C(0xffffffff));
    if (ConstexprPCodeRegionContains(region->start, region->size,
                                     region_address, size)) {
      *normalized = region_address;
      return true;
    }
  }
  return false;
}

static PCodeVMStatus ConstexprPCodeEscapeMemcpy(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime) {
  uint64_t raw_dest = ConstexprPCodeStackArgument(vm, 0);
  uint64_t raw_src = ConstexprPCodeStackArgument(vm, 1);
  size_t size = (size_t)ConstexprPCodeStackArgumentAt(
      vm, 2 * sizeof(uint64_t), runtime->source_size_t_size);
  uint64_t dest_address = 0;
  uint64_t src_address = 0;
  if (!ConstexprPCodeNormalizeAddress(vm, raw_dest, size, true,
                                      &dest_address)) {
    return kPCodeVMStatusInvalidWrite;
  }
  if (!ConstexprPCodeNormalizeAddress(vm, raw_src, size, false,
                                      &src_address)) {
    return kPCodeVMStatusInvalidRead;
  }
  void* dest = (void*)(uintptr_t)dest_address;
  void* src = (void*)(uintptr_t)src_address;
  memcpy(dest, src, size);
  if (!ConstexprPCodeCopyLifetimeEvents(
          runtime, src_address, dest_address, size)) {
    return kPCodeVMStatusAllocationFailure;
  }
  vm->iregs[PCODE_INT_RETURN_REG] = (int64_t)dest_address;
  return kPCodeVMStatusRunning;
}

static bool ConstexprPCodeExceptionStringEqual(const char* left,
                                               const char* right) {
  return left == right ||
         (left != NULL && right != NULL && strcmp(left, right) == 0);
}

static bool ConstexprPCodeExceptionTypeMatches(
    const ConstexprPCodeTypeInfo* thrown,
    const ConstexprPCodeTypeInfo* caught, int64_t* offset) {
  *offset = 0;
  if (caught == NULL) {
    return true;
  }
  if (thrown == NULL) {
    return false;
  }
  if (thrown == caught ||
      ConstexprPCodeExceptionStringEqual(thrown->name, caught->name)) {
    return true;
  }
  const ConstexprPCodeTypeInfoBase* bases = thrown->bases;
  for (int64_t i = 0; bases != NULL && i < thrown->base_count; i++) {
    if (ConstexprPCodeExceptionStringEqual(bases[i].name, caught->name)) {
      *offset = bases[i].offset;
      return true;
    }
  }
  return false;
}

static bool ConstexprPCodeRangeEncloses(uint64_t start, uint64_t end,
                                        uint64_t pc, uint64_t scope_start,
                                        uint64_t scope_end) {
  if (pc < start || pc > end) {
    return false;
  }
  if (start > scope_start || end < scope_end) {
    return false;
  }
  return start < scope_start || end > scope_end;
}

static ConstexprPCodeExceptionRange* ConstexprPCodeFindExceptionAction(
    ConstexprPCodeRuntime* runtime, uint64_t pc, uint64_t scope_start,
    uint64_t scope_end, bool* is_catch, int64_t* offset) {
  if (runtime == NULL || runtime->image == NULL ||
      runtime->image->exception_table == NULL) {
    return NULL;
  }
  size_t count = runtime->image->exception_table_size /
                 sizeof(ConstexprPCodeExceptionRange);
  ConstexprPCodeExceptionRange* ranges =
      (ConstexprPCodeExceptionRange*)runtime->image->exception_table;
  ConstexprPCodeExceptionRange* best = NULL;
  bool best_is_catch = false;
  int64_t best_offset = 0;
  const ConstexprPCodeTypeInfo* thrown =
      (const ConstexprPCodeTypeInfo*)(uintptr_t)runtime->exception_typeinfo;
  for (size_t i = 0; i < count; i++) {
    ConstexprPCodeExceptionRange* range = &ranges[i];
    if (!ConstexprPCodeRangeEncloses(range->try_start, range->try_end, pc,
                                     scope_start, scope_end)) {
      continue;
    }
    bool range_is_catch = range->catch_typeinfo !=
                          DAVECC_EH_CLEANUP_MARKER;
    int64_t range_offset = 0;
    if (range_is_catch &&
        !ConstexprPCodeExceptionTypeMatches(
            thrown,
            (const ConstexprPCodeTypeInfo*)(uintptr_t)range->catch_typeinfo,
            &range_offset)) {
      continue;
    }
    if (best == NULL || range->try_start > best->try_start ||
        (range->try_start == best->try_start &&
         range->try_end < best->try_end)) {
      best = range;
      best_is_catch = range_is_catch;
      best_offset = range_offset;
    }
  }
  if (best != NULL) {
    *is_catch = best_is_catch;
    *offset = best_offset;
  }
  return best;
}

static bool ConstexprPCodeStackContains(PCodeVM* vm, uint64_t address,
                                        size_t size);

static bool ConstexprPCodeResolveReadableAddress(PCodeVM* vm, uint64_t raw,
                                                 size_t size,
                                                 uint64_t* resolved);

static PCodeVMStatus ConstexprPCodeUnwindStep(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime, uint64_t pc, uint64_t ap,
    uint64_t fp, uint64_t scope_start, uint64_t scope_end) {
  for (;;) {
    bool is_catch = false;
    int64_t offset = 0;
    ConstexprPCodeExceptionRange* action =
        ConstexprPCodeFindExceptionAction(runtime, pc, scope_start, scope_end,
                                          &is_catch, &offset);
    if (action != NULL) {
      if (is_catch) {
        runtime->handling_exception = true;
        runtime->exception_handler_depth++;
        runtime->exception_base_offset = offset;
        runtime->resume.active = false;
      } else {
        runtime->resume = (ConstexprPCodeExceptionResume){
            .pc = pc,
            .ap = ap,
            .fp = fp,
            .scope_start = action->try_start,
            .scope_end = action->try_end,
            .active = true,
        };
      }
      vm->iregs[PCODE_PC_REG] = (int64_t)action->catch_label;
      vm->iregs[PCODE_AP_REG] = (int64_t)ap;
      vm->iregs[PCODE_FP_REG] = (int64_t)fp;
      return kPCodeVMStatusRunning;
    }
    if (ap == 0 || fp == 0) {
      return kPCodeVMStatusUncaughtException;
    }
    uint64_t readable_ap = 0;
    uint64_t readable_fp = 0;
    if (!ConstexprPCodeResolveReadableAddress(
            vm, ap, 2 * sizeof(uint64_t), &readable_ap) ||
        !ConstexprPCodeResolveReadableAddress(vm, fp, sizeof(uint64_t),
                                              &readable_fp)) {
      return kPCodeVMStatusInvalidRead;
    }
    uint64_t caller_ap = 0;
    uint64_t caller_pc = 0;
    uint64_t caller_fp = 0;
    memcpy(&caller_ap, (void*)(uintptr_t)readable_ap, sizeof(caller_ap));
    memcpy(&caller_pc,
           (void*)(uintptr_t)(readable_ap + sizeof(uint64_t)),
           sizeof(caller_pc));
    memcpy(&caller_fp, (void*)(uintptr_t)readable_fp, sizeof(caller_fp));
    pc = caller_pc;
    ap = caller_ap;
    fp = caller_fp;
    scope_start = caller_pc;
    scope_end = caller_pc;
  }
}

static void ConstexprPCodeClearException(PCodeVM* vm,
                                         ConstexprPCodeRuntime* runtime);

static bool ConstexprPCodeStackContains(PCodeVM* vm, uint64_t address,
                                        size_t size) {
  if (vm->stack == NULL || vm->stack_size == 0 || size == 0) {
    return false;
  }
  if (size > UINT64_MAX - address) {
    return false;
  }
  uint64_t start = (uint64_t)(uintptr_t)vm->stack;
  uint64_t end = start + vm->stack_size;
  return address >= start && address + size <= end;
}

static bool ConstexprPCodeResolveReadableAddress(PCodeVM* vm, uint64_t raw,
                                                 size_t size,
                                                 uint64_t* resolved) {
  if (ConstexprPCodeStackContains(vm, raw, size)) {
    *resolved = raw;
    return true;
  }
  return ConstexprPCodeNormalizeAddress(vm, raw, size, false, resolved);
}

static bool ConstexprPCodeThrowSitePC(PCodeVM* vm, uint64_t* throw_pc) {
  uint64_t sp = (uint64_t)vm->iregs[PCODE_SP_REG];
  uint64_t readable_sp = 0;
  if (ConstexprPCodeResolveReadableAddress(vm, sp, sizeof(*throw_pc),
                                           &readable_sp)) {
    memcpy(throw_pc, (void*)(uintptr_t)readable_sp, sizeof(*throw_pc));
    return true;
  }
  uint64_t ap = (uint64_t)vm->iregs[PCODE_AP_REG];
  if (ap != 0 &&
      ConstexprPCodeResolveReadableAddress(
          vm, ap + sizeof(uint64_t), sizeof(*throw_pc), &readable_sp)) {
    memcpy(throw_pc, (void*)(uintptr_t)readable_sp, sizeof(*throw_pc));
    return true;
  }
  return false;
}

static PCodeVMStatus ConstexprPCodeBeginUnwind(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime) {
  uint64_t throw_pc = 0;
  if (!ConstexprPCodeThrowSitePC(vm, &throw_pc)) {
    return kPCodeVMStatusInvalidRead;
  }
  return ConstexprPCodeUnwindStep(
      vm, runtime, throw_pc, (uint64_t)vm->iregs[PCODE_AP_REG],
      (uint64_t)vm->iregs[PCODE_FP_REG], throw_pc, throw_pc);
}

static PCodeVMStatus ConstexprPCodeInstallException(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime,
    ConstexprPCodeExceptionKind kind, uint64_t value,
    uint64_t typeinfo_address, uint64_t destructor_address) {
  if (runtime->has_exception && !runtime->handling_exception) {
    return kPCodeVMStatusInvalidConstantOperation;
  }
  if (runtime->has_exception) {
    ConstexprPCodeSavedException* saved = malloc(sizeof(*saved));
    if (saved == NULL) {
      return kPCodeVMStatusAllocationFailure;
    }
    *saved = (ConstexprPCodeSavedException){
        .typeinfo = runtime->exception_typeinfo,
        .destructor = runtime->exception_destructor,
        .direct = runtime->exception_direct,
        .i8 = runtime->exception_i8,
        .f4 = runtime->exception_f4,
        .f8 = runtime->exception_f8,
        .storage = runtime->exception_storage,
        .storage_size = runtime->exception_storage_size,
        .base_offset = runtime->exception_base_offset,
        .kind = runtime->exception_kind,
        .handling = runtime->handling_exception,
        .handler_depth = runtime->exception_handler_depth,
    };
    VectorAppend(&runtime->exception_stack, saved);
  } else {
    if (runtime->exception_storage != NULL) {
      PCodeVMUnregisterMemoryRegion(vm, runtime->exception_storage);
    }
    free(runtime->exception_storage);
  }
  runtime->exception_storage = NULL;
  runtime->exception_storage_size = 0;
  runtime->exception_direct = 0;
  runtime->exception_i8 = 0;
  runtime->exception_f4 = 0;
  runtime->exception_f8 = 0;
  runtime->exception_kind = kind;
  runtime->exception_typeinfo = typeinfo_address;
  runtime->exception_destructor = destructor_address;
  runtime->exception_base_offset = 0;
  runtime->has_exception = true;
  runtime->handling_exception = false;
  runtime->exception_handler_depth = 0;
  runtime->resume.active = false;
  if (kind == kConstexprPCodeExceptionDirect) {
    const ConstexprPCodeTypeInfo* typeinfo =
        (const ConstexprPCodeTypeInfo*)(uintptr_t)typeinfo_address;
    runtime->exception_direct = value;
    if (typeinfo != NULL && typeinfo->object_is_class &&
        typeinfo->object_size > 0) {
      uint64_t source = 0;
      if (!ConstexprPCodeResolveReadableAddress(
              vm, value, (size_t)typeinfo->object_size, &source)) {
        ConstexprPCodeClearException(vm, runtime);
        return kPCodeVMStatusInvalidRead;
      }
      runtime->exception_storage =
          malloc((size_t)typeinfo->object_size);
      if (runtime->exception_storage == NULL) {
        ConstexprPCodeClearException(vm, runtime);
        return kPCodeVMStatusAllocationFailure;
      }
      runtime->exception_storage_size = (size_t)typeinfo->object_size;
      memcpy(runtime->exception_storage, (void*)(uintptr_t)source,
             runtime->exception_storage_size);
      if (!PCodeVMRegisterMemoryRegion(
              vm, runtime->exception_storage,
              runtime->exception_storage_size, true)) {
        free(runtime->exception_storage);
        runtime->exception_storage = NULL;
        runtime->exception_storage_size = 0;
        ConstexprPCodeClearException(vm, runtime);
        return kPCodeVMStatusAllocationFailure;
      }
      runtime->exception_direct =
          (uint64_t)(uintptr_t)runtime->exception_storage;
    }
  } else if (kind == kConstexprPCodeExceptionI8) {
    runtime->exception_i8 = (int64_t)value;
  } else if (kind == kConstexprPCodeExceptionF4) {
    uint32_t bits = (uint32_t)value;
    memcpy(&runtime->exception_f4, &bits, sizeof(bits));
  } else {
    memcpy(&runtime->exception_f8, &value, sizeof(value));
  }
  return ConstexprPCodeBeginUnwind(vm, runtime);
}

static PCodeVMStatus ConstexprPCodeEscapeThrow(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime,
    ConstexprPCodeExceptionKind kind) {
  uint64_t value = ConstexprPCodeStackArgument(vm, 0);
  size_t typeinfo_offset =
      kind == kConstexprPCodeExceptionF4 ? sizeof(float) : sizeof(uint64_t);
  uint64_t typeinfo = ConstexprPCodeStackArgumentAt(
      vm, typeinfo_offset, sizeof(uint64_t));
  if (kind == kConstexprPCodeExceptionDirect && value == 0 &&
      typeinfo == 0) {
    if (!runtime->has_exception || !runtime->handling_exception) {
      return kPCodeVMStatusInvalidConstantOperation;
    }
    runtime->handling_exception = false;
    runtime->exception_base_offset = 0;
    return ConstexprPCodeBeginUnwind(vm, runtime);
  }
  return ConstexprPCodeInstallException(vm, runtime, kind, value, typeinfo, 0);
}

static uint64_t ConstexprPCodeCurrentExceptionPointer(
    ConstexprPCodeRuntime* runtime) {
  uint64_t pointer = runtime->exception_direct;
  if (runtime->exception_storage != NULL) {
    pointer = (uint64_t)(uintptr_t)runtime->exception_storage;
  }
  return pointer + (uint64_t)runtime->exception_base_offset;
}

static ConstexprPCodeExceptionHandle* ConstexprPCodeFindExceptionHandle(
    ConstexprPCodeRuntime* runtime, uint64_t address, size_t* index) {
  ConstexprPCodeExceptionHandle* requested =
      (ConstexprPCodeExceptionHandle*)(uintptr_t)address;
  for (size_t i = 0; i < runtime->exception_handles.length; i++) {
    if (runtime->exception_handles.value.p[i] == requested) {
      if (index != NULL) {
        *index = i;
      }
      return requested;
    }
  }
  return NULL;
}

static PCodeVMStatus ConstexprPCodeExceptionPtrCurrent(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime) {
  if (!runtime->has_exception) {
    vm->iregs[PCODE_INT_RETURN_REG] = 0;
    return kPCodeVMStatusRunning;
  }
  ConstexprPCodeExceptionHandle* handle = calloc(1, sizeof(*handle));
  if (handle == NULL) {
    return kPCodeVMStatusAllocationFailure;
  }
  handle->exception = (ConstexprPCodeSavedException){
      .typeinfo = runtime->exception_typeinfo,
      .destructor = runtime->exception_destructor,
      .direct = runtime->exception_direct,
      .i8 = runtime->exception_i8,
      .f4 = runtime->exception_f4,
      .f8 = runtime->exception_f8,
      .storage_size = runtime->exception_storage_size,
      .base_offset = runtime->exception_base_offset,
      .kind = runtime->exception_kind,
  };
  if (runtime->exception_storage_size != 0) {
    handle->exception.storage = malloc(runtime->exception_storage_size);
    if (handle->exception.storage == NULL) {
      free(handle);
      return kPCodeVMStatusAllocationFailure;
    }
    memcpy(handle->exception.storage, runtime->exception_storage,
           runtime->exception_storage_size);
    if (runtime->exception_direct >=
            (uint64_t)(uintptr_t)runtime->exception_storage &&
        runtime->exception_direct <
            (uint64_t)(uintptr_t)runtime->exception_storage +
                runtime->exception_storage_size) {
      handle->exception.direct =
          (uint64_t)(uintptr_t)handle->exception.storage +
          (runtime->exception_direct -
           (uint64_t)(uintptr_t)runtime->exception_storage);
    }
  }
  handle->references = 1;
  VectorAppend(&runtime->exception_handles, handle);
  vm->iregs[PCODE_INT_RETURN_REG] = (int64_t)(intptr_t)handle;
  return kPCodeVMStatusRunning;
}

static PCodeVMStatus ConstexprPCodeExceptionPtrRetainRelease(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime, bool retain) {
  uint64_t address = ConstexprPCodeStackArgument(vm, 0);
  if (address == 0) {
    return kPCodeVMStatusRunning;
  }
  size_t index = 0;
  ConstexprPCodeExceptionHandle* handle =
      ConstexprPCodeFindExceptionHandle(runtime, address, &index);
  if (handle == NULL || handle->references == 0) {
    return kPCodeVMStatusInvalidConstantOperation;
  }
  if (retain) {
    handle->references++;
    return kPCodeVMStatusRunning;
  }
  handle->references--;
  if (handle->references == 0) {
    if (handle->exception.destructor != 0) {
      return kPCodeVMStatusInvalidConstantOperation;
    }
    free(handle->exception.storage);
    free(handle);
    runtime->exception_handles.value.p[index] = NULL;
  }
  return kPCodeVMStatusRunning;
}

static PCodeVMStatus ConstexprPCodeExceptionPtrRethrow(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime) {
  uint64_t address = ConstexprPCodeStackArgument(vm, 0);
  ConstexprPCodeExceptionHandle* handle =
      ConstexprPCodeFindExceptionHandle(runtime, address, NULL);
  if (handle == NULL || handle->references == 0 ||
      (runtime->has_exception && !runtime->handling_exception)) {
    return kPCodeVMStatusInvalidConstantOperation;
  }
  if (runtime->has_exception) {
    ConstexprPCodeSavedException* saved = malloc(sizeof(*saved));
    if (saved == NULL) {
      return kPCodeVMStatusAllocationFailure;
    }
    *saved = (ConstexprPCodeSavedException){
        .typeinfo = runtime->exception_typeinfo,
        .destructor = runtime->exception_destructor,
        .direct = runtime->exception_direct,
        .i8 = runtime->exception_i8,
        .f4 = runtime->exception_f4,
        .f8 = runtime->exception_f8,
        .storage = runtime->exception_storage,
        .storage_size = runtime->exception_storage_size,
        .base_offset = runtime->exception_base_offset,
        .kind = runtime->exception_kind,
        .handling = runtime->handling_exception,
        .handler_depth = runtime->exception_handler_depth,
    };
    VectorAppend(&runtime->exception_stack, saved);
  }
  runtime->exception_storage = NULL;
  runtime->exception_storage_size = handle->exception.storage_size;
  if (handle->exception.storage_size != 0) {
    runtime->exception_storage = malloc(handle->exception.storage_size);
    if (runtime->exception_storage == NULL) {
      return kPCodeVMStatusAllocationFailure;
    }
    memcpy(runtime->exception_storage, handle->exception.storage,
           handle->exception.storage_size);
    if (!PCodeVMRegisterMemoryRegion(
            vm, runtime->exception_storage, runtime->exception_storage_size,
            true)) {
      free(runtime->exception_storage);
      runtime->exception_storage = NULL;
      runtime->exception_storage_size = 0;
      return kPCodeVMStatusAllocationFailure;
    }
  }
  runtime->exception_typeinfo = handle->exception.typeinfo;
  runtime->exception_destructor = handle->exception.destructor;
  runtime->exception_direct = handle->exception.direct;
  if (handle->exception.storage != NULL &&
      handle->exception.direct >=
          (uint64_t)(uintptr_t)handle->exception.storage &&
      handle->exception.direct <
          (uint64_t)(uintptr_t)handle->exception.storage +
              handle->exception.storage_size) {
    runtime->exception_direct =
        (uint64_t)(uintptr_t)runtime->exception_storage +
        (handle->exception.direct -
         (uint64_t)(uintptr_t)handle->exception.storage);
  }
  runtime->exception_i8 = handle->exception.i8;
  runtime->exception_f4 = handle->exception.f4;
  runtime->exception_f8 = handle->exception.f8;
  runtime->exception_base_offset = handle->exception.base_offset;
  runtime->exception_kind = handle->exception.kind;
  runtime->has_exception = true;
  runtime->handling_exception = false;
  runtime->exception_handler_depth = 0;
  runtime->ending_exception = false;
  runtime->resume.active = false;
  return ConstexprPCodeBeginUnwind(vm, runtime);
}

static PCodeVMStatus ConstexprPCodeUncaughtExceptions(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime) {
  int64_t count =
      runtime->has_exception && !runtime->handling_exception ? 1 : 0;
  for (size_t i = 0; i < runtime->exception_stack.length; i++) {
    ConstexprPCodeSavedException* saved =
        runtime->exception_stack.value.p[i];
    if (saved != NULL && !saved->handling) {
      count++;
    }
  }
  vm->iregs[PCODE_INT_RETURN_REG] = count;
  return kPCodeVMStatusRunning;
}

static void ConstexprPCodeClearException(PCodeVM* vm,
                                         ConstexprPCodeRuntime* runtime) {
  if (runtime->exception_storage != NULL) {
    PCodeVMUnregisterMemoryRegion(vm, runtime->exception_storage);
    free(runtime->exception_storage);
  }
  runtime->exception_storage = NULL;
  runtime->exception_storage_size = 0;
  runtime->exception_typeinfo = 0;
  runtime->exception_destructor = 0;
  runtime->exception_direct = 0;
  runtime->exception_base_offset = 0;
  runtime->has_exception = false;
  runtime->handling_exception = false;
  runtime->exception_handler_depth = 0;
  runtime->ending_exception = false;
  runtime->resume.active = false;
  if (runtime->exception_stack.length != 0) {
    ConstexprPCodeSavedException* saved =
        VectorLast(&runtime->exception_stack);
    runtime->exception_stack.length--;
    runtime->exception_typeinfo = saved->typeinfo;
    runtime->exception_destructor = saved->destructor;
    runtime->exception_direct = saved->direct;
    runtime->exception_i8 = saved->i8;
    runtime->exception_f4 = saved->f4;
    runtime->exception_f8 = saved->f8;
    runtime->exception_storage = saved->storage;
    runtime->exception_storage_size = saved->storage_size;
    runtime->exception_base_offset = saved->base_offset;
    runtime->exception_kind = saved->kind;
    runtime->has_exception = true;
    runtime->handling_exception = saved->handling;
    runtime->exception_handler_depth = saved->handler_depth;
    free(saved);
  }
}

static PCodeVMStatus ConstexprPCodeBeginEndCatch(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime, bool halt_after) {
  if (!runtime->has_exception || !runtime->handling_exception) {
    return kPCodeVMStatusInvalidConstantOperation;
  }
  if (runtime->exception_handler_depth > 1) {
    runtime->exception_handler_depth--;
    return halt_after ? kPCodeVMStatusHalted : kPCodeVMStatusRunning;
  }
  if (runtime->exception_handler_depth != 1) {
    return kPCodeVMStatusInvalidConstantOperation;
  }
  if (runtime->exception_destructor == 0) {
    ConstexprPCodeClearException(vm, runtime);
    return halt_after ? kPCodeVMStatusHalted : kPCodeVMStatusRunning;
  }
  uint64_t object = runtime->exception_storage != NULL
                        ? (uint64_t)(uintptr_t)runtime->exception_storage
                        : runtime->exception_direct;
  uint64_t sp = (uint64_t)vm->iregs[PCODE_SP_REG] -
                2 * sizeof(uint64_t);
  uint64_t normalized_sp = 0;
  if (!ConstexprPCodeNormalizeAddress(
          vm, sp, 2 * sizeof(uint64_t), true, &normalized_sp)) {
    return kPCodeVMStatusInvalidWrite;
  }
  memcpy((void*)(uintptr_t)(normalized_sp + sizeof(uint64_t)), &object,
         sizeof(object));
  uint64_t completion =
      (uint64_t)(uintptr_t)constexpr_pcode_end_catch_complete_stub;
  memcpy((void*)(uintptr_t)normalized_sp, &completion, sizeof(completion));
  vm->iregs[PCODE_SP_REG] = (int64_t)normalized_sp;
  vm->iregs[PCODE_PC_REG] = (int64_t)runtime->exception_destructor;
  runtime->handling_exception = false;
  runtime->ending_exception = true;
  runtime->halt_after_exception_destructor = halt_after;
  return kPCodeVMStatusRunning;
}

static PCodeVMStatus ConstexprPCodeCompleteEndCatch(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime) {
  if (!runtime->has_exception || !runtime->ending_exception) {
    return kPCodeVMStatusInvalidConstantOperation;
  }
  bool halt_after = runtime->halt_after_exception_destructor;
  vm->iregs[PCODE_SP_REG] += sizeof(uint64_t);
  ConstexprPCodeClearException(vm, runtime);
  return halt_after ? kPCodeVMStatusHalted : kPCodeVMStatusRunning;
}

static PCodeVMStatus ConstexprPCodeEscapeConstexprThrow(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime) {
  uint64_t value = ConstexprPCodeStackArgument(vm, 0);
  uint64_t typeinfo = ConstexprPCodeStackArgument(vm, 1);
  uint64_t destructor = ConstexprPCodeStackArgument(vm, 2);
  if (value == 0 && typeinfo == 0) {
    return ConstexprPCodeEscapeThrow(
        vm, runtime, kConstexprPCodeExceptionDirect);
  }
  return ConstexprPCodeInstallException(
      vm, runtime, kConstexprPCodeExceptionDirect, value, typeinfo,
      destructor);
}

static PCodeVMStatus ConstexprPCodeEscapeResume(
    PCodeVM* vm, ConstexprPCodeRuntime* runtime) {
  if (!runtime->has_exception || !runtime->resume.active) {
    return kPCodeVMStatusInvalidConstantOperation;
  }
  ConstexprPCodeExceptionResume resume = runtime->resume;
  runtime->resume.active = false;
  return ConstexprPCodeUnwindStep(
      vm, runtime, resume.pc, resume.ap, resume.fp, resume.scope_start,
      resume.scope_end);
}

static PCodeVMStatus ConstexprEscape(PCodeVM* vm, int32_t code, void* data) {
  ConstexprPCodeRuntime* runtime = data;
  switch (code) {
    case 4:
      if (runtime != NULL && runtime->has_exception &&
          runtime->handling_exception) {
        return ConstexprPCodeBeginEndCatch(vm, runtime, true);
      }
      return kPCodeVMStatusHalted;
    case kConstexprPCodeEscapeMalloc:
      return runtime != NULL ? ConstexprPCodeEscapeMalloc(vm, runtime)
                             : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeFree:
      return runtime != NULL ? ConstexprPCodeEscapeFree(vm, runtime)
                             : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeRealloc:
      return runtime != NULL ? ConstexprPCodeEscapeRealloc(vm, runtime)
                             : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapePlacementNew:
      return runtime != NULL ? ConstexprPCodeEscapePlacementNew(vm, runtime)
                             : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeStartLifetime:
      return runtime != NULL
                 ? ConstexprPCodeEscapeStartLifetime(vm, runtime)
                 : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeThrow:
      return runtime != NULL
                 ? ConstexprPCodeEscapeThrow(
                       vm, runtime, kConstexprPCodeExceptionDirect)
                 : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeThrowI8:
      return runtime != NULL
                 ? ConstexprPCodeEscapeThrow(
                       vm, runtime, kConstexprPCodeExceptionI8)
                 : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeThrowF4:
      return runtime != NULL
                 ? ConstexprPCodeEscapeThrow(
                       vm, runtime, kConstexprPCodeExceptionF4)
                 : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeThrowF8:
      return runtime != NULL
                 ? ConstexprPCodeEscapeThrow(
                       vm, runtime, kConstexprPCodeExceptionF8)
                 : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeCurrentExceptionInteger:
      if (runtime == NULL || !runtime->has_exception) {
        return kPCodeVMStatusInvalidConstantOperation;
      }
      vm->iregs[PCODE_INT_RETURN_REG] =
          runtime->exception_kind == kConstexprPCodeExceptionI8
              ? runtime->exception_i8
              : (int64_t)runtime->exception_direct;
      return kPCodeVMStatusRunning;
    case kConstexprPCodeEscapeCurrentExceptionF4:
      if (runtime == NULL ||
          runtime->exception_kind != kConstexprPCodeExceptionF4) {
        return kPCodeVMStatusInvalidConstantOperation;
      }
      vm->fregs[PCODE_FLOAT_RETURN_REG] = runtime->exception_f4;
      return kPCodeVMStatusRunning;
    case kConstexprPCodeEscapeCurrentExceptionF8:
      if (runtime == NULL ||
          runtime->exception_kind != kConstexprPCodeExceptionF8) {
        return kPCodeVMStatusInvalidConstantOperation;
      }
      vm->dregs[PCODE_DOUBLE_RETURN_REG] = runtime->exception_f8;
      return kPCodeVMStatusRunning;
    case kConstexprPCodeEscapeCurrentExceptionPointer:
      if (runtime == NULL || !runtime->has_exception) {
        return kPCodeVMStatusInvalidConstantOperation;
      }
      vm->iregs[PCODE_INT_RETURN_REG] =
          (int64_t)ConstexprPCodeCurrentExceptionPointer(runtime);
      return kPCodeVMStatusRunning;
    case kConstexprPCodeEscapeCurrentExceptionAddress:
      if (runtime == NULL || !runtime->has_exception) {
        return kPCodeVMStatusInvalidConstantOperation;
      }
      if (runtime->exception_kind == kConstexprPCodeExceptionI8) {
        vm->iregs[PCODE_INT_RETURN_REG] =
            (int64_t)(uintptr_t)&runtime->exception_i8;
      } else if (runtime->exception_kind == kConstexprPCodeExceptionF4) {
        vm->iregs[PCODE_INT_RETURN_REG] =
            (int64_t)(uintptr_t)&runtime->exception_f4;
      } else if (runtime->exception_kind == kConstexprPCodeExceptionF8) {
        vm->iregs[PCODE_INT_RETURN_REG] =
            (int64_t)(uintptr_t)&runtime->exception_f8;
      } else {
        vm->iregs[PCODE_INT_RETURN_REG] =
            (int64_t)(uintptr_t)&runtime->exception_direct;
      }
      return kPCodeVMStatusRunning;
    case kConstexprPCodeEscapeResume:
      return runtime != NULL ? ConstexprPCodeEscapeResume(vm, runtime)
                             : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeConstexprThrow:
      return runtime != NULL
                 ? ConstexprPCodeEscapeConstexprThrow(vm, runtime)
                 : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeEndCatch:
      return runtime != NULL
                 ? ConstexprPCodeBeginEndCatch(vm, runtime, false)
                 : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeEndCatchComplete:
      return runtime != NULL
                 ? ConstexprPCodeCompleteEndCatch(vm, runtime)
                 : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeExceptionPtrCurrent:
      return runtime != NULL
                 ? ConstexprPCodeExceptionPtrCurrent(vm, runtime)
                 : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeExceptionPtrRetain:
      return runtime != NULL
                 ? ConstexprPCodeExceptionPtrRetainRelease(vm, runtime, true)
                 : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeExceptionPtrRelease:
      return runtime != NULL
                 ? ConstexprPCodeExceptionPtrRetainRelease(vm, runtime, false)
                 : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeExceptionPtrRethrow:
      return runtime != NULL
                 ? ConstexprPCodeExceptionPtrRethrow(vm, runtime)
                 : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeUncaughtExceptions:
      return runtime != NULL
                 ? ConstexprPCodeUncaughtExceptions(vm, runtime)
                 : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeMemcpy:
      return runtime != NULL ? ConstexprPCodeEscapeMemcpy(vm, runtime)
                             : kPCodeVMStatusUndefinedEscape;
    case kConstexprPCodeEscapeInvalidConstantOperation:
      return kPCodeVMStatusInvalidConstantOperation;
    default:
      return kPCodeVMStatusUndefinedEscape;
  }
}

static ASTNode* IdentityConstexprThunkClone(ASTNode* node, void* data) {
  (void)data;
  return node;
}

typedef struct {
  ConstEvalContext* ctx;
  ASTNode* expression;
  bool has_unbound_automatic;
} ConstexprThunkBindingCheck;

static void CheckConstexprThunkBindings(ASTNode* node, void* data, int child_id,
                                        VisitorMode mode) {
  (void)child_id;
  ConstexprThunkBindingCheck* check = data;
  if (mode != kVisitPreChildren || node == NULL ||
      check->has_unbound_automatic || node->op != AST_OP(identifier)) {
    return;
  }
  for (ASTNode* parent = node->parent; parent != NULL;
       parent = parent->parent) {
    if (parent == check->expression) {
      break;
    }
    if ((parent->flags & kASTLambdaExpression) != 0) {
      return;
    }
  }
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  if (symbol != NULL &&
      (symbol->flags.is_argument ||
       StorageIs(symbol->storage, STO(auto) | STO(register))) &&
      !symbol->flags.value_set &&
      !ConstexprHasBinding(check->ctx, symbol)) {
    check->has_unbound_automatic = true;
  }
}

static bool ConstexprExpressionCanUseThunk(ConstEvalContext* ctx,
                                           ASTNode* expression) {
  ConstexprThunkBindingCheck check = {
      .ctx = ctx,
      .expression = expression,
      .has_unbound_automatic = false,
  };
  ASTNodeVisit(expression, CheckConstexprThunkBindings, 0, &check);
  return !check.has_unbound_automatic;
}

static TypeRecord* NewConstexprExpressionThunk(ASTNode* expression) {
  static uint64_t next_thunk_id;
  if (expression == NULL || expression->type == NULL) {
    return NULL;
  }
  if (pcode_thunk_cache_initialized) {
    for (size_t i = 0; i < pcode_thunk_cache.length; i++) {
      ConstexprPCodeThunkCacheEntry* entry = pcode_thunk_cache.value.p[i];
      if (entry != NULL && entry->expression == expression) {
        return entry->function;
      }
    }
  }
  char name[64];
  snprintf(name, sizeof(name), "__davecc_constexpr_thunk_%llu",
           (unsigned long long)next_thunk_id++);
  TypeRecord* function = NewFunctionTypeRecord();
  TypeRecordChain(function, TypeRecordCopy(expression->type));
  Symbol* symbol = NewSymbol(name, function, STO(static));
  symbol->flags.is_defined = true;
  symbol->flags.is_inline_defn = true;
  function->info.function.symbol = symbol;
  function->info.function.definition = true;
  function->info.function.is_inline = true;
  function->info.function.is_constexpr = true;

  ASTNode* cloned = ASTNodeClone(expression, IdentityConstexprThunkClone, NULL,
                                 NULL);
  if (cloned == NULL) {
    return NULL;
  }
  ASTNode* return_statement = NewCombinedStatementASTNode(
      AST_OP(return), cloned, NULL, expression->location);
  return_statement->flags |= kASTAnalyzed;
  Vector* statements = NewVector();
  VectorAppend(statements, return_statement);
  function->info.function.body =
      NewCompoundStatementASTNode(statements, expression->location);
  function->info.function.body->flags |= kASTAnalyzed;
  VectorAppend(&compiler->orphan_function_symbols, symbol);
  if (!pcode_thunk_cache_initialized) {
    VectorInit(&pcode_thunk_cache);
    pcode_thunk_cache_initialized = true;
  }
  ConstexprPCodeThunkCacheEntry* entry = malloc(sizeof(*entry));
  if (entry != NULL) {
    *entry = (ConstexprPCodeThunkCacheEntry){
        .expression = expression,
        .function = function,
    };
    VectorAppend(&pcode_thunk_cache, entry);
  }
  return function;
}

static bool RunConstexprExpressionThunk(
    ConstEvalContext* ctx, ASTNode* expression, int64_t* integer_result,
    double* floating_result, ConstexprObject** object_result,
    ConstexprValue* address_result,
    ConstexprPCodeFailureKind* failure_kind, const char** reason) {
  *failure_kind = kConstexprPCodeFailureUnsupported;
  if (!ConstexprExpressionCanUseThunk(ctx, expression)) {
    *reason = "constexpr pcode thunk has an unbound automatic variable";
    return false;
  }
  TypeRecord* thunk = NewConstexprExpressionThunk(expression);
  if (thunk == NULL || thunk->info.function.symbol == NULL) {
    *reason = "could not build constexpr pcode expression thunk";
    return false;
  }
  ASTNode* callee =
      NewIdentifierASTNode(thunk->info.function.symbol, expression->location);
  ASTNode* call = NewVectorASTNode(AST_OP(call), TypeRecordCopy(thunk->next),
                                   expression->location, callee, NewVector());
  call->flags |= kASTAnalyzed;
  bool ok = RunRealPCodeCall(ctx, call, thunk, integer_result, floating_result,
                             object_result, address_result, failure_kind,
                             reason);
  ASTNodeDelete(call);
  return ok;
}

bool ConstexprPCodeEvaluateCallAsInteger(ConstEvalContext* ctx, ASTNode* node,
                                         int64_t* result) {
  const char* reason = NULL;
  if (!ConstexprExpressionCanUseThunk(ctx, node)) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode call has an unbound automatic variable");
  }
  if (!ConstexprPCodeValidateCall(node, &reason)) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported, reason);
  }
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  // A function returning a reference leaves the address of the referent (not
  // the referent's value) in the integer return register.  Reading it as the
  // integer result would leak that address, so evaluate the call as an address
  // and dereference it to obtain the referred-to integer.
  if (callee != NULL && callee->type != NULL && callee->type->next != NULL &&
      TypeIsReference(callee->type->next)) {
    ConstexprValue value = {0};
    if (!ConstexprPCodeEvaluateCallAsAddress(ctx, node, &value)) {
      return false;
    }
    if (!ConstexprValueAsInteger(value, result)) {
      return ConstexprPCodeFailure(
          ctx, kConstexprPCodeFailureUnsupported,
          "could not decode constexpr pcode reference result");
    }
    return ConstexprPCodeSuccess(ctx);
  }
  ConstexprPCodeFailureKind failure_kind =
      kConstexprPCodeFailureUnsupported;
  bool ok = RunRealPCodeCall(ctx, node, callee->type, result, NULL, NULL, NULL,
                             &failure_kind, &reason);
  if (!ok &&
      (compiler->constexpr_eval_mode == kConstexprEvalAuto ||
       (compiler->constexpr_eval_mode == kConstexprEvalPCode &&
        (compiler->current_function == NULL ||
         compiler->constant_evaluation_required_depth > 0)))) {
    ok = RunConstexprExpressionThunk(ctx, node, result, NULL, NULL, NULL,
                                     &failure_kind, &reason);
  }
  return ok ? ConstexprPCodeSuccess(ctx)
            : ConstexprPCodeFailure(ctx, failure_kind, reason);
}

bool ConstexprPCodeEvaluateCallAsFloating(ConstEvalContext* ctx, ASTNode* node,
                                          double* result) {
  const char* reason = NULL;
  if (!ConstexprExpressionCanUseThunk(ctx, node)) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode call has an unbound automatic variable");
  }
  if (!ConstexprPCodeValidateCall(node, &reason)) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported, reason);
  }
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  // See ConstexprPCodeEvaluateCallAsInteger: a reference return yields the
  // referent's address in the return register; dereference it instead of
  // reinterpreting the address bits as a floating-point value.
  if (callee != NULL && callee->type != NULL && callee->type->next != NULL &&
      TypeIsReference(callee->type->next)) {
    ConstexprValue value = {0};
    if (!ConstexprPCodeEvaluateCallAsAddress(ctx, node, &value)) {
      return false;
    }
    if (!ConstexprValueAsFloating(value, result)) {
      return ConstexprPCodeFailure(
          ctx, kConstexprPCodeFailureUnsupported,
          "could not decode constexpr pcode reference result");
    }
    return ConstexprPCodeSuccess(ctx);
  }
  ConstexprPCodeFailureKind failure_kind =
      kConstexprPCodeFailureUnsupported;
  TypeRecord* return_type =
      callee != NULL && callee->type != NULL ? callee->type->next : NULL;
  if (TypeIsIntegral(return_type)) {
    int64_t integer = 0;
    bool ok = RunRealPCodeCall(ctx, node, callee->type, &integer, NULL, NULL,
                               NULL, &failure_kind, &reason);
    if (!ok) {
      ok = RunConstexprExpressionThunk(ctx, node, &integer, NULL, NULL, NULL,
                                       &failure_kind, &reason);
    }
    if (ok) {
      *result = TypeIsUnsigned(return_type) ? (double)(uint64_t)integer
                                            : (double)integer;
    }
    return ok ? ConstexprPCodeSuccess(ctx)
              : ConstexprPCodeFailure(ctx, failure_kind, reason);
  }
  if (return_type == NULL) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode call has no function definition");
  }
  bool ok = RunRealPCodeCall(ctx, node, callee->type, NULL, result, NULL, NULL,
                             &failure_kind, &reason);
  if (!ok) {
    ok = RunConstexprExpressionThunk(ctx, node, NULL, result, NULL, NULL,
                                     &failure_kind, &reason);
  }
  return ok ? ConstexprPCodeSuccess(ctx)
            : ConstexprPCodeFailure(ctx, failure_kind, reason);
}

bool ConstexprPCodeEvaluateCall(ConstEvalContext* ctx, ASTNode* node) {
  const char* reason = NULL;
  if (!ConstexprExpressionCanUseThunk(ctx, node)) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode call has an unbound automatic variable");
  }
  if (!ConstexprPCodeValidateCall(node, &reason)) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported, reason);
  }
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  if (callee == NULL || callee->type == NULL) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode call has no function definition");
  }
  if (callee->type->info.function.is_constructor &&
      callee->type->info.function.prototype.length != 0) {
    Symbol* this_parameter =
        callee->type->info.function.prototype.value.p[0];
    TypeRecord* object_type =
        this_parameter != NULL ? this_parameter->type : NULL;
    if (TypeIsPointer(object_type)) {
      object_type = object_type->next;
    }
    ConstexprObject* object = NULL;
    bool ok = TypeIsStructOrUnion(object_type) &&
              ConstexprPCodeEvaluateConstructorObject(ctx, object_type, node,
                                                     &object);
    DeletePCodeConstexprObject(object);
    return ok;
  }
  ConstexprPCodeFailureKind failure_kind =
      kConstexprPCodeFailureUnsupported;
  bool ok = RunRealPCodeCall(ctx, node, callee->type, NULL, NULL, NULL, NULL,
                             &failure_kind, &reason);
  if (!ok) {
    ok = RunConstexprExpressionThunk(ctx, node, NULL, NULL, NULL, NULL,
                                     &failure_kind, &reason);
  }
  return ok ? ConstexprPCodeSuccess(ctx)
            : ConstexprPCodeFailure(ctx, failure_kind, reason);
}

bool ConstexprPCodeEvaluateCallObjectResult(ConstEvalContext* ctx,
                                            ASTNode* node,
                                            ConstexprObject** result) {
  const char* reason = NULL;
  if (!ConstexprExpressionCanUseThunk(ctx, node)) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode call has an unbound automatic variable");
  }
  if (!ConstexprPCodeValidateCall(node, &reason)) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported, reason);
  }
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  if (callee == NULL || callee->type == NULL ||
      !TypeIsStructOrUnion(callee->type->next)) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode call does not return an object");
  }
  ConstexprPCodeFailureKind failure_kind =
      kConstexprPCodeFailureUnsupported;
  bool ok = RunRealPCodeCall(ctx, node, callee->type, NULL, NULL, result, NULL,
                             &failure_kind, &reason);
  if (!ok) {
    ok = RunConstexprExpressionThunk(ctx, node, NULL, NULL, result, NULL,
                                     &failure_kind, &reason);
  }
  return ok ? ConstexprPCodeSuccess(ctx)
            : ConstexprPCodeFailure(ctx, failure_kind, reason);
}

bool ConstexprPCodeEvaluateCallAsAddress(ConstEvalContext* ctx, ASTNode* node,
                                         ConstexprValue* result) {
  const char* reason = NULL;
  if (result != NULL) {
    *result = (ConstexprValue){0};
  }
  if (!ConstexprExpressionCanUseThunk(ctx, node)) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode call has an unbound automatic variable");
  }
  if (!ConstexprPCodeValidateCall(node, &reason)) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported, reason);
  }
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  if (callee == NULL || callee->type == NULL || callee->type->next == NULL ||
      (!TypeIsPointer(callee->type->next) &&
       !TypeIsReference(callee->type->next))) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode call does not return an address");
  }
  ConstexprPCodeFailureKind failure_kind =
      kConstexprPCodeFailureUnsupported;
  bool ok = RunRealPCodeCall(ctx, node, callee->type, NULL, NULL, NULL, result,
                             &failure_kind, &reason);
  if (!ok && compiler->constexpr_eval_mode == kConstexprEvalPCode &&
      (compiler->current_function == NULL ||
       compiler->constant_evaluation_required_depth > 0)) {
    ok = RunConstexprExpressionThunk(ctx, node, NULL, NULL, NULL, result,
                                     &failure_kind, &reason);
  }
  return ok ? ConstexprPCodeSuccess(ctx)
            : ConstexprPCodeFailure(ctx, failure_kind, reason);
}

static bool ConstexprPCodeEvaluateConstructorObject(ConstEvalContext* ctx,
                                                    TypeRecord* object_type,
                                                    ASTNode* node,
                                                    ConstexprObject** result) {
  const char* reason = NULL;
  if (!ConstexprExpressionCanUseThunk(ctx, node)) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode constructor has an unbound automatic variable");
  }
  ASTNode* receiver = NULL;
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  if (callee == NULL &&
      compiler->constexpr_eval_mode == kConstexprEvalPCode) {
    callee = PCodeConstexprFunctionDefinition(
        ConstexprRawConstructorCallSymbol(node, &receiver));
  }
  if (callee == NULL &&
      compiler->constexpr_eval_mode == kConstexprEvalPCode &&
      node != NULL && node->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)node;
    callee = PCodeConstexprFunctionDefinition(
        ConstexprConstructorForObjectType(
            object_type, call->children != NULL ? call->children->length : 0));
  }
  if (callee == NULL || callee->type == NULL) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode constructor has no body");
  }
  if (!callee->type->info.function.is_constexpr) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "pcode constructor is not constexpr");
  }
  if (!callee->type->info.function.is_constructor ||
      callee->type->info.function.is_destructor) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "pcode call target is not a constructor");
  }
  if (callee->type->info.function.is_virtual) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "virtual pcode constructor is unsupported");
  }
  if (callee->type->info.function.varargs) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "variadic pcode constructor is unsupported");
  }
  ValidationState state = {.reason = "ok", .ok = true};
  ASTNodeVisit(callee->type->info.function.body, ValidateASTNode, 0, &state);
  if (!state.ok) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported, state.reason);
  }
  ConstexprPCodeFailureKind failure_kind =
      kConstexprPCodeFailureUnsupported;
  bool ok = RunRealPCodeConstructor(ctx, object_type, node, callee->type,
                                    result, &failure_kind, &reason);
  return ok ? ConstexprPCodeSuccess(ctx)
            : ConstexprPCodeFailure(ctx, failure_kind, reason);
}

bool ConstexprPCodeEvaluateCallAsObject(ConstEvalContext* ctx, ASTNode* node) {
  ConstexprObject* object = NULL;
  bool ok = ConstexprPCodeEvaluateCallObjectResult(ctx, node, &object);
  if (!ok) {
    ASTNode* receiver = NULL;
    Symbol* constructor = ConstexprRawConstructorCallSymbol(node, &receiver);
    TypeRecord* object_type = receiver != NULL ? receiver->type : NULL;
    if (constructor == NULL) {
      constructor = PCodeConstexprFunctionDefinition(
          PCodeConstexprCallSymbol(node));
    }
    if (constructor != NULL && constructor->type != NULL &&
        constructor->type->info.function.is_constructor &&
        constructor->type->info.function.prototype.length != 0) {
      Symbol* this_parameter =
          constructor->type->info.function.prototype.value.p[0];
      if (this_parameter != NULL) {
        object_type = this_parameter->type;
      }
    }
    if (constructor != NULL && TypeIsPointer(object_type)) {
      object_type = object_type->next;
    }
    if (constructor != NULL && TypeIsStructOrUnion(object_type)) {
      ok = ConstexprPCodeEvaluateConstructorObject(ctx, object_type, node,
                                                   &object);
    }
  }
  DeletePCodeConstexprObject(object);
  return ok;
}

static ConstexprValue* NewPCodeConstexprValueSlot(void) {
  ConstexprValue* slot = malloc(sizeof(ConstexprValue));
  if (slot != NULL) {
    *slot = (ConstexprValue){0};
  }
  return slot;
}

static size_t PCodeConstexprObjectSlotCount(TypeRecord* type) {
  if (type != NULL && TypeIsFixedArray(type)) {
    size_t count = type->info.array.size.fixed;
    if (count == 0 && type->next != NULL && type->next->size != 0) {
      count = type->size / type->next->size;
    }
    return count;
  }
  if (type != NULL && TypeIsStructOrUnion(type) &&
      type->info.struct_info != NULL) {
    return type->info.struct_info->is_union ? 1
                                            : type->info.struct_info->members.length;
  }
  return 0;
}

static void DeletePCodeConstexprObject(ConstexprObject* object) {
  if (object == NULL) {
    return;
  }
  for (size_t i = 0; i < object->slots.length; i++) {
    ConstexprValue* slot = object->slots.value.p[i];
    if (slot != NULL && slot->is_object) {
      DeletePCodeConstexprObject(slot->object);
    }
    free(slot);
  }
  VectorDestruct(&object->slots);
  free(object);
}

void ConstexprPCodeDeleteObject(ConstexprObject* object) {
  DeletePCodeConstexprObject(object);
}

static ConstexprObject* NewPCodeConstexprObject(TypeRecord* type) {
  ConstexprObject* object = malloc(sizeof(ConstexprObject));
  if (object == NULL) {
    return NULL;
  }
  object->type = type;
  object->active_union_member = NULL;
  object->lifetime_ended = false;
  object->complete_object = object;
  object->complete_offset = 0;
  VectorInit(&object->slots);
  size_t slots = PCodeConstexprObjectSlotCount(type);
  for (size_t i = 0; i < slots; i++) {
    ConstexprValue* slot = NewPCodeConstexprValueSlot();
    if (slot == NULL) {
      DeletePCodeConstexprObject(object);
      return NULL;
    }
    VectorAppend(&object->slots, slot);
  }
  return object;
}

static ConstexprValue* PCodeConstexprObjectSlot(ConstexprObject* object,
                                                size_t index) {
  if (object == NULL || index >= object->slots.length) {
    return NULL;
  }
  return object->slots.value.p[index];
}

static ConstexprValue* PCodeConstexprSlotForOffset(TypeRecord* type,
                                                   ConstexprObject* object,
                                                   size_t offset) {
  if (type == NULL || object == NULL) {
    return NULL;
  }
  if (TypeIsFixedArray(type)) {
    size_t elem_size = type->next != NULL ? type->next->size : 0;
    if (elem_size == 0) {
      return NULL;
    }
    size_t index = offset / elem_size;
    size_t elem_offset = offset % elem_size;
    ConstexprValue* slot = PCodeConstexprObjectSlot(object, index);
    if (slot == NULL) {
      return NULL;
    }
    if (elem_offset == 0 || !slot->is_object) {
      return elem_offset == 0 ? slot : NULL;
    }
    return PCodeConstexprSlotForOffset(type->next, slot->object, elem_offset);
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* str = type->info.struct_info;
    for (size_t i = 0; i < str->members.length; i++) {
      StructMember* member = str->members.value.p[i];
      if (member == NULL || member->symbol == NULL || member->is_static ||
          member->is_member_function ||
          StorageIs(member->symbol->storage, STO(typedef))) {
        continue;
      }
      size_t member_size = member->symbol->type != NULL
                               ? member->symbol->type->size
                               : 0;
      if (offset < member->byte_offset ||
          offset >= member->byte_offset + member_size) {
        continue;
      }
      size_t slot_index = str->is_union ? 0 : member->index;
      ConstexprValue* slot = PCodeConstexprObjectSlot(object, slot_index);
      size_t member_offset = offset - member->byte_offset;
      if (slot == NULL) {
        return NULL;
      }
      if (member_offset == 0 || !slot->is_object) {
        return member_offset == 0 ? slot : NULL;
      }
      return PCodeConstexprSlotForOffset(member->symbol->type, slot->object,
                                         member_offset);
    }
  }
  return offset == 0 ? PCodeConstexprObjectSlot(object, 0) : NULL;
}

static bool PCodeEvaluatePointerInitializer(ASTNode* expr, int64_t* result) {
  if (expr == NULL || result == NULL) {
    return false;
  }
  if (expr->op == AST_OP(string)) {
    ConstantASTNode* string = (ConstantASTNode*)expr;
    if (string->value.string == NULL) {
      return false;
    }
    *result = (int64_t)(uintptr_t)string->value.string->value;
    return true;
  }
  String source_string;
  if (PCodeSourceBuiltinStringValue(expr, &source_string)) {
    size_t size = source_string.length + 1;
    char* memory = malloc(size);
    if (memory == NULL) {
      StringDestruct(&source_string);
      return false;
    }
    memcpy(memory, source_string.value, size);
    StringDestruct(&source_string);
    *result = (int64_t)(uintptr_t)memory;
    return true;
  }
  ConstEvalContext ctx;
  ConstEvalContextInit(&ctx);
  bool ok = expr->op == AST_OP(call)
                ? ConstexprPCodeEvaluateCallAsInteger(&ctx, expr, result)
                : EvaluateIntegerExpressionInContext(&ctx, expr, result);
  ConstEvalContextDestruct(&ctx);
  return ok;
}

static bool PCodeEvaluateScalarInitializer(TypeRecord* type, ASTNode* initializer,
                                           ConstexprValue* result) {
  ASTNode* expr = ConstexprInitializerExpression(initializer);
  if (expr == NULL || result == NULL) {
    return false;
  }
  if (TypeIsFloatingPoint(type)) {
    double fvalue;
    ConstEvalContext ctx;
    ConstEvalContextInit(&ctx);
    bool ok = expr->op == AST_OP(call)
                  ? ConstexprPCodeEvaluateCallAsFloating(&ctx, expr, &fvalue)
                  : EvaluateFloatingPointExpressionInContext(&ctx, expr,
                                                             &fvalue);
    ConstEvalContextDestruct(&ctx);
    if (!ok) {
      return false;
    }
    *result = (ConstexprValue){
        .is_floating = true,
        .ivalue = (int64_t)fvalue,
        .fvalue = fvalue,
    };
    return true;
  }
  if (TypeIsIntegral(type) || TypeIsPointer(type)) {
    int64_t ivalue;
    bool ok = TypeIsPointer(type)
                  ? PCodeEvaluatePointerInitializer(expr, &ivalue)
                  : false;
    if (!ok) {
      ConstEvalContext ctx;
      ConstEvalContextInit(&ctx);
      ok = expr->op == AST_OP(call)
               ? ConstexprPCodeEvaluateCallAsInteger(&ctx, expr, &ivalue)
               : EvaluateIntegerExpressionInContext(&ctx, expr, &ivalue);
      ConstEvalContextDestruct(&ctx);
    }
    if (!ok) {
      return false;
    }
    *result = (ConstexprValue){
        .is_floating = false,
        .ivalue = ivalue,
        .fvalue = (double)ivalue,
    };
    return true;
  }
  return false;
}

static bool BuildPCodeConstexprObject(ConstEvalContext* ctx, TypeRecord* type,
                                      ASTNode* initializer,
                                      ConstexprObject** result);

static ASTNode* PCodeAggregateInitializer(ASTNode* initializer) {
  while (initializer != NULL) {
    if (initializer->op == AST_OP(init)) {
      initializer = ((BinaryASTNode*)initializer)->right;
    } else if (initializer->op == AST_OP(expr_init)) {
      initializer = ((ExpressionInitializerASTNode*)initializer)->expr;
    } else if (initializer->op == AST_OP(designated_init)) {
      initializer = ((DesignatedInitializerASTNode*)initializer)->init;
    } else if (initializer->op == AST_OP(compound_literal)) {
      initializer = ((CompoundLiteralASTNode*)initializer)->initializer;
    } else {
      break;
    }
  }
  return initializer;
}

static bool PCodeStoreInitializer(ConstEvalContext* ctx, TypeRecord* type,
                                  ASTNode* initializer,
                                  ConstexprValue* slot) {
  if (type == NULL || initializer == NULL || slot == NULL) {
    return false;
  }
  if (TypeIsFixedArray(type) || TypeIsStructOrUnion(type)) {
    ConstexprObject* object = NULL;
    ASTNode* expr = ConstexprInitializerExpression(initializer);
    bool ok = expr != NULL && expr->op == AST_OP(call) &&
              (ConstexprPCodeEvaluateCallObjectResult(ctx, expr, &object) ||
               ConstexprPCodeEvaluateConstructorObject(ctx, type, expr,
                                                       &object));
    if (!ok) {
      ok = BuildPCodeConstexprObject(ctx, type, initializer, &object);
    }
    if (!ok) {
      return false;
    }
    *slot = (ConstexprValue){
        .is_object = true,
        .object = object,
    };
    return true;
  }
  return PCodeEvaluateScalarInitializer(type, initializer, slot);
}

static bool BuildPCodeArrayObject(ConstEvalContext* ctx, TypeRecord* type,
                                  ASTNode* initializer,
                                  ConstexprObject* object) {
  ASTNode* expression = ConstexprInitializerExpression(initializer);
  if (expression != NULL && expression->op == AST_OP(string) &&
      type != NULL && type->next != NULL &&
      TypeIsIntegral(type->next)) {
    String* value = ((ConstantASTNode*)expression)->value.string;
    if (value != NULL) {
      if (object->slots.length == 0) {
        for (size_t i = 0; i <= value->length; i++) {
          ConstexprValue* slot = NewPCodeConstexprValueSlot();
          if (slot == NULL) {
            return false;
          }
          VectorAppend(&object->slots, slot);
        }
      }
      for (size_t i = 0; i < object->slots.length; i++) {
        ConstexprValue* slot = PCodeConstexprObjectSlot(object, i);
        if (slot == NULL) {
          ConstexprPCodeFailure(
              ctx, kConstexprPCodeFailureUnsupported,
              "constexpr pcode string array has no object slot");
          return false;
        }
        slot->ivalue =
            i < value->length ? (unsigned char)value->value[i] : 0;
        slot->fvalue = (double)slot->ivalue;
      }
      return true;
    }
  }
  if (initializer == NULL || initializer->op != AST_OP(braced_init)) {
    ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode array initializer is not a braced list");
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
  bool sparse_lifetime_initializer =
      (initializer->flags & kASTConstexprLifetimeInitializer) != 0;
  if (sparse_lifetime_initializer) {
    for (size_t i = 0; i < object->slots.length; i++) {
      ConstexprValue* slot = PCodeConstexprObjectSlot(object, i);
      if (slot != NULL) {
        slot->lifetime_ended = true;
      }
    }
  }
  if (braced->initializers->length > object->slots.length) {
    ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode array initializer has too many elements");
    return false;
  }
  size_t next_index = 0;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* entry = braced->initializers->value.p[i];
    ASTNode* entry_initializer = entry;
    size_t slot_index = next_index;
    if (entry != NULL && entry->op == AST_OP(designated_init)) {
      DesignatedInitializerASTNode* designated =
          (DesignatedInitializerASTNode*)entry;
      if (designated->designators == NULL ||
          designated->designators->length != 1) {
        return false;
      }
      Designator* designator = designated->designators->value.p[0];
      if (designator == NULL ||
          designator->designator_type != kDesignatorArray ||
          designator->value.array_index < 0 ||
          designator->array_index_end != designator->value.array_index) {
        return false;
      }
      slot_index = (size_t)designator->value.array_index;
      entry_initializer = designated->init;
    }
    ConstexprValue* slot = PCodeConstexprObjectSlot(object, slot_index);
    if (slot == NULL ||
        !PCodeStoreInitializer(ctx, type->next, entry_initializer, slot)) {
      ConstexprPCodeFailure(
          ctx, kConstexprPCodeFailureUnsupported,
          "constexpr pcode could not store array initializer element");
      return false;
    }
    slot->lifetime_ended = false;
    next_index = slot_index + 1;
  }
  return true;
}

static bool BuildPCodeStructObject(ConstEvalContext* ctx, TypeRecord* type,
                                   ASTNode* initializer,
                                   ConstexprObject* object) {
  if (initializer == NULL || initializer->op != AST_OP(braced_init) ||
      type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
  Struct* str = type->info.struct_info;
  if (str->is_union && braced->initializers->length != 0) {
    ASTNode* first = braced->initializers->value.p[0];
    if (first != NULL && first->op == AST_OP(designated_init)) {
      DesignatedInitializerASTNode* designated =
          (DesignatedInitializerASTNode*)first;
      if (designated->designators == NULL ||
          designated->designators->length != 1) {
        return false;
      }
      Designator* designator = designated->designators->value.p[0];
      StructMember* selected =
          designator != NULL &&
                  designator->designator_type == kDesignatorStruct &&
                  designator->is_resolved_member
              ? designator->value.struct_member
              : NULL;
      if (selected == NULL && designator != NULL &&
          designator->designator_type == kDesignatorStruct &&
          designator->value.struct_member_name != NULL) {
        for (size_t i = 0; i < str->members.length; i++) {
          StructMember* candidate = str->members.value.p[i];
          if (candidate != NULL && candidate->symbol != NULL &&
              StringEqual(designator->value.struct_member_name,
                          candidate->symbol->name.value)) {
            selected = candidate;
            break;
          }
        }
      }
      if (selected == NULL || selected->symbol == NULL ||
          !PCodeStoreInitializer(
              ctx, selected->symbol->type, designated->init,
              PCodeConstexprObjectSlot(object, 0))) {
        return false;
      }
      object->active_union_member = selected;
      return braced->initializers->length == 1;
    }
  }
  size_t init_index = 0;
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function ||
        StorageIs(member->symbol->storage, STO(typedef))) {
      continue;
    }
    if (init_index >= braced->initializers->length) {
      if (str->is_union) {
        break;
      }
      // A member the braced list does not reach is initialized from its default
      // member initializer, or value-initialized when it has none
      // ([dcl.init.aggr]/5); the zeroed slot already models the latter.
      if (member->default_initializer != NULL &&
          !PCodeStoreInitializer(
              ctx, member->symbol->type, member->default_initializer,
              PCodeConstexprObjectSlot(object, member->index))) {
        return false;
      }
      continue;
    }
    size_t slot_index = str->is_union ? 0 : member->index;
    if (!PCodeStoreInitializer(ctx, member->symbol->type,
                               braced->initializers->value.p[init_index],
                               PCodeConstexprObjectSlot(object, slot_index))) {
      return false;
    }
    if (str->is_union) {
      object->active_union_member = member;
      break;
    }
    init_index++;
  }
  return init_index == braced->initializers->length || str->is_union;
}

static bool BuildPCodeConstexprObject(ConstEvalContext* ctx, TypeRecord* type,
                                      ASTNode* initializer,
                                      ConstexprObject** result) {
  initializer = PCodeAggregateInitializer(initializer);
  if (type == NULL || initializer == NULL || result == NULL) {
    ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode object initializer is incomplete");
    return false;
  }
  ConstexprObject* object = NewPCodeConstexprObject(type);
  if (object == NULL) {
    ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode could not allocate object model");
    return false;
  }
  bool ok = false;
  if (TypeIsFixedArray(type)) {
    ok = BuildPCodeArrayObject(ctx, type, initializer, object);
  } else if (TypeIsStructOrUnion(type)) {
    ok = BuildPCodeStructObject(ctx, type, initializer, object);
  } else {
    ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode object type is not an aggregate");
  }
  if (!ok) {
    DeletePCodeConstexprObject(object);
    return false;
  }
  *result = object;
  return true;
}

bool ConstexprPCodeEvaluateObjectConstantForSymbol(ConstEvalContext* ctx,
                                                   Symbol* symbol,
                                                   ASTNode* initializer) {
  if (symbol == NULL || symbol->type == NULL || initializer == NULL ||
      (!TypeIsFixedArray(symbol->type) && !TypeIsStructOrUnion(symbol->type))) {
    return ConstexprPCodeFailure(
        ctx, kConstexprPCodeFailureUnsupported,
        "constexpr pcode object initializer has an invalid type");
  }
  ConstexprObject* object = NULL;
  bool ok =
      BuildPCodeConstexprObject(ctx, symbol->type, initializer, &object);
  if (!ok) {
    ASTNode* expr = ConstexprInitializerExpression(initializer);
    if (expr == NULL || !TypeIsStructOrUnion(symbol->type)) {
      return ConstexprPCodeFailure(
          ctx, kConstexprPCodeFailureUnsupported,
          "constexpr pcode initializer has no object expression");
    }
    const char* reason = NULL;
    if (expr->op == AST_OP(call)) {
      ok = ConstexprPCodeEvaluateCallObjectResult(ctx, expr, &object) ||
           ConstexprPCodeEvaluateConstructorObject(ctx, symbol->type, expr,
                                                   &object);
    } else if (compiler->current_function == NULL ||
               compiler->constant_evaluation_required_depth > 0) {
      ConstexprPCodeFailureKind failure_kind =
          kConstexprPCodeFailureUnsupported;
      ok = RunConstexprExpressionThunk(ctx, expr, NULL, NULL, &object, NULL,
                                       &failure_kind, &reason);
      if (ok) {
        ConstexprPCodeSuccess(ctx);
      } else {
        ConstexprPCodeFailure(ctx, failure_kind, reason);
      }
    } else {
      ok = false;
      ConstexprPCodeFailure(
          ctx, kConstexprPCodeFailureUnsupported,
          "object expression is not manifestly constant-evaluated");
    }
    if (!ok) {
      return false;
    }
  }
  symbol->value.other = object;
  symbol->flags.value_set = true;
  return ConstexprPCodeSuccess(ctx);
}
