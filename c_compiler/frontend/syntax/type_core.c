//
//  type_core.c
//  c_compiler
//

#include "type_internal.h"
#include "member_pointer.h"
#include "reflection.h"
#include "reflection_semantics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stddef.h>

#include <assert.h>
#if TYPE_RECORD_GUARD_PAGES
#include <sys/mman.h>
#include <unistd.h>
#endif
#include "ast.h"
#include "compiler.h"
#include "concepts.h"
#include "constexpr.h"
#include "dstring.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "statement_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "semantics.h"
#include "errors.h"
#include "debug.h"
#include "rtti.h"
#include "set.h"

static int next_type_id = 0;

// TypeRecord struct is allocated from a bump arena and the whole arena is freed
// in one shot at CompilerDestruct (TypeRecordArenaRelease).  Reference counting
// still drives release of each record's *owned* auxiliary resources (function
// prototype symbols, struct/enum info, VLA size AST); only the struct itself is
// no longer individually free()d.
typedef struct TypeArenaBlock {
  struct TypeArenaBlock* next;
  size_t used;
  size_t capacity;
  char data[];
} TypeArenaBlock;

#define TYPE_ARENA_BLOCK_SIZE (256 * 1024)

static TypeArenaBlock* type_arena = NULL;

typedef union TemplateArgumentSlot {
  TemplateArgument argument;
  union TemplateArgumentSlot* next;
} TemplateArgumentSlot;

typedef struct TemplateArgumentArenaBlock {
  struct TemplateArgumentArenaBlock* next;
  size_t used;
  size_t capacity;
  TemplateArgumentSlot slots[];
} TemplateArgumentArenaBlock;

#define TEMPLATE_ARGUMENT_ARENA_BLOCK_SIZE (64 * 1024)

static TemplateArgumentArenaBlock* template_argument_arena = NULL;
static TemplateArgumentSlot* free_template_arguments = NULL;

static TypeArenaBlock* NewTypeArenaBlock(size_t capacity) {
  TypeArenaBlock* block = calloc(1, capacity + sizeof(TypeArenaBlock));
  block->next = NULL;
  block->capacity = capacity;
  return block;
}

#define TYPE_RECORD_ALIGN(bytes) \
  (((bytes) + _Alignof(TypeRecord) - 1) & ~(size_t)(_Alignof(TypeRecord) - 1))
#define TYPE_RECORD_FULL_SIZE TYPE_RECORD_ALIGN(sizeof(TypeRecord))
#define TYPE_RECORD_NON_FUNCTION_SIZE \
  TYPE_RECORD_ALIGN(offsetof(TypeRecord, info) + sizeof(ArrayInfo))

// Records that are not functions never use the FunctionInfo arm of the info
// union, so they may be given a shorter slot.  Every block keeps a full
// record's worth of slack so by-value copies (`TypeRecord t = *r`) of a short
// record at the end of a block stay inside the allocation.
static size_t type_alloc_full = 0;
static size_t type_alloc_short = 0;
static size_t type_copy_count = 0;
static size_t type_share_count = 0;

// DAVECC_PROFILE duplicate census: every record allocated and every record
// whose last reference was released, hashed structurally at the end.
typedef struct {
  TypeRecord* record;
  size_t bytes;
  void* caller;  // TypeRecordCopy's caller; NULL for other allocations.
} TypeCensusEntry;
static bool type_census_enabled = false;
static void* type_census_caller = NULL;
static TypeCensusEntry* type_census_allocs = NULL;
static size_t type_census_alloc_count = 0;
static size_t type_census_alloc_capacity = 0;
static TypeRecord** type_census_releases = NULL;
static size_t type_census_release_count = 0;
static size_t type_census_release_capacity = 0;

static void TypeCensusNoteAlloc(TypeRecord* record, size_t bytes) {
  if (type_census_alloc_count == type_census_alloc_capacity) {
    type_census_alloc_capacity =
        type_census_alloc_capacity ? type_census_alloc_capacity * 2 : 1 << 16;
    type_census_allocs =
        realloc(type_census_allocs,
                type_census_alloc_capacity * sizeof(TypeCensusEntry));
  }
  type_census_allocs[type_census_alloc_count].record = record;
  type_census_allocs[type_census_alloc_count].bytes = bytes;
  type_census_allocs[type_census_alloc_count].caller = type_census_caller;
  type_census_caller = NULL;
  type_census_alloc_count++;
}

static void TypeCensusNoteRelease(TypeRecord* record) {
  if (type_census_release_count == type_census_release_capacity) {
    type_census_release_capacity = type_census_release_capacity
                                       ? type_census_release_capacity * 2
                                       : 1 << 16;
    type_census_releases =
        realloc(type_census_releases,
                type_census_release_capacity * sizeof(TypeRecord*));
  }
  type_census_releases[type_census_release_count++] = record;
}

// DAVECC_PROFILE copy attribution: return address -> count.  Open addressing;
// a full table drops further callers rather than growing.
#define TYPE_COPY_CALLER_SLOTS 8192
typedef struct {
  void* caller;
  size_t count;
} TypeCopyCaller;
static TypeCopyCaller* type_copy_callers = NULL;

void TypeRecordProfileCallers(void) {
  if (type_copy_callers == NULL) {
    type_copy_callers = calloc(TYPE_COPY_CALLER_SLOTS, sizeof(TypeCopyCaller));
  }
  type_census_enabled = getenv("DAVECC_PROFILE_CENSUS") != NULL;
}

static void TypeCopyNoteCaller(void* caller) {
  size_t slot = ((uintptr_t)caller >> 2) & (TYPE_COPY_CALLER_SLOTS - 1);
  for (size_t probe = 0; probe < TYPE_COPY_CALLER_SLOTS; probe++) {
    TypeCopyCaller* entry = &type_copy_callers[slot];
    if (entry->caller == caller || entry->caller == NULL) {
      entry->caller = caller;
      entry->count++;
      return;
    }
    slot = (slot + 1) & (TYPE_COPY_CALLER_SLOTS - 1);
  }
}

static int CompareTypeCopyCallers(const void* a, const void* b) {
  size_t ca = ((const TypeCopyCaller*)a)->count;
  size_t cb = ((const TypeCopyCaller*)b)->count;
  return ca < cb ? 1 : ca > cb ? -1 : 0;
}

void TypeRecordProfileReport(FILE* out, size_t limit) {
  if (type_copy_callers == NULL) {
    return;
  }
  qsort(type_copy_callers, TYPE_COPY_CALLER_SLOTS, sizeof(TypeCopyCaller),
        CompareTypeCopyCallers);
  for (size_t i = 0; i < limit && i < TYPE_COPY_CALLER_SLOTS; i++) {
    if (type_copy_callers[i].count == 0) {
      break;
    }
    fprintf(out, "DAVECC_PROFILE copy-caller %p %zu\n",
            type_copy_callers[i].caller, type_copy_callers[i].count);
  }
  free(type_copy_callers);
  type_copy_callers = NULL;
}

// Released arena slots, by size, linked through their `next` fields.  Records
// are released when their last reference is dropped; the census keeps every
// allocation distinct, so slots are not reused while it runs.
static TypeRecord* free_short_type_records = NULL;
static TypeRecord* free_full_type_records = NULL;

#ifndef NDEBUG
#define TYPE_ARENA_CHECKS 1
#else
#define TYPE_ARENA_CHECKS 0
#endif
// The reference count of a released record in debug builds.
#define kTypeRecordReleasedRefs (-0x5A5A5A5A)

// Aborts on a reference-count update to a released record.
static inline void TypeArenaCheckLive(const TypeRecord* record) {
#if TYPE_ARENA_CHECKS
  if (record->refs == kTypeRecordReleasedRefs) {
    abort();
  }
#else
  (void)record;
#endif
}

static TypeRecord** TypeArenaFreeList(bool full) {
  return full ? &free_full_type_records : &free_short_type_records;
}

#if TYPE_RECORD_GUARD_PAGES
// A zeroed slot that ends where an inaccessible page begins, so reading or
// writing FunctionInfo past a short record's end faults at that access.  Costs
// two pages per record; for finding such accesses, not for normal builds.
static TypeRecord* TypeArenaAllocGuarded(size_t bytes) {
  size_t page = (size_t)getpagesize();
  size_t data = (bytes + page - 1) / page * page;
  char* base = mmap(NULL, data + page, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANON, -1, 0);
  if (base == MAP_FAILED || mprotect(base + data, page, PROT_NONE) != 0) {
    abort();
  }
  return (TypeRecord*)(base + data - bytes);
}
#endif

// Returns a zeroed slot of `bytes` (TYPE_RECORD_FULL_SIZE or
// TYPE_RECORD_NON_FUNCTION_SIZE), reusing a released one when available.
static TypeRecord* TypeArenaAllocSized(size_t bytes) {
  bool full = bytes >= TYPE_RECORD_FULL_SIZE;
  if (full) {
    type_alloc_full++;
  } else {
    type_alloc_short++;
  }
  TypeRecord** free_list = TypeArenaFreeList(full);
  if (*free_list != NULL) {
    TypeRecord* slot = *free_list;
    *free_list = slot->next;
    memset(slot, 0, bytes);
    slot->full_slot = full;
    return slot;
  }
#if TYPE_RECORD_GUARD_PAGES
  if (!full) {
    TypeRecord* p = TypeArenaAllocGuarded(bytes);
    p->full_slot = false;
    return p;
  }
#endif
  if (type_arena == NULL ||
      type_arena->used + TYPE_RECORD_FULL_SIZE > type_arena->capacity) {
    TypeArenaBlock* block = NewTypeArenaBlock(TYPE_ARENA_BLOCK_SIZE);
    block->next = type_arena;
    type_arena = block;
  }
  TypeRecord* p = (TypeRecord*)(type_arena->data + type_arena->used);
  type_arena->used += bytes;
  p->full_slot = full;
  if (type_census_enabled) {
    TypeCensusNoteAlloc(p, bytes);
  }
  return p;
}

#if TYPE_ARENA_CHECKS && !TYPE_RECORD_GUARD_PAGES
static bool TypeArenaContains(const TypeRecord* record) {
  for (TypeArenaBlock* block = type_arena; block != NULL; block = block->next) {
    if ((const char*)record >= block->data &&
        (const char*)record < block->data + block->used) {
      return true;
    }
  }
  return false;
}
#endif

// Returns the slot of a record whose owned resources are already freed to its
// size's free list.  Debug builds poison it so later reads fault and later
// reference-count updates abort.
static void TypeArenaRelease(TypeRecord* record) {
  bool full = record->full_slot;
#if TYPE_ARENA_CHECKS && !TYPE_RECORD_GUARD_PAGES
  if (!TypeArenaContains(record)) {
    abort();
  }
#endif
  if (type_census_enabled) {
    return;
  }
#if TYPE_ARENA_CHECKS
  memset(record, 0xA5,
         full ? TYPE_RECORD_FULL_SIZE : TYPE_RECORD_NON_FUNCTION_SIZE);
  record->refs = kTypeRecordReleasedRefs;
  record->shared_immutable = 0;
#endif
  TypeRecord** free_list = TypeArenaFreeList(full);
  record->next = *free_list;
  *free_list = record;
}

// Free every TypeRecord struct.  Call only after all type-referencing data
// structures have been torn down (their TypeRecordDelete calls have released
// the auxiliary resources); the struct memory becomes invalid afterwards.
static void TypeInternRelease(void);

void TypeRecordArenaRelease(void) {
  // The type table only holds records from this arena.
  TypeInternRelease();
  TypeArenaBlock* block = type_arena;
  while (block != NULL) {
    TypeArenaBlock* next = block->next;
    free(block);
    block = next;
  }
  type_arena = NULL;
  free_short_type_records = NULL;
  free_full_type_records = NULL;
}

size_t TypeArenaFullAllocCount(void) { return type_alloc_full; }

size_t TypeArenaShortAllocCount(void) { return type_alloc_short; }

size_t TypeRecordCopyCount(void) { return type_copy_count; }

size_t TypeRecordShareCount(void) { return type_share_count; }

size_t TypeArenaBytes(void) {
  size_t bytes = 0;
  for (TypeArenaBlock* block = type_arena; block != NULL; block = block->next) {
    bytes += block->used;
  }
  return bytes;
}

TemplateArgument* TemplateArgumentAlloc(void) {
  TemplateArgumentSlot* slot = free_template_arguments;
  if (slot != NULL) {
    free_template_arguments = slot->next;
  } else {
    if (template_argument_arena == NULL ||
        template_argument_arena->used == template_argument_arena->capacity) {
      size_t capacity =
          (TEMPLATE_ARGUMENT_ARENA_BLOCK_SIZE -
           offsetof(TemplateArgumentArenaBlock, slots)) /
          sizeof(TemplateArgumentSlot);
      TemplateArgumentArenaBlock* block =
          malloc(offsetof(TemplateArgumentArenaBlock, slots) +
                 capacity * sizeof(TemplateArgumentSlot));
      block->next = template_argument_arena;
      block->used = 0;
      block->capacity = capacity;
      template_argument_arena = block;
    }
    slot = &template_argument_arena
                ->slots[template_argument_arena->used++];
  }
  memset(&slot->argument, 0, sizeof(slot->argument));
  return &slot->argument;
}

void TemplateArgumentArenaRelease(void) {
  TemplateArgumentArenaBlock* block = template_argument_arena;
  while (block != NULL) {
    TemplateArgumentArenaBlock* next = block->next;
    free(block);
    block = next;
  }
  template_argument_arena = NULL;
  free_template_arguments = NULL;
}

// Registries of every Struct and Enum created.  Struct infos can form
// reference cycles (a member of type 'struct S *' inside 'struct S' takes a
// reference on S's own struct_info), so refcount-driven freeing in
// TypeRecordDelete never reaches zero for them.  Instead, every struct and enum
// info is freed in one bulk pass at end of compilation, after all
// type-referencing structures have been torn down.
static Vector struct_registry;
static bool struct_registry_initialized = false;
static Vector enum_registry;
static bool enum_registry_initialized = false;

static void StructTeardownMembers(Struct* s);

// Free every Struct and Enum info in one pass.  Must be called after the AST,
// symbol tables and tags are gone but before TypeRecordArenaRelease, because
// tearing down a struct's members deletes member symbols which decref
// TypeRecords (and may decref other struct/enum infos).
void StructRegistryRelease(void) {
  // Phase 1: tear down every struct's members.  This deletes member symbols,
  // whose types may decref OTHER struct/enum infos.  Every info is still
  // allocated at this point (none are freed below), so those decrements are
  // safe.  TypeRecordDelete never frees struct/enum infos itself.
  for (size_t i = 0; i < struct_registry.length; i++) {
    StructTeardownMembers((Struct*)struct_registry.value.p[i]);
  }
  // Phase 2: now that no more decrements will occur, free the info structs.
  for (size_t i = 0; i < struct_registry.length; i++) {
    free(struct_registry.value.p[i]);
  }
  for (size_t i = 0; i < enum_registry.length; i++) {
    EnumDelete((Enum*)enum_registry.value.p[i]);
  }
  if (struct_registry_initialized) {
    VectorDestruct(&struct_registry);
    struct_registry_initialized = false;
  }
  if (enum_registry_initialized) {
    VectorDestruct(&enum_registry);
    enum_registry_initialized = false;
  }
}

