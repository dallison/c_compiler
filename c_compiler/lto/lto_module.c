//
//  lto_module.c
//  c_compiler
//
//  In-memory IR module, merge, inlining, and codegen for bitcode LTO.
//

#include "lto_module.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "compiler.h"
#include "dstring.h"
#include "errors.h"
#include "expr_codegen.h"
#include "ir.h"
#include "map.h"
#include "symbol.h"
#include "type.h"
#include "type_core.h"

const char* LTOSymbolAsmName(Symbol* symbol) {
  if (symbol == NULL) {
    return "";
  }
  if (symbol->asm_name.length != 0) {
    return symbol->asm_name.value;
  }
  return symbol->name.value;
}

static int CountIRList(List* code) {
  int n = 0;
  for (ListElement* e = code->first; e != NULL; e = e->next) {
    n++;
  }
  return n;
}

static bool SymbolIsFileScopeStatic(Symbol* symbol) {
  return symbol != NULL && StorageIs(symbol->storage, STO(static)) &&
         !symbol->flags.is_local && !symbol->flags.is_argument &&
         !symbol->flags.is_temp;
}

static bool SymbolIsCOMDAT(Symbol* symbol) {
  if (symbol == NULL) {
    return false;
  }
  if (symbol->flags.is_weak || symbol->flags.is_inline_defn) {
    return true;
  }
  if (symbol->type != NULL && TypeIsFunction(symbol->type) &&
      symbol->type->info.function.is_inline) {
    return true;
  }
  return false;
}

static bool SymbolIsTULocal(Symbol* symbol) {
  return SymbolIsFileScopeStatic(symbol) && !SymbolIsCOMDAT(symbol);
}

static bool ShouldTakeIncomingDef(bool existing_discardable, int existing_score,
                                  bool incoming_discardable, int incoming_score) {
  if (!incoming_discardable && existing_discardable) {
    return true;
  }
  if (incoming_discardable && !existing_discardable) {
    return false;
  }
  if (incoming_discardable && existing_discardable) {
    return incoming_score > existing_score;
  }
  return false;
}

static bool SymbolIsDiscardableDef(Symbol* symbol) {
  if (symbol == NULL) {
    return false;
  }
  if (symbol->flags.is_weak || symbol->flags.is_inline_defn) {
    return true;
  }
  if (symbol->type != NULL && TypeIsFunction(symbol->type) &&
      symbol->type->info.function.is_inline) {
    return true;
  }
  return SymbolHasWeakBinding(symbol);
}

void LTOMakeTUId(const char* filename, char* out) {
  uint64_t h = 1469598103934665603ULL;
  if (filename == NULL) {
    filename = "";
  }
  for (const unsigned char* p = (const unsigned char*)filename; *p != 0; p++) {
    h ^= *p;
    h *= 1099511628211ULL;
  }
  snprintf(out, 17, "%016llx", (unsigned long long)h);
}

LTOModule* LTOModuleCreate(void) {
  LTOModule* module = calloc(1, sizeof(LTOModule));
  VectorInit(&module->functions);
  VectorInit(&module->initialized_static_variables);
  VectorInit(&module->uninitialized_static_variables);
  VectorInit(&module->literals);
  VectorInit(&module->init_array);
  VectorInit(&module->fini_array);
  module->next_literal_id = 1;
  return module;
}

static void LTOFunctionDelete(LTOFunction* fn) {
  if (fn == NULL) {
    return;
  }
  ListElement* e = fn->code.first;
  while (e != NULL) {
    ListElement* next = e->next;
    IRDelete((IRNode*)e);
    e = next;
  }
  ListInit(&fn->code);
  if (fn->type != NULL) {
    TypeRecordDelete(fn->type);
    fn->type = NULL;
  }
  free(fn);
}

void LTOModuleDestruct(LTOModule* module) {
  if (module == NULL) {
    return;
  }
  for (size_t i = 0; i < module->functions.length; i++) {
    LTOFunctionDelete((LTOFunction*)module->functions.value.p[i]);
  }
  VectorDestruct(&module->functions);
  if (module->owns_globals) {
    for (size_t i = 0; i < module->initialized_static_variables.length; i++) {
      InitializedStaticVariable* var =
          module->initialized_static_variables.value.p[i];
      if (var != NULL) {
        InitializedStaticVariableDelete(var);
      }
    }
    for (size_t i = 0; i < module->uninitialized_static_variables.length; i++) {
      UninitializedStaticVariable* var =
          module->uninitialized_static_variables.value.p[i];
      if (var != NULL) {
        UninitializedStaticVariableDelete(var);
      }
    }
    for (size_t i = 0; i < module->literals.length; i++) {
      Literal* lit = module->literals.value.p[i];
      if (lit != NULL) {
        LiteralDelete(lit);
      }
    }
  }
  VectorDestruct(&module->initialized_static_variables);
  VectorDestruct(&module->uninitialized_static_variables);
  VectorDestruct(&module->literals);
  VectorDestruct(&module->init_array);
  VectorDestruct(&module->fini_array);
  free(module->tu_id);
  module->tu_id = NULL;
}

void LTOModuleDelete(LTOModule* module) {
  LTOModuleDestruct(module);
  free(module);
}

LTOFunction* LTOModuleAddFunction(LTOModule* module, Generator* gen) {
  if (module == NULL || gen == NULL || gen->func == NULL) {
    return NULL;
  }
  LTOFunction* fn = calloc(1, sizeof(LTOFunction));
  fn->symbol = gen->func->info.function.symbol;
  fn->type = gen->func;
  TypeRecordIncRef(fn->type);
  ListInit(&fn->code);
  GeneratorStealCode(gen, &fn->code);
  fn->ir_node_count = CountIRList(&fn->code);
  VectorAppend(&module->functions, fn);
  return fn;
}

LTOFunction* LTOModuleFindFunction(LTOModule* module, const char* asm_name) {
  if (module == NULL || asm_name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < module->functions.length; i++) {
    LTOFunction* fn = (LTOFunction*)module->functions.value.p[i];
    if (fn != NULL && fn->symbol != NULL &&
        strcmp(LTOSymbolAsmName(fn->symbol), asm_name) == 0) {
      return fn;
    }
  }
  return NULL;
}

void LTOModuleCaptureCompilerState(LTOModule* module, Compiler* compiler) {
  if (module == NULL || compiler == NULL) {
    return;
  }
  VectorClear(&module->initialized_static_variables);
  VectorClear(&module->uninitialized_static_variables);
  VectorClear(&module->literals);
  VectorClear(&module->init_array);
  VectorClear(&module->fini_array);
  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    VectorAppend(&module->initialized_static_variables,
                 compiler->initialized_static_variables.value.p[i]);
  }
  for (size_t i = 0; i < compiler->uninitialized_static_variables.length; i++) {
    VectorAppend(&module->uninitialized_static_variables,
                 compiler->uninitialized_static_variables.value.p[i]);
  }
  for (size_t i = 0; i < compiler->literals.length; i++) {
    VectorAppend(&module->literals, compiler->literals.value.p[i]);
  }
  Vector* inits = CXXInitArrayFunctionsVector();
  Vector* finis = CXXFiniArrayFunctionsVector();
  for (size_t i = 0; i < inits->length; i++) {
    VectorAppend(&module->init_array, inits->value.p[i]);
  }
  for (size_t i = 0; i < finis->length; i++) {
    VectorAppend(&module->fini_array, finis->value.p[i]);
  }
  module->next_literal_id = compiler->next_literal_id;
  module->owns_globals = false;
}

static void RenameSymbolForTU(Symbol* symbol, const char* tu_id) {
  if (symbol == NULL || tu_id == NULL || tu_id[0] == '\0') {
    return;
  }
  if (StringContainsString(&symbol->name, ".lto.")) {
    return;
  }
  StringAppend(&symbol->name, ".lto.");
  StringAppend(&symbol->name, tu_id);
  if (symbol->asm_name.length != 0) {
    StringAppend(&symbol->asm_name, ".lto.");
    StringAppend(&symbol->asm_name, tu_id);
  }
  free(symbol->cached_target_symbol_name);
  symbol->cached_target_symbol_name = NULL;
}

static void RenameIRSymbols(List* code, const char* tu_id) {
  for (ListElement* e = code->first; e != NULL; e = e->next) {
    IRNode* node = (IRNode*)e;
    if (IRIsVariable(node)) {
      IRVariable* var = (IRVariable*)node;
      if (SymbolIsTULocal(var->symbol)) {
        RenameSymbolForTU(var->symbol, tu_id);
      }
    }
    if ((node->flags & kIRVarDef) != 0 &&
        SymbolIsTULocal(node->var.def)) {
      RenameSymbolForTU(node->var.def, tu_id);
    }
    if ((node->flags & kIRVarUse) != 0 &&
        SymbolIsTULocal(node->var.use)) {
      RenameSymbolForTU(node->var.use, tu_id);
    }
  }
}

