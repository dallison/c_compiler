//
//  injection.c
//  c_compiler
//
//  P3294 token-sequence injection frames (queue_injection / namespace_inject).
//

#include "compiler.h"

#include "errors.h"
#include "reflection.h"
#include "reflection_semantics.h"
#include "semantics.h"
#include "statement_parser.h"
#include "statement_semantics.h"
#include "symbol_table.h"
#include "syntax.h"
#include "type_class_internal.h"
#include "type_enum.h"
#include "type_parse.h"

#include <stdlib.h>
#include <string.h>

typedef struct InjectionQueuedSequence {
  ReflectionValue* sequence;
  SourceLocation location;
} InjectionQueuedSequence;

typedef struct InjectionInstalledEntry {
  Namespace* namespace_;
  LocalSymbolTable* local_table;
  Symbol* symbol;
  bool is_tag;
  bool is_global;
} InjectionInstalledEntry;

typedef struct InjectionNamespaceSnapshot {
  Namespace* namespace_;
  size_t alias_count;
  size_t child_count;
} InjectionNamespaceSnapshot;

typedef struct InjectionEnumSnapshot {
  Enum* enumeration;
  TypeRecord* enum_type;
  Symbol* tag;
  size_t constant_count;
  int64_t next_value;
  Type type;
  int size;
  int bit_width;
  bool tag_is_forward_declared;
  bool tag_is_defined;
} InjectionEnumSnapshot;

typedef struct InjectionSyntaxSnapshot {
  Namespace* current_namespace;
  Struct* cxx_class_head;
  Struct* current_class_access_context;
  ParserContext context;
  bool parsing_template_declaration;
  bool parsing_template_specialization;
  bool parsing_template_argument;
  int parsing_consteval_block_depth;
  int current_template_parameter_count;
  Vector* current_template_parameters;
  struct ConstraintExpr* current_template_requires_clause;
  LocalSymbolTable* local_symbol_stack;
  LocalSymbolTable* local_tag_stack;
  TypeRecord* current_function;
} InjectionSyntaxSnapshot;

typedef struct InjectionClassLayoutSnapshot {
  int next_offset;
  int size;
  int non_virtual_size;
  int alignment;
  int next_bit_pos;
  int current_offset;
} InjectionClassLayoutSnapshot;

typedef struct InjectionFrame {
  InjectionTargetKind target_kind;
  Namespace* target_namespace;
  Struct* target_class;
  CXXAccess target_class_access;
  CompoundStatementASTNode* target_function_body;
  Vector* target_function_statements;
  size_t function_insert_index;
  size_t initial_class_member_count;
  size_t initial_function_statement_count;
  size_t initial_all_local_symbol_count;
  InjectionClassLayoutSnapshot initial_class_layout;

  Vector queued_sequences;
  bool owns_temporary_frame;
  bool committed;

  LexCheckpoint lex_checkpoint;
  InjectionSyntaxSnapshot syntax_snapshot;
  int saved_immediate_depth;
  int saved_constant_eval_depth;
  int saved_num_errors;

  Vector installed_entries;
  Vector pending_declaration_roots;
  Vector namespace_snapshots;
  Vector enum_snapshots;
  bool snapshot_initialized;
} InjectionFrame;

static InjectionClassLayoutSnapshot InjectionClassLayoutSave(Struct* str) {
  if (str == NULL) {
    return (InjectionClassLayoutSnapshot){0};
  }
  return (InjectionClassLayoutSnapshot){
      .next_offset = str->next_offset,
      .size = str->size,
      .non_virtual_size = str->non_virtual_size,
      .alignment = str->alignment,
      .next_bit_pos = str->next_bit_pos,
      .current_offset = str->current_offset,
  };
}

static void InjectionClassLayoutRestore(
    Struct* str, const InjectionClassLayoutSnapshot* snapshot) {
  if (str == NULL || snapshot == NULL) {
    return;
  }
  str->next_offset = snapshot->next_offset;
  str->size = snapshot->size;
  str->non_virtual_size = snapshot->non_virtual_size;
  str->alignment = snapshot->alignment;
  str->next_bit_pos = snapshot->next_bit_pos;
  str->current_offset = snapshot->current_offset;
}

static void InjectionQueuedSequenceDelete(InjectionQueuedSequence* entry) {
  if (entry == NULL) {
    return;
  }
  free(entry);
}

static void InjectionInstalledEntryDelete(InjectionInstalledEntry* entry) {
  free(entry);
}

static void InjectionNamespaceSnapshotDelete(
    InjectionNamespaceSnapshot* snapshot) {
  free(snapshot);
}

static void InjectionEnumSnapshotDelete(InjectionEnumSnapshot* snapshot) {
  free(snapshot);
}

static void InjectionSyntaxSnapshotSave(Syntax* syntax,
                                        InjectionSyntaxSnapshot* snapshot) {
  snapshot->current_namespace = syntax->current_namespace;
  snapshot->cxx_class_head = syntax->cxx_class_head;
  snapshot->current_class_access_context =
      compiler->current_class_access_context;
  snapshot->context = syntax->context;
  snapshot->parsing_template_declaration =
      syntax->parsing_template_declaration;
  snapshot->parsing_template_specialization =
      syntax->parsing_template_specialization;
  snapshot->parsing_template_argument = syntax->parsing_template_argument;
  snapshot->parsing_consteval_block_depth =
      syntax->parsing_consteval_block_depth;
  snapshot->current_template_parameter_count =
      syntax->current_template_parameter_count;
  snapshot->current_template_parameters = syntax->current_template_parameters;
  snapshot->current_template_requires_clause =
      syntax->current_template_requires_clause;
  snapshot->local_symbol_stack = syntax->local_symbol_stack;
  snapshot->local_tag_stack = syntax->local_tag_stack;
  snapshot->current_function = compiler->current_function;
}