static void Trap(TypeRecord* r) {
  if (r->id == 1234) {
    printf("");
  }
}

// The size of a pointer depends on the machine architecture.
int SizeofBool(void) {
  return compiler->bool_size;
}

int SizeofShort(void) {
  return compiler->short_size;
}

int SizeofChar(void) {
  return 1;
}

int SizeofVoid(void) {
  return 1;
}


int SizeofPointer(void) {
  return compiler->pointer_size;
}

int SizeofInt(void) {
  return compiler->int_size;
}

static int SizeofChar16(void) {
  return 2;
}

static int SizeofChar32(void) {
  return 4;
}

int SizeofLong(void) {
  return compiler->long_size;
}

int SizeofLongLong(void) {
  return compiler->long_long_size;
}

static int SizeofInt128(void) {
  return 16;
}

static int SizeofWchar(void) {
  return compiler->wchar_size ? compiler->wchar_size : 4;
}

int SizeofFloat(void) {
  return compiler->float_size;
}

int SizeofDouble(void) {
  return compiler->double_size;
}

int SizeofLongDouble(void) {
  return compiler->long_double_size ? compiler->long_double_size
                                    : compiler->double_size;
}

bool TypeUsesLongDoubleRepresentation(TypeRecord* type) {
  if (!TypeIsLongDouble(type) || compiler == NULL) {
    return false;
  }
  return compiler->long_double_format == kLongDoubleFormatIntel80 ||
         compiler->long_double_format == kLongDoubleFormatIEEEf128;
}

bool TypeUsesHardwareFloatRegister(TypeRecord* type) {
  return TypeUsesFloat32Representation(type) ||
         TypeUsesFloat64Representation(type);
}

// Mapping of type to its size in bytes.
static struct {
  Type type;
  int (*func)(void);
} type_sizes[] = {
  {kTypeChar32, SizeofChar32},
  {kTypeChar16, SizeofChar16},
  {kTypeChar8, SizeofChar},
  {kTypeReflection, SizeofPointer},
  {kTypeChar, SizeofChar},
  {kTypeBool, SizeofBool},
  {kTypeShort, SizeofShort},
  {kTypeLong, SizeofLong},
  {kTypeLongLong, SizeofLongLong},
  {kTypeInt128, SizeofInt128},
  {kTypeWchar, SizeofWchar},
  {kTypeFloat, SizeofFloat},
  {kTypeFloat32, SizeofFloat},
  {kTypeDouble, SizeofDouble},
  {kTypeFloat64, SizeofDouble},
  {kTypeLongDouble, SizeofLongDouble},
  {kTypeInt, SizeofInt},
  {kTypeVoid, SizeofVoid},
  {kTypeImplicit, 0},
};


// What is the size in bytes of the given type?  Uses the mapping
// above.  Returns the size or zero if the type isn't known.
int SizeofType(Type type) {
  for (int i = 0; type_sizes[i].type != kTypeImplicit; i++) {
    if ((type & type_sizes[i].type) != 0) {
       return type_sizes[i].func();
    }
  }
  if ((type & kTypeEnum) != 0) {
    // Enums are variable in size.
    if ((type & kTypeChar) != 0) {
      return SizeofChar();
    }
    return SizeofInt();
  }
  return SizeofPointer();
}

void TemplateArgumentVectorDelete(Vector* args) {
  if (args == NULL) {
    return;
  }
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
}

static TypeRecord* NewTypeRecordSized(Type type, Qualifiers quals,
                                      size_t bytes) {
  // If a plain unsigned or signed is found, this implies an int.
  if (type == kTypeUnsigned || type == kTypeSigned) {
    type |= kTypeInt;
  }
  TypeRecord* record = TypeArenaAllocSized(bytes);
  record->id = next_type_id++;
  record->type = type;
  SetQualifiers(record, quals);
  record->template_parameter_summary =
      kTypeTemplateParameterSummaryUnknown;
  record->template_parameter_index = -1;
  record->declarator = kDeclPrimitive;
  Trap(record);
  return record;
}

TypeRecord* NewTypeRecord(Type type, Qualifiers quals) {
  return NewTypeRecordSized(type, quals, TYPE_RECORD_NON_FUNCTION_SIZE);
}

TypeRecord* NewFunctionCapableTypeRecord(Type type, Qualifiers quals) {
  return NewTypeRecordSized(type, quals, TYPE_RECORD_FULL_SIZE);
}

static int BitIntStorageSize(int bit_width) {
  int bytes = (bit_width + 7) / 8;
  if (bytes <= 1) {
    return 1;
  }
  if (bytes <= 2) {
    return 2;
  }
  if (bytes <= 4) {
    return 4;
  }
  return 8;
}

TypeRecord* NewBitIntTypeRecord(int bit_width, bool is_unsigned,
                                Qualifiers quals) {
  Type type = kTypeBitInt | (is_unsigned ? kTypeUnsigned : kTypeSigned);
  TypeRecord* record = NewTypeRecord(type, quals);
  record->bit_width = bit_width;
  record->size = BitIntStorageSize(bit_width);
  return record;
}

static bool TypeRecordUsesStructSize(TypeRecord* record) {
  return record != NULL && record->declarator == kDeclPrimitive &&
         TypeIsStructOrUnion(record) && record->info.struct_info != NULL;
}

static void TypeRecordRegisterStructSizeUser(TypeRecord* record) {
  if (!TypeRecordUsesStructSize(record)) {
    return;
  }
  record->size = record->info.struct_info->size;
  if (record->size_sync_owner == record->info.struct_info) {
    return;
  }
  VectorAppend(&record->info.struct_info->type_records, record);
  record->size_sync_owner = record->info.struct_info;
}

void TypeRecordSetStructInfo(TypeRecord* record, Struct* str) {
  if (record == NULL) {
    return;
  }
  assert(!record->shared_immutable || record->info.struct_info == str);
  record->info.struct_info = str;
  TypeRecordRegisterStructSizeUser(record);
}

const TypeDependentInfo kNoTypeDependentInfo = {0};

TypeDependentInfo* TypeMutableDependent(TypeRecord* record) {
  assert(!record->shared_immutable);
  if (record->dependent == NULL) {
    record->dependent = calloc(1, sizeof(TypeDependentInfo));
    record->dependent->owner = record;
  }
  assert(record->dependent->owner == record);
  return record->dependent;
}

static void TypeDependentInfoDelete(TypeDependentInfo* info) {
  if (info == NULL) {
    return;
  }
  if (info->dependent_member_template_arguments != NULL) {
    VectorDeleteWithContents(
        info->dependent_member_template_arguments,
        (VectorElementDestructor)TemplateArgumentVectorDelete,
        /*free_element=*/false);
  }
  if (info->dependent_member_name != NULL) {
    StringDelete(info->dependent_member_name);
  }
  if (info->template_parameter_name != NULL) {
    StringDelete(info->template_parameter_name);
  }
  if (info->pack_index_expr != NULL) {
    ASTNodeDelete(info->pack_index_expr);
  }
  TemplateArgumentDelete(info->pack_index_pack);
  if (info->dependent_splice_expr != NULL) {
    ASTNodeDelete(info->dependent_splice_expr);
  }
  free(info);
}

const FunctionInfoExtra kNoFunctionInfoExtra = {0};

FunctionInfoExtra* FunctionMutableExtra(FunctionInfo* info) {
  if (info->extra == NULL) {
    info->extra = calloc(1, sizeof(FunctionInfoExtra));
    info->extra->owner = info;
  }
  assert(info->extra->owner == info);
  return info->extra;
}

