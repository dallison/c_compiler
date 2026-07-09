//
//  constexpr_pcode.c
//  c_compiler
//

#include "constexpr_pcode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "codegen.h"
#include "compiler.h"
#include "elf.h"
#include "p_code_assembler.h"
#include "p_code_emitter.h"
#include "p_code_reg_alloc.h"
#include "p_code_target.h"
#include "symbol.h"
#include "p_code_vm.h"
#include "type.h"

typedef struct {
  const char* reason;
  bool ok;
} ValidationState;

typedef struct {
  String text;
  bool failed;
} StringFile;

typedef struct {
  unsigned char* text;
  size_t text_size;
  uint64_t text_base;
  uint64_t entry;
  bool owns_text;
} ConstexprPCodeImage;

typedef struct {
  TypeRecord* func;
  unsigned char* text;
  size_t text_size;
  uint64_t entry_offset;
} ConstexprPCodeImageCacheEntry;

// Per-function memoization of the lowered pcode *assembly* (not yet assembled
// into a placed image).  Lowering a function body to pcode (GenerateFunction:
// IR + SSA + optimization + pcode codegen) is the expensive step and is
// context-independent, so it is cached once per function and reused whenever the
// function appears in another root's closure or is called directly.
typedef struct {
  TypeRecord* func;
  String assembly;    // lowered pcode assembly text for this function alone
  Vector referenced;  // TypeRecord* callees referenced by the function body
} ConstexprPCodeAssemblyCacheEntry;

typedef struct ConstexprPCodeFreeBlock {
  size_t length;
  struct ConstexprPCodeFreeBlock* next;
} ConstexprPCodeFreeBlock;

typedef struct ConstexprObject ConstexprObject;

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
  Vector heap_blocks;
  unsigned char* heap;
  size_t heap_size;
  ConstexprPCodeFreeBlock* free_list;
} ConstexprPCodeRuntime;

static PCodeVMStatus ConstexprEscape(PCodeVM* vm, int32_t code, void* data);
static void ConstexprPCodeRuntimeInit(ConstexprPCodeRuntime* runtime);
static void ConstexprPCodeRuntimeDestruct(ConstexprPCodeRuntime* runtime);
static bool ConstexprPCodeRuntimeHasLiveHeap(ConstexprPCodeRuntime* runtime);
static Symbol* PCodeConstexprFunctionDefinition(Symbol* symbol);
static ConstexprValue* PCodeConstexprObjectSlot(ConstexprObject* object,
                                                size_t index);
static ConstexprValue* PCodeConstexprSlotForOffset(TypeRecord* type,
                                                   ConstexprObject* object,
                                                   size_t offset);
static void DeletePCodeConstexprObject(ConstexprObject* object);
static ConstexprObject* NewPCodeConstexprObject(TypeRecord* type);
static bool ConstexprPCodeEvaluateCallObjectResult(ConstEvalContext* ctx,
                                                   ASTNode* node,
                                                   ConstexprObject** result);
static bool ConstexprPCodeEvaluateConstructorObject(ConstEvalContext* ctx,
                                                    TypeRecord* object_type,
                                                    ASTNode* node,
                                                    ConstexprObject** result);

static Vector pcode_image_cache;
static bool pcode_image_cache_initialized = false;

static Vector pcode_assembly_cache;
static bool pcode_assembly_cache_initialized = false;

#define CONSTEXPR_PCODE_HEAP_SIZE (1024 * 1024)

