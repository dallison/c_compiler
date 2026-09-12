//
//  main.c
//  ltodump
//
//  Pretty-printer for DCCLTO03 IR bitcode objects (and System V .a files that
//  contain them).  Output follows the compiler's -Xbe-print IR dump, with
//  extra type, linkage, location, global-initializer, and file-table detail
//  that the in-memory dump does not show.
//

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "dstring.h"
#include "ir.h"
#include "lto_archive.h"
#include "options.h"
#include "serialize.h"
#include "source.h"
#include "symbol.h"
#include "symbol_table.h"
#include "type.h"
#include "vector.h"

static void Usage(const char* argv0) {
  fprintf(stderr,
          "usage: %s [--target ARCH] FILE [FILE...]\n"
          "Dump serialized LTO IR (DCCLTO03) to stdout.\n"
          "FILE may be a -c -flto object or a System V archive of those.\n",
          argv0);
}

static void PrintType(FILE* fp, TypeRecord* type) {
  if (type == NULL) {
    fputs("<none>", fp);
    return;
  }
  String text;
  StringInit(&text, NULL);
  TypeRecordToString(type, &text);
  if (text.length == 0) {
    fputs("<unnamed>", fp);
  } else {
    fputs(text.value, fp);
  }
  StringDestruct(&text);
}

static const char* SymbolName(Symbol* symbol) {
  if (symbol == NULL) {
    return "<null>";
  }
  if (symbol->name.value != NULL && symbol->name.value[0] != '\0') {
    return symbol->name.value;
  }
  return "<anon>";
}

static void PrintStorage(FILE* fp, Storage storage) {
  if (storage == STO(implicit)) {
    fputs("implicit", fp);
    return;
  }
  const char* sep = "";
  struct {
    Storage bit;
    const char* name;
  } bits[] = {
      {STO(auto), "auto"},         {STO(static), "static"},
      {STO(typedef), "typedef"},   {STO(extern), "extern"},
      {STO(register), "register"}, {STO(assembler), "assembler"},
      {STO(thread), "thread"},
  };
  for (size_t i = 0; i < sizeof(bits) / sizeof(bits[0]); i++) {
    if ((storage & bits[i].bit) != 0) {
      fprintf(fp, "%s%s", sep, bits[i].name);
      sep = "|";
    }
  }
  if (sep[0] == '\0') {
    fputs("implicit", fp);
  }
}

static void PrintSymbolFlags(FILE* fp, Symbol* symbol) {
  if (symbol == NULL) {
    return;
  }
  const char* sep = "";
#define FLAG(cond, name)          \
  do {                            \
    if (cond) {                   \
      fprintf(fp, "%s%s", sep, name); \
      sep = " ";                  \
    }                             \
  } while (0)
  FLAG(symbol->flags.is_defined, "defined");
  FLAG(symbol->flags.is_tentative_decl, "tentative");
  FLAG(symbol->flags.is_forward_declared, "forward");
  FLAG(symbol->flags.is_local, "local");
  FLAG(symbol->flags.is_block_scope, "block");
  FLAG(symbol->flags.is_argument, "argument");
  FLAG(symbol->flags.is_temp, "temp");
  FLAG(symbol->flags.address_taken, "address-taken");
  FLAG(symbol->flags.used, "used");
  FLAG(symbol->flags.invented, "invented");
  FLAG(symbol->flags.is_inline_defn, "inline-defn");
  FLAG(symbol->flags.noreturn, "noreturn");
  FLAG(symbol->flags.always_inline, "always_inline");
  FLAG(symbol->flags.noinline, "noinline");
  FLAG(symbol->flags.is_weak, "weak");
  FLAG(symbol->flags.is_c_linkage, "c-linkage");
  FLAG(symbol->flags.is_constexpr, "constexpr");
#undef FLAG
  if (sep[0] == '\0') {
    fputs("-", fp);
  }
}