static void InjectionSyntaxSnapshotRestore(Syntax* syntax,
                                           const InjectionSyntaxSnapshot* snapshot) {
  syntax->current_namespace = snapshot->current_namespace;
  syntax->cxx_class_head = snapshot->cxx_class_head;
  compiler->current_class_access_context =
      snapshot->current_class_access_context;
  syntax->context = snapshot->context;
  syntax->parsing_template_declaration =
      snapshot->parsing_template_declaration;
  syntax->parsing_template_specialization =
      snapshot->parsing_template_specialization;
  syntax->parsing_template_argument = snapshot->parsing_template_argument;
  syntax->parsing_consteval_block_depth =
      snapshot->parsing_consteval_block_depth;
  syntax->current_template_parameter_count =
      snapshot->current_template_parameter_count;
  syntax->current_template_parameters = snapshot->current_template_parameters;
  syntax->current_template_requires_clause =
      snapshot->current_template_requires_clause;
  syntax->local_symbol_stack = snapshot->local_symbol_stack;
  syntax->local_tag_stack = snapshot->local_tag_stack;
  compiler->current_function = snapshot->current_function;
}

static InjectionFrame* InjectionActiveFrame(void) {
  if (compiler == NULL || compiler->injection_frames.length == 0) {
    return NULL;
  }
  return compiler->injection_frames.value.p[
      compiler->injection_frames.length - 1];
}

static void InjectionFrameDelete(InjectionFrame* frame) {
  if (frame == NULL) {
    return;
  }
  for (size_t i = 0; i < frame->queued_sequences.length; i++) {
    InjectionQueuedSequenceDelete(frame->queued_sequences.value.p[i]);
  }
  for (size_t i = 0; i < frame->installed_entries.length; i++) {
    InjectionInstalledEntryDelete(frame->installed_entries.value.p[i]);
  }
  for (size_t i = 0; i < frame->pending_declaration_roots.length; i++) {
    ASTNodeDelete(frame->pending_declaration_roots.value.p[i]);
  }
  for (size_t i = 0; i < frame->namespace_snapshots.length; i++) {
    InjectionNamespaceSnapshotDelete(frame->namespace_snapshots.value.p[i]);
  }
  for (size_t i = 0; i < frame->enum_snapshots.length; i++) {
    InjectionEnumSnapshotDelete(frame->enum_snapshots.value.p[i]);
  }
  VectorDestruct(&frame->queued_sequences);
  VectorDestruct(&frame->installed_entries);
  VectorDestruct(&frame->pending_declaration_roots);
  VectorDestruct(&frame->namespace_snapshots);
  VectorDestruct(&frame->enum_snapshots);
  if (frame->snapshot_initialized) {
    LexCheckpointDestruct(&frame->lex_checkpoint);
  }
  free(frame);
}

static void InjectionRecordNamespaceSnapshot(InjectionFrame* frame,
                                             Namespace* namespace_) {
  if (frame == NULL || namespace_ == NULL) {
    return;
  }
  for (size_t i = 0; i < frame->namespace_snapshots.length; i++) {
    InjectionNamespaceSnapshot* existing =
        frame->namespace_snapshots.value.p[i];
    if (existing != NULL && existing->namespace_ == namespace_) {
      return;
    }
  }
  InjectionNamespaceSnapshot* snapshot = calloc(1, sizeof(*snapshot));
  snapshot->namespace_ = namespace_;
  snapshot->alias_count = namespace_->namespace_aliases.length;
  snapshot->child_count = namespace_->children.length;
  VectorAppend(&frame->namespace_snapshots, snapshot);
}

static void InjectionRestoreEnumSnapshot(InjectionEnumSnapshot* snapshot) {
  if (snapshot == NULL || snapshot->enumeration == NULL ||
      snapshot->enum_type == NULL || snapshot->tag == NULL) {
    return;
  }
  EnumRemoveConstantsFrom(snapshot->enumeration, snapshot->constant_count);
  snapshot->enumeration->next_value = snapshot->next_value;
  snapshot->enum_type->type = snapshot->type;
  snapshot->enum_type->size = snapshot->size;
  snapshot->enum_type->bit_width = snapshot->bit_width;
  snapshot->tag->flags.is_forward_declared =
      snapshot->tag_is_forward_declared;
  snapshot->tag->flags.is_defined = snapshot->tag_is_defined;
}

static void InjectionFrameRestoreState(InjectionFrame* frame) {
  if (frame == NULL || !frame->snapshot_initialized || compiler == NULL) {
    return;
  }
  LexCheckpointRestore(compiler->syntax.lex, &frame->lex_checkpoint);
  InjectionSyntaxSnapshotRestore(&compiler->syntax, &frame->syntax_snapshot);
  compiler->immediate_function_context_depth = frame->saved_immediate_depth;
  compiler->constant_evaluation_required_depth =
      frame->saved_constant_eval_depth;
}

static void InjectionUninstallEntries(InjectionFrame* frame) {
  for (size_t i = frame->installed_entries.length; i > 0; i--) {
    InjectionInstalledEntry* entry =
        frame->installed_entries.value.p[i - 1];
    if (entry != NULL && entry->symbol != NULL) {
      if (entry->local_table != NULL) {
        UninstallLocalSymbol(entry->local_table, entry->symbol);
      } else if (entry->is_global) {
        UninstallGlobalSymbol(entry->symbol, entry->is_tag);
      } else if (entry->namespace_ != NULL) {
        UninstallNamespaceSymbol(entry->namespace_, entry->symbol,
                                 entry->is_tag);
      }
    }
    InjectionInstalledEntryDelete(entry);
  }
  VectorClear(&frame->installed_entries);
}