enum {
  kConstexprPCodeEscapeMalloc = 100,
  kConstexprPCodeEscapeFree = 101,
  kConstexprPCodeEscapeRealloc = 102,
  kConstexprPCodeEscapePlacementNew = 103,
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

struct ConstexprValue {
  bool is_object;
  bool is_address;
  bool is_floating;
  int64_t ivalue;
  double fvalue;
  ConstexprObject* object;
  void* address_binding;
  ConstexprValue* address_slot;
};

struct ConstexprObject {
  TypeRecord* type;
  Vector slots;
  StructMember* active_union_member;
};

static bool PCodeConstexprObjectIsUnion(ConstexprObject* object) {
  return object != NULL && TypeIsStructOrUnion(object->type) &&
         object->type->info.struct_info != NULL &&
         object->type->info.struct_info->is_union;
}

static void ConstexprPCodeImageInit(ConstexprPCodeImage* image) {
  image->text = NULL;
  image->text_size = 0;
  image->text_base = 0;
  image->entry = 0;
  image->owns_text = true;
}

static void ConstexprPCodeImageDestruct(ConstexprPCodeImage* image) {
  if (image->owns_text) {
    free(image->text);
  }
  image->text = NULL;
  image->text_size = 0;
  image->text_base = 0;
  image->entry = 0;
  image->owns_text = true;
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
  image->entry = image->text_base + entry->entry_offset;
  image->owns_text = false;
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
  entry->entry_offset = image->entry - image->text_base;
  image->owns_text = false;
  VectorAppend(&pcode_image_cache, entry);
}

static ConstexprPCodeAssemblyCacheEntry* ConstexprPCodeFindCachedAssembly(
    TypeRecord* func) {
  if (!pcode_assembly_cache_initialized) {
    return NULL;
  }
  for (size_t i = 0; i < pcode_assembly_cache.length; i++) {
    ConstexprPCodeAssemblyCacheEntry* entry = pcode_assembly_cache.value.p[i];
    if (entry != NULL && entry->func == func) {
      return entry;
    }
  }
  return NULL;
}

static void ConstexprPCodeCacheAssembly(TypeRecord* func, String* assembly,
                                        Vector* referenced) {
  if (!pcode_assembly_cache_initialized) {
    VectorInit(&pcode_assembly_cache);
    pcode_assembly_cache_initialized = true;
  }
  ConstexprPCodeAssemblyCacheEntry* entry =
      malloc(sizeof(ConstexprPCodeAssemblyCacheEntry));
  if (entry == NULL) {
    return;
  }
  entry->func = func;
  StringInit(&entry->assembly, "");
  StringAppendString(&entry->assembly, assembly);
  VectorInit(&entry->referenced);
  for (size_t i = 0; i < referenced->length; i++) {
    VectorAppend(&entry->referenced, referenced->value.p[i]);
  }
  VectorAppend(&pcode_assembly_cache, entry);
}

void ConstexprPCodeClearImageCache(void) {
  if (pcode_image_cache_initialized) {
    for (size_t i = 0; i < pcode_image_cache.length; i++) {
      ConstexprPCodeImageCacheEntry* entry = pcode_image_cache.value.p[i];
      if (entry != NULL) {
        free(entry->text);
        free(entry);
      }
    }
    VectorDestruct(&pcode_image_cache);
    pcode_image_cache_initialized = false;
  }
  if (pcode_assembly_cache_initialized) {
    for (size_t i = 0; i < pcode_assembly_cache.length; i++) {
      ConstexprPCodeAssemblyCacheEntry* entry =
          pcode_assembly_cache.value.p[i];
      if (entry != NULL) {
        StringDestruct(&entry->assembly);
        VectorDestruct(&entry->referenced);
        free(entry);
      }
    }
    VectorDestruct(&pcode_assembly_cache);
    pcode_assembly_cache_initialized = false;
  }
}

static int StringFileWrite(void* cookie, const char* data, int length) {
  StringFile* file = cookie;
  if (file == NULL || length < 0) {
    return -1;
  }
  StringAppendSegment(&file->text, data, (size_t)length);
  return length;
}

static int StringFileClose(void* cookie) {
  (void)cookie;
  return 0;
}

static FILE* OpenStringFile(StringFile* file) {
  StringInit(&file->text, "");
  file->failed = false;
#if defined(__APPLE__) || defined(__FreeBSD__)
  return funopen(file, NULL, StringFileWrite, NULL, StringFileClose);
#elif defined(__GLIBC__)
  char* buffer = NULL;
  size_t size = 0;
  FILE* fp = open_memstream(&buffer, &size);
  if (fp == NULL) {
    file->failed = true;
    return NULL;
  }
  /*
   * Non-Darwin builds currently use this facade only in tests that run on
   * Darwin, but keep the code compiling by copying the stream contents on
   * close in the caller when open_memstream is available.
   */
  (void)buffer;
  (void)size;
  return fp;
#else
  file->failed = true;
  return NULL;
#endif
}

static void CloseStringFile(FILE* fp, StringFile* file) {
  if (fp == NULL) {
    return;
  }
  fflush(fp);
#if defined(__GLIBC__) && !defined(__APPLE__)
  /*
   * The open_memstream fallback is only a compile-time portability shim here;
   * the in-tree supported development target is macOS where funopen writes
   * directly to file->text.
   */
  (void)file;
#endif
  fclose(fp);
}

static bool VectorContainsPointer(Vector* vector, void* value) {
  for (size_t i = 0; i < vector->length; i++) {
    if (vector->value.p[i] == value) {
      return true;
    }
  }
  return false;
}

static void CollectReferencedConstexprSymbol(Symbol* symbol, Vector* referenced) {
  Symbol* callee = PCodeConstexprFunctionDefinition(symbol);
  if (callee == NULL || callee->type == NULL ||
      !callee->type->info.function.is_constexpr ||
      VectorContainsPointer(referenced, callee->type)) {
    return;
  }
  VectorAppend(referenced, callee->type);
}

static void CollectReferencedConstexprFunctions(PCodeGenerator* pcode,
                                                Vector* referenced) {
  for (TargetInstruction* inst = TargetFirstInstruction(&pcode->base);
       inst != NULL; inst = TargetNext(inst)) {
    if (inst->opcode == (TargetOpcode)P_OP(symbol)) {
      CollectReferencedConstexprSymbol(((TargetSymbol*)inst)->symbol,
                                       referenced);
    }
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      TargetInstruction* operand = inst->operand[i];
      if (operand != NULL && operand->opcode == (TargetOpcode)P_OP(symbol)) {
        CollectReferencedConstexprSymbol(((TargetSymbol*)operand)->symbol,
                                         referenced);
      }
    }
  }
}