static void PrintSymbolRef(FILE* fp, Symbol* symbol) {
  fprintf(fp, "%s", SymbolName(symbol));
  if (symbol != NULL && symbol->asm_name.length != 0 &&
      (symbol->name.value == NULL ||
       strcmp(symbol->asm_name.value, symbol->name.value) != 0)) {
    fprintf(fp, " asm=%s", symbol->asm_name.value);
  }
}

static void PrintLocation(FILE* fp, SourceLocation location, const char* prefix) {
  if (location == 0 || location == SOURCE_LOCATION_MISSING ||
      location == SOURCE_LOCATION_COMMAND_LINE) {
    return;
  }
  const char* filename = NULL;
  int lineno = 0;
  int start = 0;
  int end = 0;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);
  if (filename == NULL || filename[0] == '\0') {
    filename = "<unknown>";
  }
  fprintf(fp, "%s%s:%d:%d-%d", prefix != NULL ? prefix : "", filename, lineno,
          start, end);
}

static const char* ValueStateName(ValueState state) {
  switch (state) {
    case kValueStateValid:
      return NULL;
    case kValueStateErroneous:
      return "erroneous";
    case kValueStateIndeterminate:
      return "indeterminate";
  }
  return "value-state?";
}

static void DumpIRNode(IRNode* inst, FILE* fp) {
  if (inst == NULL) {
    fputs("$<null>\n", fp);
    return;
  }
  fprintf(fp, "$%d %s(", inst->id, IROpcodeName(inst->opcode));
  const char* sep = "";
  for (size_t i = 0; i < inst->inputs.length; i++) {
    IRNode* op = (IRNode*)inst->inputs.value.p[i];
    if (op == NULL) {
      fprintf(fp, "%s$<null>", sep);
    } else {
      fprintf(fp, "%s$%d", sep, op->id);
    }
    sep = ", ";
  }
  fprintf(fp, ") [");
  sep = "";
  for (size_t i = 0; i < inst->outputs.length; i++) {
    IRNode* op = (IRNode*)inst->outputs.value.p[i];
    if (op == NULL) {
      fprintf(fp, "%s$<null>", sep);
    } else {
      fprintf(fp, "%s$%d", sep, op->id);
    }
    sep = ", ";
  }
  fprintf(fp, "]");

  IRConstant* constant = (IRConstant*)inst;
  IRVariable* var = (IRVariable*)inst;
  IRLocation* loc = (IRLocation*)inst;
  IRNamedLabel* named = (IRNamedLabel*)inst;
  switch (inst->opcode) {
    case IR_OP(const32):
    case IR_OP(const8):
    case IR_OP(const16):
    case IR_OP(const64):
    case IR_OP(consta):
      fprintf(fp, " %" PRId64, constant->value.ivalue);
      break;
    case IR_OP(constf):
    case IR_OP(constd):
      fprintf(fp, " %g", constant->value.fvalue);
      break;
    case IR_OP(localvar):
    case IR_OP(externvar):
    case IR_OP(argument):
    case IR_OP(staticvar):
    case IR_OP(tempvar):
    case IR_OP(phi):
    case IR_OP(ssavar):
      fputc(' ', fp);
      PrintSymbolRef(fp, var->symbol);
      break;
    case IR_OP(named_label):
      fprintf(fp, " %s", named->name != NULL ? named->name : "<unnamed>");
      break;
    case IR_OP(loc): {
      const char* filename;
      int lineno;
      int start;
      int end;
      DecodeSourceLocation(loc->location, &filename, &lineno, &start, &end);
      fprintf(fp, " %s, %d, %d, %d",
              filename != NULL ? filename : "<unknown>", lineno, start, end);
      break;
    }
    case IR_OP(calla):
      if ((inst->flags & kIRTailCall) != 0) {
        fputs(" [tail]", fp);
      }
      break;
    default:
      break;
  }

  if (inst->type != NULL) {
    fputs(" : ", fp);
    PrintType(fp, inst->type);
  }

  fprintf(fp, " *%zd", inst->outputs.length);
  if (IRIsVarDef(inst)) {
    fprintf(fp, " DEF %s", SymbolName(inst->var.def));
  } else if (IRIsVarRef(inst)) {
    fprintf(fp, " REF %s", SymbolName(inst->var.use));
  }
  if (inst->flags != 0) {
    fprintf(fp, " {");
    sep = "";
    static const char* kFlagNames[] = {
        "vardef",
        "varuse",
        "tailcall",
        "returnjump",
        "rvocall",
        "nrvomarker",
        "jumptablebranch",
        "fakeunsigned",
        "fromcall",
        "stashedcallresult",
        "structreturncall",
        "deferredargreload",
        "deferredargrebuildaddress",
        "asmmemoryclobber",
        "bitwidth64",
        "invalidvaluedefinition",
    };
    size_t flag_name_count = sizeof(kFlagNames) / sizeof(kFlagNames[0]);
    for (size_t i = 0; i < 32; i++) {
      if ((inst->flags & (1u << i)) != 0) {
        if (i < flag_name_count) {
          fprintf(fp, "%s%s", sep, kFlagNames[i]);
        } else {
          fprintf(fp, "%sflag%zu", sep, i);
        }
        sep = ",";
      }
    }
    fprintf(fp, "}");
  }
  if (inst->dest != NULL) {
    fprintf(fp, " -> $%d", inst->dest->id);
  }
  if (inst->data.ivalue != 0) {
    fprintf(fp, " data.i=%d", inst->data.ivalue);
  }
  if (inst->data.lvalue != 0) {
    fprintf(fp, " data.l=%" PRId64, inst->data.lvalue);
  }
  const char* value_state = ValueStateName(inst->value_state);
  if (value_state != NULL) {
    fprintf(fp, " state=%s", value_state);
  }
  if (inst->opcode != IR_OP(loc)) {
    PrintLocation(fp, inst->location, " loc=");
  }
  fputc('\n', fp);
}