static void FunctionInfoExtraDelete(FunctionInfo* info) {
  FunctionInfoExtra* extra = info->extra;
  if (extra == NULL) {
    return;
  }
  info->extra = NULL;
  TypeRecordDelete(extra->coroutine_promise_type);
  TypeRecordDelete(extra->coroutine_frame_type);
  if (extra->deleted_reason != NULL) {
    StringDelete(extra->deleted_reason);
  }
  VectorDestructWithContents(&extra->template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  FunctionTemplateInstantiationCacheDelete(extra->template_instantiation_cache);
  VectorDestructWithContents(&extra->template_instantiations,
                             (VectorElementDestructor)SymbolDelete,
                             /*free_element=*/false);
  VectorDestructWithContents(&extra->contract_assertions,
                             (VectorElementDestructor)ContractAssertionDelete,
                             /*free_element=*/false);
  ConstraintExprDelete(extra->associated_constraint);
  if (extra->template_pattern_body != info->body) {
    ASTNodeDelete(extra->template_pattern_body);
  }
  free(extra);
}

// Gives the copy `to` of the function type `from`, which has no rarely set
// fields yet, its own.  Instantiations are not copied.
static void TypeRecordCopyFunctionExtra(TypeRecord* to, TypeRecord* from) {
  FunctionInfo* info = &to->info.function;
  const FunctionInfoExtra* source = from->info.function.extra;
  if (source == NULL) {
    return;
  }
  FunctionInfoExtra* extra = FunctionMutableExtra(info);
  // A pattern body other than the live body is owned, so the copy needs its
  // own.
  extra->template_pattern_body =
      source->template_pattern_body == from->info.function.body
          ? info->body
          : ASTNodeClone(source->template_pattern_body, IdentityCloneNode, NULL,
                         NULL);
  extra->coroutine_promise_type = source->coroutine_promise_type;
  TypeRecordIncRef(extra->coroutine_promise_type);
  extra->coroutine_frame_type = source->coroutine_frame_type;
  TypeRecordIncRef(extra->coroutine_frame_type);
  extra->coroutine_suspend_count = source->coroutine_suspend_count;
  extra->template_parameter_count = source->template_parameter_count;
  extra->template_parameter_base = source->template_parameter_base;
  for (size_t i = 0; i < source->template_parameters.length; i++) {
    VectorAppend(&extra->template_parameters,
                 TemplateParameterCopy(source->template_parameters.value.p[i]));
  }
  extra->associated_constraint =
      ConceptsCloneConstraint(source->associated_constraint);
  extra->explicit_condition = source->explicit_condition;
  extra->deleted_reason =
      source->deleted_reason != NULL
          ? NewStringWithLength(source->deleted_reason->value,
                                source->deleted_reason->length)
          : NULL;
  TypeRecordCopyContractAssertions(to, from);
}

// Deletes a TypeRecord with regard to the reference count.  The
// reference count is decremented and if it goes to zero the
// record can be deleted.  When deleting it, the record pointed
// to by the 'next' field is first deleted (using the same function)
// and then the memory is freed.
void TypeRecordDeleteLastReference(TypeRecord* record) {
  if (record == NULL) {
    return;
  }
  TypeArenaCheckLive(record);
  TypeRecordDecRef(record);
  if (record->refs == 0) {
    if (type_census_enabled) {
      TypeCensusNoteRelease(record);
    }
    if (record->next != NULL) {
      TypeRecordDelete(record->next);
      record->next = NULL;
    }
    if (record->template_arguments != NULL) {
      VectorDeleteWithContents(record->template_arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      record->template_arguments = NULL;
    }
    TypeDependentInfoDelete(record->dependent);
    record->dependent = NULL;
    // Delete type-specific info if refs goes to zero.
    if (TypeIsStructOrUnion(record)) {
      // Struct infos are not freed here: they can form reference cycles, so
      // they are all freed in one pass by StructRegistryRelease at end of
      // compilation.  Keep the count balanced for any code that reads it.
      if (record->info.struct_info != NULL) {
        record->info.struct_info->refs--;
      }
    } else if (TypeIsEnum(record)) {
      // Enum infos are freed in bulk by StructRegistryRelease (alongside
      // structs) so that decrements during the bulk teardown never touch an
      // already-freed info.  Keep the count balanced for any reader.
      if (record->info.enum_info != NULL) {
        record->info.enum_info->refs--;
      }
    } else if (TypeIsFunction(record)) {
      VectorDestructWithContents(&record->info.function.prototype,
                                 (VectorElementDestructor)SymbolDelete,
                                 /*free_element=*/false);
      FunctionInfoExtraDelete(&record->info.function);
    } else if (TypeIsVLA(record)) {
      // Delete the AST containing the size.
      ASTNodeDelete(record->info.array.size.vla.size);
      record->info.array.size.vla.size = NULL;
    }
    TypeArenaRelease(record);
  }
}

void TypeRecordDeleteUnowned(TypeRecord* record) {
  if (record == NULL || record->shared_immutable) {
    return;
  }
  if (record->refs == 0) {
    record->refs = 1;
  }
  TypeRecordDelete(record);
}

int TypeRecordAlignment(TypeRecord* record) {
  if (record == NULL) {
    return 1;
  }
  if (compiler->alignment == 1) {
    // No alignment necessary for this target.
    return 1;
  }
  switch (record->declarator) {
    case kDeclArray:
      return TypeRecordAlignment(record->next);
    case kDeclVector:
      return record->size > 0 ? record->size : 1;
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return SizeofPointer();
    case kDeclMemberPointer:
      return SizeofPointer();
    case kDeclFunction:
      return SizeofPointer();
    case kDeclPrimitive:
      if (TypeIsStructOrUnion(record)) {
        if (record->info.struct_info == NULL) {
          return 1;
        }
        int a = record->info.struct_info->alignment;
        return a > 0 ? a : 1;
      }
      if (TypeIsBitInt(record)) {
        return record->size > 0 ? record->size : 1;
      }
      return SizeofType(record->type);
  }
  return 1;
}

void TypeRecordIncRef(TypeRecord* record) {
  if (record == NULL) {
    return;
  }
  TypeArenaCheckLive(record);
  record->refs++;
}

void TypeRecordDecRef(TypeRecord* record) {
  if (record == NULL || record->shared_immutable) {
    return;
  }
  TypeArenaCheckLive(record);
  record->refs--;
}

TypeRecord* TypeRecordCalculateSize(TypeRecord* record) {
  if (record == NULL) {
    return NULL;
  }
  TypeRecordCalculateSize(record->next);
  if ((record->declarator == kDeclArray ||
       record->declarator == kDeclVector) &&
      !record->info.array.is_vla &&
      !record->info.array.is_dependent_bound) {
    record->size = record->info.array.size.fixed * record->next->size;
    return record;
  }
  // Class layouts can finish after a TypeRecord first caches an intermediate
  // size, particularly for template specializations and virtual bases.
  if (record->declarator == kDeclPrimitive && TypeIsStructOrUnion(record) &&
      record->info.struct_info != NULL && record->info.struct_info->size > 0) {
    record->size = record->info.struct_info->size;
    return record;
  }
  // A substituted template parameter may retain the placeholder's cached
  // primitive size even after its type changes (for example, `UInt` changing
  // from a pointer-sized placeholder to `unsigned long`). Primitive scalar
  // sizes are target properties, so recompute them from the concrete type.
  // Do not cache this probe.  Callers such as template-parameter
  // construction set template_parameter_index after the record is sized, and
  // a cached "absent" answer would hide that index for the rest of the type's
  // life.
  if (record->declarator == kDeclPrimitive && !TypeIsStructOrUnion(record) &&
      !TypeContainsTemplateParameterSlow(record)) {
    record->size = TypeIsBitInt(record)
                       ? BitIntStorageSize(record->bit_width)
                       : SizeofType(record->type);
    return record;
  }
  if (record->size == 0) {
    switch (record->declarator) {
      case kDeclArray:
      case kDeclVector:
        if (!record->info.array.is_vla &&
            !record->info.array.is_dependent_bound) {
          record->size = record->info.array.size.fixed * record->next->size;
        }
        break;
      case kDeclPointer:
      case kDeclReference:
      case kDeclRValueReference:
        record->size = SizeofPointer();
        break;
      case kDeclMemberPointer:
        record->size = MemberPointerSize(record);
        break;
      case kDeclFunction:
        record->size = SizeofPointer();
        break;
      case kDeclPrimitive:
        if (TypeIsStructOrUnion(record)) {
          if (record->info.struct_info != NULL) {
            record->size = record->info.struct_info->size;
          }
        } else {
          record->size = SizeofType(record->type);
        }
        break;
    }
  }
  return record;
}

void TypeRecordSyncStructSizes(Struct* str) {
  if (str == NULL || str->synced_type_record_size == str->size) {
    return;
  }
  for (size_t i = 0; i < str->type_records.length; i++) {
    TypeRecord* record = VectorGet(&str->type_records, i);
    if (TypeRecordUsesStructSize(record) &&
        record->info.struct_info == str) {
      record->size = str->size;
    }
  }
  str->synced_type_record_size = str->size;
}

void AppendCXXVTableClassName(String* out, Struct* str) {
  TypeRecord* type = str->tag_symbol != NULL ? str->tag_symbol->type : NULL;
  // A nested, local, or namespace-scope class shares its tag name with every
  // other class of that name (`A::F` and `B::F`, `O<int>::F` and `O<long>::F`,
  // `a::F` and `b::F`), so it is qualified by its mangled name.
  if ((str->lexical_parent == NULL && str->access_enclosing_function == NULL &&
       (str->tag_symbol == NULL || str->tag_symbol->namespace_ == NULL)) ||
      type == NULL ||
      !TypeIsStructOrUnion(type) || type->info.struct_info != str) {
    StringAppendString(out, str->tag_name);
    return;
  }
  String mangled;
  StringInit(&mangled, "");
  AppendCXXMangledTypeName(&mangled, type);
  for (size_t i = 0; i < mangled.length; i++) {
    char ch = mangled.value[i];
    bool valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                 (ch >= '0' && ch <= '9') || ch == '_';
    StringAppendChar(out, valid ? ch : '_');
  }
  StringDestruct(&mangled);
}

static uint64_t TypeIdentityHashBytes(uint64_t hash, const char* value) {
  if (value == NULL) {
    return hash;
  }
  while (*value != '\0') {
    hash ^= (unsigned char)*value++;
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}

uint64_t TypeRecordSemanticIdentityHash(TypeRecord* record) {
  uint64_t hash = UINT64_C(1469598103934665603);
  for (TypeRecord* node = record; node != NULL; node = node->next) {
    hash ^= (uint64_t)node->type;
    hash *= UINT64_C(1099511628211);
    hash ^= (uint64_t)node->declarator;
    hash *= UINT64_C(1099511628211);
    hash ^= (uint64_t)(uint32_t)node->size;
    hash *= UINT64_C(1099511628211);
    if (TypeIsFixedArray(node)) {
      hash ^= (uint64_t)node->info.array.size.fixed;
      hash *= UINT64_C(1099511628211);
    }
    if (TypeIsStructOrUnion(node) && node->info.struct_info != NULL &&
        node->info.struct_info->tag_name != NULL) {
      hash = TypeIdentityHashBytes(
          hash, node->info.struct_info->tag_name->value);
    }
  }
  return hash;
}

TypeRecord* NewTypeRecordWithSize(Type type, Qualifiers quals) {
  return TypeRecordCalculateSize(NewTypeRecord(type, quals));
}

// Join two type records through the next field.  This increments
// the reference count on the one pointed to.
void TypeRecordChain(TypeRecord* from, TypeRecord* to) {
  TypeRecordIncRef(to);
  from->next = to;
  from->template_parameter_summary =
      kTypeTemplateParameterSummaryUnknown;
}

void TypeRecordInvalidateTemplateParameterSummary(TypeRecord* record) {
  for (TypeRecord* current = record; current != NULL;
       current = current->next) {
    current->template_parameter_summary =
        kTypeTemplateParameterSummaryUnknown;
  }
}

static ASTNode* CloneVLAExpr(ASTNode* node, void* data) {
  return node;
}

/* Deep-copy a template parameter declaration (name, kind, pack flag, and any
 * default type/value), including its placeholder type record. */
TemplateParameter* TemplateParameterCopy(TemplateParameter* param) {
  if (param == NULL) {
    return NULL;
  }
  TemplateParameter* copy = malloc(sizeof(TemplateParameter));
  StringInit(&copy->name, param->name.value);
  copy->kind = param->kind;
  copy->is_parameter_pack = param->is_parameter_pack;
  copy->type = param->type != NULL ? TypeRecordCopy(param->type) : NULL;
  copy->default_type =
      param->default_type != NULL ? TypeRecordCopy(param->default_type) : NULL;
  copy->has_default_int = param->has_default_int;
  copy->default_int_value = param->default_int_value;
  copy->default_template_parameter_index =
      param->default_template_parameter_index;
  copy->associated_constraint =
      ConceptsCloneConstraint(param->associated_constraint);
  copy->index = param->index;
  copy->default_argument = TemplateArgumentCopy(param->default_argument);
  copy->template_parameters =
      TemplateParameterVectorCopy(param->template_parameters);
  copy->template_template_kind = param->template_template_kind;
  return copy;
}

Vector* TemplateParameterVectorCopy(Vector* params) {
  if (params == NULL) {
    return NULL;
  }
  Vector* copy = NewVector();
  VectorReserve(copy, params->length);
  for (size_t i = 0; i < params->length; i++) {
    VectorAppend(copy, TemplateParameterCopy(params->value.p[i]));
  }
  return copy;
}

/* Deep-copy a template argument (a concrete type/value, an unresolved parameter
 * reference, or a pack of arguments) used when instantiating templates. */
TemplateArgument* TemplateArgumentCopy(TemplateArgument* arg) {
  if (arg == NULL) {
    return NULL;
  }
  TemplateArgument* copy = TemplateArgumentAlloc();
  copy->kind = arg->kind;
  copy->is_pack_expansion = arg->is_pack_expansion;
  copy->references_parameter_pack = arg->references_parameter_pack;
  copy->type = TypeRecordInternArgumentOrCopy(arg->type);
  copy->int_value = arg->int_value;
  copy->template_parameter_index = arg->template_parameter_index;
  copy->pack_arguments = TemplateArgumentVectorCopy(arg->pack_arguments);
  // The dependent expression is arena-owned and only read (cloned) at
  // re-evaluation, so the pointer may be shared across copies.
  copy->dependent_expr = arg->dependent_expr;
  copy->location = arg->location;
  copy->value_kind = arg->value_kind;
  TemplateArgumentCopyExtra(copy, arg);
  return copy;
}

const TemplateArgumentExtra kNoTemplateArgumentExtra = {0};

TemplateArgumentExtra* TemplateArgumentMutableExtra(TemplateArgument* arg) {
  if (arg->extra == NULL) {
    arg->extra = calloc(1, sizeof(TemplateArgumentExtra));
    arg->extra->owner = arg;
  }
  assert(arg->extra->owner == arg);
  return arg->extra;
}

void TemplateArgumentCopyExtra(TemplateArgument* to,
                               const TemplateArgument* from) {
  if (from->extra == NULL) {
    return;
  }
  TemplateArgumentExtra* extra = TemplateArgumentMutableExtra(to);
  ASTNodeDelete(extra->object_initializer);
  TemplateArgument* owner = extra->owner;
  *extra = *from->extra;
  extra->owner = owner;
  extra->object_initializer = ASTNodeClone(from->extra->object_initializer,
                                           IdentityCloneNode, NULL, NULL);
}

TemplateArgument* NewTypeTemplateArgument(TypeRecord* type) {
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kTemplateParameterType;
  arg->template_parameter_index = -1;
  arg->type = TypeRecordInternArgumentOrCopy(type);
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

TemplateArgument* NewIntegralTemplateArgument(long long value) {
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kTemplateParameterNonType;
  arg->value_kind = kTemplateValueIntegral;
  arg->int_value = value;
  arg->template_parameter_index = -1;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

TemplateArgument* NewTemplateTemplateArgument(Symbol* symbol,
                                               int parameter_index) {
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kTemplateParameterTemplate;
  arg->template_parameter_index = parameter_index;
  TemplateArgumentMutableExtra(arg)->template_symbol = symbol;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

TemplateValueKind TemplateArgumentConcreteValueKind(
    const TemplateArgument* arg) {
  if (arg == NULL || arg->kind != kTemplateParameterNonType) {
    return kTemplateValueNone;
  }
  if (arg->value_kind != kTemplateValueNone) {
    return arg->value_kind;
  }
  // Preserve the meaning of arguments made by old code and old modules.
  if (arg->template_parameter_index < 0 && arg->dependent_expr == NULL) {
    return kTemplateValueIntegral;
  }
  return kTemplateValueNone;
}

static ASTNode* TemplatePointerConstantCore(ASTNode* expr) {
  while (expr != NULL &&
         (expr->op == AST_OP(cast) || expr->op == AST_OP(expr_init))) {
    if (expr->op == AST_OP(cast)) {
      expr = ((CastASTNode*)expr)->expr;
    } else {
      expr = ((ExpressionInitializerASTNode*)expr)->expr;
    }
  }
  return expr;
}

static Symbol* TemplatePointerConstantSymbol(ASTNode* expr) {
  expr = TemplatePointerConstantCore(expr);
  if (expr == NULL) {
    return NULL;
  }
  if (expr->op == AST_OP(address)) {
    expr = TemplatePointerConstantCore(((UnaryASTNode*)expr)->sub);
  }
  if (expr != NULL && expr->op == AST_OP(identifier)) {
    return ((IdentifierASTNode*)expr)->symbol;
  }
  if (expr != NULL && expr->op == AST_OP(structmember)) {
    StructMember* member = ((StructMemberASTNode*)expr)->member;
    return member != NULL && member->is_static ? member->symbol : NULL;
  }
  return NULL;
}

static bool TemplateExpressionIsNullAddress(ASTNode* expr) {
  ASTNode* core = TemplatePointerConstantCore(expr);
  if (core == NULL) {
    return false;
  }
  if (core->type != NULL && TypeIsNullPointer(core->type)) {
    return true;
  }
  if (core->op == AST_OP(number) &&
      ((ConstantASTNode*)core)->value.ivalue == 0) {
    return true;
  }
  int64_t value = 1;
  return EvaluateIntegerExpression(core, &value) && value == 0;
}

bool TemplateArgumentSetFromExpression(TemplateArgument* arg, ASTNode* expr) {
  if (arg == NULL || expr == NULL || expr->type == NULL) {
    return false;
  }
  TypeRecordDelete(arg->type);
  arg->type = TypeRecordCopy(expr->type);
  if (arg->extra != NULL) {
    TemplateArgumentExtra* extra = TemplateArgumentMutableExtra(arg);
    extra->value_symbol = NULL;
    extra->value_offset = 0;
    extra->value_adjustment = 0;
    extra->member_function = NULL;
    extra->reflection_value = NULL;
    ASTNodeDelete(extra->object_initializer);
    extra->object_initializer = NULL;
  }

  if (TypeIsReflection(expr->type)) {
    ReflectionValue* reflection = SemanticEvaluateReflection(expr);
    if (reflection == NULL) {
      return false;
    }
    arg->value_kind = kTemplateValueReflection;
    TemplateArgumentMutableExtra(arg)->reflection_value = reflection;
    return true;
  }
  if (TypeIsStructOrUnion(expr->type)) {
    SetQualifiers(arg->type,
                  GetQualifiers(arg->type) & ~(kQualConst | kQualVolatile));
    // The object initializer (and its clones in copies of `arg`) share
    // `arg->type`, so the argument counts its own reference.
    TypeRecordAdopt(arg->type);
    ASTNode* initializer =
        ConstexprTemplateArgumentObjectInitializerForExpression(arg->type,
                                                                expr);
    if (initializer == NULL) {
      return false;
    }
    arg->value_kind = kTemplateValueObject;
    TemplateArgumentMutableExtra(arg)->object_initializer = initializer;
    TemplateArgumentMutableExtra(arg)->value_symbol =
        GetCXXTemplateParameterObject(arg);
    if (TemplateArgumentExtraOf(arg)->value_symbol == NULL) {
      return false;
    }
    return true;
  }
  if (TypeIsNullPointer(expr->type)) {
    arg->value_kind = kTemplateValueNull;
    arg->int_value = 0;
    return true;
  }
  if (TypeIsMemberPointer(expr->type)) {
    MemberPointerValue value;
    bool is_null = TemplateExpressionIsNullAddress(expr);
    if (is_null) {
      MemberPointerEncodeNull(expr->type, &value);
    } else if (!MemberPointerTryEvaluateConstant(expr, expr->type, &value)) {
      return false;
    }
    StructMember* member = MemberPointerReferencedMember(expr);
    arg->value_kind = kTemplateValueMemberPointer;
    TemplateArgumentMutableExtra(arg)->value_symbol =
        member != NULL ? member->symbol : value.fn_symbol;
    TemplateArgumentMutableExtra(arg)->value_offset = value.ptr;
    TemplateArgumentMutableExtra(arg)->value_adjustment = value.adj;
    TemplateArgumentMutableExtra(arg)->member_function = value.fn_symbol;
    return true;
  }
  if (TypeIsArray(expr->type)) {
    Symbol* symbol = TemplatePointerConstantSymbol(expr);
    if (symbol == NULL || symbol->flags.is_block_scope) {
      return false;
    }
    arg->value_kind = kTemplateValuePointer;
    TemplateArgumentMutableExtra(arg)->value_symbol = symbol;
    TypeRecordDelete(arg->type);
    arg->type = NewPointerTo(GetQualifiers(expr->type), expr->type->next);
    return true;
  }
  if (TypeIsPointer(expr->type) || TypeIsFunction(expr->type)) {
    if (TypeIsPointer(expr->type) &&
        TemplateExpressionIsNullAddress(expr)) {
      arg->value_kind = kTemplateValueNull;
      arg->int_value = 0;
      return true;
    }
    Symbol* symbol = TemplatePointerConstantSymbol(expr);
    if (symbol == NULL || symbol->flags.is_block_scope) {
      return false;
    }
    arg->value_kind = kTemplateValuePointer;
    TemplateArgumentMutableExtra(arg)->value_symbol = symbol;
    if (TypeIsFunction(expr->type)) {
      TypeRecordDelete(arg->type);
      arg->type =
          NewPointerTo(GetQualifiers(expr->type), TypeRecordCopy(expr->type));
    }
    return true;
  }
  if (expr->type->declarator == kDeclPrimitive &&
      (expr->type->type &
       (kTypeFloat | kTypeDouble | kTypeLongDouble |
        kTypeFloat32 | kTypeFloat64)) != 0) {
    return false;
  }

  int64_t value = 0;
  if (!EvaluateIntegerExpression(expr, &value)) {
    return false;
  }
  arg->value_kind = kTemplateValueIntegral;
  arg->int_value = value;
  return true;
}

Symbol* TemplateArgumentLvalueSymbol(ASTNode* expr) {
  if (expr == NULL || expr->op != AST_OP(identifier)) {
    return NULL;
  }
  Symbol* symbol = ((IdentifierASTNode*)expr)->symbol;
  if (symbol == NULL || symbol->type == NULL || TypeIsFunction(symbol->type) ||
      TypeIsReference(symbol->type) || symbol->flags.is_template_parameter ||
      symbol->flags.is_argument || symbol->flags.is_temp ||
      symbol->flags.is_concept ||
      SymbolExtraOf(symbol)->variable_template != NULL ||
      (symbol->storage &
       (STO(typedef) | STO(auto) | STO(register) | STO(thread))) != 0 ||
      (symbol->flags.is_block_scope && (symbol->storage & STO(static)) == 0) ||
      (symbol->flags.value_set && !symbol->flags.is_defined)) {
    return NULL;
  }
  return symbol;
}

bool TemplateArgumentSetLvalue(TemplateArgument* arg, Symbol* symbol,
                               bool value_ok, bool may_be_dependent) {
  TemplateArgumentMutableExtra(arg)->lvalue_symbol = symbol;
  if (value_ok) {
    return true;
  }
  if (may_be_dependent && !symbol->flags.value_set &&
      (symbol->flags.is_constexpr || TypeIsConst(symbol->type))) {
    return false;
  }
  TypeRecordDelete(arg->type);
  arg->type = NewReferenceTypeRecord(kQualPlain, false);
  TypeRecordChain(arg->type, TypeRecordCopy(symbol->type));
  arg->type->type = symbol->type->type;
  TypeRecordCalculateSize(arg->type);
  arg->value_kind = kTemplateValuePointer;
  TemplateArgumentMutableExtra(arg)->value_symbol = symbol;
  TemplateArgumentMutableExtra(arg)->value_offset = 0;
  arg->int_value = 0;
  return true;
}

bool TemplateArgumentBindReferenceParameter(TemplateArgument* arg,
                                            TypeRecord* parameter_type) {
  if (arg == NULL || parameter_type == NULL ||
      !TypeIsReference(parameter_type) ||
      TemplateArgumentExtraOf(arg)->lvalue_symbol == NULL ||
      arg->dependent_expr != NULL || arg->template_parameter_index >= 0) {
    return false;
  }
  TemplateValueKind kind = TemplateArgumentConcreteValueKind(arg);
  if (kind != kTemplateValueIntegral && kind != kTemplateValueObject &&
      !(kind == kTemplateValuePointer && arg->type != NULL &&
        TypeIsReference(arg->type))) {
    return false;
  }
  TypeRecord* object_type = TemplateArgumentExtraOf(arg)->lvalue_symbol->type;
  TypeRecord* referent = parameter_type->next;
  if (object_type == NULL || referent == NULL ||
      !TypeEqualIgnoringTopLevelQualifierMask(object_type, referent,
                                              kQualConst | kQualVolatile)) {
    return false;
  }
  TypeRecord* object_element = object_type;
  TypeRecord* referent_element = referent;
  while (TypeIsArray(object_element) && TypeIsArray(referent_element)) {
    object_element = object_element->next;
    referent_element = referent_element->next;
  }
  if ((GetQualifiers(object_element) & ~GetQualifiers(referent_element) &
       (kQualConst | kQualVolatile)) != 0) {
    return false;
  }
  TypeRecordDelete(arg->type);
  arg->type = TypeRecordCopy(parameter_type);
  ASTNodeDelete(TemplateArgumentExtraOf(arg)->object_initializer);
  TemplateArgumentMutableExtra(arg)->object_initializer = NULL;
  arg->value_kind = kTemplateValuePointer;
  TemplateArgumentMutableExtra(arg)->value_symbol =
      TemplateArgumentExtraOf(arg)->lvalue_symbol;
  TemplateArgumentMutableExtra(arg)->value_offset = 0;
  arg->int_value = 0;
  return true;
}

bool TemplateArgumentValuesEqual(const TemplateArgument* left,
                                 const TemplateArgument* right) {
  if (left == NULL || right == NULL ||
      left->kind != right->kind) {
    return false;
  }
  if (left->kind == kTemplateParameterType) {
    return TypeEqual(left->type, right->type);
  }
  if (left->kind == kTemplateParameterTemplate) {
    return TemplateArgumentExtraOf(left)->template_symbol ==
               TemplateArgumentExtraOf(right)->template_symbol &&
           left->template_parameter_index == right->template_parameter_index;
  }
  if (left->template_parameter_index != right->template_parameter_index) {
    return false;
  }
  TemplateValueKind left_kind = TemplateArgumentConcreteValueKind(left);
  TemplateValueKind right_kind = TemplateArgumentConcreteValueKind(right);
  if (left_kind != right_kind) {
    return false;
  }
  if (left->type != NULL && right->type != NULL &&
      !TypeEqual(left->type, right->type)) {
    return false;
  }
  switch (left_kind) {
    case kTemplateValueIntegral:
      return left->int_value == right->int_value;
    case kTemplateValueNull:
      return true;
    case kTemplateValuePointer:
      return TemplateArgumentExtraOf(left)->value_symbol ==
                 TemplateArgumentExtraOf(right)->value_symbol &&
             TemplateArgumentExtraOf(left)->value_offset ==
                 TemplateArgumentExtraOf(right)->value_offset;
    case kTemplateValueMemberPointer:
      return TemplateArgumentExtraOf(left)->value_symbol ==
                 TemplateArgumentExtraOf(right)->value_symbol &&
             TemplateArgumentExtraOf(left)->value_offset ==
                 TemplateArgumentExtraOf(right)->value_offset &&
             TemplateArgumentExtraOf(left)->value_adjustment ==
                 TemplateArgumentExtraOf(right)->value_adjustment &&
             TemplateArgumentExtraOf(left)->member_function ==
                 TemplateArgumentExtraOf(right)->member_function;
    case kTemplateValueReflection:
      return ReflectionValueEqual(
          TemplateArgumentExtraOf(left)->reflection_value,
          TemplateArgumentExtraOf(right)->reflection_value);
    case kTemplateValueObject:
      return ConstexprObjectInitializersEquivalent(
          left->type, TemplateArgumentExtraOf(left)->object_initializer,
          TemplateArgumentExtraOf(right)->object_initializer);
    case kTemplateValueNone:
      return left->dependent_expr == right->dependent_expr;
  }
  return false;
}

static StructMember* TemplateArgumentReferencedMember(
    const TemplateArgument* arg) {
  if (arg == NULL || TemplateArgumentExtraOf(arg)->value_symbol == NULL ||
      arg->type == NULL) {
    return NULL;
  }
  Struct* owner = TypeMemberPointerClass(arg->type);
  if (owner == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member != NULL &&
        member->symbol == TemplateArgumentExtraOf(arg)->value_symbol) {
      return member;
    }
  }
  return NULL;
}

ASTNode* TemplateArgumentMaterializeExpression(
    const TemplateArgument* arg, SourceLocation location) {
  if (arg == NULL || arg->kind != kTemplateParameterNonType) {
    return NULL;
  }
  TypeRecord* type = arg->type != NULL
                         ? TypeRecordCopy(arg->type)
                         : NewTypeRecordWithSize(kTypeInt, kQualPlain);
  switch (TemplateArgumentConcreteValueKind(arg)) {
    case kTemplateValueIntegral:
      return NewIntConstantASTNode(arg->int_value, type, location);
    case kTemplateValueNull: {
      // A null data member pointer is not all-zero bits under every ABI.
      MemberPointerValue null_member = {0};
      if (TypeIsMemberPointer(type)) {
        MemberPointerEncodeNull(type, &null_member);
      }
      return NewIntConstantASTNode(null_member.ptr, type, location);
    }
    case kTemplateValuePointer: {
      if (TemplateArgumentExtraOf(arg)->value_symbol == NULL) {
        TypeRecordDelete(type);
        return NULL;
      }
      ASTNode* id = NewIdentifierASTNode(
          TemplateArgumentExtraOf(arg)->value_symbol, location);
      if (TypeIsReference(type)) {
        ASTNodeSetType(id, TypeRecordCopy(type->next));
        TypeRecordDelete(type);
        id->value_category = kValueCategoryLvalue;
        return id;
      }
      ASTNode* address = NewUnaryASTNode(AST_OP(address), NULL, location, id);
      ASTNodeSetType(address, type);
      return address;
    }
    case kTemplateValueMemberPointer: {
      StructMember* member = TemplateArgumentReferencedMember(arg);
      if (member == NULL) {
        return NewIntConstantASTNode(TemplateArgumentExtraOf(arg)->value_offset,
                                     type, location);
      }
      ASTNode* member_node = NewStructMemberASTNode(member, location);
      ASTNode* member_pointer =
          NewUnaryASTNode(AST_OP(member_ptr), NULL, location, member_node);
      ASTNodeSetType(member_pointer, type);
      return member_pointer;
    }
    case kTemplateValueReflection:
      TypeRecordDelete(type);
      return TemplateArgumentExtraOf(arg)->reflection_value != NULL
                 ? NewReflectionConstantASTNode(
                       TemplateArgumentExtraOf(arg)->reflection_value, location)
                 : NULL;
    case kTemplateValueObject: {
      if (TemplateArgumentExtraOf(arg)->value_symbol == NULL) {
        TypeRecordDelete(type);
        return NULL;
      }
      ASTNode* object = NewIdentifierASTNode(
          TemplateArgumentExtraOf(arg)->value_symbol, location);
      ASTNodeSetType(object, type);
      return object;
    }
    case kTemplateValueNone:
      TypeRecordDelete(type);
      return NULL;
  }
  TypeRecordDelete(type);
  return NULL;
}

/* Deep-copy a vector of template arguments (NULL-safe). */
Vector* TemplateArgumentVectorCopy(Vector* args) {
  if (args == NULL) {
    return NULL;
  }
  Vector* copy = NewVector();
  VectorReserve(copy, args->length);
  for (size_t i = 0; i < args->length; i++) {
    VectorAppend(copy, TemplateArgumentCopy(args->value.p[i]));
  }
  return copy;
}

Vector* TemplateArgumentVectorListCopy(Vector* list) {
  if (list == NULL) {
    return NULL;
  }
  Vector* copy = NewVector();
  VectorReserve(copy, list->length);
  for (size_t i = 0; i < list->length; i++) {
    VectorAppend(copy, TemplateArgumentVectorCopy(list->value.p[i]));
  }
  return copy;
}

// Copy a type record and chain it to its existing next,
// incrementing the ref count.
typedef struct {
  TypeRecord* from_func;
  TypeRecord* to_func;
  Symbol* from_result;
  Symbol* to_result;
} ContractCloneMap;

static ASTNode* RebindContractCloneNode(ASTNode* node, void* data) {
  if (node == NULL || node->op != AST_OP(identifier)) {
    return node;
  }
  ContractCloneMap* map = data;
  IdentifierASTNode* identifier = (IdentifierASTNode*)node;
  if (identifier->symbol == map->from_result) {
    identifier->symbol = map->to_result;
    return node;
  }
  Vector* from = &map->from_func->info.function.prototype;
  Vector* to = &map->to_func->info.function.prototype;
  for (size_t i = 0; i < from->length && i < to->length; i++) {
    if (identifier->symbol == from->value.p[i]) {
      identifier->symbol = to->value.p[i];
      break;
    }
  }
  return node;
}

void TypeRecordCopyContractAssertions(TypeRecord* to, TypeRecord* from) {
  for (size_t i = 0;
       i < FunctionExtraOf(&from->info.function)->contract_assertions.length;
       i++) {
    ContractAssertion* source =
        FunctionExtraOf(&from->info.function)->contract_assertions.value.p[i];
    Symbol* result_binding =
        source->result_binding != NULL
            ? NewSymbol(source->result_binding->name.value,
                        source->result_binding->type,
                        source->result_binding->storage)
            : NULL;
    if (result_binding != NULL) {
      result_binding->flags = source->result_binding->flags;
      result_binding->location = source->result_binding->location;
      result_binding->alignment = source->result_binding->alignment;
      result_binding->namespace_ = source->result_binding->namespace_;
      result_binding->value = source->result_binding->value;
    }
    Vector attrs = {0};
    AttributeListClone(&attrs, &source->attributes);
    ContractCloneMap map = {
        .from_func = from,
        .to_func = to,
        .from_result = source->result_binding,
        .to_result = result_binding,
    };
    ASTNode* predicate = ASTNodeClone(source->predicate,
                                      RebindContractCloneNode, &map, NULL);
    VectorAppend(&FunctionMutableExtra(&to->info.function)->contract_assertions,
                 NewContractAssertion(source->kind, predicate, result_binding,
                                      &attrs, source->location));
  }
}

struct ASTNode* FunctionTemplateCloneSourceBody(TypeRecord* func_type) {
  if (func_type == NULL || !TypeIsFunction(func_type)) {
    return NULL;
  }
  if (FunctionExtraOf(&func_type->info.function)->template_pattern_body !=
      NULL) {
    return FunctionExtraOf(&func_type->info.function)->template_pattern_body;
  }
  return func_type->info.function.body;
}

// A class written inside a template (the template itself, its nested and local
// classes, closures) is rebuilt for each instantiation even though its record
// names no template parameter.  Closure capture fields are also renumbered in
// place.
static bool StructIsTemplatePattern(Struct* str) {
  for (Struct* s = str; s != NULL; s = s->lexical_parent) {
    if (s->is_template || s->defining_template_scope_count > 0 ||
        s->in_namespace_template || s->local_class_awaits_member_arguments ||
        (s->tag_symbol != NULL && s->tag_symbol->flags.invented)) {
      return true;
    }
  }
  return false;
}

static bool TypeSpineCanShareAtDepth(TypeRecord* type, bool allow_class,
                                     int depth);

// `W<closure>` is rebuilt with each instantiation's closure, so a class is
// checked through its type arguments as well.
static bool TemplateArgumentsCanShare(Vector* args, int depth) {
  if (args == NULL) {
    return true;
  }
  for (size_t i = 0; i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    if (arg != NULL && arg->kind == kTemplateParameterType &&
        !TypeSpineCanShareAtDepth(arg->type, true, depth + 1)) {
      return false;
    }
  }
  return true;
}

static bool TypeSpineCanShare(TypeRecord* type, bool allow_class) {
  return TypeSpineCanShareAtDepth(type, allow_class, 0);
}

static bool TypeSpineCanShareAtDepth(TypeRecord* type, bool allow_class,
                                     int depth) {
  if (depth > 16) {
    return false;
  }
  int guard = 0;
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    // A cycle or a pathological declarator is copied, not shared.
    if (++guard > 64) {
      return false;
    }
    if (t->template_parameter_index >= 0 ||
        TypeDependentOf(t)->dependent_member_name != NULL ||
        TypeDependentOf(t)->dependent_decltype_expr != NULL ||
        TypeDependentOf(t)->dependent_splice_expr != NULL || t->is_pack_index ||
        TypeDependentOf(t)->pack_index_expr != NULL || TypeIsFunction(t)) {
      return false;
    }
    // An unknown bound (`int a[] = {...}`) is completed in place from the
    // initializer.
    if (t->declarator == kDeclArray &&
        (t->info.array.is_vla || t->info.array.is_dependent_bound ||
         t->info.array.template_parameter_index >= 0 ||
         t->info.array.is_flexible || t->info.array.size.fixed <= 0)) {
      return false;
    }
    // Callers that cannot see the active instantiation must not share a
    // class: its injected-class-name is rewritten during substitution.
    if (!allow_class &&
        (TypeIsStructOrUnion(t) || t->declarator == kDeclMemberPointer)) {
      return false;
    }
    if (TypeIsStructOrUnion(t) || t->declarator == kDeclMemberPointer) {
      Struct* str = t->info.struct_info;
      if (StructIsTemplatePattern(str)) {
        return false;
      }
      Vector* args = t->template_arguments;
      if (args == NULL && str != NULL && str->tag_symbol != NULL &&
          str->tag_symbol->type != t && str->tag_symbol->type != NULL) {
        args = str->tag_symbol->type->template_arguments;
      }
      if (!TemplateArgumentsCanShare(args, depth)) {
        return false;
      }
    }
  }
  return true;
}

static bool TypeRecordCanShare(TypeRecord* type, bool allow_class) {
  if (type == NULL) {
    return true;
  }
  // Reject function types and dependent declarators before the parameter
  // walk.  A function record is copied so its prototype stays mutable.
  if (!TypeSpineCanShare(type, allow_class)) {
    return false;
  }
  return !TypeContainsTemplateParameter(type);
}

bool TypeRecordCanShareThroughSubstitution(TypeRecord* type) {
  return TypeRecordCanShare(type, false);
}

bool TypeRecordCanShareConcreteClass(TypeRecord* type) {
  return TypeRecordCanShare(type, true);
}

TypeRecord* TypeRecordShareExisting(TypeRecord* type) {
  type_share_count++;
  TypeRecordIncRef(type);
  return type;
}

TypeRecord* TypeRecordAdopt(TypeRecord* type) {
  if (type != NULL && type->refs == 0) {
    TypeRecordIncRef(type);
  }
  return type;
}

// Concrete types stored once.  A canonical record is marked
// kTypeRecordCanonical; its `next` and the types of its template arguments
// are canonical as well, so nothing reachable from it changes.
#define kTypeRecordCanonical 2

typedef struct {
  TypeRecord* record;
  uint64_t hash;
} TypeInternSlot;

// Canonical records by structural hash, and each canonical record's hash by
// address so hashing stops at canonical children.
static TypeInternSlot* type_intern_by_hash = NULL;
static TypeInternSlot* type_intern_by_record = NULL;
static size_t type_intern_capacity = 0;
static size_t type_intern_count = 0;
static size_t type_intern_hits = 0;
static size_t type_intern_misses = 0;

static void TypeInternRelease(void) {
  free(type_intern_by_hash);
  free(type_intern_by_record);
  type_intern_by_hash = NULL;
  type_intern_by_record = NULL;
  type_intern_capacity = 0;
  type_intern_count = 0;
  type_intern_hits = 0;
  type_intern_misses = 0;
}

static bool TypeRecordIsCanonical(const TypeRecord* type) {
  return type != NULL && type->shared_immutable == kTypeRecordCanonical;
}

static uint64_t TypeInternMix(uint64_t h, uint64_t v) {
  h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
  return h;
}

static size_t TypeInternRecordSlot(const TypeRecord* type) {
  size_t mask = type_intern_capacity - 1;
  size_t s = ((uintptr_t)type >> 4) & mask;
  while (type_intern_by_record[s].record != NULL &&
         type_intern_by_record[s].record != type) {
    s = (s + 1) & mask;
  }
  return s;
}

static void TypeInternInsert(TypeRecord* canonical, uint64_t hash) {
  size_t mask = type_intern_capacity - 1;
  size_t s = hash & mask;
  while (type_intern_by_hash[s].record != NULL) {
    s = (s + 1) & mask;
  }
  type_intern_by_hash[s].record = canonical;
  type_intern_by_hash[s].hash = hash;
  s = TypeInternRecordSlot(canonical);
  type_intern_by_record[s].record = canonical;
  type_intern_by_record[s].hash = hash;
}

static void TypeInternReserve(void) {
  if (type_intern_count * 2 < type_intern_capacity) {
    return;
  }
  TypeInternSlot* old = type_intern_by_hash;
  size_t old_capacity = type_intern_capacity;
  type_intern_capacity = old_capacity ? old_capacity * 2 : 1 << 16;
  type_intern_by_hash = calloc(type_intern_capacity, sizeof(TypeInternSlot));
  free(type_intern_by_record);
  type_intern_by_record = calloc(type_intern_capacity, sizeof(TypeInternSlot));
  for (size_t i = 0; i < old_capacity; i++) {
    if (old[i].record != NULL) {
      TypeInternInsert(old[i].record, old[i].hash);
    }
  }
  free(old);
}

static uint64_t TypeInternHash(const TypeRecord* type, Qualifiers quals);

static uint64_t TypeInternHashString(const char* text) {
  uint64_t h = 1469598103934665603ull;
  for (const char* c = text; *c != '\0'; c++) {
    h = (h ^ (uint8_t)*c) * 1099511628211ull;
  }
  return h;
}

static uint64_t TypeInternHashChild(const TypeRecord* type) {
  if (type == NULL) {
    return 0;
  }
  if (TypeRecordIsCanonical(type)) {
    return type_intern_by_record[TypeInternRecordSlot(type)].hash;
  }
  return TypeInternHash(type, GetQualifiers(type));
}

static uint64_t TypeInternHashArguments(const Vector* args) {
  if (args == NULL) {
    return 0;
  }
  uint64_t h = TypeInternMix(0x51, args->length);
  for (size_t i = 0; i < args->length; i++) {
    const TemplateArgument* arg = args->value.p[i];
    if (arg == NULL) {
      h = TypeInternMix(h, 1);
      continue;
    }
    h = TypeInternMix(h, arg->kind);
    h = TypeInternMix(h, arg->is_pack_expansion);
    h = TypeInternMix(h, TypeInternHashChild(arg->type));
    h = TypeInternMix(h, (uint64_t)arg->int_value);
    h = TypeInternMix(h, (uintptr_t)TemplateArgumentExtraOf(arg)->value_symbol);
    h = TypeInternMix(h,
                      (uintptr_t)TemplateArgumentExtraOf(arg)->template_symbol);
    h = TypeInternMix(h, TypeInternHashArguments(arg->pack_arguments));
  }
  return h;
}

static uint64_t TypeInternHash(const TypeRecord* type, Qualifiers quals) {
  uint64_t h = 1469598103934665603ull;
  h = TypeInternMix(h, type->type);
  h = TypeInternMix(h, quals);
  h = TypeInternMix(h, type->declarator);
  h = TypeInternMix(h, (uint64_t)type->bit_width);
  h = TypeInternMix(h, (uint64_t)(int64_t)type->template_parameter_index);
  if (TypeDependentOf(type)->template_parameter_name != NULL) {
    h = TypeInternMix(
        h, TypeInternHashString(
               TypeDependentOf(type)->template_parameter_name->value));
  }
  h = TypeInternMix(h, (uintptr_t)type->template_origin);
  h = TypeInternMix(h, TypeInternHashChild(type->next));
  h = TypeInternMix(h, TypeInternHashArguments(type->template_arguments));
  if (type->declarator == kDeclArray || type->declarator == kDeclVector) {
    h = TypeInternMix(h, (uint64_t)type->info.array.size.fixed);
  } else if (TypeIsStructOrUnion(type) ||
             type->declarator == kDeclMemberPointer) {
    h = TypeInternMix(h, (uintptr_t)type->info.struct_info);
  } else if (TypeIsEnum(type)) {
    h = TypeInternMix(h, (uintptr_t)type->info.enum_info);
  }
  return h;
}

static bool TypeInternEqual(const TypeRecord* type, Qualifiers quals,
                            const TypeRecord* canonical);

static bool TypeInternChildEqual(const TypeRecord* type,
                                 const TypeRecord* canonical) {
  if (type == canonical) {
    return true;
  }
  if (type == NULL || canonical == NULL || TypeRecordIsCanonical(type)) {
    return false;
  }
  return TypeInternEqual(type, GetQualifiers(type), canonical);
}

static bool TypeInternArgumentsEqual(const Vector* args,
                                     const Vector* canonical) {
  if (args == NULL || canonical == NULL) {
    return args == canonical;
  }
  if (args->length != canonical->length) {
    return false;
  }
  for (size_t i = 0; i < args->length; i++) {
    const TemplateArgument* a = args->value.p[i];
    const TemplateArgument* b = canonical->value.p[i];
    if (a == NULL || b == NULL) {
      if (a != b) {
        return false;
      }
      continue;
    }
    if (a->kind != b->kind || a->is_pack_expansion != b->is_pack_expansion ||
        a->references_parameter_pack != b->references_parameter_pack ||
        a->int_value != b->int_value ||
        a->template_parameter_index != b->template_parameter_index ||
        a->dependent_expr != b->dependent_expr ||
        a->value_kind != b->value_kind ||
        TemplateArgumentExtraOf(a)->value_symbol !=
            TemplateArgumentExtraOf(b)->value_symbol ||
        TemplateArgumentExtraOf(a)->value_offset !=
            TemplateArgumentExtraOf(b)->value_offset ||
        TemplateArgumentExtraOf(a)->value_adjustment !=
            TemplateArgumentExtraOf(b)->value_adjustment ||
        TemplateArgumentExtraOf(a)->member_function !=
            TemplateArgumentExtraOf(b)->member_function ||
        TemplateArgumentExtraOf(a)->template_symbol !=
            TemplateArgumentExtraOf(b)->template_symbol ||
        TemplateArgumentExtraOf(a)->reflection_value !=
            TemplateArgumentExtraOf(b)->reflection_value ||
        TemplateArgumentExtraOf(a)->pack_index_expr !=
            TemplateArgumentExtraOf(b)->pack_index_expr ||
        TemplateArgumentExtraOf(a)->object_initializer !=
            TemplateArgumentExtraOf(b)->object_initializer ||
        !TypeInternChildEqual(a->type, b->type) ||
        !TypeInternArgumentsEqual(a->pack_arguments, b->pack_arguments)) {
      return false;
    }
  }
  return true;
}

static bool TypeInternEqual(const TypeRecord* type, Qualifiers quals,
                            const TypeRecord* canonical) {
  if (type->type != canonical->type || quals != GetQualifiers(canonical) ||
      type->declarator != canonical->declarator ||
      type->bit_width != canonical->bit_width ||
      type->template_parameter_index != canonical->template_parameter_index ||
      type->template_origin != canonical->template_origin ||
      type->is_pack_index != canonical->is_pack_index ||
      TypeDependentOf(type)->dependent_member_template_arguments !=
          TypeDependentOf(canonical)->dependent_member_template_arguments) {
    return false;
  }
  // Parameters at the same index of different templates print differently.
  const String* name = TypeDependentOf(type)->template_parameter_name;
  const String* canonical_name =
      TypeDependentOf(canonical)->template_parameter_name;
  if ((name == NULL) != (canonical_name == NULL) ||
      (name != NULL && strcmp(name->value, canonical_name->value) != 0)) {
    return false;
  }
  if (type->declarator == kDeclArray || type->declarator == kDeclVector) {
    const ArrayInfo* a = &type->info.array;
    const ArrayInfo* b = &canonical->info.array;
    if (a->size.fixed != b->size.fixed || a->is_flexible != b->is_flexible ||
        a->is_static != b->is_static || a->is_vla != b->is_vla ||
        a->is_placeholder_vla != b->is_placeholder_vla ||
        a->is_dependent_bound != b->is_dependent_bound ||
        a->template_parameter_index != b->template_parameter_index) {
      return false;
    }
  } else if (TypeIsStructOrUnion(type) ||
             type->declarator == kDeclMemberPointer) {
    if (type->info.struct_info != canonical->info.struct_info) {
      return false;
    }
  } else if (TypeIsEnum(type)) {
    if (type->info.enum_info != canonical->info.enum_info) {
      return false;
    }
  }
  return TypeInternChildEqual(type->next, canonical->next) &&
         TypeInternArgumentsEqual(type->template_arguments,
                                  canonical->template_arguments);
}

static TypeRecord* TypeRecordInternWithQualifiers(TypeRecord* type,
                                                  Qualifiers quals);

static void TypeInternArgumentTypes(Vector* args) {
  if (args == NULL) {
    return;
  }
  for (size_t i = 0; i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    if (arg == NULL) {
      continue;
    }
    if (arg->type != NULL && !TypeRecordIsCanonical(arg->type)) {
      TypeRecord* canonical =
          TypeRecordInternWithQualifiers(arg->type, GetQualifiers(arg->type));
      TypeRecordDelete(arg->type);
      arg->type = canonical;
    }
    TypeInternArgumentTypes(arg->pack_arguments);
  }
}

// The caller has checked TypeRecordCanShareConcreteClass(type) or
// TypeRecordIsBareTemplateParameter(type).
static TypeRecord* TypeRecordInternWithQualifiers(TypeRecord* type,
                                                  Qualifiers quals) {
  if (TypeRecordIsCanonical(type) && GetQualifiers(type) == quals) {
    return type;
  }
  TypeInternReserve();
  uint64_t hash = TypeInternHash(type, quals);
  size_t mask = type_intern_capacity - 1;
  for (size_t s = hash & mask; type_intern_by_hash[s].record != NULL;
       s = (s + 1) & mask) {
    if (type_intern_by_hash[s].hash == hash &&
        TypeInternEqual(type, quals, type_intern_by_hash[s].record)) {
      type_intern_hits++;
      return type_intern_by_hash[s].record;
    }
  }
  type_intern_misses++;
  // A fresh record: the source may still be modified by its owner.
  TypeRecord* canonical = TypeRecordCopy(type);
  SetQualifiers(canonical, quals);
  if (canonical->next != NULL && !TypeRecordIsCanonical(canonical->next)) {
    TypeRecord* next = TypeRecordInternWithQualifiers(
        canonical->next, GetQualifiers(canonical->next));
    TypeRecordDelete(canonical->next);
    canonical->next = next;
  }
  TypeInternArgumentTypes(canonical->template_arguments);
  canonical = TypeRecordCalculateSize(canonical);
  assert(TypeInternHash(canonical, GetQualifiers(canonical)) == hash);
  canonical->shared_immutable = kTypeRecordCanonical;
  canonical->refs = 2;
  // Interning children may have grown the tables.
  TypeInternReserve();
  TypeInternInsert(canonical, hash);
  type_intern_count++;
  return canonical;
}

bool TypeRecordCanIntern(TypeRecord* type) {
  return type != NULL &&
         (TypeRecordIsCanonical(type) || TypeRecordCanShareConcreteClass(type));
}

TypeRecord* TypeRecordIntern(TypeRecord* type) {
  return TypeRecordInternWithQualifiers(type, GetQualifiers(type));
}

TypeRecord* TypeRecordInternQualified(TypeRecord* type, Qualifiers quals) {
  return TypeRecordInternWithQualifiers(type, quals);
}

TypeRecord* TypeRecordInternOrCloneSpine(TypeRecord* type) {
  if (TypeRecordCanIntern(type)) {
    return TypeRecordIntern(type);
  }
  return TypeRecordCloneSpine(type);
}

TypeRecord* TypeRecordInternOrCopy(TypeRecord* type) {
  if (TypeRecordCanIntern(type)) {
    return TypeRecordIntern(type);
  }
  return TypeRecordCopy(type);
}

static bool TypeRecordIsBareTemplateParameter(const TypeRecord* type) {
  return type->declarator == kDeclPrimitive &&
         type->template_parameter_index >= 0 && type->next == NULL &&
         type->template_arguments == NULL &&
         TypeDependentOf(type)->dependent_member_name == NULL &&
         TypeDependentOf(type)->dependent_member_template_arguments == NULL &&
         TypeDependentOf(type)->dependent_decltype_expr == NULL &&
         !type->is_pack_index &&
         TypeDependentOf(type)->pack_index_expr == NULL &&
         TypeDependentOf(type)->pack_index_pack == NULL &&
         TypeDependentOf(type)->dependent_splice_expr == NULL &&
         type->size_sync_owner == NULL &&
         !TypeIsStructOrUnion((TypeRecord*)type) &&
         !TypeIsEnum((TypeRecord*)type);
}

TypeRecord* TypeRecordInternArgumentOrCopy(TypeRecord* type) {
  if (type != NULL && TypeRecordIsBareTemplateParameter(type)) {
    return TypeRecordInternWithQualifiers(type, GetQualifiers(type));
  }
  return TypeRecordInternOrCopy(type);
}

size_t TypeRecordInternHitCount(void) {
  return type_intern_hits;
}

size_t TypeRecordInternMissCount(void) {
  return type_intern_misses;
}

TypeRecord* TypeRecordForWrite(TypeRecord* type) {
  if (type == NULL || (type->refs == 0 && !type->shared_immutable)) {
    return type;
  }
  return TypeRecordCopy(type);
}

TypeRecord* TypeRecordAddQualifiers(TypeRecord* type, Qualifiers quals) {
  if (type == NULL) {
    return NULL;
  }
  Qualifiers added = quals & ~GetQualifiers(type);
  if (added == 0) {
    return type;
  }
  // A fresh copy is exclusive.  A record another owner already holds must
  // not gain qualifiers in place.
  if (type->refs == 0) {
    SetQualifiers(type, GetQualifiers(type) | added);
    return type;
  }
  TypeRecord* copy = TypeRecordCopy(type);
  SetQualifiers(copy, GetQualifiers(copy) | added);
  return copy;
}

TypeRecord* TypeRecordCopy(TypeRecord* record) {
  if (record == NULL) {
    return NULL;
  }
  TypeArenaCheckLive(record);
  type_copy_count++;
  if (type_copy_callers != NULL) {
    TypeCopyNoteCaller(__builtin_return_address(0));
  }
  type_census_caller = __builtin_return_address(0);
  TypeRecord* r = TypeArenaAllocSized(TypeIsFunction(record)
                                          ? TYPE_RECORD_FULL_SIZE
                                          : TYPE_RECORD_NON_FUNCTION_SIZE);
  bool full_slot = r->full_slot;
  memcpy(r, record, offsetof(TypeRecord, info));
  r->full_slot = full_slot;
  r->template_parameter_summary =
      kTypeTemplateParameterSummaryUnknown;
  if (TypeIsFunction(record)) {
    memcpy(&r->info.function, &record->info.function, sizeof(FunctionInfo));
    r->info.function.extra = NULL;
    r->info.function.references_marked = false;
    r->info.function.body_analyzed = false;
    r->info.function.native_code_emitted = false;
  } else if (record->declarator == kDeclArray ||
             record->declarator == kDeclVector) {
    memcpy(&r->info.array, &record->info.array, sizeof(ArrayInfo));
  } else if (TypeIsStructOrUnion(record) ||
             record->declarator == kDeclMemberPointer) {
    r->info.struct_info = record->info.struct_info;
  } else if (TypeIsEnum(record)) {
    r->info.enum_info = record->info.enum_info;
  }
  r->id = next_type_id;
  r->refs = 0;  // No refs to this yet.
  r->shared_immutable = 0;
  r->size_sync_owner = NULL;
  r->dependent = NULL;
  if (r->next == NULL && record->dependent == NULL &&
      r->template_arguments == NULL && !TypeIsFunction(record) &&
      !TypeIsStructOrUnion(record) && !TypeIsEnum(record) &&
      !TypeIsVLA(record)) {
    return r;
  }
  if (record->dependent != NULL) {
    const TypeDependentInfo* from = record->dependent;
    TypeDependentInfo* to = TypeMutableDependent(r);
    to->template_parameter_name =
        from->template_parameter_name != NULL
            ? NewString(from->template_parameter_name->value)
            : NULL;
    to->dependent_member_name =
        from->dependent_member_name != NULL
            ? NewString(from->dependent_member_name->value)
            : NULL;
    to->dependent_member_template_arguments = TemplateArgumentVectorListCopy(
        from->dependent_member_template_arguments);
    to->dependent_decltype_expr = from->dependent_decltype_expr;
    to->pack_index_expr =
        ASTNodeClone(from->pack_index_expr, IdentityCloneNode, NULL, NULL);
    to->pack_index_pack = TemplateArgumentCopy(from->pack_index_pack);
    to->dependent_splice_expr = ASTNodeClone(from->dependent_splice_expr,
                                             IdentityCloneNode, NULL, NULL);
  }
  if (r->next != NULL) {
    TypeRecordIncRef(r->next);  // Another ref to next.
  }
  r->template_arguments =
      TemplateArgumentVectorCopy(record->template_arguments);
  if (TypeIsFunction(record)) {
    VectorInit(&r->info.function.prototype);
    for (size_t i = 0; i < record->info.function.prototype.length; i++) {
      Symbol* formal = record->info.function.prototype.value.p[i];
      if (formal == NULL) {
        VectorAppend(&r->info.function.prototype, NULL);
        continue;
      }
      Symbol* clone = NewSymbol(
          formal->name.value,
          formal->type != NULL ? TypeRecordCopy(formal->type) : NULL,
          formal->storage);
      clone->flags = formal->flags;
      clone->location = formal->location;
      clone->alignment = formal->alignment;
      clone->namespace_ = formal->namespace_;
      clone->value = formal->value;
      AttributeListDestruct(&clone->attributes);
      AttributeListClone(&clone->attributes, &formal->attributes);
      SymbolCloneDefaultArgument(clone, formal);
      VectorAppend(&r->info.function.prototype, clone);
    }
    TypeRecordCopyFunctionExtra(r, record);
  }
  // Increment ref counts for type-specific objects.
  if (TypeIsStructOrUnion(record)) {
    if (record->info.struct_info != NULL) {
      record->info.struct_info->refs++;
    }
  } else if (TypeIsEnum(record)) {
    if (record->info.enum_info != NULL) {
      record->info.enum_info->refs++;
    }
  } else if (TypeIsVLA(record)) {
    // Copy the expression AST.
    r->info.array.size.vla.size =
      ASTNodeClone(record->info.array.size.vla.size, CloneVLAExpr, NULL, NULL);
  }
  TypeRecordRegisterStructSizeUser(r);
  return r;
}

TypeRecord* TypeRecordCloneSpine(TypeRecord* record) {
  if (record == NULL) {
    return NULL;
  }
  TypeRecord* copy = TypeRecordCopy(record);
  if (copy->next != NULL) {
    TypeRecord* shared_next = copy->next;
    copy->next = NULL;
    TypeRecordDelete(shared_next);
    TypeRecordChain(copy, TypeRecordCloneSpine(record->next));
  }
  return copy;
}

//
// Type creation functions.
//

TypeRecord* NewPointerTypeRecord(Qualifiers quals) {
  TypeRecord* t = NewTypeRecord(kTypeImplicit, quals);
  t->declarator = kDeclPointer;
  t->size = compiler->pointer_size;
  return t;
}

TypeRecord* NewMemberPointerTypeRecord(Struct* class_info, Qualifiers quals) {
  TypeRecord* t = NewTypeRecord(kTypeImplicit, quals);
  t->declarator = kDeclMemberPointer;
  TypeRecordSetStructInfo(t, class_info);
  return t;
}

TypeRecord* TypeMemberPointerPointeeFromMember(StructMember* member) {
  if (member == NULL || member->symbol == NULL || member->symbol->type == NULL) {
    return NULL;
  }
  // A pointer-to-member freezes the member's type, so a member function with a
  // placeholder return type has to be deduced before it is copied.  Closures
  // reach this through `&C::operator()` in deduction guides such as
  // std::function's.
  SemanticEnsureAutoReturnTypeDeduced(member->symbol->type);
  TypeRecord* type = TypeRecordCopy(member->symbol->type);
  if (!member->is_member_function || !TypeIsFunction(type)) {
    return type;
  }
  FunctionInfo* info = &type->info.function;
  if (info->prototype.length == 0) {
    return type;
  }
  Symbol* first = (Symbol*)info->prototype.value.p[0];
  if (first == NULL || !StringEqual(&first->name, "this")) {
    return type;
  }
  Vector new_prototype;
  VectorInit(&new_prototype);
  for (size_t i = 1; i < info->prototype.length; i++) {
    VectorAppend(&new_prototype, info->prototype.value.p[i]);
  }
  SymbolDelete(first);
  VectorDestruct(&info->prototype);
  info->prototype = new_prototype;
  for (size_t i = 0; i < info->prototype.length; i++) {
    Symbol* formal = (Symbol*)info->prototype.value.p[i];
    if (formal != NULL) {
      formal->value.arg_number = (int)i;
    }
  }
  return type;
}

bool FunctionHasImplicitThisParameter(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.has_explicit_object_parameter ||
      func->info.function.prototype.length == 0) {
    return false;
  }
  Symbol* first = func->info.function.prototype.value.p[0];
  return first != NULL && first->flags.invented &&
         StringEqual(&first->name, "this");
}

bool FunctionHasExplicitObjectParameter(TypeRecord* func) {
  return func != NULL && TypeIsFunction(func) &&
         func->info.function.has_explicit_object_parameter &&
         func->info.function.prototype.length > 0;
}

TypeRecord* NewReferenceTypeRecord(Qualifiers quals, bool rvalue) {
  TypeRecord* t = NewTypeRecord(kTypeImplicit, quals);
  t->declarator = rvalue ? kDeclRValueReference : kDeclReference;
  t->size = compiler->pointer_size;
  return t;
}

TypeRecord* NewPointerTo(Qualifiers quals, TypeRecord* type) {
  TypeRecord* ptr = NewPointerTypeRecord(quals);
  TypeRecordChain(ptr, type);
  return ptr;
}

Symbol* NewCXXThisSymbol(Struct* owner, bool is_const_member,
                         bool is_volatile_member, SourceLocation location) {
  Qualifiers object_qualifiers = kQualPlain;
  if (is_const_member) {
    object_qualifiers |= kQualConst;
  }
  if (is_volatile_member) {
    object_qualifiers |= kQualVolatile;
  }
  TypeRecord* class_type =
      NewTypeRecord(owner != NULL && owner->is_union ? kTypeUnion : kTypeStruct,
                    object_qualifiers);
  TypeRecordSetStructInfo(class_type, owner);
  TypeRecord* this_type = NewPointerTo(kQualPlain, class_type);
  // Members of a concrete class share one `this` type per cv-qualification.
  if (TypeRecordCanIntern(this_type)) {
    TypeRecord* canonical = TypeRecordIntern(this_type);
    TypeRecordDeleteUnowned(this_type);
    this_type = canonical;
  }
  Symbol* this_symbol = NewSymbol("this", this_type, STO(implicit));
  this_symbol->flags.is_argument = true;
  this_symbol->flags.invented = true;
  this_symbol->flags.is_defined = true;
  this_symbol->value.arg_number = 0;
  this_symbol->location = location;
  return this_symbol;
}

static Symbol* NewCXXCompleteObjectSymbol(SourceLocation location) {
  Symbol* symbol =
      NewSymbol("__complete_object",
                NewTypeRecordWithSize(kTypeInt, kQualPlain), STO(implicit));
  symbol->flags.is_argument = true;
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = location;
  return symbol;
}

void TypeRecordRemoveImplicitThisParameter(TypeRecord* func) {
  if (!FunctionHasImplicitThisParameter(func)) {
    return;
  }
  Symbol* this_symbol = func->info.function.prototype.value.p[0];
  Vector new_prototype;
  VectorInit(&new_prototype);
  for (size_t i = 1; i < func->info.function.prototype.length; i++) {
    VectorAppend(&new_prototype, func->info.function.prototype.value.p[i]);
  }
  SymbolDelete(this_symbol);
  VectorDestruct(&func->info.function.prototype);
  func->info.function.prototype = new_prototype;
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL) {
      formal->value.arg_number = (int32_t)i;
    }
  }
}

void TypeRecordAddCXXThisParameter(TypeRecord* func, Struct* owner,
                                   SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) || owner == NULL ||
      func->info.function.cxx_member_owner != NULL) {
    return;
  }
  Symbol* this_symbol =
      NewCXXThisSymbol(owner, func->info.function.is_const_member,
                       func->info.function.is_volatile_member, location);
  if (func->info.function.prototype.length == 0) {
    VectorAppend(&func->info.function.prototype, this_symbol);
  } else {
    VectorInsertBefore(&func->info.function.prototype, 0, this_symbol);
  }
  if ((func->info.function.is_constructor ||
       func->info.function.is_destructor) &&
      StructHasVirtualBases(owner)) {
    Symbol* complete_object = NewCXXCompleteObjectSymbol(location);
    if (func->info.function.prototype.length <= 1) {
      VectorAppend(&func->info.function.prototype, complete_object);
    } else {
      VectorInsertBefore(&func->info.function.prototype, 1, complete_object);
    }
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    formal->value.arg_number = (int32_t)i;
  }
  func->info.function.cxx_member_owner = owner;
}