void LTOModuleRenameInternalSymbols(LTOModule* module, const char* tu_id) {
  if (module == NULL) {
    return;
  }
  free(module->tu_id);
  module->tu_id = tu_id != NULL ? strdup(tu_id) : NULL;
  for (size_t i = 0; i < module->functions.length; i++) {
    LTOFunction* fn = (LTOFunction*)module->functions.value.p[i];
    if (fn == NULL) {
      continue;
    }
    if (SymbolIsTULocal(fn->symbol)) {
      RenameSymbolForTU(fn->symbol, tu_id);
    }
    RenameIRSymbols(&fn->code, tu_id);
  }
  for (size_t i = 0; i < module->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        module->initialized_static_variables.value.p[i];
    if (var != NULL && SymbolIsTULocal(var->symbol) && !var->is_weak) {
      RenameSymbolForTU(var->symbol, tu_id);
    }
  }
  for (size_t i = 0; i < module->uninitialized_static_variables.length; i++) {
    UninitializedStaticVariable* var =
        module->uninitialized_static_variables.value.p[i];
    if (var != NULL && SymbolIsTULocal(var->symbol) && !var->is_weak) {
      RenameSymbolForTU(var->symbol, tu_id);
    }
  }
}

static void RemapIRSymbol(IRNode* node, Map* remap) {
  if (IRIsVariable(node)) {
    IRVariable* var = (IRVariable*)node;
    void* found = MapFindPointerKey(remap, var->symbol);
    if (found != NULL) {
      var->symbol = (Symbol*)found;
    }
  }
  if ((node->flags & kIRVarDef) != 0) {
    void* found = MapFindPointerKey(remap, node->var.def);
    if (found != NULL) {
      node->var.def = (Symbol*)found;
    }
  }
  if ((node->flags & kIRVarUse) != 0) {
    void* found = MapFindPointerKey(remap, node->var.use);
    if (found != NULL) {
      node->var.use = (Symbol*)found;
    }
  }
}

static void RemapSymbolPointer(Symbol** symbol, Map* remap) {
  if (symbol == NULL || *symbol == NULL) {
    return;
  }
  void* found = MapFindPointerKey(remap, *symbol);
  if (found != NULL) {
    *symbol = (Symbol*)found;
  }
}

static void RemapInitializedVariable(InitializedStaticVariable* var, Map* remap) {
  if (var == NULL) {
    return;
  }
  RemapSymbolPointer(&var->symbol, remap);
  for (size_t i = 0; i < var->initializers.length; i++) {
    Initializer* init = var->initializers.value.p[i];
    if (init != NULL && init->type == kInitTypeSymbol) {
      RemapSymbolPointer(&init->value.symbol, remap);
    }
  }
}

static void RemapSymbolVector(Vector* symbols, Map* remap) {
  if (symbols == NULL) {
    return;
  }
  for (size_t i = 0; i < symbols->length; i++) {
    Symbol* symbol = (Symbol*)symbols->value.p[i];
    if (symbol == NULL) {
      continue;
    }
    void* found = MapFindPointerKey(remap, symbol);
    if (found != NULL) {
      symbols->value.p[i] = found;
    }
  }
}

static void RemapModuleSymbols(LTOModule* module, Map* remap) {
  if (module == NULL || remap == NULL || remap->length == 0) {
    return;
  }
  for (size_t i = 0; i < module->functions.length; i++) {
    LTOFunction* fn = (LTOFunction*)module->functions.value.p[i];
    if (fn == NULL) {
      continue;
    }
    RemapSymbolPointer(&fn->symbol, remap);
    for (ListElement* e = fn->code.first; e != NULL; e = e->next) {
      RemapIRSymbol((IRNode*)e, remap);
    }
  }
  for (size_t i = 0; i < module->initialized_static_variables.length; i++) {
    RemapInitializedVariable(module->initialized_static_variables.value.p[i],
                             remap);
  }
  for (size_t i = 0; i < module->uninitialized_static_variables.length; i++) {
    UninitializedStaticVariable* var =
        module->uninitialized_static_variables.value.p[i];
    if (var != NULL) {
      RemapSymbolPointer(&var->symbol, remap);
    }
  }
  RemapSymbolVector(&module->init_array, remap);
  RemapSymbolVector(&module->fini_array, remap);
}

static int GlobalDefScore(InitializedStaticVariable* var) {
  if (var == NULL) {
    return 0;
  }
  return (int)var->initializers.length * 16 + (int)var->size;
}

static bool GlobalIsDiscardable(InitializedStaticVariable* var) {
  return var != NULL &&
         (var->is_weak || SymbolIsDiscardableDef(var->symbol) ||
          SymbolIsCOMDAT(var->symbol));
}

static void RemapLiteralIdValue(int* id, Map* id_map) {
  if (id == NULL || *id == 0) {
    return;
  }
  void* found = MapFindInt64Key(id_map, *id);
  if (found != NULL) {
    *id = (int)(intptr_t)found;
  }
}

static void RemapLiteralIdsInIR(List* code, Map* id_map) {
  for (ListElement* e = code->first; e != NULL; e = e->next) {
    IRNode* node = (IRNode*)e;
    if ((node->opcode == IR_OP(literalref) || node->opcode == IR_OP(asm)) &&
        node->inputs.length != 0) {
      IRNode* c = (IRNode*)node->inputs.value.p[0];
      if (c != NULL && IRIsIntConst(c)) {
        int id = (int)IRIntConstValue(c);
        void* found = MapFindInt64Key(id_map, id);
        if (found != NULL) {
          ((IRConstant*)c)->value.ivalue = (int64_t)(intptr_t)found;
        }
      }
    }
  }
}

static InitializedStaticVariable* FindInitializedByName(LTOModule* module,
                                                        const char* name) {
  for (size_t i = 0; i < module->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        module->initialized_static_variables.value.p[i];
    if (var != NULL && var->symbol != NULL &&
        strcmp(LTOSymbolAsmName(var->symbol), name) == 0) {
      return var;
    }
  }
  return NULL;
}

static size_t FindUninitializedIndexByName(LTOModule* module, const char* name) {
  for (size_t i = 0; i < module->uninitialized_static_variables.length; i++) {
    UninitializedStaticVariable* var =
        module->uninitialized_static_variables.value.p[i];
    if (var != NULL && var->symbol != NULL &&
        strcmp(LTOSymbolAsmName(var->symbol), name) == 0) {
      return i;
    }
  }
  return (size_t)-1;
}

static bool MergeGlobals(LTOModule* dest, LTOModule* src, Map* remap) {
  Map id_map;
  MapInitForInt64Keys(&id_map);
  for (size_t i = 0; i < src->literals.length; i++) {
    Literal* lit = src->literals.value.p[i];
    if (lit == NULL) {
      continue;
    }
    int old_id = lit->id;
    lit->id = dest->next_literal_id++;
    MapInsert(&id_map, (MapKeyValue){.key.w = old_id,
                                     .value.p = (void*)(intptr_t)lit->id});
    VectorAppend(&dest->literals, lit);
    src->literals.value.p[i] = NULL;
  }
  if (id_map.length > 0) {
    for (size_t i = 0; i < src->functions.length; i++) {
      LTOFunction* fn = (LTOFunction*)src->functions.value.p[i];
      if (fn != NULL) {
        RemapLiteralIdsInIR(&fn->code, &id_map);
      }
    }
    for (size_t i = 0; i < src->initialized_static_variables.length; i++) {
      InitializedStaticVariable* var =
          src->initialized_static_variables.value.p[i];
      if (var == NULL) {
        continue;
      }
      for (size_t j = 0; j < var->initializers.length; j++) {
        Initializer* init = var->initializers.value.p[j];
        if (init != NULL && init->type == kInitTypeString) {
          RemapLiteralIdValue(&init->value.literal_id, &id_map);
        }
      }
    }
  }
  MapDestruct(&id_map);

  for (size_t i = 0; i < src->initialized_static_variables.length; i++) {
    InitializedStaticVariable* incoming =
        src->initialized_static_variables.value.p[i];
    if (incoming == NULL || incoming->symbol == NULL) {
      continue;
    }
    const char* name = LTOSymbolAsmName(incoming->symbol);
    InitializedStaticVariable* existing = FindInitializedByName(dest, name);
    size_t uninit_i = FindUninitializedIndexByName(dest, name);
    if (existing != NULL) {
      if (!GlobalIsDiscardable(existing) && !GlobalIsDiscardable(incoming) &&
          !SymbolIsTULocal(existing->symbol)) {
        fprintf(stderr, "LTO: duplicate definition of %s\n", name);
        return false;
      }
      if (ShouldTakeIncomingDef(GlobalIsDiscardable(existing),
                                GlobalDefScore(existing),
                                GlobalIsDiscardable(incoming),
                                GlobalDefScore(incoming))) {
        if (remap != NULL) {
          MapInsert(remap, (MapKeyValue){.key.p = existing->symbol,
                                         .value.p = incoming->symbol});
        }
        if (dest->owns_globals) {
          InitializedStaticVariableDelete(existing);
        }
        for (size_t j = 0; j < dest->initialized_static_variables.length; j++) {
          if (dest->initialized_static_variables.value.p[j] == existing) {
            dest->initialized_static_variables.value.p[j] = incoming;
            break;
          }
        }
        src->initialized_static_variables.value.p[i] = NULL;
        continue;
      }
      if (remap != NULL) {
        MapInsert(remap, (MapKeyValue){.key.p = incoming->symbol,
                                       .value.p = existing->symbol});
      }
      if (src->owns_globals) {
        InitializedStaticVariableDelete(incoming);
      }
      src->initialized_static_variables.value.p[i] = NULL;
      continue;
    }
    if (uninit_i != (size_t)-1) {
      UninitializedStaticVariable* old =
          dest->uninitialized_static_variables.value.p[uninit_i];
      if (remap != NULL && old != NULL) {
        MapInsert(remap, (MapKeyValue){.key.p = old->symbol,
                                       .value.p = incoming->symbol});
      }
      if (dest->owns_globals) {
        UninitializedStaticVariableDelete(old);
      }
      VectorDeleteElement(&dest->uninitialized_static_variables, uninit_i);
    }
    VectorAppend(&dest->initialized_static_variables, incoming);
    src->initialized_static_variables.value.p[i] = NULL;
  }

  for (size_t i = 0; i < src->uninitialized_static_variables.length; i++) {
    UninitializedStaticVariable* incoming =
        src->uninitialized_static_variables.value.p[i];
    if (incoming == NULL || incoming->symbol == NULL) {
      continue;
    }
    const char* name = LTOSymbolAsmName(incoming->symbol);
    if (FindInitializedByName(dest, name) != NULL) {
      if (remap != NULL) {
        InitializedStaticVariable* kept = FindInitializedByName(dest, name);
        MapInsert(remap, (MapKeyValue){.key.p = incoming->symbol,
                                       .value.p = kept->symbol});
      }
      if (src->owns_globals) {
        UninitializedStaticVariableDelete(incoming);
      }
      src->uninitialized_static_variables.value.p[i] = NULL;
      continue;
    }
    size_t uninit_i = FindUninitializedIndexByName(dest, name);
    if (uninit_i != (size_t)-1) {
      UninitializedStaticVariable* existing =
          dest->uninitialized_static_variables.value.p[uninit_i];
      if (incoming->size > existing->size) {
        existing->size = incoming->size;
      }
      if (incoming->alignment > existing->alignment) {
        existing->alignment = incoming->alignment;
      }
      if (remap != NULL) {
        MapInsert(remap, (MapKeyValue){.key.p = incoming->symbol,
                                       .value.p = existing->symbol});
      }
      if (src->owns_globals) {
        UninitializedStaticVariableDelete(incoming);
      }
      src->uninitialized_static_variables.value.p[i] = NULL;
      continue;
    }
    VectorAppend(&dest->uninitialized_static_variables, incoming);
    src->uninitialized_static_variables.value.p[i] = NULL;
  }

  for (size_t i = 0; i < src->init_array.length; i++) {
    VectorAppend(&dest->init_array, src->init_array.value.p[i]);
  }
  VectorClear(&src->init_array);
  for (size_t i = 0; i < src->fini_array.length; i++) {
    VectorAppend(&dest->fini_array, src->fini_array.value.p[i]);
  }
  VectorClear(&src->fini_array);
  if (src->next_literal_id > dest->next_literal_id) {
    dest->next_literal_id = src->next_literal_id;
  }
  return true;
}