static void DumpFunction(LTOFunction* fn, FILE* fp) {
  for (int i = 0; i < 80; i++) {
    fputc('-', fp);
  }
  fputc('\n', fp);
  const char* name = fn != NULL && fn->symbol != NULL
                         ? LTOSymbolAsmName(fn->symbol)
                         : "<null>";
  fprintf(fp, "**** IR for function %s\n", name);
  if (fn != NULL && fn->symbol != NULL) {
    if (fn->symbol->name.value != NULL &&
        strcmp(fn->symbol->name.value, name) != 0) {
      fprintf(fp, "  source-name: %s\n", fn->symbol->name.value);
    }
    fprintf(fp, "  storage: ");
    PrintStorage(fp, fn->symbol->storage);
    fprintf(fp, "\n  flags: ");
    PrintSymbolFlags(fp, fn->symbol);
    fputc('\n', fp);
    if (fn->symbol->location != 0 &&
        fn->symbol->location != SOURCE_LOCATION_MISSING &&
        fn->symbol->location != SOURCE_LOCATION_COMMAND_LINE) {
      PrintLocation(fp, fn->symbol->location, "  location: ");
      fputc('\n', fp);
    }
  }
  fprintf(fp, "  type: ");
  PrintType(fp, fn != NULL ? fn->type : NULL);
  fprintf(fp, "\n  ir-nodes: %d\n\n", fn != NULL ? fn->ir_node_count : 0);
  if (fn == NULL) {
    return;
  }
  IRRenumberList(&fn->code);
  for (ListElement* e = fn->code.first; e != NULL; e = e->next) {
    DumpIRNode((IRNode*)e, fp);
  }
  fputc('\n', fp);
}