static void InjectionRollbackTargetMutations(InjectionFrame* frame) {
  for (size_t i = frame->enum_snapshots.length; i > 0; i--) {
    InjectionRestoreEnumSnapshot(frame->enum_snapshots.value.p[i - 1]);
  }
  for (size_t i = frame->namespace_snapshots.length; i > 0; i--) {
    InjectionNamespaceSnapshot* snapshot =
        frame->namespace_snapshots.value.p[i - 1];
    if (snapshot != NULL) {
      NamespaceRollbackToSizes(snapshot->namespace_, snapshot->alias_count,
                               snapshot->child_count);
    }
  }
  if (frame->target_class != NULL) {
    while (frame->target_class->members.length >
           frame->initial_class_member_count) {
      size_t last = frame->target_class->members.length - 1;
      StructMember* member = frame->target_class->members.value.p[last];
      VectorDeleteElement(&frame->target_class->members, last);
      StructMemberDelete(member);
    }
    InjectionClassLayoutRestore(frame->target_class,
                                &frame->initial_class_layout);
    StructRebuildMemberLookupTables(frame->target_class);
  }
  Vector* function_statements =
      frame->target_function_statements != NULL
          ? frame->target_function_statements
          : (frame->target_function_body != NULL
                 ? frame->target_function_body->statements
                 : NULL);
  if (function_statements != NULL) {
    while (function_statements->length >
           frame->initial_function_statement_count) {
      size_t last = function_statements->length - 1;
      ASTNode* statement = function_statements->value.p[last];
      VectorDeleteElement(function_statements, last);
      ASTNodeDelete(statement);
    }
  }
  if (compiler != NULL &&
      compiler->syntax.all_local_symbols.length >
          frame->initial_all_local_symbol_count) {
    compiler->syntax.all_local_symbols.length =
        frame->initial_all_local_symbol_count;
  }
}

static void InjectionRecordInstalledSymbol(InjectionFrame* frame, Symbol* symbol,
                                           bool is_tag) {
  if (frame == NULL || symbol == NULL) {
    return;
  }
  Namespace* ns = symbol->namespace_;
  if (ns == NULL) {
    return;
  }
  for (size_t i = 0; i < frame->installed_entries.length; i++) {
    InjectionInstalledEntry* existing = frame->installed_entries.value.p[i];
    if (existing != NULL && existing->symbol == symbol &&
        existing->is_tag == is_tag) {
      return;
    }
  }
  InjectionInstalledEntry* entry = calloc(1, sizeof(*entry));
  entry->namespace_ = ns;
  entry->symbol = symbol;
  entry->is_tag = is_tag;
  VectorAppend(&frame->installed_entries, entry);
}

void CompilerRecordInjectedSymbol(Namespace* namespace_,
                                  LocalSymbolTable* local_table, Symbol* symbol,
                                  bool is_tag, bool is_global) {
  InjectionFrame* frame = InjectionActiveFrame();
  if (frame == NULL || symbol == NULL) {
    return;
  }
  for (size_t i = 0; i < frame->installed_entries.length; i++) {
    InjectionInstalledEntry* existing = frame->installed_entries.value.p[i];
    if (existing != NULL && existing->symbol == symbol &&
        existing->is_tag == is_tag) {
      return;
    }
  }
  InjectionInstalledEntry* entry = calloc(1, sizeof(*entry));
  entry->namespace_ = namespace_;
  entry->local_table = local_table;
  entry->symbol = symbol;
  entry->is_tag = is_tag;
  entry->is_global = is_global;
  VectorAppend(&frame->installed_entries, entry);
}

static Vector* InjectionSequenceTokens(const ReflectionValue* sequence) {
  if (sequence == NULL || sequence->kind != kReflectionTokenSequence) {
    return NULL;
  }
  Vector* tokens = NewVector();
  for (size_t i = 0; i < sequence->token_sequence.length; i++) {
    VectorAppend(tokens,
                 TokenSequenceTokenCopy(sequence->token_sequence.value.p[i]));
  }
  return tokens;
}

static Namespace* InjectionNamespaceFromReflection(const ReflectionValue* value) {
  if (value == NULL) {
    return NULL;
  }
  if (value->kind == kReflectionGlobalNamespace) {
    return compiler->global_namespace;
  }
  if (value->kind == kReflectionNamespace) {
    return value->namespace_;
  }
  if (value->kind == kReflectionNamespaceAlias) {
    return value->namespace_alias_target;
  }
  return NULL;
}

static bool InjectionReplayFinished(Lex* lex) {
  return LexLookingAt(lex, TOK(eof));
}

static bool InjectionParseReplayedExternal(Syntax* syntax, InjectionFrame* frame,
                                         SourceLocation location) {
  bool trapped_saved = DiagnosticErrorTrapBegin();
  int errors_before = compiler->num_errors;

  while (!InjectionReplayFinished(syntax->lex)) {
    if (LexLookingAt(syntax->lex, TOK(semicolon))) {
      LexNextToken(syntax->lex);
      continue;
    }
    ASTNode* decl = SyntaxParseExternalDeclaration(syntax);
    if (decl == NULL) {
      continue;
    }
    VectorAppend(&frame->pending_declaration_roots, decl);
    if (decl->op == AST_OP(decl_list)) {
      DeclarationListASTNode* list = (DeclarationListASTNode*)decl;
      for (size_t i = 0; list->declarations != NULL &&
                         i < list->declarations->length;
           i++) {
        ASTNode* child = list->declarations->value.p[i];
        if (child != NULL && child->op == AST_OP(vardecl)) {
          VariableDeclarationASTNode* var =
              (VariableDeclarationASTNode*)child;
          if (var->symbol != NULL) {
            InjectionRecordInstalledSymbol(
                frame, var->symbol, SymbolIsTagSymbol(var->symbol));
          }
        }
      }
    } else if (decl->op == AST_OP(vardecl)) {
      VariableDeclarationASTNode* var = (VariableDeclarationASTNode*)decl;
      if (var->symbol != NULL) {
        InjectionRecordInstalledSymbol(
            frame, var->symbol, SymbolIsTagSymbol(var->symbol));
      }
    }
  }

  bool ok = !DiagnosticErrorTrapped() && compiler->num_errors == errors_before;
  DiagnosticErrorTrapEnd(trapped_saved);
  if (!ok) {
    SyntaxErrorAtLocation(syntax, location,
                          "injected declaration could not be parsed");
  }
  return ok;
}