bool LTOModuleMerge(LTOModule* dest, LTOModule* src) {
  if (dest == NULL || src == NULL) {
    return false;
  }
  Map remap;
  MapInitForPointerKeys(&remap);
  if (!MergeGlobals(dest, src, &remap)) {
    MapDestruct(&remap);
    return false;
  }

  Map by_name;
  MapInitForCharPointerKeys(&by_name);
  for (size_t i = 0; i < dest->functions.length; i++) {
    LTOFunction* fn = (LTOFunction*)dest->functions.value.p[i];
    if (fn != NULL && fn->symbol != NULL) {
      MapInsert(&by_name, (MapKeyValue){
                              .key.p = (void*)LTOSymbolAsmName(fn->symbol),
                              .value.p = fn});
    }
  }

  for (size_t i = 0; i < src->functions.length; i++) {
    LTOFunction* incoming = (LTOFunction*)src->functions.value.p[i];
    if (incoming == NULL || incoming->symbol == NULL) {
      continue;
    }
    const char* name = LTOSymbolAsmName(incoming->symbol);
    LTOFunction* existing =
        (LTOFunction*)MapFind(&by_name, (MapKeyType){.p = (void*)name});
    if (existing == NULL) {
      VectorAppend(&dest->functions, incoming);
      src->functions.value.p[i] = NULL;
      MapInsert(&by_name,
                (MapKeyValue){.key.p = (void*)LTOSymbolAsmName(incoming->symbol),
                              .value.p = incoming});
      continue;
    }
    bool existing_d = SymbolIsDiscardableDef(existing->symbol) ||
                      SymbolIsCOMDAT(existing->symbol);
    bool incoming_d = SymbolIsDiscardableDef(incoming->symbol) ||
                      SymbolIsCOMDAT(incoming->symbol);
    if (!existing_d && !incoming_d && !SymbolIsTULocal(existing->symbol) &&
        !SymbolIsTULocal(incoming->symbol)) {
      fprintf(stderr, "LTO: duplicate definition of %s\n", name);
      MapDestruct(&by_name);
      MapDestruct(&remap);
      return false;
    }
    if (ShouldTakeIncomingDef(existing_d, existing->ir_node_count, incoming_d,
                              incoming->ir_node_count)) {
      MapInsert(&remap, (MapKeyValue){.key.p = existing->symbol,
                                      .value.p = incoming->symbol});
      TypeRecordDelete(existing->type);
      existing->symbol = incoming->symbol;
      existing->type = incoming->type;
      incoming->type = NULL;
      ListElement* e = existing->code.first;
      while (e != NULL) {
        ListElement* next = e->next;
        IRDelete((IRNode*)e);
        e = next;
      }
      ListInit(&existing->code);
      existing->code = incoming->code;
      ListInit(&incoming->code);
      existing->ir_node_count = incoming->ir_node_count;
      LTOFunctionDelete(incoming);
      src->functions.value.p[i] = NULL;
      continue;
    }
    MapInsert(&remap, (MapKeyValue){.key.p = incoming->symbol,
                                    .value.p = existing->symbol});
    LTOFunctionDelete(incoming);
    src->functions.value.p[i] = NULL;
  }

  RemapModuleSymbols(dest, &remap);

  if (src->owns_globals) {
    dest->owns_globals = true;
  }
  MapDestruct(&by_name);
  MapDestruct(&remap);
  return true;
}

static Symbol* CalleeSymbolFromCall(IRNode* call) {
  if (call == NULL || call->opcode != IR_OP(calla) || call->inputs.length == 0) {
    return NULL;
  }
  IRNode* target = (IRNode*)call->inputs.value.p[0];
  if (target != NULL && target->opcode == IR_OP(addressof) &&
      target->inputs.length != 0) {
    target = (IRNode*)target->inputs.value.p[0];
  }
  if (target != NULL && IRIsVariable(target)) {
    return ((IRVariable*)target)->symbol;
  }
  if (target != NULL && (target->flags & kIRVarUse) != 0) {
    return target->var.use;
  }
  return NULL;
}

static bool IRListContainsOpcode(List* code, IROpcode opcode) {
  for (ListElement* e = code->first; e != NULL; e = e->next) {
    if (((IRNode*)e)->opcode == opcode) {
      return true;
    }
  }
  return false;
}

static bool LTOFunctionCanInline(LTOFunction* fn, LTOFunction* caller) {
  if (fn == NULL || fn->symbol == NULL || fn == caller) {
    return false;
  }
  if (fn->symbol->flags.noinline) {
    return false;
  }
  if (fn->type != NULL && TypeIsFunction(fn->type) &&
      (fn->type->info.function.unknown_args ||
       fn->type->info.function.varargs)) {
    return false;
  }
  if (IRListContainsOpcode(&fn->code, IR_OP(asm))) {
    return false;
  }
  bool force_inline = fn->symbol->flags.always_inline;
  bool is_inline = fn->type != NULL && TypeIsFunction(fn->type) &&
                   fn->type->info.function.is_inline;
  if (force_inline) {
    return true;
  }
  int max_nodes = is_inline ? (OptLevel3() ? 250 : 100) : 40;
  return fn->ir_node_count < max_nodes;
}

static size_t IRAllocSizeForOpcode(IROpcode opcode) {
  switch (opcode) {
    case IR_OP(const8):
    case IR_OP(const16):
    case IR_OP(const32):
    case IR_OP(const64):
    case IR_OP(constf):
    case IR_OP(constd):
    case IR_OP(consta):
      return sizeof(IRConstant);
    case IR_OP(named_label):
      return sizeof(IRNamedLabel);
    case IR_OP(loc):
      return sizeof(IRLocation);
    case IR_OP(localvar):
    case IR_OP(externvar):
    case IR_OP(argument):
    case IR_OP(staticvar):
    case IR_OP(tempvar):
    case IR_OP(ssavar):
    case IR_OP(phi):
      return sizeof(IRVariable);
    default:
      return sizeof(IRNode);
  }
}

static IRNode* CloneIRNodeShallow(IRNode* old) {
  size_t size = IRAllocSizeForOpcode(old->opcode);
  IRNode* node = calloc(1, size);
  memcpy(node, old, size);
  ListElementInit(&node->header);
  VectorInit(&node->inputs);
  VectorInit(&node->outputs);
  node->block = NULL;
  if (node->type != NULL) {
    TypeRecordIncRef(node->type);
  }
  if (old->opcode == IR_OP(named_label)) {
    IRNamedLabel* named = (IRNamedLabel*)node;
    if (named->name != NULL) {
      named->name = strdup(named->name);
    }
  }
  return node;
}