static void DumpInitializer(Initializer* init, FILE* fp) {
  if (init == NULL) {
    fputs("    <null initializer>\n", fp);
    return;
  }
  fprintf(fp, "    +%d ", init->offset);
  switch (init->type) {
    case kInitTypeByte:
      fprintf(fp, "byte %u\n", (unsigned)init->value.byte);
      break;
    case kInitTypeHalf:
      fprintf(fp, "half %u\n", (unsigned)init->value.half);
      break;
    case kInitTypeWord:
      fprintf(fp, "word %u\n", init->value.word);
      break;
    case kInitTypeLong:
      fprintf(fp, "long %" PRIu64 "\n", init->value._long);
      break;
    case kInitTypeSymbol:
      fputs("symbol ", fp);
      PrintSymbolRef(fp, init->value.symbol);
      if (init->symbol_addend != 0) {
        fprintf(fp, "%+" PRId64, init->symbol_addend);
      }
      fputc('\n', fp);
      break;
    case kInitTypeString:
      fprintf(fp, "string-id %d\n", init->value.literal_id);
      break;
    case kInitTypeMemory: {
      fprintf(fp, "memory [%zu]", init->value.memory.length);
      size_t n = init->value.memory.length;
      if (n > 32) {
        n = 32;
      }
      for (size_t i = 0; i < n; i++) {
        fprintf(fp, " %02x", init->value.memory.value[i]);
      }
      if (init->value.memory.length > n) {
        fputs(" ...", fp);
      }
      fputc('\n', fp);
      break;
    }
    default:
      fprintf(fp, "kind %d\n", (int)init->type);
      break;
  }
}

static void DumpInitialized(InitializedStaticVariable* var, FILE* fp) {
  if (var == NULL) {
    return;
  }
  fputs("  ", fp);
  PrintSymbolRef(fp, var->symbol);
  fprintf(fp, " size=%zu align=%d", var->size, var->alignment);
  if (var->is_global) {
    fputs(" global", fp);
  }
  if (var->is_weak) {
    fputs(" weak", fp);
  }
  if (var->is_tls) {
    fputs(" tls", fp);
  }
  if (var->is_local) {
    fputs(" local", fp);
  }
  fputs("\n    type: ", fp);
  PrintType(fp, var->symbol != NULL ? var->symbol->type : NULL);
  fputc('\n', fp);
  for (size_t i = 0; i < var->initializers.length; i++) {
    DumpInitializer(var->initializers.value.p[i], fp);
  }
}

static void DumpUninitialized(UninitializedStaticVariable* var, FILE* fp) {
  if (var == NULL) {
    return;
  }
  fputs("  ", fp);
  PrintSymbolRef(fp, var->symbol);
  fprintf(fp, " size=%zu align=%zu", var->size, var->alignment);
  if (var->is_global) {
    fputs(" global", fp);
  }
  if (var->is_weak) {
    fputs(" weak", fp);
  }
  if (var->is_tls) {
    fputs(" tls", fp);
  }
  if (var->is_local) {
    fputs(" local", fp);
  }
  fputs("\n    type: ", fp);
  PrintType(fp, var->symbol != NULL ? var->symbol->type : NULL);
  fputc('\n', fp);
}

static void DumpLiteral(Literal* lit, FILE* fp) {
  if (lit == NULL) {
    return;
  }
  const char* kind = "unknown";
  switch (lit->type) {
    case kLiteralString:
      kind = "string";
      break;
    case kLiteralWideString:
      kind = "wide-string";
      break;
    case kLiteralBuffer:
      kind = "buffer";
      break;
  }
  fprintf(fp, "  .str.%d %s%s", lit->id, kind, lit->disabled ? " disabled" : "");
  if (lit->type == kLiteralBuffer) {
    BufferLiteral* buf = (BufferLiteral*)lit;
    fprintf(fp, " [%zu]", buf->value.length);
    size_t n = buf->value.length < 32 ? buf->value.length : 32;
    for (size_t i = 0; i < n; i++) {
      fprintf(fp, " %02x", buf->value.value[i]);
    }
    if (buf->value.length > n) {
      fputs(" ...", fp);
    }
  } else {
    StringLiteral* str = (StringLiteral*)lit;
    fprintf(fp, " elem=%d \"", str->element_size);
    for (size_t i = 0; i < str->value.length; i++) {
      unsigned char ch = (unsigned char)str->value.value[i];
      if (ch == '"' || ch == '\\') {
        fprintf(fp, "\\%c", ch);
      } else if (ch >= 32 && ch < 127) {
        fputc((char)ch, fp);
      } else {
        fprintf(fp, "\\x%02x", ch);
      }
    }
    fputc('"', fp);
  }
  fputc('\n', fp);
}