static bool CompileFunctionToPCodeAssembly(TypeRecord* func, String* assembly,
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

  StringFile output;
  FILE* fp = OpenStringFile(&output);
  if (fp == NULL) {
    compiler->current_function = saved_current_function;
    compiler->target = saved_target;
    pcode_target->cleanup(pcode);
    pcode_target->cleanup = NULL;
    free(pcode_target);
    GeneratorDestruct(&gen);
    *reason = "could not open in-memory pcode assembly stream";
    return false;
  }

  fprintf(fp, ".file 1 \"<constexpr-pcode>\"\n");
  fprintf(fp, ".text\n");
  PCodeEmitter emitter;
  PCodeEmitterInit(&emitter, pcode);
  emitter.emit_locations = false;
  PCodePrintFunction(&emitter, fp);
  PCodeEmitterDestruct(&emitter);
  CloseStringFile(fp, &output);
  StringSetString(assembly, &output.text);
  StringDestruct(&output.text);

  compiler->current_function = saved_current_function;
  compiler->target = saved_target;
  pcode_target->cleanup(pcode);
  pcode_target->cleanup = NULL;
  free(pcode_target);
  GeneratorDestruct(&gen);
  return true;
}

static AssemblerSection* FindAssemblerSection(Assembler* assembler,
                                              const char* name) {
  for (size_t i = 0; i < assembler->sections.length; i++) {
    AssemblerSection* section = assembler->sections.value.p[i];
    if (section != NULL && section->name != NULL &&
        StringEqual(section->name, name)) {
      return section;
    }
  }
  return NULL;
}

static bool IsConstexprPCodeRuntimeSymbol(AssemblerSymbol* symbol,
                                          const char* name) {
  return symbol != NULL && symbol->name.value != NULL &&
         strcmp(symbol->name.value, name) == 0;
}

static bool IsConstexprPCodeRuntimeSymbolPrefix(AssemblerSymbol* symbol,
                                                const char* prefix) {
  return symbol != NULL && symbol->name.value != NULL &&
         strncmp(symbol->name.value, prefix, strlen(prefix)) == 0;
}

static bool ConstexprPCodeRuntimeSymbolAddress(AssemblerSymbol* symbol,
                                               uint64_t* address) {
  if (IsConstexprPCodeRuntimeSymbol(symbol, "malloc") ||
      IsConstexprPCodeRuntimeSymbolPrefix(symbol, "_Z6malloc")) {
    *address = (uint64_t)(uintptr_t)constexpr_pcode_malloc_stub;
    return true;
  }
  if (IsConstexprPCodeRuntimeSymbolPrefix(symbol, "_ZnwmPv") ||
      IsConstexprPCodeRuntimeSymbolPrefix(symbol, "_ZnwyPv")) {
    *address = (uint64_t)(uintptr_t)constexpr_pcode_placement_new_stub;
    return true;
  }
  if (IsConstexprPCodeRuntimeSymbol(symbol, "operator new") ||
      IsConstexprPCodeRuntimeSymbol(symbol, "operator new[]") ||
      IsConstexprPCodeRuntimeSymbolPrefix(symbol, "_Znwm") ||
      IsConstexprPCodeRuntimeSymbolPrefix(symbol, "_Znwy") ||
      IsConstexprPCodeRuntimeSymbolPrefix(symbol, "_Znam") ||
      IsConstexprPCodeRuntimeSymbolPrefix(symbol, "_Znay")) {
    *address = (uint64_t)(uintptr_t)constexpr_pcode_malloc_stub;
    return true;
  }
  if (IsConstexprPCodeRuntimeSymbol(symbol, "free") ||
      IsConstexprPCodeRuntimeSymbolPrefix(symbol, "_Z4free")) {
    *address = (uint64_t)(uintptr_t)constexpr_pcode_free_stub;
    return true;
  }
  if (IsConstexprPCodeRuntimeSymbol(symbol, "operator delete") ||
      IsConstexprPCodeRuntimeSymbol(symbol, "operator delete[]") ||
      IsConstexprPCodeRuntimeSymbolPrefix(symbol, "_ZdlPv") ||
      IsConstexprPCodeRuntimeSymbolPrefix(symbol, "_ZdaPv")) {
    *address = (uint64_t)(uintptr_t)constexpr_pcode_free_stub;
    return true;
  }
  if (IsConstexprPCodeRuntimeSymbol(symbol, "realloc") ||
      IsConstexprPCodeRuntimeSymbolPrefix(symbol, "_Z7realloc")) {
    *address = (uint64_t)(uintptr_t)constexpr_pcode_realloc_stub;
    return true;
  }
  return false;
}

static bool IsConstexprPCodeRuntimeCallSymbol(Symbol* symbol) {
  if (symbol == NULL || symbol->name.value == NULL) {
    return false;
  }
  return strcmp(symbol->name.value, "malloc") == 0 ||
         strcmp(symbol->name.value, "free") == 0 ||
         strcmp(symbol->name.value, "realloc") == 0 ||
         strcmp(symbol->name.value, "operator new") == 0 ||
         strcmp(symbol->name.value, "operator new[]") == 0 ||
         strcmp(symbol->name.value, "operator delete") == 0 ||
         strcmp(symbol->name.value, "operator delete[]") == 0;
}

static uint64_t SymbolRuntimeAddress(Assembler* assembler,
                                     ConstexprPCodeImage* image,
                                     AssemblerSymbol* symbol,
                                     bool* ok) {
  uint64_t runtime_address = 0;
  if (ConstexprPCodeRuntimeSymbolAddress(symbol, &runtime_address)) {
    return runtime_address;
  }
  if (symbol == NULL || !symbol->defined || symbol->section < 0 ||
      (size_t)symbol->section >= assembler->sections.length) {
    *ok = false;
    return 0;
  }
  AssemblerSection* section = assembler->sections.value.p[symbol->section];
  if (section == FindAssemblerSection(assembler, ".text")) {
    return image->text_base + (uint64_t)symbol->value;
  }
  *ok = false;
  return 0;
}