TypeRecord* NewArrayTypeRecord(Qualifiers quals, bool is_static) {
  TypeRecord* t = NewTypeRecord(kTypeImplicit, quals);
  t->declarator = kDeclArray;
  t->info.array.size.fixed = 0;
  t->info.array.is_flexible = false;
  t->info.array.is_static = is_static;
  t->info.array.size.vla.size = NULL;
  t->info.array.size.vla.codegen_info = NULL;
  t->info.array.is_vla = false;
  t->info.array.is_placeholder_vla = false;
  t->info.array.is_dependent_bound = false;
  t->info.array.template_parameter_index = -1;
  t->size = 0;  // Don't know yet.
  return t;
}

TypeRecord* NewBasicArrayTypeRecord(Qualifiers quals, int size, bool is_flexible) {
  TypeRecord* t = NewArrayTypeRecord(quals, false);
  t->info.array.size.fixed = size;
  t->info.array.is_flexible = is_flexible;
  return t;
}

TypeRecord* NewVectorTypeRecord(TypeRecord* element_type, int lane_count) {
  TypeRecord* t = NewArrayTypeRecord(kQualPlain, false);
  t->declarator = kDeclVector;
  t->info.array.size.fixed = lane_count;
  TypeRecordChain(t, element_type);
  return TypeRecordCalculateSize(t);
}