static void DumpFiles(FILE* fp) {
  size_t n = SourceFileCount();
  fprintf(fp, "files: %zu\n", n);
  for (size_t i = 0; i < n; i++) {
    File* file = SourceFileAt(i);
    if (file == NULL) {
      fprintf(fp, "  [%zu] <null>\n", i);
      continue;
    }
    fprintf(fp, "  [%zu] \"%s\" lines=%zu%s\n", i,
            file->name.value != NULL ? file->name.value : "",
            file->lines.length, file->is_system_header ? " system" : "");
  }
}

static void DumpPools(DeserializeContext* ctx, FILE* fp) {
  if (ctx == NULL) {
    return;
  }
  static const char* names[kSerialKindCount] = {
      "strings", "types", "symbols", "structs", "enums", "members",
      "namespaces", "ast", "ir",
  };
  fputs("interned pools:\n", fp);
  fprintf(fp, "  strings: %zu\n", ctx->string_pool.length);
  for (int kind = 1; kind < kSerialKindCount; kind++) {
    fprintf(fp, "  %s: %zu\n", names[kind], ctx->objects[kind].length);
  }
}

static void DumpModule(const char* label, LoadedLTOModule* loaded, FILE* fp) {
  LTOModule* module = &loaded->module;
  fprintf(fp, "=== LTO module: %s ===\n", label);
  fprintf(fp, "magic: %s\n", DCC_LTO_MAGIC);
  fprintf(fp, "tu-id: %s\n", module->tu_id != NULL ? module->tu_id : "<none>");
  fprintf(fp, "next-literal-id: %d\n", module->next_literal_id);
  DumpPools(loaded->ctx, fp);
  DumpFiles(fp);
  fprintf(fp, "functions: %zu\n", module->functions.length);
  fprintf(fp, "initialized-globals: %zu\n",
          module->initialized_static_variables.length);
  fprintf(fp, "uninitialized-globals: %zu\n",
          module->uninitialized_static_variables.length);
  fprintf(fp, "literals: %zu\n", module->literals.length);
  fprintf(fp, "init-array: %zu  fini-array: %zu\n\n", module->init_array.length,
          module->fini_array.length);

  for (size_t i = 0; i < module->functions.length; i++) {
    DumpFunction((LTOFunction*)module->functions.value.p[i], fp);
  }

  if (module->initialized_static_variables.length != 0) {
    fputs("==== initialized globals ====\n", fp);
    for (size_t i = 0; i < module->initialized_static_variables.length; i++) {
      DumpInitialized(module->initialized_static_variables.value.p[i], fp);
    }
    fputc('\n', fp);
  }
  if (module->uninitialized_static_variables.length != 0) {
    fputs("==== uninitialized globals ====\n", fp);
    for (size_t i = 0; i < module->uninitialized_static_variables.length; i++) {
      DumpUninitialized(module->uninitialized_static_variables.value.p[i], fp);
    }
    fputc('\n', fp);
  }
  if (module->literals.length != 0) {
    fputs("==== literals ====\n", fp);
    for (size_t i = 0; i < module->literals.length; i++) {
      DumpLiteral(module->literals.value.p[i], fp);
    }
    fputc('\n', fp);
  }
  if (module->init_array.length != 0) {
    fputs("==== init_array ====\n", fp);
    for (size_t i = 0; i < module->init_array.length; i++) {
      fputs("  ", fp);
      PrintSymbolRef(fp, module->init_array.value.p[i]);
      fputc('\n', fp);
    }
    fputc('\n', fp);
  }
  if (module->fini_array.length != 0) {
    fputs("==== fini_array ====\n", fp);
    for (size_t i = 0; i < module->fini_array.length; i++) {
      fputs("  ", fp);
      PrintSymbolRef(fp, module->fini_array.value.p[i]);
      fputc('\n', fp);
    }
    fputc('\n', fp);
  }
}