static bool InjectionParseReplayedClassMember(Syntax* syntax,
                                              InjectionFrame* frame,
                                              SourceLocation location) {
  bool trapped_saved = DiagnosticErrorTrapBegin();
  int errors_before = compiler->num_errors;
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 kParsingStructOrUnion);
  parser.cxx_member_owner = frame->target_class;
  while (!InjectionReplayFinished(syntax->lex)) {
    if (LexLookingAt(syntax->lex, TOK(semicolon))) {
      LexNextToken(syntax->lex);
      continue;
    }
    if (!ParseInjectedClassMember(&parser, frame->target_class,
                                  frame->target_class_access, location)) {
      break;
    }
  }
  TypeParserDestruct(&parser);
  bool ok = !DiagnosticErrorTrapped() && compiler->num_errors == errors_before &&
            InjectionReplayFinished(syntax->lex);
  DiagnosticErrorTrapEnd(trapped_saved);
  if (!ok) {
    SyntaxErrorAtLocation(syntax, location,
                          "injected class member could not be parsed");
  }
  return ok;
}

static bool InjectionParseReplayedFunctionBody(Syntax* syntax,
                                               InjectionFrame* frame,
                                               SourceLocation location) {
  bool trapped_saved = DiagnosticErrorTrapBegin();
  int errors_before = compiler->num_errors;
  bool ok = true;

  while (!InjectionReplayFinished(syntax->lex)) {
    if (LexLookingAt(syntax->lex, TOK(semicolon))) {
      LexNextToken(syntax->lex);
      continue;
    }
    ParserContext saved_context = syntax->context;
    syntax->context = kParsingBlockScope;
    ASTNode* decl = SyntaxParseLocalDeclaration(syntax);
    syntax->context = saved_context;
    if (decl == NULL || decl->op != AST_OP(decl_list)) {
      SyntaxErrorAtLocation(
          syntax, location,
          "injected function statement must be a declaration");
      ok = false;
      break;
    }
    if (frame->target_function_body != NULL) {
      CompoundASTNodeInsertStatement(frame->target_function_body, decl,
                                     frame->function_insert_index++);
    } else if (frame->target_function_statements != NULL) {
      if (frame->function_insert_index >=
          frame->target_function_statements->length) {
        VectorAppend(frame->target_function_statements, decl);
      } else {
        VectorInsertBefore(frame->target_function_statements,
                           frame->function_insert_index, decl);
      }
      frame->function_insert_index++;
    }
  }

  ok = ok && !DiagnosticErrorTrapped() &&
       compiler->num_errors == errors_before;
  DiagnosticErrorTrapEnd(trapped_saved);
  if (!ok) {
    (void)location;
  }
  return ok;
}