ContractAssertion* NewContractAssertion(ContractAssertionKind kind,
                                        ASTNode* predicate,
                                        Symbol* result_binding,
                                        Vector* attributes,
                                        SourceLocation location) {
  ContractAssertion* assertion = malloc(sizeof(*assertion));
  memset(assertion, 0, sizeof(*assertion));
  assertion->kind = kind;
  assertion->predicate = predicate;
  assertion->result_binding = result_binding;
  assertion->location = location;
  if (attributes != NULL) {
    assertion->attributes = *attributes;
    memset(attributes, 0, sizeof(*attributes));
  } else {
    VectorInit(&assertion->attributes);
  }
  return assertion;
}

void ContractAssertionDelete(ContractAssertion* assertion) {
  if (assertion == NULL) {
    return;
  }
  ASTNodeDelete(assertion->predicate);
  SymbolDelete(assertion->result_binding);
  AttributeListDestruct(&assertion->attributes);
  free(assertion);
}

TypeRecord* NewFunctionTypeRecord() {
  TypeRecord* t = NewFunctionCapableTypeRecord(kTypeImplicit, kQualPlain);
  t->declarator = kDeclFunction;
  // NewTypeRecord zero-initializes FunctionInfo, including its vectors and
  // enum fields whose "none" values are zero. Only nonzero defaults belong
  // here.
  t->info.function.is_noexcept_eligible = true;
  t->info.function.virtual_index = -1;
  return t;
}