static bool ApplyConstexprRelocation(Assembler* assembler,
                                     ConstexprPCodeImage* image,
                                     AssemblerRelocation* reloc,
                                     const char** reason) {
  if (reloc == NULL || reloc->section < 0 ||
      (size_t)reloc->section >= assembler->sections.length) {
    *reason = "bad pcode relocation section";
    return false;
  }
  AssemblerSection* section = assembler->sections.value.p[reloc->section];
  AssemblerSection* text = FindAssemblerSection(assembler, ".text");
  if (section != text || reloc->offset < 0 ||
      (size_t)reloc->offset + sizeof(uint64_t) > image->text_size) {
    *reason = "constexpr pcode only supports text relocations";
    return false;
  }

  bool ok = true;
  uint64_t S = SymbolRuntimeAddress(assembler, image, reloc->symbol, &ok);
  if (!ok) {
    *reason = "unresolved constexpr pcode symbol";
    return false;
  }
  uint64_t P = image->text_base + (uint64_t)reloc->offset + 12;
  unsigned char* target = image->text + reloc->offset;
  int opcode = target[3] & 0x3f;
  switch (reloc->type) {
    case R_PCODE_ABS:
      if (opcode == PCODE_OP(movxc)) {
        *((uint64_t*)(target + 4)) = S + reloc->addend;
      } else if (opcode == PCODE_OP(adr)) {
        *((uint64_t*)(target + 4)) = S + reloc->addend - P;
      } else {
        *reason = "unsupported pcode absolute relocation opcode";
        return false;
      }
      return true;
    case R_PCODE_CALL:
    case R_PCODE_JMP:
    case R_PCODE_PCREL:
      *((uint64_t*)(target + 4)) = S + reloc->addend - P;
      return true;
    case R_PCODE_DATA64:
      *((uint64_t*)target) = S + reloc->addend;
      return true;
    case R_PCODE_DATA32:
      *((uint32_t*)target) = (uint32_t)(S + reloc->addend);
      return true;
    default:
      *reason = "unsupported pcode relocation in constexpr image";
      return false;
  }
}

static bool AssemblePCodeImage(String* assembly, const char* entry_name,
                               ConstexprPCodeImage* image,
                               const char** reason) {
  String outfile;
  StringInit(&outfile, "/dev/null");
  PCodeAssembler assembler;
  if (!PCodeAssemblerInitFromString(&assembler, "<constexpr-pcode>", assembly,
                                    &outfile)) {
    StringDestruct(&outfile);
    *reason = "could not initialize pcode assembler";
    return false;
  }
  AssemblerRun(&assembler.base, AssemblePCodeInstruction);
  StringDestruct(&outfile);
  if (assembler.base.num_errors != 0) {
    PCodeAssemblerDestruct(&assembler);
    *reason = "pcode assembly failed";
    return false;
  }

  AssemblerSection* text = FindAssemblerSection(&assembler.base, ".text");
  if (text == NULL ||
      text->contents.data_location != kSectionContentsBuffered) {
    PCodeAssemblerDestruct(&assembler);
    *reason = "pcode image has no text section";
    return false;
  }

  image->text_size = text->contents.data.buffered.length;
  image->text = malloc(image->text_size == 0 ? 1 : image->text_size);
  if (image->text == NULL) {
    PCodeAssemblerDestruct(&assembler);
    *reason = "could not allocate pcode image";
    return false;
  }
  memcpy(image->text, text->contents.data.buffered.value, image->text_size);
  image->text_base = (uint64_t)(uintptr_t)image->text;

  for (size_t i = 0; i < assembler.base.relocations.length; i++) {
    AssemblerRelocation* reloc = assembler.base.relocations.value.p[i];
    if (!ApplyConstexprRelocation(&assembler.base, image, reloc, reason)) {
      PCodeAssemblerDestruct(&assembler);
      return false;
    }
  }

  AssemblerSymbol* entry = AssemblerFindSymbol(&assembler.base, entry_name);
  bool ok = true;
  image->entry = SymbolRuntimeAddress(&assembler.base, image, entry, &ok);
  PCodeAssemblerDestruct(&assembler);
  if (!ok) {
    *reason = "could not resolve constexpr pcode entry";
    return false;
  }
  return true;
}