static bool InjectionDrainOneSequence(Syntax* syntax, InjectionFrame* frame,
                                      ReflectionValue* sequence,
                                      SourceLocation location,
                                      InjectionTargetKind target_kind,
                                      Namespace* target_namespace,
                                      Struct* target_class,
                                      CXXAccess target_class_access,
                                      CompoundStatementASTNode* target_body,
                                      size_t* function_insert_index) {
  Vector* tokens = InjectionSequenceTokens(sequence);
  if (tokens == NULL) {
    VectorDeleteWithContents(tokens,
                             (VectorElementDestructor)TokenSequenceTokenDelete,
                             /*free_element=*/false);
    return false;
  }
  if (tokens->length == 0) {
    VectorDelete(tokens);
    return true;
  }
  InjectionRecordNamespaceSnapshot(frame, target_namespace);

  LexCheckpoint saved_lex;
  LexCheckpointSave(syntax->lex, &saved_lex);
  InjectionSyntaxSnapshot saved_syntax;
  InjectionSyntaxSnapshotSave(syntax, &saved_syntax);
  int saved_immediate = compiler->immediate_function_context_depth;
  int saved_constant = compiler->constant_evaluation_required_depth;
  void* diag_state = DiagnosticSnapshotState();

  frame->target_kind = target_kind;
  frame->target_namespace = target_namespace;
  frame->target_class = target_class;
  frame->target_class_access = target_class_access;
  frame->target_function_body = target_body;
  if (function_insert_index != NULL) {
    frame->function_insert_index = *function_insert_index;
  }

  if (target_kind == kInjectionTargetNamespace) {
    syntax->current_namespace = target_namespace != NULL ? target_namespace
                                                         : compiler->global_namespace;
    syntax->context = kParsingFileScope;
    syntax->local_symbol_stack = NULL;
    syntax->local_tag_stack = NULL;
  } else if (target_kind == kInjectionTargetClass) {
    syntax->cxx_class_head = target_class;
    syntax->context = kParsingStructOrUnion;
    compiler->current_class_access_context = target_class;
    syntax->local_symbol_stack = NULL;
    syntax->local_tag_stack = NULL;
  } else {
    syntax->context = kParsingBlockScope;
  }

  size_t installed_before = frame->installed_entries.length;
  size_t pending_before = frame->pending_declaration_roots.length;
  size_t namespace_aliases_before =
      target_namespace != NULL ? target_namespace->namespace_aliases.length : 0;
  size_t namespace_children_before =
      target_namespace != NULL ? target_namespace->children.length : 0;
  size_t class_members_before =
      target_class != NULL ? target_class->members.length : 0;
  InjectionClassLayoutSnapshot class_layout_before =
      InjectionClassLayoutSave(target_class);
  Vector* target_statements =
      target_body != NULL
          ? target_body->statements
          : (target_kind == kInjectionTargetFunction
                 ? frame->target_function_statements
                 : NULL);
  size_t function_statements_before =
      target_statements != NULL ? target_statements->length : 0;

  LexBeginTokenReplay(syntax->lex, tokens);
  bool parsed = false;
  switch (target_kind) {
    case kInjectionTargetNamespace:
      parsed = InjectionParseReplayedExternal(syntax, frame, location);
      break;
    case kInjectionTargetClass:
      parsed = InjectionParseReplayedClassMember(syntax, frame, location);
      break;
    case kInjectionTargetFunction:
      parsed = InjectionParseReplayedFunctionBody(syntax, frame, location);
      if (parsed && function_insert_index != NULL) {
        *function_insert_index = frame->function_insert_index;
      }
      break;
  }
  LexEndTokenReplay(syntax->lex);

  if (!parsed) {
    while (frame->installed_entries.length > installed_before) {
      size_t last = frame->installed_entries.length - 1;
      InjectionInstalledEntry* entry =
          frame->installed_entries.value.p[last];
      if (entry != NULL && entry->symbol != NULL) {
        if (entry->local_table != NULL) {
          UninstallLocalSymbol(entry->local_table, entry->symbol);
        } else if (entry->is_global) {
          UninstallGlobalSymbol(entry->symbol, entry->is_tag);
        } else if (entry->namespace_ != NULL) {
          UninstallNamespaceSymbol(entry->namespace_, entry->symbol,
                                   entry->is_tag);
        }
      }
      VectorDeleteElement(&frame->installed_entries, last);
      InjectionInstalledEntryDelete(entry);
    }
    while (frame->pending_declaration_roots.length > pending_before) {
      size_t last = frame->pending_declaration_roots.length - 1;
      ASTNode* root = frame->pending_declaration_roots.value.p[last];
      VectorDeleteElement(&frame->pending_declaration_roots, last);
      ASTNodeDelete(root);
    }
    if (target_namespace != NULL) {
      NamespaceRollbackToSizes(target_namespace, namespace_aliases_before,
                               namespace_children_before);
    }
    if (target_class != NULL) {
      while (target_class->members.length > class_members_before) {
        size_t last = target_class->members.length - 1;
        StructMember* member = target_class->members.value.p[last];
        VectorDeleteElement(&target_class->members, last);
        StructMemberDelete(member);
      }
      InjectionClassLayoutRestore(target_class, &class_layout_before);
      StructRebuildMemberLookupTables(target_class);
    }
    if (target_statements != NULL) {
      while (target_statements->length > function_statements_before) {
        size_t last = target_statements->length - 1;
        ASTNode* statement = target_statements->value.p[last];
        VectorDeleteElement(target_statements, last);
        ASTNodeDelete(statement);
      }
    }
  }

  LexCheckpointRestore(syntax->lex, &saved_lex);
  LexCheckpointDestruct(&saved_lex);
  InjectionSyntaxSnapshotRestore(syntax, &saved_syntax);
  compiler->immediate_function_context_depth = saved_immediate;
  compiler->constant_evaluation_required_depth = saved_constant;
  DiagnosticSwapState(diag_state);
  DiagnosticFreeState(diag_state);

  VectorDeleteWithContents(tokens,
                           (VectorElementDestructor)TokenSequenceTokenDelete,
                           /*free_element=*/false);
  return parsed;
}