TypeRecord* NewSizeTypeRecord() {
  // size_t must match the headers' typedef (`unsigned long` on LP64 targets,
  // `unsigned int` on 32-bit) so that an explicitly declared `operator new`
  // / `operator delete` (e.g. from <new>) mangles identically to the implicit
  // allocation function the compiler synthesises for `new`/`delete`
  // expressions.  On LP64 this yields the Itanium ABI names `_Znwm` / `_Znam`.
  if (compiler->pointer_size == 8) {
    return NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain);
  }
  return NewTypeRecordWithSize(kTypeInt | kTypeUnsigned, kQualPlain);
}

StructMember* NewStructMember(Symbol* symbol) {
  StructMember* mem = malloc(sizeof(StructMember));
  mem->symbol = symbol;
  mem->default_initializer = NULL;
  mem->bit_width_expr = NULL;
  mem->byte_offset = 0;
  mem->bit_offset = 0;
  mem->bit_size = 0;
  mem->is_bit_field = false;
  mem->cxx_vcall_offset = 0;
  mem->is_anon = false;
  mem->is_static = false;
  mem->is_mutable = false;
  mem->is_member_function = false;
  mem->is_using_declaration = false;
  mem->access = kAccessPublic;
  mem->overload_next = NULL;
  return mem;
}