// Wraps CompileFunctionToPCodeAssembly with per-function memoization.  The
// lowering of a single function body is context-independent, so it is performed
// at most once and reused across every root closure that references the
// function (and across direct calls).  On a hit the cached assembly text and
// referenced-callee list are copied into the caller-owned outputs so the closure
// walk in CompileConstexprFunctionImage proceeds exactly as with a fresh
// lowering.
static bool CompileFunctionToPCodeAssemblyCached(TypeRecord* func,
                                                 String* assembly,
                                                 Vector* referenced,
                                                 const char** reason) {
  ConstexprPCodeAssemblyCacheEntry* cached =
      ConstexprPCodeFindCachedAssembly(func);
  if (cached != NULL) {
    StringAppendString(assembly, &cached->assembly);
    for (size_t i = 0; i < cached->referenced.length; i++) {
      VectorAppend(referenced, cached->referenced.value.p[i]);
    }
    return true;
  }
  if (!CompileFunctionToPCodeAssembly(func, assembly, referenced, reason)) {
    return false;
  }
  ConstexprPCodeCacheAssembly(func, assembly, referenced);
  return true;
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

  String assembly;
  StringInit(&assembly, "");
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
    String part;
    StringInit(&part, "");
    Vector referenced;
    VectorInit(&referenced);
    ok = CompileFunctionToPCodeAssemblyCached(current, &part, &referenced,
                                              reason);
    if (ok) {
      StringAppendString(&assembly, &part);
      for (size_t j = 0; j < referenced.length; j++) {
        TypeRecord* callee = referenced.value.p[j];
        if (!VectorContainsPointer(&emitted, callee) &&
            !VectorContainsPointer(&pending, callee)) {
          VectorAppend(&pending, callee);
        }
      }
    }
    VectorDestruct(&referenced);
    StringDestruct(&part);
  }
  VectorDestruct(&pending);
  VectorDestruct(&emitted);
  if (!ok) {
    StringDestruct(&assembly);
    return false;
  }

  char namebuf[256];
  const char* entry_name =
      TargetSymbolName(func->info.function.symbol, namebuf, sizeof(namebuf));
  ok = AssemblePCodeImage(&assembly, entry_name, image, reason);
  StringDestruct(&assembly);
  if (ok) {
    ConstexprPCodeCacheImage(func, image);
  }
  return ok;
}