static bool InjectionDrainFrame(Syntax* syntax, InjectionFrame* frame) {
  if (frame == NULL || frame->committed) {
    return true;
  }

  bool ok = true;
  size_t function_insert = frame->function_insert_index;
  for (size_t i = 0; i < frame->queued_sequences.length; i++) {
    InjectionQueuedSequence* queued = frame->queued_sequences.value.p[i];
    if (queued == NULL || queued->sequence == NULL) {
      ok = false;
      break;
    }
    if (!InjectionDrainOneSequence(
            syntax, frame, queued->sequence, queued->location,
            frame->target_kind, frame->target_namespace, frame->target_class,
            frame->target_class_access, frame->target_function_body,
            frame->target_kind == kInjectionTargetFunction ? &function_insert
                                                           : NULL)) {
      ok = false;
      break;
    }
  }
  frame->function_insert_index = function_insert;

  if (ok) {
    InjectionFrame* parent = NULL;
    if (compiler->injection_frames.length > 1 &&
        compiler->injection_frames.value.p[
            compiler->injection_frames.length - 1] == frame) {
      parent = compiler->injection_frames.value.p[
          compiler->injection_frames.length - 2];
    }
    for (size_t i = 0; i < frame->pending_declaration_roots.length; i++) {
      ASTNode* root = frame->pending_declaration_roots.value.p[i];
      if (parent != NULL) {
        VectorAppend(&parent->pending_declaration_roots, root);
      } else {
        CompilerQueueInjectedDeclaration(root);
      }
    }
    VectorClear(&frame->pending_declaration_roots);
    if (parent != NULL) {
      for (size_t i = 0; i < frame->installed_entries.length; i++) {
        InjectionInstalledEntry* entry = frame->installed_entries.value.p[i];
        if (entry != NULL && entry->local_table != NULL &&
            parent->target_kind != kInjectionTargetFunction) {
          InjectionInstalledEntryDelete(entry);
        } else {
          VectorAppend(&parent->installed_entries, entry);
        }
      }
      VectorClear(&frame->installed_entries);
      for (size_t i = 0; i < frame->namespace_snapshots.length; i++) {
        InjectionNamespaceSnapshot* snapshot =
            frame->namespace_snapshots.value.p[i];
        bool already_recorded = false;
        for (size_t j = 0; j < parent->namespace_snapshots.length; j++) {
          InjectionNamespaceSnapshot* parent_snapshot =
              parent->namespace_snapshots.value.p[j];
          if (parent_snapshot != NULL && snapshot != NULL &&
              parent_snapshot->namespace_ == snapshot->namespace_) {
            already_recorded = true;
            break;
          }
        }
        if (already_recorded) {
          InjectionNamespaceSnapshotDelete(snapshot);
        } else {
          VectorAppend(&parent->namespace_snapshots, snapshot);
        }
      }
      VectorClear(&frame->namespace_snapshots);
      for (size_t i = 0; i < frame->enum_snapshots.length; i++) {
        InjectionEnumSnapshot* snapshot = frame->enum_snapshots.value.p[i];
        bool already_recorded = false;
        for (size_t j = 0; j < parent->enum_snapshots.length; j++) {
          InjectionEnumSnapshot* parent_snapshot =
              parent->enum_snapshots.value.p[j];
          if (parent_snapshot != NULL && snapshot != NULL &&
              parent_snapshot->enumeration == snapshot->enumeration) {
            already_recorded = true;
            break;
          }
        }
        if (already_recorded) {
          InjectionEnumSnapshotDelete(snapshot);
        } else {
          VectorAppend(&parent->enum_snapshots, snapshot);
        }
      }
      VectorClear(&frame->enum_snapshots);
    }
    frame->committed = true;
    for (size_t i = 0; i < frame->queued_sequences.length; i++) {
      InjectionQueuedSequenceDelete(frame->queued_sequences.value.p[i]);
    }
    frame->queued_sequences.length = 0;
  } else {
    InjectionUninstallEntries(frame);
    for (size_t i = 0; i < frame->pending_declaration_roots.length; i++) {
      ASTNodeDelete(frame->pending_declaration_roots.value.p[i]);
    }
    VectorClear(&frame->pending_declaration_roots);
  }

  return ok;
}

static InjectionTargetKind InjectionInferTargetKind(Syntax* syntax,
                                                    Namespace** out_ns,
                                                    Struct** out_class,
                                                    CompoundStatementASTNode** out_body,
                                                    size_t* out_insert_index) {
  if (syntax->cxx_class_head != NULL) {
    *out_class = syntax->cxx_class_head;
    *out_ns = NULL;
    *out_body = NULL;
    *out_insert_index = 0;
    return kInjectionTargetClass;
  }
  if (compiler->current_function != NULL &&
      syntax->local_symbol_stack != NULL &&
      syntax->local_symbol_stack->prev != NULL) {
    TypeRecord* func = compiler->current_function;
    if (func->info.function.body != NULL &&
        func->info.function.body->op == AST_OP(compound)) {
      *out_body = (CompoundStatementASTNode*)func->info.function.body;
      *out_insert_index = out_body != NULL && *out_body != NULL &&
                                  (*out_body)->statements != NULL
                              ? (*out_body)->statements->length
                              : 0;
      *out_class = NULL;
      *out_ns = syntax->current_namespace != NULL ? syntax->current_namespace
                                                  : compiler->global_namespace;
      return kInjectionTargetFunction;
    }
  }
  *out_ns = syntax->current_namespace != NULL ? syntax->current_namespace
                                              : compiler->global_namespace;
  *out_class = NULL;
  *out_body = NULL;
  *out_insert_index = 0;
  return kInjectionTargetNamespace;
}

bool CompilerInjectionFrameActive(void) {
  return InjectionActiveFrame() != NULL;
}

void CompilerRecordSynthesizedEnum(Enum* enumeration, TypeRecord* enum_type,
                                   Symbol* tag) {
  InjectionFrame* frame = InjectionActiveFrame();
  if (frame == NULL || enumeration == NULL || enum_type == NULL || tag == NULL) {
    return;
  }
  for (size_t i = 0; i < frame->enum_snapshots.length; i++) {
    InjectionEnumSnapshot* existing = frame->enum_snapshots.value.p[i];
    if (existing != NULL && existing->enumeration == enumeration) {
      return;
    }
  }
  InjectionEnumSnapshot* snapshot = calloc(1, sizeof(*snapshot));
  snapshot->enumeration = enumeration;
  snapshot->enum_type = enum_type;
  snapshot->tag = tag;
  snapshot->constant_count = enumeration->constants.length;
  snapshot->next_value = enumeration->next_value;
  snapshot->type = enum_type->type;
  snapshot->size = enum_type->size;
  snapshot->bit_width = enum_type->bit_width;
  snapshot->tag_is_forward_declared = tag->flags.is_forward_declared;
  snapshot->tag_is_defined = tag->flags.is_defined;
  VectorAppend(&frame->enum_snapshots, snapshot);
}