/* Free a template parameter and its owned type records. */
void TemplateParameterDelete(TemplateParameter* param) {
  if (param == NULL) {
    return;
  }
  StringDestruct(&param->name);
  TypeRecordDelete(param->type);
  TypeRecordDelete(param->default_type);
  TemplateArgumentDelete(param->default_argument);
  if (param->template_parameters != NULL) {
    VectorDeleteWithContents(
        param->template_parameters,
        (VectorElementDestructor)TemplateParameterDelete,
        /*free_element=*/false);
  }
  ConstraintExprDelete(param->associated_constraint);
  free(param);
}

/* Free a template argument, recursively freeing any pack elements. */
void TemplateArgumentDelete(TemplateArgument* arg) {
  if (arg == NULL) {
    return;
  }
  TypeRecordDelete(arg->type);
  if (arg->pack_arguments != NULL) {
    VectorDeleteWithContents(arg->pack_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  if (arg->extra != NULL) {
    ASTNodeDelete(arg->extra->object_initializer);
    free(arg->extra);
  }
  memset(arg, 0, sizeof(*arg));
  TemplateArgumentSlot* slot = (TemplateArgumentSlot*)arg;
  slot->next = free_template_arguments;
  free_template_arguments = slot;
}

/* Create a record describing a class-template partial specialization: its own
 * template parameters and the argument pattern (e.g. `<T, T*>`) that incoming
 * instantiation arguments are matched against. */
ClassTemplatePartialSpecialization* NewClassTemplatePartialSpecialization(
    Symbol* tag_symbol, Vector* template_parameters, Vector* pattern_arguments) {
  ClassTemplatePartialSpecialization* partial =
      malloc(sizeof(ClassTemplatePartialSpecialization));
  partial->tag_symbol = tag_symbol;
  partial->associated_constraint = NULL;
  partial->variable_initializer = NULL;
  partial->variable_type = NULL;
  VectorInit(&partial->template_parameters);
  VectorInit(&partial->pattern_arguments);
  for (size_t i = 0; template_parameters != NULL &&
                     i < template_parameters->length; i++) {
    VectorAppend(&partial->template_parameters,
                 TemplateParameterCopy(template_parameters->value.p[i]));
  }
  for (size_t i = 0; pattern_arguments != NULL &&
                     i < pattern_arguments->length; i++) {
    VectorAppend(&partial->pattern_arguments,
                 TemplateArgumentCopy(pattern_arguments->value.p[i]));
  }
  return partial;
}

/* Build a partial/explicit specialization record for a *variable* template.
 * Reuses the class-template specialization record (tag_symbol left NULL) and
 * additionally captures the specialization's unanalyzed initializer and type,
 * which are folded per use once its parameters are deduced from actual args. */
ClassTemplatePartialSpecialization* NewVariableTemplatePartialSpecialization(
    Vector* template_parameters, Vector* pattern_arguments,
    struct ASTNode* initializer, TypeRecord* type) {
  ClassTemplatePartialSpecialization* partial =
      NewClassTemplatePartialSpecialization(NULL, template_parameters,
                                            pattern_arguments);
  partial->variable_initializer = initializer;
  partial->variable_type = type != NULL ? TypeRecordCopy(type) : NULL;
  return partial;
}

/* Free a partial-specialization record and its parameter/pattern vectors. */
void ClassTemplatePartialSpecializationDelete(
    ClassTemplatePartialSpecialization* partial) {
  if (partial == NULL) {
    return;
  }
  VectorDestructWithContents(&partial->template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorDestructWithContents(&partial->pattern_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  ConstraintExprDelete(partial->associated_constraint);
  if (partial->variable_initializer != NULL) {
    ASTNodeDelete(partial->variable_initializer);
  }
  if (partial->variable_type != NULL) {
    TypeRecordDelete(partial->variable_type);
  }
  free(partial);
}

CXXBaseSpecifier* NewCXXBaseSpecifier(TypeRecord* type,
                                             CXXAccess access,
                                             bool is_virtual) {
  CXXBaseSpecifier* base = malloc(sizeof(CXXBaseSpecifier));
  base->type = type;
  TypeRecordIncRef(type);
  base->access = access;
  base->byte_offset = 0;
  base->is_virtual = is_virtual;
  base->is_pack_expansion = false;
  return base;
}

void CXXBaseSpecifierDelete(CXXBaseSpecifier* base) {
  TypeRecordDelete(base->type);
  free(base);
}

void CXXMemberUsingDeclarationDelete(CXXMemberUsingDeclaration* decl) {
  if (decl == NULL) {
    return;
  }
  TypeRecordDelete(decl->base_type);
  StringDestruct(&decl->member_name);
  free(decl);
}

CXXVirtualBaseInfo* NewCXXVirtualBaseInfo(TypeRecord* type,
                                                CXXAccess access,
                                                int vbtable_index) {
  CXXVirtualBaseInfo* base = malloc(sizeof(CXXVirtualBaseInfo));
  base->type = type;
  TypeRecordIncRef(type);
  base->access = access;
  base->byte_offset = 0;
  base->vbtable_index = vbtable_index;
  return base;
}

void CXXVirtualBaseInfoDelete(CXXVirtualBaseInfo* base) {
  TypeRecordDelete(base->type);
  free(base);
}

void CXXVBTableInfoDelete(CXXVBTableInfo* info) {
  free(info);
}

void CXXVTableInfoDelete(CXXVTableInfo* info) {
  free(info);
}

void StructMemberDelete(StructMember* member) {
  if (member->default_initializer != NULL) {
    ASTNodeDelete(member->default_initializer);
  }
  if (member->bit_width_expr != NULL) {
    ASTNodeDelete(member->bit_width_expr);
  }
  SymbolDelete(member->symbol);
  free(member);
}

bool StructMemberIsBitField(StructMember* member) {
  return member != NULL && (member->is_bit_field || member->bit_size > 0);
}

bool TypeIsCompleteClass(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return false;
  }
  Symbol* tag = type->info.struct_info->tag_symbol;
  return tag == NULL || !tag->flags.is_forward_declared;
}

static int CompareStructMember(const void* a, const void* b) {
  const MapKeyValue* key1 = (const MapKeyValue*)a;
  const MapKeyValue* key2 = (const MapKeyValue*)b;
  return StringCompareString(key1->key.p, key2->key.p);
}

Struct* NewStruct(bool is_union) {
  Struct* s = malloc(sizeof(Struct));
  s->refs = 1;
  s->lexical_parent = NULL;
  s->access_enclosing_function = NULL;
  s->in_namespace_template = false;
  s->lambda_body_needs_reanalysis = false;
  s->lambda_has_init_capture = false;
  s->consteval_only_cached = 0;
  s->consteval_only_member_count = 0;
  s->consteval_only_base_count = 0;
  s->local_class_arguments = NULL;
  s->local_class_discriminator = 0;
  s->local_class_awaits_member_arguments = false;
  s->local_class_partial_owner = NULL;
  s->tag_symbol = NULL;
  VectorInit(&s->bases);
  VectorInit(&s->friend_classes);
  VectorInit(&s->friend_functions);
  VectorInit(&s->member_using_declarations);
  VectorInit(&s->friend_type_declarations);
  VectorInit(&s->virtual_bases);
  VectorInit(&s->members);
  VectorInit(&s->virtual_members);
  VectorInit(&s->template_parameters);
  VectorInit(&s->partial_specializations);
  VectorInit(&s->deduction_guides);
  s->associated_constraint = NULL;
  s->vptr_member = NULL;
  s->vtable_symbol = NULL;
  s->vbptr_member = NULL;
  s->vbtable_symbol = NULL;
  VectorInit(&s->vtable_symbols);
  VectorInit(&s->vbtable_symbols);
  MapInit(&s->symbol_table, CompareStructMember);
  MapInitForCharPointerKeys(&s->symbol_name_table);
  VectorInit(&s->type_records);
  s->is_union = is_union;
  s->is_class = false;
  s->is_final = false;
  s->is_template = false;
  s->is_aggregate = CompilerIsCXX();
  s->cxx_special_members_complete = false;
  s->vtables_registered = false;
  s->instantiation_in_progress = false;
  s->resolving_in_progress_member = false;
  s->layout_pending_completion = false;
  s->template_parameter_count = 0;
  s->defining_template_scope_count = 0;
  s->partial_specialization_primary = NULL;
  s->next_offset = 0;
  s->current_offset = 0;
  s->size = 0;
  s->synced_type_record_size = -1;
  s->non_virtual_size = 0;
  s->alignment = 1;
  s->packed = false;
  s->is_abstract = false;
  s->explicit_alignment = 0;
  s->pack = 0;
  s->next_bit_pos = 65;
  VectorInit(&s->static_asserts);
  // Track every struct so it can be freed in bulk at end of compilation; see
  // StructRegistryRelease (struct infos can form reference cycles).
  if (!struct_registry_initialized) {
    VectorInit(&struct_registry);
    struct_registry_initialized = true;
  }
  s->serial = (int)struct_registry.length;
  VectorAppend(&struct_registry, s);
  return s;
}

TypeRecord* NewComplexTypeRecord(Type element_type, Qualifiers quals) {
  assert(element_type == kTypeFloat || element_type == kTypeDouble ||
         element_type == kTypeLongDouble);

  Struct* layout = NewStruct(false);
  // This is an implementation layout for a scalar arithmetic type, not a C++
  // class.  Keeping the ordinary two-member aggregate machinery underneath it
  // lets every backend reuse its established by-value memory paths.
  layout->is_aggregate = false;

  const char* names[] = {"__real", "__imag"};
  for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
    TypeRecord* member_type =
        NewTypeRecordWithSize(element_type, kQualPlain);
    Symbol* symbol = NewSymbol(names[i], member_type, STO(implicit));
    symbol->flags.invented = true;
    symbol->flags.is_defined = true;
    StructAddSyntheticMember(layout, NewStructMember(symbol));
  }

  TypeRecord* result =
      NewTypeRecord(kTypeComplex | kTypeStruct | element_type, quals);
  TypeRecordSetStructInfo(result, layout);
  return TypeRecordCalculateSize(result);
}

// Tear down a struct's members and member tables, but do NOT free the Struct
// itself (see StructRegistryRelease's two-phase teardown).
static void StructTeardownMembers(Struct* s) {
  VectorDestructWithContents(&s->bases,
                             (VectorElementDestructor)CXXBaseSpecifierDelete,
                             /*free_element=*/false);
  // Friend lists hold borrowed references (the structs and function symbols are
  // owned elsewhere), so just release the backing storage.
  VectorDestruct(&s->friend_classes);
  VectorDestruct(&s->friend_functions);
  VectorDestructWithContents(
      &s->member_using_declarations,
      (VectorElementDestructor)CXXMemberUsingDeclarationDelete,
      /*free_element=*/false);
  VectorDestructWithContents(
      &s->friend_type_declarations,
      (VectorElementDestructor)CXXFriendTypeDeclarationDelete,
      /*free_element=*/false);
  VectorDestructWithContents(&s->virtual_bases,
                             (VectorElementDestructor)CXXVirtualBaseInfoDelete,
                             /*free_element=*/false);
  VectorDestructWithContents(&s->members,
                             (VectorElementDestructor)StructMemberDelete, /*free_element=*/false);
  VectorDestruct(&s->virtual_members);
  VectorDestructWithContents(&s->vtable_symbols,
                             (VectorElementDestructor)CXXVTableInfoDelete,
                             /*free_element=*/false);
  VectorDestructWithContents(&s->vbtable_symbols,
                             (VectorElementDestructor)CXXVBTableInfoDelete,
                             /*free_element=*/false);
  VectorDestructWithContents(&s->template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  ConstraintExprDelete(s->associated_constraint);
  s->associated_constraint = NULL;
  VectorDestructWithContents(
      &s->partial_specializations,
      (VectorElementDestructor)ClassTemplatePartialSpecializationDelete,
      /*free_element=*/false);
  VectorDestructWithContents(&s->deduction_guides,
                             (VectorElementDestructor)SymbolDestruct,
                             /*free_element=*/true);
  VectorDestruct(&s->type_records);
  VectorDestructWithContents(&s->static_asserts,
                             (VectorElementDestructor)ASTNodeDelete,
                             /*free_element=*/false);
  MapDestruct(&s->symbol_table);
  MapDestruct(&s->symbol_name_table);
}

void StructDelete(Struct* s) {
  StructTeardownMembers(s);
  TemplateArgumentVectorDelete(s->local_class_arguments);
  free(s);
}

void StructAddFriendClass(Struct* str, Struct* friend_class) {
  if (str == NULL || friend_class == NULL) {
    return;
  }
  for (size_t i = 0; i < str->friend_classes.length; i++) {
    if (str->friend_classes.value.p[i] == friend_class) {
      return;
    }
  }
  VectorAppend(&str->friend_classes, friend_class);
}

CXXFriendTypeDeclaration* NewCXXFriendTypeDeclaration(
    TypeRecord* type, bool is_pack_expansion, SourceLocation location) {
  CXXFriendTypeDeclaration* declaration =
      malloc(sizeof(CXXFriendTypeDeclaration));
  declaration->type = TypeRecordCopy(type);
  declaration->is_pack_expansion = is_pack_expansion;
  declaration->location = location;
  return declaration;
}

void CXXFriendTypeDeclarationDelete(
    CXXFriendTypeDeclaration* declaration) {
  if (declaration == NULL) {
    return;
  }
  TypeRecordDelete(declaration->type);
  free(declaration);
}

void StructAddFriendFunction(Struct* str, Symbol* friend_function) {
  if (str == NULL || friend_function == NULL) {
    return;
  }
  for (size_t i = 0; i < str->friend_functions.length; i++) {
    if (str->friend_functions.value.p[i] == friend_function) {
      return;
    }
  }
  VectorAppend(&str->friend_functions, friend_function);
}

Symbol* NewEnumConstant(const char* name, int64_t value) {
  TypeRecord* int_type = NewTypeRecordWithSize(kTypeInt, kQualConst);
  Symbol* c = NewSymbol(name, int_type, STO(implicit));
  c->value.ivalue = value;
  c->flags.value_set = true;
  c->flags.is_constexpr =
      !CompilerIsCXX() && CompilerCAtLeast(kLanguageStandardC23);
  return c;
}

Symbol* NewScopedEnumConstant(const char* name, int64_t value,
                              TypeRecord* enum_type) {
  TypeRecord* type = TypeRecordCopy(enum_type);
  SetQualifiers(type, GetQualifiers(type) | kQualConst);
  Symbol* c = NewSymbol(name, type, STO(implicit));
  c->value.ivalue = value;
  c->flags.value_set = true;
  c->flags.is_constexpr =
      !CompilerIsCXX() && CompilerCAtLeast(kLanguageStandardC23);
  return c;
}

Enum* NewEnum() {
  Enum* e = malloc(sizeof(Enum));
  e->refs = 1;
  e->tag_symbol = NULL;
  VectorInit(&e->constants);
  e->next_value = 0;
  e->is_scoped = false;
  e->has_fixed_underlying = false;
  e->fixed_underlying_type = 0;
  e->fixed_underlying_size = 0;
  e->fixed_underlying_bit_width = 0;
  // Track every enum so it can be freed in bulk by StructRegistryRelease; this
  // keeps composite-info teardown uniform and cycle/UAF-safe.
  if (!enum_registry_initialized) {
    VectorInit(&enum_registry);
    enum_registry_initialized = true;
  }
  VectorAppend(&enum_registry, e);
  return e;
}

void EnumDelete(Enum* e) {
  VectorDestruct(&e->constants);
  free(e);
}

Symbol* EnumFindConstant(Enum* e, String* name) {
  if (e == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < e->constants.length; i++) {
    Symbol* constant = e->constants.value.p[i];
    if (StringEqualString(&constant->name, name)) {
      return constant;
    }
  }
  return NULL;
}

typedef struct {
  TypeRecord* key;
  uint64_t hash;
} TypeCensusMemo;
static TypeCensusMemo* type_census_memo = NULL;
static size_t type_census_memo_mask = 0;

static uint64_t TypeCensusMix(uint64_t h, uint64_t v) {
  h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
  return h;
}

static TypeCensusMemo* TypeCensusMemoSlot(TypeRecord* t) {
  size_t s = ((uintptr_t)t >> 4) & type_census_memo_mask;
  while (type_census_memo[s].key != NULL && type_census_memo[s].key != t) {
    s = (s + 1) & type_census_memo_mask;
  }
  return &type_census_memo[s];
}

static uint64_t TypeCensusHash(TypeRecord* t, int depth);

static uint64_t TypeCensusArgsHash(Vector* args, int depth) {
  if (args == NULL) {
    return 0;
  }
  uint64_t h = TypeCensusMix(17, args->length);
  for (size_t i = 0; i < args->length; i++) {
    TemplateArgument* a = args->value.p[i];
    if (a == NULL) {
      h = TypeCensusMix(h, 1);
      continue;
    }
    h = TypeCensusMix(h, a->kind);
    h = TypeCensusMix(h, a->is_pack_expansion);
    h = TypeCensusMix(h, TypeCensusHash(a->type, depth + 1));
    h = TypeCensusMix(h, (uint64_t)a->int_value);
    h = TypeCensusMix(h, (uint64_t)a->template_parameter_index);
    h = TypeCensusMix(h, TypeCensusArgsHash(a->pack_arguments, depth + 1));
    h = TypeCensusMix(h, (uintptr_t)a->dependent_expr);
    h = TypeCensusMix(h, a->value_kind);
    h = TypeCensusMix(h, (uintptr_t)TemplateArgumentExtraOf(a)->value_symbol);
    h = TypeCensusMix(h, (uint64_t)TemplateArgumentExtraOf(a)->value_offset);
    h = TypeCensusMix(h,
                      (uintptr_t)TemplateArgumentExtraOf(a)->member_function);
    h = TypeCensusMix(h,
                      (uintptr_t)TemplateArgumentExtraOf(a)->template_symbol);
    h = TypeCensusMix(h,
                      (uintptr_t)TemplateArgumentExtraOf(a)->reflection_value);
    h = TypeCensusMix(
        h, (uintptr_t)TemplateArgumentExtraOf(a)->object_initializer);
  }
  return h;
}

static uint64_t TypeCensusHash(TypeRecord* t, int depth) {
  if (t == NULL) {
    return 0;
  }
  if (depth > 64) {
    return (uintptr_t)t;
  }
  TypeCensusMemo* slot = TypeCensusMemoSlot(t);
  if (slot->key == t) {
    return slot->hash;
  }
  slot->key = t;
  slot->hash = (uintptr_t)t;  // Breaks cycles.
  uint64_t h = 1469598103934665603ull;
  h = TypeCensusMix(h, t->type);
  h = TypeCensusMix(h, GetQualifiers(t));
  h = TypeCensusMix(h, t->declarator);
  h = TypeCensusMix(h, (uint64_t)t->bit_width);
  h = TypeCensusMix(h, (uint64_t)t->template_parameter_index);
  if (TypeDependentOf(t)->dependent_member_name != NULL) {
    for (const char* c = TypeDependentOf(t)->dependent_member_name->value;
         c && *c; c++) {
      h = TypeCensusMix(h, (unsigned char)*c);
    }
  }
  h = TypeCensusMix(h, (uintptr_t)t->template_origin);
  h = TypeCensusMix(h, t->is_pack_index);
  h = TypeCensusMix(h, (uintptr_t)TypeDependentOf(t)->dependent_decltype_expr);
  h = TypeCensusMix(h, (uintptr_t)TypeDependentOf(t)->pack_index_expr);
  h = TypeCensusMix(h, (uintptr_t)TypeDependentOf(t)->dependent_splice_expr);
  h = TypeCensusMix(h, TypeCensusArgsHash(t->template_arguments, depth));
  if (TypeDependentOf(t)->dependent_member_template_arguments != NULL) {
    for (size_t i = 0;
         i < TypeDependentOf(t)->dependent_member_template_arguments->length;
         i++) {
      h = TypeCensusMix(
          h, TypeCensusArgsHash(
                 TypeDependentOf(t)
                     ->dependent_member_template_arguments->value.p[i],
                 depth));
    }
  }
  h = TypeCensusMix(h, TypeCensusHash(t->next, depth + 1));
  if (t->declarator == kDeclArray || t->declarator == kDeclVector) {
    h = TypeCensusMix(h, t->info.array.is_vla
                             ? (uintptr_t)t->info.array.size.vla.size
                             : (uint64_t)t->info.array.size.fixed);
    h = TypeCensusMix(h, t->info.array.is_vla | t->info.array.is_flexible << 1 |
                             t->info.array.is_dependent_bound << 2);
    h = TypeCensusMix(h, (uint64_t)t->info.array.template_parameter_index);
  } else if (TypeIsFunction(t)) {
    // Function records carry their declaration; only identical owners merge.
    h = TypeCensusMix(h, (uintptr_t)t->info.function.symbol);
    h = TypeCensusMix(h, t->info.function.prototype.length);
    for (size_t i = 0; i < t->info.function.prototype.length; i++) {
      Symbol* formal = t->info.function.prototype.value.p[i];
      h = TypeCensusMix(
          h, formal != NULL ? TypeCensusHash(formal->type, depth + 1) : 0);
    }
  } else if (TypeIsStructOrUnion(t) || t->declarator == kDeclMemberPointer) {
    h = TypeCensusMix(h, (uintptr_t)t->info.struct_info);
  } else if (TypeIsEnum(t)) {
    h = TypeCensusMix(h, (uintptr_t)t->info.enum_info);
  }
  slot = TypeCensusMemoSlot(t);
  slot->hash = h;
  return h;
}

typedef struct {
  uint64_t hash;
  size_t bytes;
  void* caller;
} TypeCensusItem;

static int CompareTypeCensusItems(const void* a, const void* b) {
  uint64_t x = ((const TypeCensusItem*)a)->hash;
  uint64_t y = ((const TypeCensusItem*)b)->hash;
  return x < y ? -1 : x > y ? 1 : 0;
}

static int CompareTypeRecordPointers(const void* a, const void* b) {
  uintptr_t x = (uintptr_t)*(TypeRecord* const*)a;
  uintptr_t y = (uintptr_t)*(TypeRecord* const*)b;
  return x < y ? -1 : x > y ? 1 : 0;
}

#define TYPE_CENSUS_CALLER_SLOTS 4096
static TypeCopyCaller type_census_dup_callers[TYPE_CENSUS_CALLER_SLOTS];

static void TypeCensusNoteDuplicate(void* caller) {
  size_t slot = ((uintptr_t)caller >> 2) & (TYPE_CENSUS_CALLER_SLOTS - 1);
  for (size_t probe = 0; probe < TYPE_CENSUS_CALLER_SLOTS; probe++) {
    TypeCopyCaller* entry = &type_census_dup_callers[slot];
    if (entry->count == 0 || entry->caller == caller) {
      entry->caller = caller;
      entry->count++;
      return;
    }
    slot = (slot + 1) & (TYPE_CENSUS_CALLER_SLOTS - 1);
  }
}

static void TypeCensusReportDuplicates(FILE* out, const char* name) {
  qsort(type_census_dup_callers, TYPE_CENSUS_CALLER_SLOTS,
        sizeof(TypeCopyCaller), CompareTypeCopyCallers);
  for (size_t i = 0; i < 25 && type_census_dup_callers[i].count > 0; i++) {
    fprintf(out, "DAVECC_PROFILE census-dup %s %p %zu\n", name,
            type_census_dup_callers[i].caller,
            type_census_dup_callers[i].count);
  }
  memset(type_census_dup_callers, 0, sizeof(type_census_dup_callers));
}

typedef struct {
  const char* name;
  TypeCensusItem* items;
  size_t count;
} TypeCensusClass;

static void TypeCensusClassReport(FILE* out, TypeCensusClass* c) {
  qsort(c->items, c->count, sizeof(TypeCensusItem), CompareTypeCensusItems);
  size_t distinct = 0;
  size_t total_bytes = 0;
  size_t dup_bytes = 0;
  for (size_t i = 0; i < c->count; i++) {
    total_bytes += c->items[i].bytes;
    if (i == 0 || c->items[i].hash != c->items[i - 1].hash) {
      distinct++;
    } else {
      dup_bytes += c->items[i].bytes;
      TypeCensusNoteDuplicate(c->items[i].caller);
    }
  }
  fprintf(out,
          "DAVECC_PROFILE census %s records=%zu distinct=%zu bytes=%zuMB "
          "duplicate_bytes=%zuMB\n",
          c->name, c->count, distinct, total_bytes >> 20, dup_bytes >> 20);
  TypeCensusReportDuplicates(out, c->name);
}

void TypeRecordProfileCensus(FILE* out) {
  if (!type_census_enabled) {
    return;
  }
  type_census_enabled = false;
  size_t n = type_census_alloc_count;
  qsort(type_census_releases, type_census_release_count, sizeof(TypeRecord*),
        CompareTypeRecordPointers);
  size_t memo_slots = 1;
  while (memo_slots < 2 * n + 16) {
    memo_slots <<= 1;
  }
  type_census_memo = calloc(memo_slots, sizeof(TypeCensusMemo));
  type_census_memo_mask = memo_slots - 1;
  TypeCensusClass classes[8] = {
      {"concrete", NULL, 0},   {"function", NULL, 0},
      {"other", NULL, 0},      {"dep-param", NULL, 0},
      {"dep-member", NULL, 0}, {"dep-template-id", NULL, 0},
      {"dep-spine", NULL, 0},  {"dep-pattern-class", NULL, 0},
  };
  for (int i = 0; i < 8; i++) {
    classes[i].items = malloc((n + 1) * sizeof(TypeCensusItem));
  }
  size_t released = 0;
  size_t released_bytes = 0;
  for (size_t i = 0; i < n; i++) {
    TypeRecord* t = type_census_allocs[i].record;
    size_t bytes = type_census_allocs[i].bytes;
    if (bsearch(&t, type_census_releases, type_census_release_count,
                sizeof(TypeRecord*), CompareTypeRecordPointers) != NULL) {
      released++;
      released_bytes += bytes;
      continue;
    }
    uint64_t hash = TypeCensusHash(t, 0);
    int which = TypeIsFunction(t)                    ? 1
                : TypeRecordCanShareConcreteClass(t) ? 0
                                                     : 2;
    if (which == 2) {
      if (TypeDependentOf(t)->dependent_member_name != NULL) {
        which = 4;
      } else if (t->next != NULL) {
        which = 6;
      } else if (t->template_parameter_index >= 0 &&
                 t->template_arguments == NULL) {
        which = 3;
      } else if (t->template_arguments != NULL) {
        which = 5;
      } else if (TypeIsStructOrUnion(t) &&
                 StructIsTemplatePattern(t->info.struct_info)) {
        which = 7;
      }
    }
    TypeCensusClass* c = &classes[which];
    c->items[c->count].hash = hash;
    c->items[c->count].bytes = bytes;
    c->items[c->count].caller = type_census_allocs[i].caller;
    c->count++;
  }
  fprintf(out,
          "DAVECC_PROFILE census allocated=%zu released=%zu "
          "released_bytes=%zuMB full_size=%zu short_size=%zu\n",
          n, released, released_bytes >> 20, (size_t)TYPE_RECORD_FULL_SIZE,
          (size_t)TYPE_RECORD_NON_FUNCTION_SIZE);
  for (int i = 0; i < 8; i++) {
    TypeCensusClassReport(out, &classes[i]);
    free(classes[i].items);
  }
  free(type_census_memo);
  type_census_memo = NULL;
  free(type_census_allocs);
  type_census_allocs = NULL;
  free(type_census_releases);
  type_census_releases = NULL;
}