static size_t ConstexprPCodeArgumentSize(TypeRecord* type) {
  if (type == NULL) {
    return 0;
  }
  if (TypeIsFloat(type) || TypeIsInt(type) || TypeIsShort(type) ||
      TypeIsChar(type)) {
    return 4;
  }
  if (TypeIsDouble(type) || TypeIsLong(type) || TypeIsLongLong(type) ||
      TypeIsPointerOrArray(type) || TypeIsFunction(type)) {
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
    if (TypeIsFloat(type)) {
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

static bool LoadConstexprScalarBytes(TypeRecord* type, unsigned char* src,
                                     ConstexprValue* value) {
  if (type == NULL || src == NULL || value == NULL) {
    return false;
  }
  *value = (ConstexprValue){0};
  if (TypeIsFloatingPoint(type)) {
    if (TypeIsFloat(type)) {
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
                                     ConstexprObject** result);

static bool LoadConstexprValueBytes(TypeRecord* type, unsigned char* src,
                                    ConstexprValue* value) {
  if (TypeIsFixedArray(type) || TypeIsStructOrUnion(type)) {
    ConstexprObject* object = NULL;
    if (!LoadConstexprObjectBytes(type, src, &object)) {
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
    for (size_t i = 0; i < object->slots.length; i++) {
      ConstexprValue* slot = PCodeConstexprObjectSlot(object, i);
      if (slot == NULL ||
          !LoadConstexprValueBytes(type->next, src + i * elem_size, slot)) {
        DeletePCodeConstexprObject(object);
        return false;
      }
    }
    *result = object;
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
      size_t slot_index = str->is_union ? 0 : member->index;
      ConstexprValue* slot = PCodeConstexprObjectSlot(object, slot_index);
      if (slot == NULL ||
          !LoadConstexprValueBytes(member->symbol->type,
                                   src + member->byte_offset, slot)) {
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
    if (symbol != NULL && symbol->flags.value_set && symbol->value.other != NULL) {
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

static bool StoreConstexprPCodeObjectPointer(PCodeVM* vm,
                                             unsigned char** sp,
                                             TypeRecord* type, ASTNode* arg,
                                             Vector* allocations,
                                             Vector* address_regions,
                                             const char** reason) {
  TypeRecord* object_type = type != NULL ? type->next : NULL;
  ConstexprObject* object = ConstexprObjectArgument(arg);
  if (object_type == NULL || object == NULL) {
    return false;
  }
  size_t memory_size = ConstexprPCodeHostBufferSize(object_type->size);
  unsigned char* memory = calloc(1, memory_size);
  if (memory == NULL) {
    *reason = "could not allocate constexpr object argument";
    return false;
  }
  if (!StoreConstexprObjectBytes(object_type, object, memory)) {
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
  if (!PCodeSourceBuiltinStringValue(arg, &value)) {
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
  if (TypeIsPointerOrArray(type) &&
      StoreConstexprPCodeObjectPointer(vm, sp, type, arg, allocations,
                                       address_regions, reason)) {
    return true;
  }
  if (TypeIsPointer(type) &&
      StoreConstexprPCodeSourceStringArgument(vm, sp, arg, allocations,
                                             reason)) {
    return true;
  }
  *sp -= size;
  if (TypeIsStructOrUnion(type)) {
    ConstexprObject* object = ConstexprObjectArgument(arg);
    if (object == NULL || !StoreConstexprObjectBytes(type, object, *sp)) {
      *reason = "could not marshal aggregate constexpr argument";
      return false;
    }
    return true;
  }
  if (TypeIsFloatingPoint(type)) {
    double value;
    if (!EvaluateFloatingPointExpressionInContext(ctx, arg, &value)) {
      *reason = "could not evaluate floating constexpr argument";
      return false;
    }
    if (TypeIsFloat(type)) {
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
  size_t explicit_count = call->children != NULL ? call->children->length : 0;
  if (explicit_count + 1 != func->info.function.prototype.length) {
    *reason = "constexpr pcode constructor argument count mismatch";
    return false;
  }

  unsigned char* sp = (unsigned char*)vm->stack + vm->stack_size;
  for (size_t i = explicit_count; i > 0; i--) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    ASTNode* arg = call->children->value.p[i - 1];
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
                                              uint32_t* halt_instruction,
                                              const char** reason) {
  if (!PCodeVMEnableCheckedMemory(vm) ||
      !PCodeVMRegisterMemoryRegion(vm, image->text, image->text_size,
                                   false) ||
      !PCodeVMRegisterMemoryRegion(vm, halt_instruction,
                                   sizeof(*halt_instruction), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_malloc_stub,
          sizeof(constexpr_pcode_malloc_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_placement_new_stub,
          sizeof(constexpr_pcode_placement_new_stub), false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_free_stub, sizeof(constexpr_pcode_free_stub),
          false) ||
      !PCodeVMRegisterMemoryRegion(
          vm, (void*)constexpr_pcode_realloc_stub,
          sizeof(constexpr_pcode_realloc_stub), false)) {
    *reason = "could not enable checked constexpr pcode memory";
    return false;
  }
  return true;
}

static bool RunRealPCodeCall(ConstEvalContext* ctx, ASTNode* node,
                             TypeRecord* func, int64_t* int_result,
                             double* double_result,
                             ConstexprObject** object_result,
                             ConstexprValue* address_result,
                             const char** reason) {
  ConstexprPCodeImage image;
  ConstexprPCodeImageInit(&image);
  if (!CompileConstexprFunctionImage(func, &image, reason)) {
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
  PCodeVMSetEscapeHandler(&vm, ConstexprEscape, &runtime);
  uint32_t halt_instruction = 0;
  if (!EnableConstexprPCodeCheckedMemory(&vm, &image, &halt_instruction,
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
      *double_result = vm.dregs[PCODE_DOUBLE_RETURN_REG];
    }
    if (address_result != NULL) {
      uint64_t address = (uint64_t)vm.iregs[PCODE_INT_RETURN_REG];
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
    if (object_result != NULL &&
        !LoadConstexprObjectBytes(func->next, struct_return, object_result)) {
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
      *reason = "constexpr pcode evaluation leaked allocation";
      for (size_t i = 0; i < allocations.length; i++) {
        free(allocations.value.p[i]);
      }
      VectorDestruct(&allocations);
      ConstexprPCodeRuntimeDestruct(&runtime);
      PCodeVMDestruct(&vm);
      ConstexprPCodeImageDestruct(&image);
      return false;
    }
  } else {
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
                                    const char** reason) {
  ConstexprPCodeImage image;
  ConstexprPCodeImageInit(&image);
  if (!CompileConstexprFunctionImage(func, &image, reason)) {
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
  PCodeVMSetEscapeHandler(&vm, ConstexprEscape, &runtime);
  uint32_t halt_instruction = 0;
  if (!EnableConstexprPCodeCheckedMemory(&vm, &image, &halt_instruction,
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
            LoadConstexprObjectBytes(object_type, object_memory, object_result);
  if (status != kPCodeVMStatusHalted) {
    *reason = PCodeVMStatusName(status);
  } else if (!ok) {
    *reason = "could not decode constexpr pcode constructor object";
  } else if (ConstexprPCodeRuntimeHasLiveHeap(&runtime)) {
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
    case AST_OP(asm):
      ValidationReject(state, "asm is not allowed in constexpr evaluation");
      return;
    case AST_OP(goto):
    case AST_OP(label):
      ValidationReject(state, "goto and labels are not pcode-constexpr eligible");
      return;
    case AST_OP(try):
    case AST_OP(catch):
    case AST_OP(throw):
      ValidationReject(state, "exceptions are not allowed in constexpr evaluation");
      return;
    case AST_OP(builtin_va_start):
    case AST_OP(builtin_va_arg):
    case AST_OP(builtin_va_end):
    case AST_OP(builtin_va_copy):
      ValidationReject(state, "varargs builtins are not constexpr eligible");
      return;
    case AST_OP(builtin_atomic_load):
    case AST_OP(builtin_atomic_store):
    case AST_OP(builtin_atomic_fetch_add):
    case AST_OP(builtin_atomic_fetch_sub):
    case AST_OP(builtin_atomic_add_fetch):
    case AST_OP(builtin_atomic_sub_fetch):
    case AST_OP(builtin_atomic_compare_exchange_bool):
    case AST_OP(builtin_atomic_compare_exchange_val):
    case AST_OP(builtin_atomic_compare_exchange_n):
    case AST_OP(builtin_atomic_fence):
      ValidationReject(state, "atomic builtins are not constexpr eligible");
      return;
    case AST_OP(call): {
      if (PCodeConstexprCallSymbol(node) == NULL) {
        ValidationReject(state, "indirect calls are not pcode-constexpr eligible");
        return;
      }
      if (IsConstexprPCodeRuntimeCallSymbol(PCodeConstexprCallSymbol(node))) {
        return;
      }
      Symbol* callee =
          PCodeConstexprFunctionDefinition(PCodeConstexprCallSymbol(node));
      if (callee == NULL || callee->type == NULL ||
          !callee->type->info.function.is_constexpr) {
        ValidationReject(state, "call target is not constexpr");
        return;
      }
      if (callee->type->info.function.is_virtual ||
          callee->type->info.function.varargs) {
        ValidationReject(state, "virtual or varargs calls are not constexpr eligible");
        return;
      }
      return;
    }
    case AST_OP(subscript): {
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
  if (func->info.function.varargs) {
    *reason = "varargs constexpr functions are not pcode-eligible";
    return false;
  }
  if (func->info.function.is_virtual) {
    *reason = "virtual constexpr functions are not pcode-eligible";
    return false;
  }
  if (func->info.function.is_constructor || func->info.function.is_destructor) {
    *reason = "constructor/destructor constexpr calls use object evaluation";
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

static void ConstexprPCodeRuntimeInit(ConstexprPCodeRuntime* runtime) {
  VectorInit(&runtime->heap_blocks);
  runtime->heap = NULL;
  runtime->heap_size = 0;
  runtime->free_list = NULL;
}

static void ConstexprPCodeRuntimeDestruct(ConstexprPCodeRuntime* runtime) {
  for (size_t i = 0; i < runtime->heap_blocks.length; i++) {
    ConstexprPCodeHeapBlock* block = runtime->heap_blocks.value.p[i];
    if (block != NULL) {
      free(block);
    }
  }
  VectorDestruct(&runtime->heap_blocks);
  free(runtime->heap);
  runtime->heap = NULL;
  runtime->heap_size = 0;
  runtime->free_list = NULL;
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

static uint64_t ConstexprPCodeStackArgument(PCodeVM* vm, size_t index) {
  uint64_t value = 0;
  memcpy(&value, (void*)(uintptr_t)(vm->iregs[PCODE_SP_REG] + 8 +
                                    (int64_t)(index * sizeof(uint64_t))),
         sizeof(value));
  return value;
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
  return ConstexprPCodeAllocateHeapBlock(
      vm, runtime, (size_t)ConstexprPCodeStackArgument(vm, 0));
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
  size_t size = (size_t)ConstexprPCodeStackArgument(vm, 1);
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

static PCodeVMStatus ConstexprPCodeEscapePlacementNew(PCodeVM* vm) {
  vm->iregs[PCODE_INT_RETURN_REG] =
      (int64_t)(uintptr_t)ConstexprPCodeStackArgument(vm, 1);
  return kPCodeVMStatusRunning;
}

static PCodeVMStatus ConstexprEscape(PCodeVM* vm, int32_t code, void* data) {
  ConstexprPCodeRuntime* runtime = data;
  switch (code) {
    case 4:
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
      return ConstexprPCodeEscapePlacementNew(vm);
    default:
      return kPCodeVMStatusUndefinedEscape;
  }
}

bool ConstexprPCodeEvaluateCallAsInteger(ConstEvalContext* ctx, ASTNode* node,
                                         int64_t* result) {
  const char* reason = NULL;
  if (!ConstexprPCodeValidateCall(node, &reason)) {
    return false;
  }
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  return RunRealPCodeCall(ctx, node, callee->type, result, NULL, NULL, NULL,
                          &reason);
}

bool ConstexprPCodeEvaluateCallAsFloating(ConstEvalContext* ctx, ASTNode* node,
                                          double* result) {
  const char* reason = NULL;
  if (!ConstexprPCodeValidateCall(node, &reason)) {
    return false;
  }
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  return RunRealPCodeCall(ctx, node, callee->type, NULL, result, NULL, NULL,
                          &reason);
}

static bool ConstexprPCodeEvaluateCallObjectResult(ConstEvalContext* ctx,
                                                   ASTNode* node,
                                                   ConstexprObject** result) {
  const char* reason = NULL;
  if (!ConstexprPCodeValidateCall(node, &reason)) {
    return false;
  }
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  if (callee == NULL || callee->type == NULL ||
      !TypeIsStructOrUnion(callee->type->next)) {
    return false;
  }
  return RunRealPCodeCall(ctx, node, callee->type, NULL, NULL, result, NULL,
                          &reason);
}

bool ConstexprPCodeEvaluateCallAsAddress(ConstEvalContext* ctx, ASTNode* node,
                                         ConstexprValue* result) {
  const char* reason = NULL;
  if (!ConstexprPCodeValidateCall(node, &reason)) {
    return false;
  }
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  if (callee == NULL || callee->type == NULL || callee->type->next == NULL ||
      (!TypeIsPointer(callee->type->next) &&
       !TypeIsReference(callee->type->next))) {
    return false;
  }
  return RunRealPCodeCall(ctx, node, callee->type, NULL, NULL, NULL, result,
                          &reason);
}

static bool ConstexprPCodeEvaluateConstructorObject(ConstEvalContext* ctx,
                                                    TypeRecord* object_type,
                                                    ASTNode* node,
                                                    ConstexprObject** result) {
  const char* reason = NULL;
  Symbol* callee = PCodeConstexprFunctionDefinition(PCodeConstexprCallSymbol(node));
  if (callee == NULL || callee->type == NULL ||
      !callee->type->info.function.is_constexpr ||
      !callee->type->info.function.is_constructor ||
      callee->type->info.function.is_destructor ||
      callee->type->info.function.is_virtual ||
      callee->type->info.function.varargs) {
    return false;
  }
  ValidationState state = {.reason = "ok", .ok = true};
  ASTNodeVisit(callee->type->info.function.body, ValidateASTNode, 0, &state);
  if (!state.ok) {
    return false;
  }
  return RunRealPCodeConstructor(ctx, object_type, node, callee->type, result,
                                 &reason);
}

bool ConstexprPCodeEvaluateCallAsObject(ConstEvalContext* ctx, ASTNode* node) {
  ConstexprObject* object = NULL;
  bool ok = ConstexprPCodeEvaluateCallObjectResult(ctx, node, &object);
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
    return type->info.array.size.fixed;
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

static ConstexprObject* NewPCodeConstexprObject(TypeRecord* type) {
  ConstexprObject* object = malloc(sizeof(ConstexprObject));
  if (object == NULL) {
    return NULL;
  }
  object->type = type;
  object->active_union_member = NULL;
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

static bool BuildPCodeConstexprObject(TypeRecord* type, ASTNode* initializer,
                                      ConstexprObject** result);

static bool PCodeStoreInitializer(TypeRecord* type, ASTNode* initializer,
                                  ConstexprValue* slot) {
  if (type == NULL || initializer == NULL || slot == NULL) {
    return false;
  }
  if (TypeIsFixedArray(type) || TypeIsStructOrUnion(type)) {
    ConstexprObject* object = NULL;
    ASTNode* expr = ConstexprInitializerExpression(initializer);
    ConstEvalContext ctx;
    ConstEvalContextInit(&ctx);
    bool ok = expr != NULL && expr->op == AST_OP(call) &&
              (ConstexprPCodeEvaluateCallObjectResult(&ctx, expr, &object) ||
               ConstexprPCodeEvaluateConstructorObject(&ctx, type, expr,
                                                       &object));
    ConstEvalContextDestruct(&ctx);
    if (!ok && !BuildPCodeConstexprObject(type, initializer, &object)) {
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

static bool BuildPCodeArrayObject(TypeRecord* type, ASTNode* initializer,
                                  ConstexprObject* object) {
  if (initializer == NULL || initializer->op != AST_OP(braced_init)) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
  if (braced->initializers->length > object->slots.length) {
    return false;
  }
  for (size_t i = 0; i < braced->initializers->length; i++) {
    if (!PCodeStoreInitializer(type->next, braced->initializers->value.p[i],
                               PCodeConstexprObjectSlot(object, i))) {
      return false;
    }
  }
  return true;
}

static bool BuildPCodeStructObject(TypeRecord* type, ASTNode* initializer,
                                   ConstexprObject* object) {
  if (initializer == NULL || initializer->op != AST_OP(braced_init) ||
      type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
  Struct* str = type->info.struct_info;
  size_t init_index = 0;
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function ||
        StorageIs(member->symbol->storage, STO(typedef))) {
      continue;
    }
    if (init_index >= braced->initializers->length) {
      break;
    }
    size_t slot_index = str->is_union ? 0 : member->index;
    if (!PCodeStoreInitializer(member->symbol->type,
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

static bool BuildPCodeConstexprObject(TypeRecord* type, ASTNode* initializer,
                                      ConstexprObject** result) {
  if (type == NULL || initializer == NULL || result == NULL) {
    return false;
  }
  ConstexprObject* object = NewPCodeConstexprObject(type);
  if (object == NULL) {
    return false;
  }
  bool ok = false;
  if (TypeIsFixedArray(type)) {
    ok = BuildPCodeArrayObject(type, initializer, object);
  } else if (TypeIsStructOrUnion(type)) {
    ok = BuildPCodeStructObject(type, initializer, object);
  }
  if (!ok) {
    DeletePCodeConstexprObject(object);
    return false;
  }
  *result = object;
  return true;
}

bool ConstexprPCodeEvaluateObjectConstantForSymbol(Symbol* symbol,
                                                   ASTNode* initializer) {
  if (symbol == NULL || symbol->type == NULL || initializer == NULL ||
      (!TypeIsFixedArray(symbol->type) && !TypeIsStructOrUnion(symbol->type))) {
    return false;
  }
  ConstexprObject* object = NULL;
  if (initializer->op == AST_OP(braced_init)) {
    if (!BuildPCodeConstexprObject(symbol->type, initializer, &object)) {
      return false;
    }
  } else {
    ASTNode* expr = ConstexprInitializerExpression(initializer);
    if (expr == NULL || expr->op != AST_OP(call) ||
        !TypeIsStructOrUnion(symbol->type)) {
      return false;
    }
    ConstEvalContext ctx;
    ConstEvalContextInit(&ctx);
    bool ok = ConstexprPCodeEvaluateCallObjectResult(&ctx, expr, &object) ||
              ConstexprPCodeEvaluateConstructorObject(&ctx, symbol->type, expr,
                                                      &object);
    ConstEvalContextDestruct(&ctx);
    if (!ok) {
      return false;
    }
  }
  symbol->value.other = object;
  symbol->flags.value_set = true;
  return true;
}