bool CompilerBeginInjectionFrame(InjectionTargetKind kind, struct Namespace* target_ns,
                                   struct Struct* target_class,
                                   int target_class_access,
                                   struct ASTNode* target_body,
                                   size_t function_insert_index,
                                   bool temporary) {
  if (compiler == NULL) {
    return false;
  }
  InjectionFrame* frame = calloc(1, sizeof(*frame));
  frame->target_kind = kind;
  frame->target_namespace = target_ns;
  frame->target_class = target_class;
  frame->target_class_access = (CXXAccess)target_class_access;
  frame->target_function_body =
      target_body != NULL && target_body->op == AST_OP(compound)
          ? (CompoundStatementASTNode*)target_body
          : NULL;
  frame->target_function_statements = NULL;
  frame->function_insert_index = function_insert_index;
  frame->initial_class_member_count =
      target_class != NULL ? target_class->members.length : 0;
  frame->initial_class_layout = InjectionClassLayoutSave(target_class);
  frame->initial_function_statement_count =
      frame->target_function_body != NULL &&
              frame->target_function_body->statements != NULL
          ? frame->target_function_body->statements->length
          : 0;
  frame->initial_all_local_symbol_count =
      compiler->syntax.all_local_symbols.length;
  LexCheckpointSave(compiler->syntax.lex, &frame->lex_checkpoint);
  InjectionSyntaxSnapshotSave(&compiler->syntax, &frame->syntax_snapshot);
  frame->saved_immediate_depth = compiler->immediate_function_context_depth;
  frame->saved_constant_eval_depth =
      compiler->constant_evaluation_required_depth;
  frame->saved_num_errors = compiler->num_errors;
  frame->snapshot_initialized = true;
  frame->owns_temporary_frame = temporary;
  VectorInit(&frame->queued_sequences);
  VectorInit(&frame->installed_entries);
  VectorInit(&frame->pending_declaration_roots);
  VectorInit(&frame->namespace_snapshots);
  VectorInit(&frame->enum_snapshots);
  InjectionRecordNamespaceSnapshot(frame, target_ns);
  VectorAppend(&compiler->injection_frames, frame);
  return true;
}

bool CompilerBeginFunctionInjectionFrame(Vector* statements,
                                         size_t function_insert_index) {
  if (!CompilerBeginInjectionFrame(
          kInjectionTargetFunction, NULL, NULL, kAccessPublic, NULL,
          function_insert_index, /*temporary=*/false)) {
    return false;
  }
  InjectionFrame* frame = InjectionActiveFrame();
  frame->target_function_statements = statements;
  frame->initial_function_statement_count =
      statements != NULL ? statements->length : 0;
  return true;
}

bool CompilerBeginInjectionFrameForSyntax(Syntax* syntax, int class_access) {
  Namespace* ns = NULL;
  Struct* cls = NULL;
  CompoundStatementASTNode* body = NULL;
  size_t insert_index = 0;
  InjectionTargetKind kind =
      InjectionInferTargetKind(syntax, &ns, &cls, &body, &insert_index);
  return CompilerBeginInjectionFrame(kind, ns, cls, class_access,
                                     (ASTNode*)body, insert_index,
                                     /*temporary=*/false);
}

void CompilerRollbackInjectionFrame(void) {
  if (compiler == NULL || compiler->injection_frames.length == 0) {
    return;
  }
  size_t last = compiler->injection_frames.length - 1;
  InjectionFrame* frame = compiler->injection_frames.value.p[last];
  VectorDeleteElement(&compiler->injection_frames, last);
  InjectionUninstallEntries(frame);
  for (size_t i = 0; i < frame->pending_declaration_roots.length; i++) {
    ASTNodeDelete(frame->pending_declaration_roots.value.p[i]);
  }
  VectorClear(&frame->pending_declaration_roots);
  InjectionRollbackTargetMutations(frame);
  InjectionFrameRestoreState(frame);
  InjectionFrameDelete(frame);
}

bool CompilerCommitInjectionFrame(Syntax* syntax) {
  if (compiler == NULL || compiler->injection_frames.length == 0) {
    return false;
  }
  InjectionFrame* frame = InjectionActiveFrame();
  if (frame == NULL) {
    return false;
  }
  bool ok = InjectionDrainFrame(syntax, frame);
  if (!ok) {
    CompilerRollbackInjectionFrame();
    return false;
  }
  size_t last = compiler->injection_frames.length - 1;
  VectorDeleteElement(&compiler->injection_frames, last);
  InjectionFrameDelete(frame);
  return true;
}

static bool CompilerQueueInjectionOnFrame(InjectionFrame* frame,
                                          ReflectionValue* sequence,
                                          SourceLocation location) {
  if (frame == NULL || sequence == NULL ||
      sequence->kind != kReflectionTokenSequence) {
    return false;
  }
  ReflectionValue* canonical = ReflectionCanonicalize(sequence);
  if (canonical == NULL) {
    return false;
  }
  InjectionQueuedSequence* queued = calloc(1, sizeof(*queued));
  queued->sequence = canonical;
  queued->location = location;
  VectorAppend(&frame->queued_sequences, queued);
  return true;
}

bool CompilerQueueInjection(ReflectionValue* sequence, SourceLocation location,
                            ASTNode* diagnostic) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX29)) {
    SemanticError(diagnostic, "queue_injection requires C++29");
    return false;
  }
  if (sequence == NULL || sequence->kind != kReflectionTokenSequence) {
    SemanticError(diagnostic,
                  "queue_injection requires a constant token sequence");
    return false;
  }

  InjectionFrame* frame = InjectionActiveFrame();
  if (frame == NULL) {
    if (compiler->constant_evaluation_required_depth <= 0 &&
        compiler->immediate_function_context_depth <= 0) {
      SemanticError(
          diagnostic,
          "queue_injection requires an active mandatory constant evaluation");
      return false;
    }
    Syntax* syntax = &compiler->syntax;
    Namespace* ns = NULL;
    Struct* cls = NULL;
    CompoundStatementASTNode* body = NULL;
    size_t insert_index = 0;
    InjectionTargetKind kind =
        InjectionInferTargetKind(syntax, &ns, &cls, &body, &insert_index);
    if (!CompilerBeginInjectionFrame(kind, ns, cls, kAccessPublic, (ASTNode*)body,
                                     insert_index, /*temporary=*/true)) {
      return false;
    }
    frame = InjectionActiveFrame();
    if (!CompilerQueueInjectionOnFrame(frame, sequence, location)) {
      CompilerRollbackInjectionFrame();
      return false;
    }
    bool ok = CompilerCommitInjectionFrame(syntax);
    if (!ok) {
      SemanticError(diagnostic, "queue_injection failed");
    }
    return ok;
  }

  if (!CompilerQueueInjectionOnFrame(frame, sequence, location)) {
    SemanticError(diagnostic, "queue_injection failed");
    return false;
  }
  return true;
}