static void ReplaceUsesWith(IRNode* old, IRNode* replacement) {
  while (old->outputs.length > 0) {
    IRNode* user = (IRNode*)old->outputs.value.p[0];
    bool replaced = false;
    for (size_t i = 0; i < user->inputs.length; i++) {
      if (user->inputs.value.p[i] == old) {
        IRReplaceInput(user, i, replacement);
        replaced = true;
        break;
      }
    }
    if (!replaced) {
      VectorDeleteElement(&old->outputs, 0);
    }
  }
}

static void InsertBefore(LTOFunction* caller, IRNode* node, IRNode* at) {
  ListInsertBefore(&caller->code, &node->header, &at->header);
}

static IRNode* BranchTarget(IRNode* node) {
  if (node == NULL || node->inputs.length == 0) {
    return NULL;
  }
  if (node->opcode == IR_OP(bra)) {
    return (IRNode*)node->inputs.value.p[0];
  }
  if ((node->opcode == IR_OP(btrue) || node->opcode == IR_OP(bfalse) ||
       node->opcode == IR_OP(cbra)) &&
      node->inputs.length >= 2) {
    return (IRNode*)node->inputs.value.p[1];
  }
  return NULL;
}

static bool ArgumentNeedsHome(IRNode* arg) {
  if (arg == NULL) {
    return false;
  }
  if (IRVariableAddressEscapes(arg)) {
    return true;
  }
  for (size_t i = 0; i < arg->outputs.length; i++) {
    IRNode* user = (IRNode*)arg->outputs.value.p[i];
    if (user != NULL && IRIsStoreOnly(user) && user->inputs.length > 0 &&
        user->inputs.value.p[0] == arg) {
      return true;
    }
  }
  return false;
}

static bool FormalPassedByAddress(IRNode* arg) {
  TypeRecord* type = arg != NULL ? arg->type : NULL;
  if (type == NULL) {
    return false;
  }
  return TypeIsStructOrUnion(type) || TypeIsPointerOrArray(type) ||
         TypeIsReference(type);
}

static bool IRIsAddressValue(IRNode* node) {
  if (node == NULL) {
    return false;
  }
  if (node->opcode == IR_OP(addressof) || node->opcode == IR_OP(adda) ||
      node->opcode == IR_OP(structarg) || node->opcode == IR_OP(structreturn) ||
      node->opcode == IR_OP(tmp)) {
    return true;
  }
  if (IRIsVariable(node)) {
    return true;
  }
  return node->type != NULL &&
         (TypeIsPointerOrArray(node->type) || TypeIsReference(node->type));
}

static bool TypeHasScalarMemoryOps(TypeRecord* type) {
  return type != NULL && !TypeIsVoid(type) && !TypeIsStructOrUnion(type);
}

static IRNode* NewInlineLocal(TypeRecord* type) {
  if (compiler == NULL || type == NULL) {
    return NULL;
  }
  Symbol* sym = SyntaxNewTemporary(&compiler->syntax, type);
  sym->flags.is_temp = false;
  sym->flags.is_local = true;
  return NewIRVariable(sym);
}

static bool VectorContainsPointer(Vector* v, void* p) {
  for (size_t i = 0; i < v->length; i++) {
    if (v->value.p[i] == p) {
      return true;
    }
  }
  return false;
}

static bool InlineCallInto(LTOFunction* caller, IRNode* call,
                           LTOFunction* callee) {
  size_t nformals = 0;
  int nresult = 0;
  IRNode* result_src = NULL;
  bool has_structreturn = false;
  TypeRecord* result_type = NULL;
  if (callee->type != NULL && TypeIsFunction(callee->type)) {
    result_type = callee->type->next;
  }
  if (result_type == NULL) {
    result_type = call->type;
  }
  Map epilogue;
  MapInitForPointerKeys(&epilogue);
  for (ListElement* e = callee->code.first; e != NULL; e = e->next) {
    IRNode* old = (IRNode*)e;
    if (old->opcode == IR_OP(argument)) {
      nformals++;
    } else if (old->opcode == IR_OP(structreturn)) {
      has_structreturn = true;
    } else if (IRIsResult(old)) {
      nresult++;
      if (old->inputs.length != 0) {
        result_src = (IRNode*)old->inputs.value.p[0];
        if (result_type == NULL && result_src != NULL) {
          result_type = result_src->type;
        }
      }
    }
    if (old->opcode == IR_OP(bra) && (old->flags & kIRReturnJump) != 0) {
      IRNode* dest = BranchTarget(old);
      if (dest != NULL) {
        MapInsert(&epilogue, (MapKeyValue){.key.p = dest, .value.p = dest});
      }
    }
    if (old->opcode == IR_OP(leave)) {
      IRNode* prev = IRPrev(old);
      if (prev != NULL && (prev->opcode == IR_OP(label) ||
                           prev->opcode == IR_OP(named_label))) {
        MapInsert(&epilogue, (MapKeyValue){.key.p = prev, .value.p = prev});
      }
    }
  }

  Vector actuals = {0};
  for (size_t i = 1; i < call->inputs.length; i++) {
    IRNode* input = (IRNode*)call->inputs.value.p[i];
    if (input != NULL && input->opcode == IR_OP(pusharg) &&
        input->inputs.length != 0) {
      VectorAppend(&actuals, input->inputs.value.p[0]);
    } else {
      VectorAppend(&actuals, input);
    }
  }

  size_t actual_off = 0;
  IRNode* struct_dest = NULL;
  if (has_structreturn) {
    if ((call->flags & kIRStructReturnCall) != 0 && actuals.length > 0) {
      struct_dest = (IRNode*)actuals.value.p[0];
      actual_off = 1;
    } else if (actuals.length == nformals + 1 && actuals.length > 0) {
      struct_dest = (IRNode*)actuals.value.p[0];
      actual_off = 1;
    } else {
      VectorDestruct(&actuals);
      MapDestruct(&epilogue);
      return false;
    }
  }
  if (actuals.length < actual_off + nformals) {
    VectorDestruct(&actuals);
    MapDestruct(&epilogue);
    return false;
  }

  Vector formals = {0};
  bool refuse = false;
  for (ListElement* e = callee->code.first; e != NULL; e = e->next) {
    IRNode* old = (IRNode*)e;
    if (old->opcode != IR_OP(argument)) {
      continue;
    }
    size_t idx = formals.length;
    IRNode* actual = (IRNode*)actuals.value.p[actual_off + idx];
    VectorAppend(&formals, old);
    if (actual == NULL) {
      refuse = true;
      break;
    }
    if (ArgumentNeedsHome(old) && FormalPassedByAddress(old) &&
        !IRIsAddressValue(actual)) {
      refuse = true;
      break;
    }
    if (ArgumentNeedsHome(old) && !FormalPassedByAddress(old) &&
        (compiler == NULL || !TypeHasScalarMemoryOps(old->type))) {
      refuse = true;
      break;
    }
  }
  if (refuse) {
    VectorDestruct(&formals);
    VectorDestruct(&actuals);
    MapDestruct(&epilogue);
    return false;
  }

  bool call_used = call->outputs.length > 0;
  bool need_result_slot =
      call_used && nresult > 1 && TypeHasScalarMemoryOps(result_type);
  if (call_used && nresult > 1 && !need_result_slot && !has_structreturn) {
    VectorDestruct(&formals);
    VectorDestruct(&actuals);
    MapDestruct(&epilogue);
    return false;
  }
  if (need_result_slot && compiler == NULL) {
    VectorDestruct(&formals);
    VectorDestruct(&actuals);
    MapDestruct(&epilogue);
    return false;
  }

  Map clone_map;
  MapInitForPointerKeys(&clone_map);

  IRNode* cont = NewIR(IR_OP(label));
  InsertBefore(caller, cont, call);
  for (size_t i = 0; i < epilogue.length; i++) {
    MapInsert(&clone_map, (MapKeyValue){.key.p = epilogue.values[i].key.p,
                                        .value.p = cont});
  }

  if (struct_dest != NULL) {
    for (ListElement* e = callee->code.first; e != NULL; e = e->next) {
      IRNode* old = (IRNode*)e;
      if (old->opcode == IR_OP(structreturn)) {
        MapInsert(&clone_map,
                  (MapKeyValue){.key.p = old, .value.p = struct_dest});
      }
    }
  }

  IRNode* result_slot = NULL;
  if (need_result_slot) {
    result_slot = NewInlineLocal(result_type);
    if (result_slot == NULL) {
      VectorDestruct(&formals);
      VectorDestruct(&actuals);
      MapDestruct(&clone_map);
      MapDestruct(&epilogue);
      return false;
    }
    InsertBefore(caller, result_slot, cont);
  }

  for (size_t i = 0; i < formals.length; i++) {
    IRNode* old = (IRNode*)formals.value.p[i];
    IRNode* actual = (IRNode*)actuals.value.p[actual_off + i];
    if (ArgumentNeedsHome(old) && !FormalPassedByAddress(old)) {
      IRNode* slot = NewInlineLocal(old->type);
      if (slot == NULL) {
        VectorDestruct(&formals);
        VectorDestruct(&actuals);
        MapDestruct(&clone_map);
        MapDestruct(&epilogue);
        return false;
      }
      InsertBefore(caller, slot, cont);
      IROpcode store_op = GetStoreOpcodeForType(old->type);
      IRNode* st = NewIR2(store_op, slot, actual);
      InsertBefore(caller, st, cont);
      MapInsert(&clone_map, (MapKeyValue){.key.p = old, .value.p = slot});
    } else if (actual != NULL) {
      MapInsert(&clone_map, (MapKeyValue){.key.p = old, .value.p = actual});
    }
  }

  Vector cloned = {0};
  Vector cloned_from = {0};
  for (ListElement* e = callee->code.first; e != NULL; e = e->next) {
    IRNode* old = (IRNode*)e;
    if (old->opcode == IR_OP(enter) || old->opcode == IR_OP(leave) ||
        old->opcode == IR_OP(ret) || old->opcode == IR_OP(argument) ||
        old->opcode == IR_OP(structreturn)) {
      continue;
    }
    if (MapFindPointerKey(&epilogue, old) != NULL) {
      continue;
    }
    if (IRIsResult(old)) {
      if (result_slot != NULL && old->inputs.length != 0) {
        IRNode* src = (IRNode*)old->inputs.value.p[0];
        IRNode* mapped = (IRNode*)MapFindPointerKey(&clone_map, src);
        if (mapped == NULL) {
          mapped = src;
        }
        IROpcode store_op = GetStoreOpcodeForType(result_slot->type);
        IRNode* st = NewIR2(store_op, result_slot, mapped);
        InsertBefore(caller, st, cont);
      }
      IRNode* br = NewIR1(IR_OP(bra), cont);
      InsertBefore(caller, br, cont);
      continue;
    }
    if (old->opcode == IR_OP(bra) &&
        ((old->flags & kIRReturnJump) != 0 ||
         MapFindPointerKey(&epilogue, BranchTarget(old)) != NULL)) {
      IRNode* br = NewIR1(IR_OP(bra), cont);
      InsertBefore(caller, br, cont);
      continue;
    }
    if (IRIsLoadOnly(old) && old->inputs.length == 1) {
      IRNode* addr = (IRNode*)old->inputs.value.p[0];
      IRNode* mapped = (IRNode*)MapFindPointerKey(&clone_map, addr);
      if (mapped != NULL && VectorContainsPointer(&actuals, mapped)) {
        MapInsert(&clone_map, (MapKeyValue){.key.p = old, .value.p = mapped});
        continue;
      }
    }
    IRNode* copy = CloneIRNodeShallow(old);
    MapInsert(&clone_map, (MapKeyValue){.key.p = old, .value.p = copy});
    VectorAppend(&cloned, copy);
    VectorAppend(&cloned_from, old);
    InsertBefore(caller, copy, cont);
  }

  for (size_t i = 0; i < cloned.length; i++) {
    IRNode* copy = (IRNode*)cloned.value.p[i];
    IRNode* old = (IRNode*)cloned_from.value.p[i];
    for (size_t j = 0; j < old->inputs.length; j++) {
      IRNode* input = (IRNode*)old->inputs.value.p[j];
      IRNode* mapped = (IRNode*)MapFindPointerKey(&clone_map, input);
      if (mapped == NULL) {
        mapped = input;
      }
      if (mapped == NULL) {
        continue;
      }
      VectorAppend(&copy->inputs, mapped);
      VectorAppend(&mapped->outputs, copy);
    }
    if (old->dest != NULL) {
      IRNode* mapped = (IRNode*)MapFindPointerKey(&clone_map, old->dest);
      copy->dest = mapped != NULL ? mapped : old->dest;
    }
  }

  IRNode* result_value = NULL;
  if (result_slot != NULL) {
    IROpcode load_op = GetLoadOpcodeForType(result_slot->type);
    result_value = NewIR1(load_op, result_slot);
    InsertBefore(caller, result_value, call);
  } else if (call_used && result_src != NULL) {
    result_value = (IRNode*)MapFindPointerKey(&clone_map, result_src);
    if (result_value == NULL) {
      result_value = result_src;
    }
  }

  Vector pushargs = {0};
  for (size_t i = 1; i < call->inputs.length; i++) {
    VectorAppend(&pushargs, call->inputs.value.p[i]);
  }

  if (result_value != NULL) {
    ReplaceUsesWith(call, result_value);
  }
  IRRemoveNode(call);
  ListDeleteElement(&caller->code, &call->header);
  IRDelete(call);

  for (size_t i = 0; i < pushargs.length; i++) {
    IRNode* push = (IRNode*)pushargs.value.p[i];
    if (push != NULL && push->opcode == IR_OP(pusharg) &&
        push->outputs.length == 0) {
      IRRemoveNode(push);
      if (IRInList(push)) {
        ListDeleteElement(&caller->code, &push->header);
      }
      IRDelete(push);
    }
  }

  VectorDestruct(&pushargs);
  VectorDestruct(&cloned_from);
  VectorDestruct(&cloned);
  VectorDestruct(&formals);
  VectorDestruct(&actuals);
  MapDestruct(&clone_map);
  MapDestruct(&epilogue);
  caller->ir_node_count = CountIRList(&caller->code);
  caller->received_inline = true;
  return true;
}

