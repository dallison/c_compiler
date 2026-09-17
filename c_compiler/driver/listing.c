//
//  listing.c
//  c_compiler
//
//  Interleaved source listing with optional AST, IR, lowered IR and assembly.
//  Source is shown on IR basic-block boundaries.  The file name is printed
//  only when it changes.  Each source line is emitted at most once.
//

#include "listing.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "basic_block.h"
#include "bitset.h"
#include "codegen.h"
#include "compiler.h"
#include "ir.h"
#include "options.h"
#include "source.h"
#include "symbol.h"
#include "target_basic_block.h"
#include "target_generator.h"
#include "type.h"

static const char* listing_current_file;

static void ReplaceSourceExtension(String* filename, const char* extension) {
  const char* suffixes[] = {".cpp", ".cxx", ".cc", ".c"};
  for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
    const char* suffix = suffixes[i];
    size_t suffix_len = strlen(suffix);
    if (filename->length >= suffix_len &&
        strcmp(filename->value + filename->length - suffix_len, suffix) == 0) {
      filename->value[filename->length - suffix_len] = '\0';
      filename->length -= suffix_len;
      StringAppend(filename, extension);
      return;
    }
  }
  StringAppend(filename, extension);
}

static void ResetListingCursor(int index, File* file, void* data) {
  (void)index;
  (void)data;
  file->listing_last_line = 0;
  BitSetClear(&file->listing_printed);
}

void ListingApplyOptions(Compiler* compiler, Vector* options) {
  bool listing = OptionBoolValue(kOptionListing, options, false);
  compiler->listing_ast = OptionBoolValue(kOptionListingAST, options, false);
  compiler->listing_ir = OptionBoolValue(kOptionListingIR, options, false);
  compiler->listing_lowered =
      OptionBoolValue(kOptionListingLowered, options, false);
  compiler->listing_asm = OptionBoolValue(kOptionListingAsm, options, false);
  compiler->listing_enabled = listing || compiler->listing_ast ||
                              compiler->listing_ir ||
                              compiler->listing_lowered ||
                              compiler->listing_asm;
  // Bare -flisting is a classic source+assembly listing.  The layer flags
  // enable listing on their own, so -flisting-ir does not pull in assembly.
  if (listing) {
    compiler->listing_asm = true;
  }
  String* path = OptionStringValue(kOptionListingFile, options);
  if (path != NULL) {
    StringSet(&compiler->listing_path, path->value);
  }
}

void ListingOpen(Compiler* compiler) {
  listing_current_file = NULL;
  compiler->listing_file = NULL;
  if (!compiler->listing_enabled) {
    return;
  }

  if (compiler->listing_path.length > 0) {
    if (StringEqual(&compiler->listing_path, "-")) {
      compiler->listing_file = stdout;
    } else {
      compiler->listing_file = fopen(compiler->listing_path.value, "w");
    }
  } else {
    String filename;
    StringInit(&filename, compiler->infile.value);
    ReplaceSourceExtension(&filename, ".lst");
    compiler->listing_file = fopen(filename.value, "w");
    StringDestruct(&filename);
  }
  if (compiler->listing_file == NULL) {
    fprintf(stderr, "Cannot open listing file\n");
    compiler->listing_enabled = false;
    return;
  }

  SourceTraverseFiles(NULL, ResetListingCursor);
  FILE* fp = compiler->listing_file;
  fprintf(fp, "# DaveCC listing of %s\n", compiler->infile.value);
  if (compiler->target_name != NULL) {
    fprintf(fp, "# target %s  -O%d\n", compiler->target_name->value,
            compiler->opt_level);
  }
  fprintf(fp, "# sections: source");
  if (compiler->listing_ast) {
    fputs(" ast", fp);
  }
  if (compiler->listing_ir) {
    fputs(" ir", fp);
  }
  if (compiler->listing_lowered) {
    fputs(" lowered", fp);
  }
  if (compiler->listing_asm) {
    fputs(" asm", fp);
  }
  fputc('\n', fp);
  fputc('\n', fp);
}

void ListingClose(Compiler* compiler) {
  if (compiler->listing_file != NULL && compiler->listing_file != stdout) {
    fclose(compiler->listing_file);
  }
  compiler->listing_file = NULL;
  listing_current_file = NULL;
}