bool CompilerNamespaceInject(ReflectionValue* ns_value,
                             ReflectionValue* sequence, SourceLocation location,
                             ASTNode* diagnostic) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX29)) {
    SemanticError(diagnostic, "namespace_inject requires C++29");
    return false;
  }
  if (sequence == NULL || sequence->kind != kReflectionTokenSequence) {
    SemanticError(diagnostic,
                  "namespace_inject requires a constant token sequence");
    return false;
  }
  Namespace* target = InjectionNamespaceFromReflection(ns_value);
  if (target == NULL) {
    SemanticError(diagnostic,
                  "namespace_inject requires a namespace reflection");
    return false;
  }

  Syntax* syntax = &compiler->syntax;
  InjectionFrame* active = InjectionActiveFrame();
  if (active != NULL) {
    InjectionTargetKind saved_kind = active->target_kind;
    Namespace* saved_namespace = active->target_namespace;
    Struct* saved_class = active->target_class;
    CXXAccess saved_access = active->target_class_access;
    CompoundStatementASTNode* saved_body = active->target_function_body;
    Vector* saved_statements = active->target_function_statements;
    size_t saved_insert_index = active->function_insert_index;
    bool ok = InjectionDrainOneSequence(
        syntax, active, sequence, location, kInjectionTargetNamespace, target,
        NULL, kAccessPublic, NULL, NULL);
    active->target_kind = saved_kind;
    active->target_namespace = saved_namespace;
    active->target_class = saved_class;
    active->target_class_access = saved_access;
    active->target_function_body = saved_body;
    active->target_function_statements = saved_statements;
    active->function_insert_index = saved_insert_index;
    if (!ok) {
      SemanticError(diagnostic, "namespace_inject failed");
    }
    return ok;
  }

  InjectionFrame scratch = {0};
  VectorInit(&scratch.installed_entries);
  VectorInit(&scratch.pending_declaration_roots);
  VectorInit(&scratch.namespace_snapshots);
  VectorInit(&scratch.enum_snapshots);
  bool ok = InjectionDrainOneSequence(syntax, &scratch, sequence, location,
                                      kInjectionTargetNamespace, target, NULL,
                                      kAccessPublic, NULL, NULL);
  if (ok) {
    for (size_t i = 0; i < scratch.pending_declaration_roots.length; i++) {
      CompilerQueueInjectedDeclaration(
          scratch.pending_declaration_roots.value.p[i]);
    }
  } else {
    SemanticError(diagnostic, "namespace_inject failed");
    InjectionUninstallEntries(&scratch);
    for (size_t i = 0; i < scratch.pending_declaration_roots.length; i++) {
      ASTNodeDelete(scratch.pending_declaration_roots.value.p[i]);
    }
    InjectionRollbackTargetMutations(&scratch);
  }
  for (size_t i = 0; i < scratch.installed_entries.length; i++) {
    InjectionInstalledEntryDelete(scratch.installed_entries.value.p[i]);
  }
  for (size_t i = 0; i < scratch.namespace_snapshots.length; i++) {
    InjectionNamespaceSnapshotDelete(scratch.namespace_snapshots.value.p[i]);
  }
  for (size_t i = 0; i < scratch.enum_snapshots.length; i++) {
    InjectionEnumSnapshotDelete(scratch.enum_snapshots.value.p[i]);
  }
  VectorDestruct(&scratch.installed_entries);
  VectorDestruct(&scratch.pending_declaration_roots);
  VectorDestruct(&scratch.namespace_snapshots);
  VectorDestruct(&scratch.enum_snapshots);
  return ok;
}

void CompilerQueueInjectedDeclaration(ASTNode* declaration_root) {
  if (compiler == NULL || declaration_root == NULL) {
    return;
  }
  VectorAppend(&compiler->pending_injected_declarations, declaration_root);
}

ASTNode* CompilerPopPendingInjectedDeclaration(void) {
  if (compiler == NULL || compiler->pending_injected_declarations.length == 0) {
    return NULL;
  }
  ASTNode* declaration = compiler->pending_injected_declarations.value.p[0];
  VectorDeleteElement(&compiler->pending_injected_declarations, 0);
  return declaration;
}

void CompilerDrainPendingInjectedDeclarations(Syntax* syntax) {
  if (compiler == NULL || syntax == NULL) {
    return;
  }
  while (compiler->pending_injected_declarations.length > 0) {
    ASTNode* node = compiler->pending_injected_declarations.value.p[0];
    VectorDeleteElement(&compiler->pending_injected_declarations, 0);
    CompilerCompileQueuedDeclaration(syntax, node);
  }
}

void CompilerInjectionFramesTeardown(void) {
  if (compiler == NULL) {
    return;
  }
  while (compiler->injection_frames.length > 0) {
    CompilerRollbackInjectionFrame();
  }
  VectorDestruct(&compiler->injection_frames);
  for (size_t i = 0; i < compiler->pending_injected_declarations.length; i++) {
    ASTNodeDelete(compiler->pending_injected_declarations.value.p[i]);
  }
  VectorDestruct(&compiler->pending_injected_declarations);
}