int LTOInlineModule(LTOModule* module) {
  if (module == NULL || !OptLevel2()) {
    return 0;
  }
  int inlined = 0;
  bool changed = true;
  int passes = 0;
  while (changed && passes++ < 8) {
    changed = false;
    for (size_t i = 0; i < module->functions.length; i++) {
      LTOFunction* caller = (LTOFunction*)module->functions.value.p[i];
      if (caller == NULL) {
        continue;
      }
      ListElement* e = caller->code.first;
      while (e != NULL) {
        ListElement* next = e->next;
        IRNode* node = (IRNode*)e;
        if (node->opcode != IR_OP(calla)) {
          e = next;
          continue;
        }
        Symbol* callee_sym = CalleeSymbolFromCall(node);
        if (callee_sym == NULL) {
          e = next;
          continue;
        }
        LTOFunction* callee =
            LTOModuleFindFunction(module, LTOSymbolAsmName(callee_sym));
        if (!LTOFunctionCanInline(callee, caller)) {
          e = next;
          continue;
        }
        if (InlineCallInto(caller, node, callee)) {
          inlined++;
          changed = true;
        }
        e = next;
      }
    }
  }
  return inlined;
}

static IRNode* PeelCopy(IRNode* node) {
  while (node != NULL && node->opcode == IR_OP(mova) &&
         node->inputs.length == 1) {
    node = (IRNode*)node->inputs.value.p[0];
  }
  return node;
}

static IRNode* StripAddressOf(IRNode* node) {
  node = PeelCopy(node);
  if (node != NULL && node->opcode == IR_OP(addressof) &&
      node->inputs.length == 1) {
    return PeelCopy((IRNode*)node->inputs.value.p[0]);
  }
  return node;
}

static IRNode* StripAddaConsts(IRNode* node, int64_t* offset) {
  node = PeelCopy(node);
  while (node != NULL && node->opcode == IR_OP(adda) &&
         node->inputs.length == 2) {
    IRNode* left = PeelCopy((IRNode*)node->inputs.value.p[0]);
    IRNode* right = PeelCopy((IRNode*)node->inputs.value.p[1]);
    if (right != NULL && IRIsIntConst(right)) {
      *offset += IRIntConstValue(right);
      node = left;
    } else if (left != NULL && IRIsIntConst(left)) {
      *offset += IRIntConstValue(left);
      node = right;
    } else {
      break;
    }
    node = PeelCopy(node);
  }
  return node;
}

static TypeRecord* IRNodeType(IRNode* node) {
  if (node == NULL) {
    return NULL;
  }
  if (node->type != NULL) {
    return node->type;
  }
  if (IRIsVariable(node) && ((IRVariable*)node)->symbol != NULL) {
    return ((IRVariable*)node)->symbol->type;
  }
  return NULL;
}

static Symbol* IRNodeSymbol(IRNode* node) {
  node = StripAddressOf(node);
  if (node != NULL && IRIsVariable(node)) {
    return ((IRVariable*)node)->symbol;
  }
  if (node != NULL && (node->flags & kIRVarUse) != 0) {
    return node->var.use;
  }
  return NULL;
}

static bool SameIRObject(IRNode* left, IRNode* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  IRNode* a = PeelCopy(left);
  IRNode* b = PeelCopy(right);
  if (a == b) {
    return true;
  }
  a = StripAddressOf(left);
  b = StripAddressOf(right);
  if (a == b) {
    return true;
  }
  Symbol* sa = IRNodeSymbol(left);
  Symbol* sb = IRNodeSymbol(right);
  return sa != NULL && sa == sb;
}

static bool SymbolIsVTable(Symbol* symbol) {
  const char* name = symbol != NULL ? symbol->name.value : NULL;
  return name != NULL && strncmp(name, "__davecc_vtbl_", 14) == 0;
}

static bool SymbolsSameFunction(Symbol* left, Symbol* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  if (left == right) {
    return true;
  }
  return strcmp(LTOSymbolAsmName(left), LTOSymbolAsmName(right)) == 0;
}