static File* FileFromLocation(SourceLocation location, int* lineno) {
  if (location == SOURCE_LOCATION_COMMAND_LINE ||
      location == SOURCE_LOCATION_MISSING) {
    return NULL;
  }
  const char* filename = NULL;
  int line = 0;
  int start = 0;
  int end = 0;
  DecodeSourceLocation(location, &filename, &line, &start, &end);
  if (filename == NULL || line <= 0 ||
      SourceLocationIsSystemHeader(location) ||
      strcmp(filename, "builtin") == 0 ||
      strcmp(filename, "<unknown>") == 0 ||
      strcmp(filename, "command-line") == 0) {
    return NULL;
  }
  if (lineno != NULL) {
    *lineno = line;
  }
  uint32_t file_index =
      (uint32_t)((location >> LOC_FILE_SHIFT) & LOC_FILE_MASK);
  return SourceFileAt(file_index);
}

static void PrintOneSourceLine(FILE* fp, File* file, int lineno) {
  if (file == NULL || lineno <= 0) {
    return;
  }
  if (BitSetContains(&file->listing_printed, (size_t)lineno)) {
    return;
  }
  BitSetInsert(&file->listing_printed, (size_t)lineno);
  if (listing_current_file == NULL ||
      strcmp(listing_current_file, file->name.value) != 0) {
    fprintf(fp, "file %s\n", file->name.value);
    listing_current_file = file->name.value;
  }
  const char* text = SourceFileLineText(file, lineno);
  fprintf(fp, "%6d | %s\n", lineno, text != NULL ? text : "");
}

static void PrintSourceThrough(FILE* fp, File* file, int lineno) {
  if (file == NULL || lineno <= 0) {
    return;
  }
  if (file->listing_last_line > 0 && lineno > file->listing_last_line + 1) {
    for (int line = file->listing_last_line + 1; line < lineno; line++) {
      PrintOneSourceLine(fp, file, line);
    }
  }
  PrintOneSourceLine(fp, file, lineno);
  if (lineno > file->listing_last_line) {
    file->listing_last_line = lineno;
  }
}

static void PrintLocation(FILE* fp, SourceLocation location) {
  int lineno = 0;
  File* file = FileFromLocation(location, &lineno);
  PrintSourceThrough(fp, file, lineno);
}

typedef struct {
  File* file;
  int line;
} ListingPos;

static void MaxLineVisitor(ASTNode* node, void* data, int child_id,
                           VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  ListingPos* pos = data;
  int lineno = 0;
  File* file = FileFromLocation(node->location, &lineno);
  if (file == NULL || file != pos->file) {
    return;
  }
  if (lineno > pos->line) {
    pos->line = lineno;
  }
}

static bool LocationWouldPrint(SourceLocation location) {
  int lineno = 0;
  File* file = FileFromLocation(location, &lineno);
  if (file == NULL || lineno <= 0) {
    return false;
  }
  if (file->listing_last_line > 0 && lineno > file->listing_last_line + 1) {
    for (int line = file->listing_last_line + 1; line < lineno; line++) {
      if (!BitSetContains(&file->listing_printed, (size_t)line)) {
        return true;
      }
    }
  }
  return !BitSetContains(&file->listing_printed, (size_t)lineno);
}

static bool BlockWouldPrintSource(BasicBlock* block) {
  if (BasicBlockIsEmpty(block)) {
    return false;
  }
  for (IRNode* inst = BasicBlockBegin(block); inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (LocationWouldPrint(inst->location)) {
      return true;
    }
  }
  return false;
}

static void PrintBlockSource(FILE* fp, BasicBlock* block) {
  if (BasicBlockIsEmpty(block)) {
    return;
  }
  for (IRNode* inst = BasicBlockBegin(block); inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    PrintLocation(fp, inst->location);
  }
}

static void PrintBlockIR(FILE* fp, BasicBlock* block) {
  if (BasicBlockIsEmpty(block)) {
    return;
  }
  fputs("  IR:\n", fp);
  for (IRNode* inst = BasicBlockBegin(block); inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    fputs("    ", fp);
    IRPrint(inst, fp);
  }
}