static bool InitCompiler(const char* path, const char* target) {
  Vector options;
  VectorInit(&options);
  CompilerOptionValue* target_opt = calloc(1, sizeof(CompilerOptionValue));
  target_opt->opt = kOptionTarget;
  StringInit(&target_opt->value.svalue, target);
  VectorAppend(&options, target_opt);

  compiler = malloc(sizeof(Compiler));
  if (!CompilerInitForLTOLink(compiler, path, &options, NULL)) {
    fprintf(stderr, "ltodump: failed to initialize compiler for %s\n", path);
    free(compiler);
    compiler = NULL;
    return false;
  }
  compiler->lto = true;
  CompilerPrepareForIRLoad();
  return true;
}

static bool DumpLoadedPath(const char* path) {
  LoadedLTOModule loaded;
  if (!LTOArchiveRead(path, &loaded)) {
    fprintf(stderr, "ltodump: failed to read LTO object %s\n", path);
    return false;
  }
  DumpModule(path, &loaded, stdout);
  LoadedLTOModuleDestruct(&loaded);
  return true;
}

static bool DumpArchive(const char* path) {
  Vector blobs = {0};
  Vector lens = {0};
  bool has_native = false;
  int n = LTOArchiveExtractLTOMembers(path, &blobs, &lens, &has_native);
  if (n < 0) {
    fprintf(stderr, "ltodump: not an LTO object or archive: %s\n", path);
    return false;
  }
  if (n == 0) {
    fprintf(stderr, "ltodump: archive %s has no DCCLTO03 members\n", path);
    VectorDestruct(&blobs);
    VectorDestruct(&lens);
    return false;
  }
  bool ok = true;
  for (size_t i = 0; i < blobs.length; i++) {
    char label[512];
    snprintf(label, sizeof(label), "%s[member %zu]", path, i);
    LoadedLTOModule loaded;
    size_t len = (size_t)(intptr_t)lens.value.p[i];
    if (!LTOArchiveReadFromMemory(blobs.value.p[i], len, &loaded)) {
      fprintf(stderr, "ltodump: failed to read %s\n", label);
      ok = false;
      continue;
    }
    DumpModule(label, &loaded, stdout);
    LoadedLTOModuleDestruct(&loaded);
  }
  for (size_t i = 0; i < blobs.length; i++) {
    free(blobs.value.p[i]);
  }
  VectorDestruct(&blobs);
  VectorDestruct(&lens);
  return ok;
}

static bool DumpPath(const char* path) {
  if (LTOArchiveIsLTOObject(path)) {
    return DumpLoadedPath(path);
  }
  return DumpArchive(path);
}

int main(int argc, char** argv) {
  const char* target = "x86_64";
  Vector files = {0};
  for (int i = 1; i < argc; i++) {
    const char* a = argv[i];
    if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
      Usage(argv[0]);
      return 0;
    }
    if (strcmp(a, "--target") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "ltodump: --target needs a value\n");
        return 1;
      }
      target = argv[++i];
    } else if (strncmp(a, "--target=", 9) == 0) {
      target = a + 9;
    } else if (a[0] == '-' && a[1] != '\0') {
      fprintf(stderr, "ltodump: unknown option %s\n", a);
      Usage(argv[0]);
      return 1;
    } else {
      VectorAppend(&files, (void*)a);
    }
  }
  if (files.length == 0) {
    Usage(argv[0]);
    return 1;
  }

  if (!InitCompiler((const char*)files.value.p[0], target)) {
    VectorDestruct(&files);
    return 1;
  }

  int status = 0;
  for (size_t i = 0; i < files.length; i++) {
    if (!DumpPath((const char*)files.value.p[i])) {
      status = 1;
    }
  }

  CompilerDelete(compiler);
  compiler = NULL;
  ClearAllFiles();
  VectorDestruct(&files);
  return status;
}