static Symbol* VTableSlotFunction(LTOModule* module, Symbol* vtable,
                                  int64_t slot_index) {
  if (module == NULL || vtable == NULL || slot_index < 0) {
    return NULL;
  }
  int ptr_size = SizeofPointer();
  if (ptr_size <= 0) {
    return NULL;
  }
  int64_t offset = (2 + slot_index) * (int64_t)ptr_size;
  InitializedStaticVariable* var =
      FindInitializedByName(module, LTOSymbolAsmName(vtable));
  if (var == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < var->initializers.length; i++) {
    Initializer* init = var->initializers.value.p[i];
    if (init == NULL || init->type != kInitTypeSymbol ||
        init->value.symbol == NULL) {
      continue;
    }
    if ((int64_t)init->offset == offset) {
      if (init->value.symbol->type == NULL ||
          !TypeIsFunction(init->value.symbol->type)) {
        return NULL;
      }
      return init->value.symbol;
    }
  }
  return NULL;
}

static Symbol* VTableSymbolFromAddress(IRNode* addr, int64_t* offset) {
  int64_t add = 0;
  IRNode* base = StripAddaConsts(addr, &add);
  base = StripAddressOf(base);
  if (base == NULL || !IRIsVariable(base)) {
    return NULL;
  }
  Symbol* symbol = ((IRVariable*)base)->symbol;
  if (!SymbolIsVTable(symbol)) {
    return NULL;
  }
  if (offset != NULL) {
    *offset = add;
  }
  return symbol;
}

static IRNode* CallThisArg(IRNode* call) {
  if (call == NULL || call->opcode != IR_OP(calla)) {
    return NULL;
  }
  size_t index = 1;
  if ((call->flags & kIRStructReturnCall) != 0) {
    index = 2;
  }
  if (call->inputs.length <= index) {
    return NULL;
  }
  IRNode* push = (IRNode*)call->inputs.value.p[index];
  if (push == NULL || push->opcode != IR_OP(pusharg) ||
      push->inputs.length == 0) {
    return NULL;
  }
  return PeelCopy((IRNode*)push->inputs.value.p[0]);
}

static bool MatchVTableCallee(IRNode* callee, IRNode** object,
                              Symbol** const_vtable, int64_t* slot_index) {
  callee = PeelCopy(callee);
  if (callee == NULL || callee->opcode != IR_OP(loada) ||
      callee->inputs.length != 1) {
    return false;
  }
  int ptr_size = SizeofPointer();
  if (ptr_size <= 0) {
    return false;
  }
  int64_t offset = 0;
  IRNode* addr = StripAddaConsts((IRNode*)callee->inputs.value.p[0], &offset);
  if (offset < 0 || offset % ptr_size != 0) {
    return false;
  }
  int64_t vt_offset = 0;
  Symbol* vtable = VTableSymbolFromAddress(addr, &vt_offset);
  if (vtable != NULL) {
    int64_t total = offset + vt_offset;
    if (total < 2 * (int64_t)ptr_size || total % ptr_size != 0) {
      return false;
    }
    *const_vtable = vtable;
    *object = NULL;
    *slot_index = total / ptr_size - 2;
    return true;
  }
  addr = PeelCopy(addr);
  if (addr == NULL || addr->opcode != IR_OP(loada) || addr->inputs.length != 1) {
    return false;
  }
  int64_t obj_offset = 0;
  IRNode* obj = StripAddaConsts((IRNode*)addr->inputs.value.p[0], &obj_offset);
  if (obj_offset != 0) {
    return false;
  }
  *const_vtable = NULL;
  *object = PeelCopy(obj);
  *slot_index = offset / ptr_size;
  return true;
}

static Symbol* VTableStoredInto(LTOFunction* fn, IRNode* object) {
  if (fn == NULL || object == NULL) {
    return NULL;
  }
  Symbol* found = NULL;
  int ptr_size = SizeofPointer();
  for (ListElement* e = fn->code.first; e != NULL; e = e->next) {
    IRNode* node = (IRNode*)e;
    if (!IRIsStoreOnly(node) || node->inputs.length < 2) {
      continue;
    }
    IRNode* dest = PeelCopy((IRNode*)node->inputs.value.p[0]);
    int64_t dest_off = 0;
    dest = StripAddaConsts(dest, &dest_off);
    if (dest_off != 0 || !SameIRObject(dest, object)) {
      continue;
    }
    int64_t src_off = 0;
    Symbol* vtable =
        VTableSymbolFromAddress((IRNode*)node->inputs.value.p[1], &src_off);
    if (vtable == NULL) {
      continue;
    }
    if (src_off != 0 && src_off != 2 * (int64_t)ptr_size) {
      continue;
    }
    found = vtable;
  }
  return found;
}

static Symbol* PrimaryVTableSymbol(LTOModule* module, Struct* str) {
  if (str == NULL) {
    return NULL;
  }
  if (str->vtable_symbol != NULL) {
    return str->vtable_symbol;
  }
  if (module == NULL || str->tag_name == NULL || str->tag_name->value == NULL) {
    return NULL;
  }
  char name[256];
  if (snprintf(name, sizeof(name), "__davecc_vtbl_%s", str->tag_name->value) >=
      (int)sizeof(name)) {
    return NULL;
  }
  InitializedStaticVariable* var = FindInitializedByName(module, name);
  return var != NULL ? var->symbol : NULL;
}

static TypeRecord* StripPointerAndRef(TypeRecord* type) {
  while (type != NULL &&
         (TypeIsPointer(type) || TypeIsReference(type) || TypeIsArray(type))) {
    type = type->next;
  }
  return type;
}

static Struct* StructFromType(TypeRecord* type) {
  type = StripPointerAndRef(type);
  if (type != NULL && TypeIsStructOrUnion(type)) {
    return type->info.struct_info;
  }
  return NULL;
}

static Struct* CompleteObjectStruct(IRNode* object) {
  IRNode* root = StripAddressOf(object);
  TypeRecord* type = IRNodeType(root);
  if (type != NULL && TypeIsStructOrUnion(type)) {
    return type->info.struct_info;
  }
  return NULL;
}

static Struct* StaticPointeeStruct(IRNode* object) {
  Struct* complete = CompleteObjectStruct(object);
  if (complete != NULL) {
    return complete;
  }
  TypeRecord* type = IRNodeType(PeelCopy(object));
  if (type == NULL) {
    type = IRNodeType(StripAddressOf(object));
  }
  return StructFromType(type);
}

static bool SameClass(Struct* left, Struct* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  if (left == right) {
    return true;
  }
  if (left->tag_name == NULL || right->tag_name == NULL ||
      left->tag_name->value == NULL || right->tag_name->value == NULL ||
      strcmp(left->tag_name->value, right->tag_name->value) != 0) {
    return false;
  }
  if (left->vtable_symbol != NULL && right->vtable_symbol != NULL) {
    return strcmp(LTOSymbolAsmName(left->vtable_symbol),
                  LTOSymbolAsmName(right->vtable_symbol)) == 0;
  }
  return left->size == right->size &&
         left->virtual_members.length == right->virtual_members.length;
}

static bool StructDerivesFrom(Struct* derived, Struct* base) {
  if (derived == NULL || base == NULL) {
    return false;
  }
  if (SameClass(derived, base)) {
    return true;
  }
  for (size_t i = 0; i < derived->bases.length; i++) {
    CXXBaseSpecifier* spec = derived->bases.value.p[i];
    if (spec == NULL || spec->type == NULL ||
        !TypeIsStructOrUnion(spec->type) ||
        spec->type->info.struct_info == NULL) {
      continue;
    }
    if (StructDerivesFrom(spec->type->info.struct_info, base)) {
      return true;
    }
  }
  return false;
}

static void AddStructAndBases(Vector* structs, Map* seen, Struct* str) {
  if (str == NULL || MapFindPointerKey(seen, str) != NULL) {
    return;
  }
  MapInsert(seen, (MapKeyValue){.key.p = str, .value.p = str});
  VectorAppend(structs, str);
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* spec = str->bases.value.p[i];
    if (spec != NULL && spec->type != NULL && TypeIsStructOrUnion(spec->type)) {
      AddStructAndBases(structs, seen, spec->type->info.struct_info);
    }
  }
}

static void CollectStructFromType(Vector* structs, Map* seen, TypeRecord* type) {
  if (type == NULL) {
    return;
  }
  if (TypeIsFunction(type)) {
    AddStructAndBases(structs, seen, type->info.function.cxx_member_owner);
    for (size_t i = 0; i < type->info.function.prototype.length; i++) {
      Symbol* formal = type->info.function.prototype.value.p[i];
      if (formal != NULL) {
        CollectStructFromType(structs, seen, formal->type);
      }
    }
    CollectStructFromType(structs, seen, type->next);
    return;
  }
  AddStructAndBases(structs, seen, StructFromType(type));
  if (type->next != NULL && type->next != type) {
    CollectStructFromType(structs, seen, type->next);
  }
}