static void PrintTargetBlock(FILE* fp, TargetBasicBlock* block,
                             TargetGenerator* target, char* printed) {
  if (block == NULL || TargetBasicBlockIsEmpty(block)) {
    return;
  }
  if (printed[block->block_id]) {
    return;
  }
  printed[block->block_id] = 1;
  fputs("  lowered:\n", fp);
  TargetOpcodeNameFunc name_func =
      target->virtuals != NULL ? target->virtuals->opcode_name
                               : TargetOpcodeName;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
       inst != TargetBasicBlockEnd(block); inst = TargetNext(inst)) {
    fputs("    ", fp);
    TargetPrintInstruction(inst, name_func, fp);
  }
}

static TargetBasicBlock* TargetBlockForIRBlock(BasicBlock* block) {
  if (BasicBlockIsEmpty(block)) {
    return NULL;
  }
  for (IRNode* inst = BasicBlockBegin(block); inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (inst->data.ptr != NULL) {
      TargetInstruction* lowered = (TargetInstruction*)inst->data.ptr;
      if (lowered->block != NULL) {
        return lowered->block;
      }
    }
  }
  return NULL;
}

void ListingEmitFunction(Generator* gen, void* target_code) {
  if (compiler == NULL || !compiler->listing_enabled ||
      compiler->listing_file == NULL || gen == NULL ||
      gen->for_constant_evaluation) {
    return;
  }

  FILE* fp = compiler->listing_file;
  const char* name = "";
  if (gen->func != NULL && TypeIsFunction(gen->func) &&
      gen->func->info.function.symbol != NULL) {
    Symbol* symbol = gen->func->info.function.symbol;
    name = symbol->asm_name.length != 0 ? symbol->asm_name.value
                                        : symbol->name.value;
    fprintf(fp, "function %s\n", name);
    PrintLocation(fp, symbol->location);
  } else {
    fprintf(fp, "function\n");
  }

  if (compiler->listing_ast && gen->func != NULL &&
      TypeIsFunction(gen->func) && gen->func->info.function.body != NULL) {
    fputs("  AST:\n", fp);
    ASTNodePrintTree(gen->func->info.function.body, 2, fp);
  }

  TargetGenerator* target = (TargetGenerator*)target_code;
  char* printed_target_blocks = NULL;
  if (compiler->listing_lowered && target != NULL &&
      target->basic_blocks.length > 0) {
    printed_target_blocks = calloc(target->basic_blocks.length, 1);
  }

  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    if (BasicBlockIsEmpty(block)) {
      continue;
    }
    TargetBasicBlock* target_block =
        printed_target_blocks != NULL ? TargetBlockForIRBlock(block) : NULL;
    bool show_source = BlockWouldPrintSource(block);
    bool show_ir = compiler->listing_ir;
    bool show_lowered = target_block != NULL &&
                        !TargetBasicBlockIsEmpty(target_block) &&
                        !printed_target_blocks[target_block->block_id];
    if (!show_source && !show_ir && !show_lowered) {
      continue;
    }
    fprintf(fp, "BB %zu:\n", block->block_id);
    PrintBlockSource(fp, block);
    if (show_ir) {
      PrintBlockIR(fp, block);
    }
    if (show_lowered) {
      PrintTargetBlock(fp, target_block, target, printed_target_blocks);
    }
  }

  if (printed_target_blocks != NULL) {
    for (size_t i = 0; i < target->basic_blocks.length; i++) {
      TargetBasicBlock* tblock = target->basic_blocks.value.p[i];
      if (TargetBasicBlockIsEmpty(tblock) || printed_target_blocks[i]) {
        continue;
      }
      fprintf(fp, "lowered BB %zu:\n", tblock->block_id);
      PrintTargetBlock(fp, tblock, target, printed_target_blocks);
    }
    free(printed_target_blocks);
  }

  if (gen->func != NULL && TypeIsFunction(gen->func) &&
      gen->func->info.function.body != NULL) {
    ListingPos pos = {0};
    pos.file = FileFromLocation(gen->func->info.function.body->location,
                                &pos.line);
    ASTNodeVisit(gen->func->info.function.body, MaxLineVisitor, 0, &pos);
    PrintSourceThrough(fp, pos.file, pos.line);
  }

  if (compiler->listing_asm && target_code != NULL &&
      compiler->target != NULL &&
      compiler->target->emit_function_assembly != NULL) {
    fputs("  assembly:\n", fp);
    compiler->target->emit_function_assembly(target_code, fp);
  }
  fputc('\n', fp);
}