static void CollectModuleStructs(LTOModule* module, Vector* structs) {
  Map seen;
  MapInitForPointerKeys(&seen);
  for (size_t i = 0; i < module->functions.length; i++) {
    LTOFunction* fn = (LTOFunction*)module->functions.value.p[i];
    if (fn == NULL) {
      continue;
    }
    CollectStructFromType(structs, &seen, fn->type);
    if (fn->symbol != NULL) {
      CollectStructFromType(structs, &seen, fn->symbol->type);
    }
    for (ListElement* e = fn->code.first; e != NULL; e = e->next) {
      IRNode* node = (IRNode*)e;
      CollectStructFromType(structs, &seen, node->type);
      if (IRIsVariable(node) && ((IRVariable*)node)->symbol != NULL) {
        CollectStructFromType(structs, &seen, ((IRVariable*)node)->symbol->type);
      }
      if ((node->flags & kIRVarUse) != 0) {
        CollectStructFromType(structs, &seen,
                              node->var.use != NULL ? node->var.use->type : NULL);
      }
      if ((node->flags & kIRVarDef) != 0) {
        CollectStructFromType(structs, &seen,
                              node->var.def != NULL ? node->var.def->type : NULL);
      }
    }
  }
  for (size_t i = 0; i < module->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        module->initialized_static_variables.value.p[i];
    if (var == NULL || var->symbol == NULL) {
      continue;
    }
    CollectStructFromType(structs, &seen, var->symbol->type);
    for (size_t j = 0; j < var->initializers.length; j++) {
      Initializer* init = var->initializers.value.p[j];
      if (init != NULL && init->type == kInitTypeSymbol &&
          init->value.symbol != NULL) {
        CollectStructFromType(structs, &seen, init->value.symbol->type);
      }
    }
  }
  MapDestruct(&seen);
}

static Symbol* UniqueOverrider(LTOModule* module, Vector* structs, Struct* base,
                               int64_t slot_index) {
  if (base == NULL || structs == NULL) {
    return NULL;
  }
  Symbol* found = NULL;
  bool any = false;
  for (size_t i = 0; i < structs->length; i++) {
    Struct* str = (Struct*)structs->value.p[i];
    if (str == NULL || !StructDerivesFrom(str, base)) {
      continue;
    }
    Symbol* vtable = PrimaryVTableSymbol(module, str);
    if (vtable == NULL) {
      continue;
    }
    Symbol* fn = VTableSlotFunction(module, vtable, slot_index);
    if (fn == NULL) {
      continue;
    }
    if (!any) {
      found = fn;
      any = true;
    } else if (!SymbolsSameFunction(found, fn)) {
      return NULL;
    }
  }
  return any ? found : NULL;
}

static IRNode* FindOrInsertFunctionVar(LTOFunction* fn, Symbol* symbol,
                                       IRNode* before) {
  if (fn == NULL || symbol == NULL || before == NULL) {
    return NULL;
  }
  const char* name = LTOSymbolAsmName(symbol);
  for (ListElement* e = fn->code.first; e != NULL; e = e->next) {
    IRNode* node = (IRNode*)e;
    if (!IRIsVariable(node)) {
      continue;
    }
    Symbol* existing = ((IRVariable*)node)->symbol;
    if (existing == symbol ||
        (existing != NULL && strcmp(LTOSymbolAsmName(existing), name) == 0)) {
      return node;
    }
  }
  IRNode* var = NewIRVariable(symbol);
  InsertBefore(fn, var, before);
  return var;
}

static bool CallIsDirectFunction(IRNode* call) {
  Symbol* symbol = CalleeSymbolFromCall(call);
  return symbol != NULL && symbol->type != NULL && TypeIsFunction(symbol->type);
}

static Symbol* ResolveVirtualTarget(LTOModule* module, LTOFunction* fn,
                                    IRNode* call, Vector* structs,
                                    bool whole_program) {
  IRNode* callee = call->inputs.length != 0 ? (IRNode*)call->inputs.value.p[0]
                                            : NULL;
  if (CallIsDirectFunction(call)) {
    return NULL;
  }
  IRNode* object = NULL;
  Symbol* const_vtable = NULL;
  int64_t slot_index = 0;
  if (!MatchVTableCallee(callee, &object, &const_vtable, &slot_index)) {
    return NULL;
  }
  if (const_vtable != NULL) {
    return VTableSlotFunction(module, const_vtable, slot_index);
  }
  IRNode* this_arg = CallThisArg(call);
  if (object == NULL || (this_arg != NULL && !SameIRObject(this_arg, object))) {
    return NULL;
  }
  Symbol* stored = VTableStoredInto(fn, object);
  if (stored != NULL) {
    return VTableSlotFunction(module, stored, slot_index);
  }
  Struct* complete = CompleteObjectStruct(object);
  if (complete != NULL) {
    Symbol* vtable = PrimaryVTableSymbol(module, complete);
    if (vtable != NULL) {
      return VTableSlotFunction(module, vtable, slot_index);
    }
  }
  Struct* static_type = StaticPointeeStruct(this_arg != NULL ? this_arg : object);
  if (static_type != NULL && static_type->is_final) {
    Symbol* vtable = PrimaryVTableSymbol(module, static_type);
    if (vtable != NULL) {
      return VTableSlotFunction(module, vtable, slot_index);
    }
  }
  if (whole_program && static_type != NULL) {
    return UniqueOverrider(module, structs, static_type, slot_index);
  }
  return NULL;
}

int LTODevirtualizeModule(LTOModule* module, bool whole_program) {
  if (module == NULL || !OptLevel2()) {
    return 0;
  }
  Vector structs = {0};
  CollectModuleStructs(module, &structs);
  int converted = 0;
  for (size_t i = 0; i < module->functions.length; i++) {
    LTOFunction* fn = (LTOFunction*)module->functions.value.p[i];
    if (fn == NULL) {
      continue;
    }
    for (ListElement* e = fn->code.first; e != NULL; e = e->next) {
      IRNode* call = (IRNode*)e;
      if (call->opcode != IR_OP(calla)) {
        continue;
      }
      Symbol* target =
          ResolveVirtualTarget(module, fn, call, &structs, whole_program);
      if (target == NULL) {
        continue;
      }
      IRNode* fnvar = FindOrInsertFunctionVar(fn, target, call);
      if (fnvar == NULL) {
        continue;
      }
      IRReplaceInput(call, 0, fnvar);
      converted++;
    }
  }
  VectorDestruct(&structs);
  return converted;
}

void LTOModuleInstallIntoCompiler(Compiler* compiler, LTOModule* module) {
  if (compiler == NULL || module == NULL) {
    return;
  }
  for (size_t i = 0; i < module->initialized_static_variables.length; i++) {
    VectorAppend(&compiler->initialized_static_variables,
                 module->initialized_static_variables.value.p[i]);
  }
  VectorClear(&module->initialized_static_variables);
  for (size_t i = 0; i < module->uninitialized_static_variables.length; i++) {
    VectorAppend(&compiler->uninitialized_static_variables,
                 module->uninitialized_static_variables.value.p[i]);
  }
  VectorClear(&module->uninitialized_static_variables);
  for (size_t i = 0; i < module->literals.length; i++) {
    VectorAppend(&compiler->literals, module->literals.value.p[i]);
  }
  VectorClear(&module->literals);
  Vector* inits = CXXInitArrayFunctionsVector();
  Vector* finis = CXXFiniArrayFunctionsVector();
  for (size_t i = 0; i < module->init_array.length; i++) {
    VectorAppend(inits, module->init_array.value.p[i]);
  }
  VectorClear(&module->init_array);
  for (size_t i = 0; i < module->fini_array.length; i++) {
    VectorAppend(finis, module->fini_array.value.p[i]);
  }
  VectorClear(&module->fini_array);
  if (module->next_literal_id > compiler->next_literal_id) {
    compiler->next_literal_id = module->next_literal_id;
  }
  module->owns_globals = false;
}

static bool NameIsPreserved(const char* name, Vector* preserve) {
  if (name == NULL || name[0] == '\0') {
    return false;
  }
  if (strcmp(name, "main") == 0 || strcmp(name, "_start") == 0 ||
      strcmp(name, "__start") == 0) {
    return true;
  }
  if (preserve == NULL) {
    return false;
  }
  for (size_t i = 0; i < preserve->length; i++) {
    const char* extra = (const char*)preserve->value.p[i];
    if (extra != NULL && strcmp(name, extra) == 0) {
      return true;
    }
  }
  return false;
}

static bool SymbolIsLTORoot(Symbol* symbol, bool whole_program,
                            Vector* preserve) {
  if (symbol == NULL) {
    return false;
  }
  if (SymbolHasAttribute(symbol, "used")) {
    return true;
  }
  if (NameIsPreserved(LTOSymbolAsmName(symbol), preserve)) {
    return true;
  }
  if (!whole_program && !SymbolIsFileScopeStatic(symbol)) {
    return true;
  }
  return false;
}

static Symbol* FindModuleGlobalSymbol(LTOModule* module, Symbol* hint) {
  if (hint == NULL) {
    return NULL;
  }
  const char* name = LTOSymbolAsmName(hint);
  InitializedStaticVariable* init = FindInitializedByName(module, name);
  if (init != NULL && init->symbol != NULL) {
    return init->symbol;
  }
  size_t uninit_i = FindUninitializedIndexByName(module, name);
  if (uninit_i != (size_t)-1) {
    UninitializedStaticVariable* var =
        module->uninitialized_static_variables.value.p[uninit_i];
    if (var != NULL) {
      return var->symbol;
    }
  }
  return NULL;
}

static void MarkLiveFunction(LTOModule* module, Map* live, Vector* work,
                             Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return;
  }
  LTOFunction* fn =
      LTOModuleFindFunction(module, LTOSymbolAsmName(symbol));
  if (fn == NULL || MapFindPointerKey(live, fn) != NULL) {
    return;
  }
  MapInsert(live, (MapKeyValue){.key.p = fn, .value.p = fn});
  VectorAppend(work, fn);
}

static void MarkLiveGlobal(LTOModule* module, Map* live, Vector* work,
                           Symbol* hint) {
  Symbol* symbol = FindModuleGlobalSymbol(module, hint);
  if (symbol == NULL || MapFindPointerKey(live, symbol) != NULL) {
    return;
  }
  MapInsert(live, (MapKeyValue){.key.p = symbol, .value.p = symbol});
  VectorAppend(work, symbol);
}

static void MarkLiveSymbol(LTOModule* module, Map* live_fns, Vector* fn_work,
                           Map* live_globals, Vector* global_work,
                           Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL) {
    return;
  }
  if (TypeIsFunction(symbol->type)) {
    MarkLiveFunction(module, live_fns, fn_work, symbol);
  } else {
    MarkLiveGlobal(module, live_globals, global_work, symbol);
  }
}

static bool IRValueIsUnused(IRNode* node, int depth) {
  if (node == NULL || depth > 8) {
    return true;
  }
  if (IRHasSideEffects(node) || node->dest != NULL) {
    return false;
  }
  for (size_t i = 0; i < node->outputs.length; i++) {
    if (!IRValueIsUnused((IRNode*)node->outputs.value.p[i], depth + 1)) {
      return false;
    }
  }
  return true;
}

static void CollectRefsFromIR(LTOModule* module, Map* live_fns, Vector* fn_work,
                              Map* live_globals, Vector* global_work,
                              List* code) {
  if (code == NULL) {
    return;
  }
  for (ListElement* e = code->first; e != NULL; e = e->next) {
    IRNode* node = (IRNode*)e;
    MarkLiveSymbol(module, live_fns, fn_work, live_globals, global_work,
                   CalleeSymbolFromCall(node));
    if (IRIsVariable(node) && !IRValueIsUnused(node, 0)) {
      MarkLiveSymbol(module, live_fns, fn_work, live_globals, global_work,
                     ((IRVariable*)node)->symbol);
    }
    if ((node->flags & kIRVarDef) != 0) {
      MarkLiveSymbol(module, live_fns, fn_work, live_globals, global_work,
                     node->var.def);
    }
    if ((node->flags & kIRVarUse) != 0) {
      MarkLiveSymbol(module, live_fns, fn_work, live_globals, global_work,
                     node->var.use);
    }
  }
}

static void CollectRefsFromGlobal(LTOModule* module, Map* live_fns,
                                  Vector* fn_work, Map* live_globals,
                                  Vector* global_work, Symbol* symbol) {
  if (symbol == NULL) {
    return;
  }
  const char* name = LTOSymbolAsmName(symbol);
  InitializedStaticVariable* var = FindInitializedByName(module, name);
  if (var == NULL) {
    return;
  }
  for (size_t j = 0; j < var->initializers.length; j++) {
    Initializer* init = var->initializers.value.p[j];
    if (init != NULL && init->type == kInitTypeSymbol) {
      MarkLiveSymbol(module, live_fns, fn_work, live_globals, global_work,
                     init->value.symbol);
    }
  }
}

static void ComputeLiveSymbols(LTOModule* module, bool whole_program,
                               Vector* preserve, Map* live_fns,
                               Map* live_globals) {
  Vector fn_work = {0};
  Vector global_work = {0};
  for (size_t i = 0; i < module->functions.length; i++) {
    LTOFunction* fn = (LTOFunction*)module->functions.value.p[i];
    if (fn != NULL && SymbolIsLTORoot(fn->symbol, whole_program, preserve)) {
      MarkLiveFunction(module, live_fns, &fn_work, fn->symbol);
    }
  }
  for (size_t i = 0; i < module->init_array.length; i++) {
    MarkLiveFunction(module, live_fns, &fn_work,
                     module->init_array.value.p[i]);
  }
  for (size_t i = 0; i < module->fini_array.length; i++) {
    MarkLiveFunction(module, live_fns, &fn_work,
                     module->fini_array.value.p[i]);
  }
  for (size_t i = 0; i < module->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        module->initialized_static_variables.value.p[i];
    if (var != NULL &&
        SymbolIsLTORoot(var->symbol, whole_program, preserve)) {
      MarkLiveGlobal(module, live_globals, &global_work, var->symbol);
    }
  }
  for (size_t i = 0; i < module->uninitialized_static_variables.length; i++) {
    UninitializedStaticVariable* var =
        module->uninitialized_static_variables.value.p[i];
    if (var != NULL &&
        SymbolIsLTORoot(var->symbol, whole_program, preserve)) {
      MarkLiveGlobal(module, live_globals, &global_work, var->symbol);
    }
  }
  while (fn_work.length > 0 || global_work.length > 0) {
    if (fn_work.length > 0) {
      LTOFunction* fn = (LTOFunction*)fn_work.value.p[fn_work.length - 1];
      VectorDeleteElement(&fn_work, fn_work.length - 1);
      if (fn != NULL) {
        CollectRefsFromIR(module, live_fns, &fn_work, live_globals,
                          &global_work, &fn->code);
      }
      continue;
    }
    Symbol* g = (Symbol*)global_work.value.p[global_work.length - 1];
    VectorDeleteElement(&global_work, global_work.length - 1);
    CollectRefsFromGlobal(module, live_fns, &fn_work, live_globals,
                          &global_work, g);
  }
  VectorDestruct(&fn_work);
  VectorDestruct(&global_work);
}

static void DropDeadGlobals(LTOModule* module, Map* live_globals) {
  Vector keep = {0};
  for (size_t i = 0; i < module->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        module->initialized_static_variables.value.p[i];
    if (var == NULL) {
      continue;
    }
    if (var->symbol != NULL &&
        MapFindPointerKey(live_globals, var->symbol) != NULL) {
      VectorAppend(&keep, var);
    } else if (module->owns_globals) {
      InitializedStaticVariableDelete(var);
    }
  }
  VectorDestruct(&module->initialized_static_variables);
  module->initialized_static_variables = keep;

  Vector keep_u = {0};
  for (size_t i = 0; i < module->uninitialized_static_variables.length; i++) {
    UninitializedStaticVariable* var =
        module->uninitialized_static_variables.value.p[i];
    if (var == NULL) {
      continue;
    }
    if (var->symbol != NULL &&
        MapFindPointerKey(live_globals, var->symbol) != NULL) {
      VectorAppend(&keep_u, var);
    } else if (module->owns_globals) {
      UninitializedStaticVariableDelete(var);
    }
  }
  VectorDestruct(&module->uninitialized_static_variables);
  module->uninitialized_static_variables = keep_u;
}

static void SweepUnusedIR(List* code) {
  if (code == NULL) {
    return;
  }
  bool changed = true;
  while (changed) {
    changed = false;
    ListElement* e = code->first;
    while (e != NULL) {
      ListElement* next = e->next;
      IRNode* node = (IRNode*)e;
      bool keep = node->dest != NULL || IRHasSideEffects(node);
      if (!keep) {
        switch (node->opcode) {
          case IR_OP(enter):
          case IR_OP(leave):
          case IR_OP(ret):
          case IR_OP(label):
          case IR_OP(named_label):
          case IR_OP(argument):
            keep = true;
            break;
          default:
            keep = node->outputs.length != 0;
            break;
        }
      }
      if (!keep) {
        IRRemoveNode(node);
        if (IRInList(node)) {
          ListDeleteElement(code, &node->header);
        }
        IRDelete(node);
        changed = true;
      }
      e = next;
    }
  }
}

static void SweepModuleIR(LTOModule* module) {
  if (module == NULL) {
    return;
  }
  for (size_t i = 0; i < module->functions.length; i++) {
    LTOFunction* fn = (LTOFunction*)module->functions.value.p[i];
    if (fn != NULL) {
      SweepUnusedIR(&fn->code);
      fn->ir_node_count = CountIRList(&fn->code);
    }
  }
}

bool LTOCodegenModule(Compiler* compiler, LTOModule* module,
                      bool whole_program, Vector* preserve_asm_names) {
  if (compiler == NULL || module == NULL) {
    return false;
  }
  SweepModuleIR(module);
  Map live_fns;
  Map live_globals;
  MapInitForPointerKeys(&live_fns);
  MapInitForPointerKeys(&live_globals);
  ComputeLiveSymbols(module, whole_program, preserve_asm_names, &live_fns,
                     &live_globals);
  DropDeadGlobals(module, &live_globals);
  for (size_t i = 0; i < module->functions.length; i++) {
    LTOFunction* fn = (LTOFunction*)module->functions.value.p[i];
    if (fn == NULL || fn->type == NULL ||
        MapFindPointerKey(&live_fns, fn) == NULL) {
      continue;
    }
    IRRepairVarDefUse(&fn->code);
    Generator gen;
    GeneratorInit(&gen, &compiler->syntax, fn->type);
    GeneratorAdoptCode(&gen, &fn->code);
    IRRenumberList(&gen.code);
    GeneratorRebuildPools(&gen);
    if (fn->received_inline) {
      OptimizeFunctionIR(&gen);
    } else {
      BuildBasicBlocks(&gen);
    }
    void* code = compiler->target->codegen(&gen);
    VectorAppend(&compiler->functions, code);
    const char* emit_name = LTOSymbolAsmName(fn->symbol);
    VectorAppend(&compiler->emitted_function_asm_names, NewString(emit_name));
    GeneratorDestruct(&gen);
  }
  MapDestruct(&live_fns);
  MapDestruct(&live_globals);
  LTOModuleInstallIntoCompiler(compiler, module);
  return NumErrors() == 0;
}