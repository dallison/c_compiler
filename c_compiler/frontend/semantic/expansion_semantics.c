//
//  expansion_semantics.c
//  c_compiler
//

#include "expansion_semantics.h"

#include "compiler.h"
#include "constexpr.h"
#include "errors.h"
#include "expr_semantics.h"
#include "reflection.h"
#include "reflection_semantics.h"
#include "semantics.h"
#include "statement_parser.h"
#include "statement_semantics.h"
#include "symbol_table.h"
#include "syntax.h"
#include "type.h"
#include "type_compare.h"
#include "type_internal.h"
#include "type_template.h"
#include "type_template_internal.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  kExpansionMaterializeEnumerating,
  kExpansionMaterializeIterating,
  kExpansionMaterializeDestructuring,
} ExpansionMaterializeKind;

static Symbol* CloneExpansionLocalSymbol(Symbol* source, const char* name) {
  TypeRecord* type =
      source != NULL && source->type != NULL
          ? TypeRecordCopy(source->type)
          : NewTypeRecord(kTypeAuto, kQualPlain);
  Symbol* replacement =
      NewSymbol(name, type, source != NULL ? source->storage : STO(auto));
  if (source != NULL) {
    replacement->flags = source->flags;
    replacement->flags.is_parameter_pack = false;
    replacement->location = source->location;
    replacement->alignment = source->alignment;
    AttributeListDestruct(&replacement->attributes);
    AttributeListClone(&replacement->attributes, &source->attributes);
  }
  replacement->flags.is_local = true;
  replacement->flags.is_defined = true;
  replacement->structured_binding_pack_size = -2;
  return replacement;
}

typedef struct {
  const char* break_label;
  const char* continue_label;
} ExpansionJumpRewrite;

typedef struct {
  int id;
  char break_label[48];
  char end_label[48];
} ExpansionLabels;

static int g_next_expansion_label_id = 1;

static ExpansionLabels AllocateExpansionLabels(void) {
  ExpansionLabels labels = {.id = g_next_expansion_label_id++};
  snprintf(labels.break_label, sizeof(labels.break_label),
           "__expansion_%d_break", labels.id);
  snprintf(labels.end_label, sizeof(labels.end_label), "__expansion_%d_end",
           labels.id);
  return labels;
}

static void FormatExpansionContinueLabel(const ExpansionLabels* labels,
                                       size_t index, char* buffer,
                                       size_t buffer_size) {
  snprintf(buffer, buffer_size, "__expansion_%d_continue_%zu", labels->id,
           index);
}

static ASTNode* ExpansionIdentityCloneNode(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static TypeRecord* ExpansionObjectType(TypeRecord* type) {
  TypeRecord* object_type = type;
  if (object_type != NULL && TypeIsReference(object_type)) {
    object_type = object_type->next;
  }
  return TypeMaterializeClassTemplateSpecialization(&compiler->syntax,
                                                     object_type);
}

static Symbol* FindStdSymbolByName(const char* name) {
  Namespace* std_ns = NamespaceFindStdNamespace();
  if (std_ns == NULL) {
    return NULL;
  }
  String symbol_name;
  StringInit(&symbol_name, name);
  NamespaceInlineSymbolLookup result =
      NamespaceResolveSymbolInInlineSet(std_ns, &symbol_name);
  StringDestruct(&symbol_name);
  if (result.status != kInlineLookupUnique) {
    return NULL;
  }
  return result.symbol;
}

static bool ExpansionTupleLikeElementCount(TypeRecord* type,
                                           size_t* element_count) {
  Symbol* tuple_size = FindStdSymbolByName("tuple_size");
  if (tuple_size == NULL || !tuple_size->flags.is_template ||
      tuple_size->type == NULL || !TypeIsStructOrUnion(tuple_size->type)) {
    return false;
  }
  TypeRecord* object_type = type;
  if (object_type != NULL && TypeIsReference(object_type)) {
    object_type = object_type->next;
  }
  if (object_type == NULL) {
    return false;
  }
  Vector* args = NewVector();
  VectorAppend(args, NewTypeTemplateArgument(object_type));
  TypeRecord* tuple_size_type =
      TypeInstantiateClassTemplate(&compiler->syntax, tuple_size, args);
  VectorDeleteWithContents(args, (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (tuple_size_type == NULL || !TypeIsStructOrUnion(tuple_size_type) ||
      tuple_size_type->info.struct_info == NULL) {
    TypeRecordDelete(tuple_size_type);
    return false;
  }
  StructMember* value = FindStructMemberByName(tuple_size_type->info.struct_info,
                                                 "value");
  bool ok = value != NULL && value->symbol != NULL &&
            value->symbol->flags.value_set && value->symbol->value.ivalue >= 0;
  if (ok) {
    *element_count = (size_t)value->symbol->value.ivalue;
  }
  TypeRecordDelete(tuple_size_type);
  return ok;
}

static bool TypeIsExpansionArray(TypeRecord* type) {
  TypeRecord* object_type = ExpansionObjectType(type);
  return object_type != NULL && TypeIsArray(object_type);
}

static bool CallUsesInventedFunction(ASTNode* call) {
  if (call == NULL || call->op != AST_OP(call)) {
    return false;
  }
  VectorASTNode* vector_call = (VectorASTNode*)call;
  if (vector_call->left == NULL || vector_call->left->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* callee = ((IdentifierASTNode*)vector_call->left)->symbol;
  return callee != NULL && callee->flags.invented;
}

static bool ExpansionRangeHasConstexprBeginEnd(TypeRecord* range_type,
                                               Symbol* range_sym,
                                               SourceLocation location,
                                               ASTNode* diagnostic) {
  (void)diagnostic;
  TypeRecord* object_type = ExpansionObjectType(range_type);
  if (object_type != NULL && SyntaxTypeHasRangeMemberBeginEnd(object_type)) {
    return true;
  }
  ASTNode* begin_bound =
      SyntaxNewRangeForBoundExpr(&compiler->syntax, range_sym, true, location);
  begin_bound = SyntaxResolveRangeForIterator(begin_bound);
  bool saved_error_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  begin_bound = AnalyzeExpression(begin_bound);
  bool begin_failed =
      begin_bound == NULL || begin_bound->type == NULL || DiagnosticErrorTrapped();
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_error_trap);
  if (begin_failed) {
    ASTNodeDelete(begin_bound);
    return false;
  }
  bool valid_begin = true;
  if (begin_bound->op == AST_OP(range_begin)) {
    ASTNode* adl_call = ((BinaryASTNode*)begin_bound)->right;
    if (CallUsesInventedFunction(adl_call)) {
      valid_begin = false;
    }
  } else if (begin_bound->op == AST_OP(call) &&
             CallUsesInventedFunction(begin_bound)) {
    valid_begin = false;
  }
  ASTNodeDelete(begin_bound);
  if (!valid_begin) {
    return false;
  }
  ASTNode* end_bound =
      SyntaxNewRangeForBoundExpr(&compiler->syntax, range_sym, false, location);
  end_bound = SyntaxResolveRangeForIterator(end_bound);
  saved_error_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  end_bound = AnalyzeExpression(end_bound);
  bool end_failed =
      end_bound == NULL || end_bound->type == NULL || DiagnosticErrorTrapped();
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_error_trap);
  if (end_failed) {
    ASTNodeDelete(end_bound);
    return false;
  }
  bool valid_end = true;
  if (end_bound->op == AST_OP(range_end)) {
    ASTNode* adl_call = ((BinaryASTNode*)end_bound)->right;
    if (CallUsesInventedFunction(adl_call)) {
      valid_end = false;
    }
  } else if (end_bound->op == AST_OP(call) &&
             CallUsesInventedFunction(end_bound)) {
    valid_end = false;
  }
  ASTNodeDelete(end_bound);
  return valid_end;
}

typedef struct {
  Map* symbol_map;
  Map* pack_symbol_map;
} ExpansionCloneData;

static ExpansionMaterializeKind ClassifyExpansionStatement(
    ExpansionStatementASTNode* node) {
  if (node->init_kind == kExpansionInitializerInitList) {
    return kExpansionMaterializeEnumerating;
  }
  ASTNode* init_expr = node->initializer;
  TypeRecord* type = init_expr != NULL ? init_expr->type : NULL;
  if (type == NULL) {
    return kExpansionMaterializeDestructuring;
  }
  if (TypeIsExpansionArray(type)) {
    return kExpansionMaterializeDestructuring;
  }
  Symbol* range_probe =
      NewSymbol("__expansion_range_probe", TypeRecordCopy(type), STO(auto));
  range_probe->flags.invented = true;
  range_probe->location = node->base.location;
  bool iterable = ExpansionRangeHasConstexprBeginEnd(
      type, range_probe, node->base.location, (ASTNode*)node);
  SymbolDelete(range_probe);
  if (iterable) {
    return kExpansionMaterializeIterating;
  }
  return kExpansionMaterializeDestructuring;
}

static bool ExpansionInitializerIsDependent(ExpansionStatementASTNode* node,
                                            Map* pack_symbol_map) {
  if (node->initializer == NULL) {
    return true;
  }
  if (ExpressionIsTemplateDependent(node->initializer)) {
    return true;
  }
  if (node->init_kind == kExpansionInitializerInitList) {
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)node->initializer;
    for (size_t i = 0; braced != NULL && braced->initializers != NULL &&
                      i < braced->initializers->length;
         i++) {
      ASTNode* initializer = braced->initializers->value.p[i];
      if (initializer == NULL || initializer->op != AST_OP(expr_init)) {
        continue;
      }
      ExpressionInitializerASTNode* expr_init =
          (ExpressionInitializerASTNode*)initializer;
      if (expr_init->expr == NULL ||
          (expr_init->expr->flags & kASTPackExpansion) == 0 ||
          expr_init->expr->op != AST_OP(identifier)) {
        continue;
      }
      Symbol* pack_symbol = ((IdentifierASTNode*)expr_init->expr)->symbol;
      if (pack_symbol != NULL) {
        if (pack_symbol_map == NULL ||
            MapFindPointerKey(pack_symbol_map, pack_symbol) == NULL) {
          return true;
        }
      }
    }
  }
  return false;
}

bool ExpansionStatementIsDependent(ExpansionStatementASTNode* node,
                                   Map* symbol_map, Map* pack_symbol_map) {
  (void)symbol_map;
  if (node == NULL) {
    return true;
  }
  if (node->init_stmt != NULL &&
      ExpressionIsTemplateDependent(node->init_stmt)) {
    return true;
  }
  if (ExpansionInitializerIsDependent(node, pack_symbol_map)) {
    return true;
  }
  if (node->binding_type != NULL &&
      (TypeContainsTemplateParameter(node->binding_type) ||
       (TypeContainsAuto(node->binding_type) &&
        ExpansionInitializerIsDependent(node, pack_symbol_map)))) {
    return true;
  }
  if (node->item_kind == kExpansionItemSimple && node->item_symbol != NULL &&
      node->item_symbol->type != NULL) {
    if (TypeContainsTemplateParameter(node->item_symbol->type)) {
      return true;
    }
    if (TypeContainsAuto(node->item_symbol->type) &&
        ExpansionInitializerIsDependent(node, pack_symbol_map)) {
      return true;
    }
  }
  if (node->item_kind == kExpansionItemStructuredBinding &&
      node->binding_type != NULL &&
      TypeContainsAuto(node->binding_type) &&
      ExpansionInitializerIsDependent(node, pack_symbol_map)) {
    return true;
  }
  return false;
}

static ASTNode* BracedInitializerElementExpression(ASTNode* initializer) {
  if (initializer == NULL) {
    return NULL;
  }
  if (initializer->op == AST_OP(expr_init)) {
    return ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  if (initializer->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
    if (braced->initializers != NULL && braced->initializers->length == 1) {
      return BracedInitializerElementExpression(
          braced->initializers->value.p[0]);
    }
  }
  return initializer;
}

static Vector* PackSymbolReplacements(Symbol* pack, Map* pack_symbol_map) {
  if (pack == NULL || pack_symbol_map == NULL) {
    return NULL;
  }
  return MapFindPointerKey(pack_symbol_map, pack);
}

bool ExpandExpansionBracedInitializerElements(BracedInitializerASTNode* braced,
                                              Map* pack_symbol_map,
                                              Vector* out_elements,
                                              ASTNode* diagnostic) {
  if (braced == NULL || braced->initializers == NULL || out_elements == NULL) {
    return false;
  }
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* initializer = braced->initializers->value.p[i];
    if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
      ExpressionInitializerASTNode* expr_init =
          (ExpressionInitializerASTNode*)initializer;
      ASTNode* expr = expr_init->expr;
      if (expr != NULL && (expr->flags & kASTPackExpansion) != 0 &&
          expr->op == AST_OP(identifier)) {
        Symbol* pack_symbol = ((IdentifierASTNode*)expr)->symbol;
        Vector* replacements = PackSymbolReplacements(pack_symbol,
                                                      pack_symbol_map);
        if (replacements == NULL) {
          SemanticError(diagnostic != NULL ? diagnostic : (ASTNode*)braced,
                        "Cannot expand pack in expansion statement initializer");
          return false;
        }
        for (size_t j = 0; j < replacements->length; j++) {
          Symbol* replacement = replacements->value.p[j];
          VectorAppend(out_elements,
                       NewIdentifierASTNode(replacement, initializer->location));
        }
        continue;
      }
    }
    ASTNode* element = BracedInitializerElementExpression(initializer);
    if (element == NULL) {
      SemanticError(diagnostic != NULL ? diagnostic : (ASTNode*)braced,
                    "Invalid element in expansion statement initializer list");
      return false;
    }
    VectorAppend(out_elements,
                 ASTNodeClone(element, ExpansionIdentityCloneNode, NULL, NULL));
  }
  return true;
}

static ASTNode* NewSemanticInitExpression(Symbol* sym, ASTNode* initializer,
                                          SourceLocation location) {
  ASTNode* id = NewIdentifierASTNode(sym, location);
  id->flags |= kASTNeedAddress | kASTIsDeclaration;
  return NewBinaryASTNode(AST_OP(init), TypeRecordCopy(sym->type), location, id,
                          initializer);
}

static void BuildIterationItemSymbolMap(ExpansionStatementASTNode* expansion,
                                        Map* outer_symbol_map,
                                        Map* iteration_map) {
  if (expansion->item_kind != kExpansionItemSimple) {
    return;
  }
  Symbol* source = expansion->item_symbol;
    if (outer_symbol_map != NULL) {
      Symbol* mapped = MapFindPointerKey(outer_symbol_map, source);
      if (mapped != NULL) {
        source = mapped;
      }
    }
    Symbol* fresh = SemanticCloneExpansionIterationSymbol(source);
    MapKeyValue kv = {.key.p = expansion->item_symbol, .value.p = fresh};
    MapInsert(iteration_map, kv);
    if (source != expansion->item_symbol) {
      MapKeyValue mapped_kv = {.key.p = source, .value.p = fresh};
      MapInsert(iteration_map, mapped_kv);
    }
}

static void RegisterFreshBodyLocalSymbol(Map* local_map, Symbol* old_symbol,
                                         Symbol* new_symbol) {
  if (local_map == NULL || old_symbol == NULL || new_symbol == NULL) {
    return;
  }
  MapKeyValue kv = {.key.p = old_symbol, .value.p = new_symbol};
  MapInsert(local_map, kv);
}

static bool IsStaticOrThreadLocalVardecl(ASTNode* node) {
  if (node == NULL || node->op != AST_OP(vardecl)) {
    return false;
  }
  VariableDeclarationASTNode* decl = (VariableDeclarationASTNode*)node;
  if (decl->symbol == NULL) {
    return false;
  }
  if (StorageIs(decl->symbol->storage, STO(static) | STO(thread))) {
    return true;
  }
  return decl->symbol->asm_name.value != NULL &&
         strncmp(decl->symbol->asm_name.value, ".local.", 7) == 0;
}

static void CollectStaticLocalDeclarations(ASTNode* stmt, Vector* out) {
  if (stmt == NULL || out == NULL) {
    return;
  }
  if (IsStaticOrThreadLocalVardecl(stmt)) {
    VectorAppend(out, ASTNodeClone(stmt, ExpansionIdentityCloneNode, NULL, NULL));
    return;
  }
  if (stmt->op != AST_OP(compound)) {
    return;
  }
  CompoundStatementASTNode* compound = (CompoundStatementASTNode*)stmt;
  for (size_t i = 0; compound->statements != NULL &&
                    i < compound->statements->length;
       i++) {
    CollectStaticLocalDeclarations(compound->statements->value.p[i], out);
  }
}

static ASTNode* StripStaticLocalDeclarations(ASTNode* stmt) {
  if (stmt == NULL) {
    return NULL;
  }
  if (IsStaticOrThreadLocalVardecl(stmt)) {
    return NewCompoundStatementASTNode(NewVector(), stmt->location);
  }
  if (stmt->op != AST_OP(compound)) {
    return stmt;
  }
  CompoundStatementASTNode* compound = (CompoundStatementASTNode*)stmt;
  Vector* kept = NewVector();
  for (size_t i = 0; compound->statements != NULL &&
                    i < compound->statements->length;
       i++) {
    ASTNode* child = compound->statements->value.p[i];
    if (IsStaticOrThreadLocalVardecl(child)) {
      continue;
    }
    if (child != NULL && child->op == AST_OP(compound)) {
      child = StripStaticLocalDeclarations(child);
    }
    VectorAppend(kept, child);
  }
  return NewCompoundStatementASTNode(kept, compound->base.location);
}

static void CloneBodyLocalSymbolsVisitor(ASTNode* node, void* data, int child_id,
                                         VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL || data == NULL) {
    return;
  }
  Map* local_map = data;
  if (node->op == AST_OP(vardecl)) {
    VariableDeclarationASTNode* decl = (VariableDeclarationASTNode*)node;
    Symbol* old_symbol = decl->symbol;
    if (old_symbol == NULL || MapFindPointerKey(local_map, old_symbol) != NULL) {
      return;
    }
    if (IsStaticOrThreadLocalVardecl(node)) {
      return;
    }
    Symbol* fresh = SemanticCloneExpansionIterationSymbol(old_symbol);
    RegisterFreshBodyLocalSymbol(local_map, old_symbol, fresh);
    SyntaxAddSymbol(&compiler->syntax, fresh);
    decl->symbol = fresh;
    ASTNodeSetType(node, fresh->type);
    node->flags &= ~kASTAnalyzed;
    return;
  }
  if (node->op == AST_OP(structured_binding)) {
    StructuredBindingASTNode* binding = (StructuredBindingASTNode*)node;
    for (size_t i = 0; binding->symbols != NULL &&
                      i < binding->symbols->length;
         i++) {
      Symbol* old_symbol = binding->symbols->value.p[i];
      if (old_symbol == NULL ||
          MapFindPointerKey(local_map, old_symbol) != NULL) {
        continue;
      }
      if (IsStaticOrThreadLocalVardecl(node)) {
        continue;
      }
      String* name =
          binding->names != NULL && i < binding->names->length
              ? binding->names->value.p[i]
              : NULL;
      Symbol* fresh = CloneExpansionLocalSymbol(
          old_symbol, name != NULL ? name->value : "__binding");
      RegisterFreshBodyLocalSymbol(local_map, old_symbol, fresh);
      SyntaxAddSymbol(&compiler->syntax, fresh);
      binding->symbols->value.p[i] = fresh;
    }
    node->flags &= ~kASTAnalyzed;
  }
}

static ASTNode* RewriteClonedIdentifier(ASTNode* node, void* data,
                                        ASTNodeTransformAction* action) {
  (void)action;
  Map* maps = data;
  if (node == NULL || node->op != AST_OP(identifier) || maps == NULL) {
    return node;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  Symbol* replacement = MapFindPointerKey(maps, id->symbol);
  if (replacement == NULL) {
    return node;
  }
  ASTNode* cloned = NewIdentifierASTNode(replacement, node->location);
  cloned->flags = node->flags;
  ASTNodeSetType(cloned, replacement->type);
  return cloned;
}

static ASTNode* CloneExpansionBodyWithFreshLocals(ASTNode* stmt,
                                                    Map* item_map,
                                                    bool strip_static_locals) {
  if (stmt == NULL) {
    return NULL;
  }
  ASTNode* clone =
      ASTNodeClone(stmt, ExpansionIdentityCloneNode, NULL, NULL);
  Map local_map;
  MapInitForPointerKeys(&local_map);
  ASTNodeVisit(clone, CloneBodyLocalSymbolsVisitor, 0, &local_map);
  for (size_t i = 0; item_map != NULL && i < item_map->length; i++) {
    MapInsert(&local_map, item_map->values[i]);
  }
  clone = ASTNodeVisitAndTransform(clone, RewriteClonedIdentifier, &local_map);
  MapDestruct(&local_map);
  if (strip_static_locals) {
    clone = StripStaticLocalDeclarations(clone);
  }
  return clone;
}

static ASTNode* AnalyzeExpansionElement(ASTNode* element) {
  if (element == NULL) {
    return NULL;
  }
  ASTNode* analyzed =
      ASTNodeClone(element, ExpansionIdentityCloneNode, NULL, NULL);
  analyzed->flags &= ~kASTAnalyzed;
  return AnalyzeExpression(analyzed);
}

static void AppendHoistedStaticLocals(Vector* statements, Vector* hoisted) {
  if (statements == NULL || hoisted == NULL) {
    return;
  }
  for (size_t i = 0; i < hoisted->length; i++) {
    VectorAppend(statements, hoisted->value.p[i]);
  }
}

static void ClearExpansionAnalyzedFlagVisitor(ASTNode* node, void* data,
                                              int child_id, VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL) {
    node->flags &= ~kASTAnalyzed;
  }
}

static ASTNode* RewriteExpansionJump(ASTNode* node, void* data,
                                       ASTNodeTransformAction* action) {
  ExpansionJumpRewrite* ctx = data;
  if (node == NULL || ctx == NULL) {
    return node;
  }
  if (node->op == AST_OP(break) &&
      (node->flags & kASTExpansionLoopBreak) != 0) {
    return NewGotoStatementASTNode(NewString(ctx->break_label), node->location);
  }
  if (node->op == AST_OP(continue) &&
      (node->flags & kASTExpansionLoopContinue) != 0) {
    return NewGotoStatementASTNode(NewString(ctx->continue_label),
                                   node->location);
  }
  return node;
}

static ASTNode* NewLabelStatement(const char* name, SourceLocation location) {
  return NewLabelASTNode(name, NULL, false, location);
}

static TypeRecord* ExpansionElementConcreteType(ASTNode* element_expr) {
  if (element_expr == NULL) {
    return NULL;
  }
  TypeRecord* element_type = element_expr->type;
  if (element_type == NULL && element_expr->op == AST_OP(identifier)) {
    Symbol* element_sym = ((IdentifierASTNode*)element_expr)->symbol;
    element_type = element_sym != NULL ? element_sym->type : NULL;
  }
  if (element_type == NULL || TypeContainsAuto(element_type) ||
      TypeContainsTemplateParameter(element_type)) {
    return NULL;
  }
  return element_type;
}

static void DeduceExpansionItemType(Symbol* item, ASTNode* element_expr,
                                    ASTNode* diagnostic) {
  if (item == NULL || element_expr == NULL || item->type == NULL ||
      !TypeContainsAuto(item->type)) {
    return;
  }
  TypeRecord* element_type = ExpansionElementConcreteType(element_expr);
  if (element_type == NULL) {
    return;
  }
  TypeRecord* deduced = TypeDeduceAuto(item->type, element_type);
  if (deduced != NULL) {
    SymbolSetType(item, deduced);
  } else {
    SemanticError(diagnostic, "Cannot deduce auto type for %s",
                  item->name.value);
  }
}

static ASTNode* NewItemBindingStatement(ExpansionStatementASTNode* expansion,
                                        ASTNode* element_expr,
                                        Map* iteration_map,
                                        Map* pack_symbol_map,
                                        SourceLocation location,
                                        bool analyze_item_binding) {
  if (expansion->item_kind == kExpansionItemStructuredBinding) {
    Vector* names = NewVector();
    Vector* symbols = NewVector();
    for (size_t i = 0; i < expansion->binding_names->length; i++) {
      String* name = expansion->binding_names->value.p[i];
      VectorAppend(names, NewString(name->value));
      VectorAppend(symbols, expansion->binding_symbols->value.p[i]);
    }
    StructuredBindingASTNode* binding =
        (StructuredBindingASTNode*)NewStructuredBindingASTNode(
            TypeRecordCopy(expansion->binding_type), names, symbols,
            expansion->binding_pack_index,
            NewExpressionInitializerASTNode(
                ASTNodeClone(element_expr, ExpansionIdentityCloneNode, NULL,
                             NULL),
                location),
            location);
    return SemanticMaterializeClonedStructuredBinding((ASTNode*)binding,
                                                      iteration_map,
                                                      pack_symbol_map);
  }

  Symbol* item = MapFindPointerKey(iteration_map, expansion->item_symbol);
  if (item == NULL && expansion->item_symbol != NULL) {
    for (size_t i = 0; i < iteration_map->length; i++) {
      if (iteration_map->values[i].value.p != NULL) {
        item = iteration_map->values[i].value.p;
        break;
      }
    }
  }
  if (item == NULL) {
    SemanticError((ASTNode*)expansion,
                  "Cannot bind expansion statement iteration item");
    return NULL;
  }
  TypeRecord* element_type = ExpansionElementConcreteType(element_expr);
  if (element_type != NULL && item->type != NULL &&
      TypeContainsAuto(item->type)) {
    SymbolSetType(item, TypeRecordCopy(element_type));
  }
  ASTNode* element_clone =
      ASTNodeClone(element_expr, ExpansionIdentityCloneNode, NULL, NULL);
  element_clone->flags &= ~kASTAnalyzed;
  element_clone = AnalyzeExpression(element_clone);
  element_type = ExpansionElementConcreteType(element_clone);
  if (element_type == NULL) {
    element_type = ExpansionElementConcreteType(element_expr);
  }
  if (element_type != NULL && item->type != NULL &&
      TypeContainsAuto(item->type)) {
    SymbolSetType(item, TypeRecordCopy(element_type));
    ASTNodeSetType(element_clone, element_type);
  }
  DeduceExpansionItemType(item, element_clone, (ASTNode*)expansion);
  if (!SyntaxAddSymbol(&compiler->syntax, item)) {
    SemanticError((ASTNode*)expansion,
                  "Duplicate definition of local symbol %s", item->name.value);
  }
  ASTNode* decl = NewVariableDeclarationASTNode(
      item,
      NewSemanticInitExpression(
          item, NewExpressionInitializerASTNode(element_clone, location),
          location),
      location);
  if (item->type != NULL && TypeContainsAuto(item->type)) {
    SemanticDeduceAutoType(item, ((VariableDeclarationASTNode*)decl)->initializer,
                           (ASTNode*)expansion);
  }
  if (analyze_item_binding) {
    AnalyzeStatement(decl);
  }
  if (item->type != NULL && TypeIsReflection(item->type)) {
    ReflectionValue* reflection =
        SemanticReflectionValueFromExpression(element_clone);
    if (reflection == NULL) {
      ConstEvalContext context;
      ConstEvalContextInit(&context);
      reflection =
          ConstexprEvaluateReflectionExpression(&context, element_clone);
      ConstEvalContextDestruct(&context);
    }
    if (reflection != NULL) {
      item->value.other = reflection;
      item->flags.value_set = true;
    }
  }
  return decl;
}

static void AppendExpansionIteration(ExpansionStatementASTNode* node,
                                     Vector* statements, size_t index,
                                     size_t element_count, ASTNode* element,
                                     Map* symbol_map, Map* pack_symbol_map,
                                     const ExpansionLabels* labels,
                                     bool analyze_bindings);

static ASTNode* FoldExpansionReflectionIdentifier(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  (void)data;
  if (node == NULL || node->op != AST_OP(identifier)) {
    return node;
  }
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  if (symbol == NULL || symbol->type == NULL ||
      !TypeIsReflection(symbol->type) || !symbol->flags.value_set ||
      symbol->value.other == NULL) {
    return node;
  }
  *action = kASTTransformSkipChildren;
  return NewReflectionConstantASTNode(symbol->value.other, node->location);
}

static ASTNode* NewExpansionIterationBlock(ExpansionStatementASTNode* expansion,
                                           ASTNode* element_expr,
                                           Map* outer_symbol_map,
                                           Map* pack_symbol_map,
                                           SourceLocation location,
                                           const char* continue_label,
                                           const ExpansionLabels* labels,
                                           Vector* prefix_statements,
                                           bool analyze_bindings,
                                           bool strip_static_locals) {
  Map iteration_map;
  MapInitForPointerKeys(&iteration_map);
  BuildIterationItemSymbolMap(expansion, outer_symbol_map, &iteration_map);

  Vector* statements = NewVector();
  if (prefix_statements != NULL) {
    for (size_t i = 0; i < prefix_statements->length; i++) {
      VectorAppend(statements, prefix_statements->value.p[i]);
    }
  }

  bool iteration_scope = true;
  if (iteration_scope) {
    SyntaxOpenScope(&compiler->syntax);
  }
  ASTNode* item_binding =
      NewItemBindingStatement(expansion, element_expr, &iteration_map,
                              pack_symbol_map, location, analyze_bindings);
  if (item_binding != NULL) {
    VectorAppend(statements, item_binding);
  }
  ASTNode* body = CloneExpansionBodyWithFreshLocals(expansion->stmt,
                                                    &iteration_map,
                                                    strip_static_locals);
  body = ASTNodeVisitAndTransform(
      body, FoldExpansionReflectionIdentifier, NULL);
  body->flags &= ~kASTAnalyzed;
  ASTNodeVisit(body, ClearExpansionAnalyzedFlagVisitor, 0, NULL);
  if (outer_symbol_map != NULL) {
    RewriteTemplateBodyIdentifiers(body, outer_symbol_map);
  }
  if (continue_label != NULL && labels != NULL) {
    ExpansionJumpRewrite rewrite = {
        .break_label = labels->break_label,
        .continue_label = continue_label,
    };
    body = ASTNodeVisitAndTransform(body, RewriteExpansionJump, &rewrite);
  }
  if (iteration_scope) {
    SyntaxCloseScope(&compiler->syntax);
  }
  VectorAppend(statements, body);
  MapDestruct(&iteration_map);
  return NewCompoundStatementASTNode(statements, location);
}

static void AppendExpansionInitializerTemporaries(ASTNode* hidden_decl,
                                                  Vector* statements) {
  SemanticAppendHiddenInitializerTemporaries(hidden_decl, statements);
}

static ASTNode* FinalizeExpansionCompound(Vector* statements,
                                          ASTNode* hidden_decl,
                                          SourceLocation location,
                                          const ExpansionLabels* labels) {
  VectorAppend(statements,
               NewLabelStatement(labels->end_label, location));
  if (hidden_decl != NULL) {
    AppendExpansionInitializerTemporaries(hidden_decl, statements);
  }
  ASTNode* compound =
      NewCompoundStatementASTNode(statements, location);
  compound->flags |= kASTExpansionInitializer;
  return compound;
}

static ASTNode* MaterializeEmptyTupleLikeExpansion(
    ExpansionStatementASTNode* node, ASTNode* hidden_decl,
    const ExpansionLabels* labels) {
  Vector hoisted_static;
  VectorInit(&hoisted_static);
  CollectStaticLocalDeclarations(node->stmt, &hoisted_static);
  Vector* statements = NewVector();
  if (node->init_stmt != NULL) {
    VectorAppend(statements,
                 ASTNodeClone(node->init_stmt, ExpansionIdentityCloneNode, NULL,
                              NULL));
  }
  AppendHoistedStaticLocals(statements, &hoisted_static);
  VectorAppend(statements, hidden_decl);
  VectorAppend(statements,
               NewLabelStatement(labels->break_label, node->base.location));
  VectorDestruct(&hoisted_static);
  return FinalizeExpansionCompound(statements, hidden_decl, node->base.location,
                                   labels);
}

static ASTNode* MaterializeEnumeratingExpansion(ExpansionStatementASTNode* node,
                                                Map* symbol_map,
                                                Map* pack_symbol_map,
                                                const ExpansionLabels* labels) {
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)node->initializer;
  Vector elements;
  VectorInit(&elements);
  if (!ExpandExpansionBracedInitializerElements(braced, pack_symbol_map,
                                                &elements, (ASTNode*)node)) {
    VectorDestruct(&elements);
    return (ASTNode*)node;
  }
  Vector hoisted_static;
  VectorInit(&hoisted_static);
  CollectStaticLocalDeclarations(node->stmt, &hoisted_static);
  Vector* statements = NewVector();
  if (node->init_stmt != NULL) {
    VectorAppend(statements,
                 ASTNodeClone(node->init_stmt, ExpansionIdentityCloneNode, NULL,
                              NULL));
  }
  AppendHoistedStaticLocals(statements, &hoisted_static);
  for (size_t i = 0; i < elements.length; i++) {
    ASTNode* element = AnalyzeExpansionElement(elements.value.p[i]);
    AppendExpansionIteration(node, statements, i, elements.length, element,
                             symbol_map, pack_symbol_map, labels, true);
  }
  VectorAppend(statements,
               NewLabelStatement(labels->break_label, node->base.location));
  VectorDestruct(&elements);
  VectorDestruct(&hoisted_static);
  return FinalizeExpansionCompound(statements, NULL, node->base.location, labels);
}

static ASTNode* MaterializeDestructuringExpansion(ExpansionStatementASTNode* node,
                                                  Map* symbol_map,
                                                  Map* pack_symbol_map,
                                                  const ExpansionLabels* labels) {
  ASTNode* init_expr = node->initializer;
  if (init_expr == NULL || init_expr->type == NULL) {
    SemanticError((ASTNode*)node,
                  "Cannot determine type for expansion statement initializer");
    return (ASTNode*)node;
  }

  Symbol* hidden = NULL;
  ASTNode* hidden_decl =
      SemanticCreateHiddenReferenceBinding(init_expr, node->base.location,
                                           &hidden);
  if (hidden_decl == NULL || hidden == NULL) {
    SemanticError((ASTNode*)node,
                  "Cannot bind expansion statement initializer");
    return (ASTNode*)node;
  }
  AnalyzeStatement(hidden_decl);

  size_t tuple_like_count = 0;
  if (ExpansionTupleLikeElementCount(hidden->type, &tuple_like_count) &&
      tuple_like_count == 0) {
    return MaterializeEmptyTupleLikeExpansion(node, hidden_decl, labels);
  }

  StructuredBindingDecomposition decomposition;
  if (!SemanticAnalyzeStructuredBindingDecomposition(
          hidden->type, hidden_decl, &decomposition)) {
    SemanticError((ASTNode*)node,
                  "Cannot decompose type in expansion statement");
    return (ASTNode*)node;
  }

  Vector hoisted_static;
  VectorInit(&hoisted_static);
  CollectStaticLocalDeclarations(node->stmt, &hoisted_static);
  Vector* statements = NewVector();
  if (node->init_stmt != NULL) {
    VectorAppend(statements,
                 ASTNodeClone(node->init_stmt, ExpansionIdentityCloneNode, NULL,
                              NULL));
  }
  AppendHoistedStaticLocals(statements, &hoisted_static);
  VectorAppend(statements, hidden_decl);

  for (size_t i = 0; i < decomposition.element_count; i++) {
    StructMember* member =
        !decomposition.tuple_like && !decomposition.array_like &&
                i < decomposition.members.length
            ? decomposition.members.value.p[i]
            : NULL;
    ASTNode* element = SemanticStructuredBindingElementAccess(
        hidden, hidden->type, i, member, decomposition.tuple_like,
        node->base.location);
    if (element == NULL) {
      SemanticError((ASTNode*)node,
                    "Cannot decompose element in expansion statement");
      SemanticStructuredBindingDecompositionDestruct(&decomposition);
      return (ASTNode*)node;
    }
    element = AnalyzeExpression(element);
    AppendExpansionIteration(node, statements, i, decomposition.element_count,
                             element, symbol_map, pack_symbol_map, labels, true);
  }
  VectorAppend(statements,
               NewLabelStatement(labels->break_label, node->base.location));
  SemanticStructuredBindingDecompositionDestruct(&decomposition);
  VectorDestruct(&hoisted_static);
  return FinalizeExpansionCompound(statements, hidden_decl, node->base.location,
                                   labels);
}

static void AppendExpansionIteration(ExpansionStatementASTNode* node,
                                     Vector* statements, size_t index,
                                     size_t element_count, ASTNode* element,
                                     Map* symbol_map, Map* pack_symbol_map,
                                     const ExpansionLabels* labels,
                                     bool analyze_bindings) {
  char continue_label[48];
  if (index + 1 < element_count) {
    FormatExpansionContinueLabel(labels, index + 2, continue_label,
                                 sizeof(continue_label));
  } else {
    snprintf(continue_label, sizeof(continue_label), "%s", labels->break_label);
  }
  if (index > 0) {
    char entry_label[48];
    FormatExpansionContinueLabel(labels, index + 1, entry_label,
                                 sizeof(entry_label));
    VectorAppend(statements,
                   NewLabelStatement(entry_label, node->base.location));
  }
  VectorAppend(statements,
               NewExpansionIterationBlock(node, element, symbol_map,
                                          pack_symbol_map, node->base.location,
                                          continue_label, labels, NULL,
                                          analyze_bindings, true));
}

static bool ComputeConstexprIteratorDistanceByStepping(ConstEvalContext* ctx,
                                                       ASTNode* begin_expr,
                                                       ASTNode* end_expr,
                                                       size_t* count,
                                                       ASTNode* diagnostic) {
  if (ctx == NULL) {
    return false;
  }
  ASTNode* current =
      ASTNodeClone(begin_expr, ExpansionIdentityCloneNode, NULL, NULL);
  size_t steps = 0;
  const size_t kMaxSteps = 4096;
  while (steps <= kMaxSteps) {
    ASTNode* ne = NewBinaryASTNode(
        AST_OP(noteq), NULL, diagnostic->location,
        ASTNodeClone(current, ExpansionIdentityCloneNode, NULL, NULL),
        ASTNodeClone(end_expr, ExpansionIdentityCloneNode, NULL, NULL));
    int64_t not_equal = 0;
    if (!ConstexprEvaluatePointerComparison(ctx, ne, &not_equal)) {
      ne = AnalyzeExpression(ne);
      if (!EvaluateIntegerExpressionInContext(ctx, ne, &not_equal)) {
        ASTNodeDelete(ne);
        ASTNodeDelete(current);
        return false;
      }
    }
    ASTNodeDelete(ne);
    if (not_equal == 0) {
      *count = steps;
      ASTNodeDelete(current);
      return true;
    }
    TypeRecord* cur_type = current->type;
    ASTNode* next = NULL;
    if (cur_type != NULL && TypeIsPointer(cur_type)) {
      next = NewBinaryASTNode(
          AST_OP(plus), TypeRecordCopy(cur_type), diagnostic->location, current,
          NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                diagnostic->location));
      next = AnalyzeExpression(next);
    } else {
      next = NewUnaryASTNode(AST_OP(preinc), TypeRecordCopy(cur_type),
                             diagnostic->location, current);
      next = AnalyzeExpression(next);
    }
    if (next == NULL) {
      ASTNodeDelete(current);
      return false;
    }
    current = next;
    steps++;
  }
  ASTNodeDelete(current);
  return false;
}

static bool ComputeConstexprIteratorDistance(ConstEvalContext* ctx,
                                             ASTNode* begin_expr,
                                             ASTNode* end_expr, size_t* count,
                                             ASTNode* diagnostic) {
  if (ctx == NULL) {
    return false;
  }
  if (begin_expr != NULL && (begin_expr->flags & kASTAnalyzed) == 0) {
    begin_expr = AnalyzeExpression(begin_expr);
  }
  if (end_expr != NULL && (end_expr->flags & kASTAnalyzed) == 0) {
    end_expr = AnalyzeExpression(end_expr);
  }
  if (ConstexprSameObjectPointerDistance(ctx, begin_expr, end_expr, count)) {
    return true;
  }
  ASTNode* diff = NewBinaryASTNode(
      AST_OP(minus), NULL, diagnostic->location,
      ASTNodeClone(end_expr, ExpansionIdentityCloneNode, NULL, NULL),
      ASTNodeClone(begin_expr, ExpansionIdentityCloneNode, NULL, NULL));
  diff = AnalyzeExpression(diff);
  int64_t distance = 0;
  if (EvaluateIntegerExpressionInContext(ctx, diff, &distance) &&
      distance >= 0) {
    *count = (size_t)distance;
    ASTNodeDelete(diff);
    return true;
  }
  ASTNodeDelete(diff);
  if (ComputeConstexprIteratorDistanceByStepping(ctx, begin_expr, end_expr, count,
                                                 diagnostic)) {
    return true;
  }
  SemanticError(diagnostic,
                "Expansion statement range is not a compile-time iterable "
                "range");
  return false;
}

static bool IteratorSupportsRandomAccessAddition(TypeRecord* iter_type) {
  return iter_type != NULL && TypeIsPointer(iter_type);
}

static bool ExpansionIterationItemIsConstexpr(
    ExpansionStatementASTNode* expansion) {
  if (expansion->item_kind == kExpansionItemSimple) {
    return expansion->item_symbol != NULL &&
           expansion->item_symbol->flags.is_constexpr;
  }
  for (size_t i = 0; expansion->binding_symbols != NULL &&
                     i < expansion->binding_symbols->length;
       i++) {
    Symbol* symbol = expansion->binding_symbols->value.p[i];
    if (symbol != NULL && symbol->flags.is_constexpr) {
      return true;
    }
  }
  return false;
}

static ASTNode* FoldConstexprIteratingElement(ConstEvalContext* ctx,
                                              ASTNode* begin_expr,
                                              size_t index,
                                              SourceLocation location) {
  ASTNode* iterator =
      ASTNodeClone(begin_expr, ExpansionIdentityCloneNode, NULL, NULL);
  if (index != 0) {
    iterator = NewBinaryASTNode(
        AST_OP(plus), NULL, location, iterator,
        NewIntConstantASTNode((int64_t)index,
                              NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location));
  }
  ASTNode* element =
      NewUnaryASTNode(AST_OP(contents), NULL, location, iterator);
  element = AnalyzeExpression(element);
  if (element == NULL || element->type == NULL) {
    ASTNodeDelete(element);
    return NULL;
  }
  TypeRecord* value_type = element->type;
  int64_t integer = 0;
  double floating = 0;
  ASTNode* folded = NULL;
  if (TypeIsIntegral(value_type) &&
      ConstexprEvaluatePointerDereferenceAsInteger(ctx, element, &integer)) {
    folded =
        NewIntConstantASTNode(integer, TypeRecordCopy(value_type), location);
  } else if (TypeIsFloatingPoint(value_type) &&
             ConstexprEvaluatePointerDereferenceAsFloating(
                 ctx, element, &floating)) {
    folded =
        NewRealConstantASTNode(floating, TypeRecordCopy(value_type), location);
  } else if (TypeContainsReflection(value_type)) {
    ReflectionValue* reflection =
        ConstexprEvaluatePointerDereferenceAsReflection(ctx, element);
    if (reflection != NULL) {
      folded = NewReflectionConstantASTNode(reflection, location);
    }
  }
  ASTNodeDelete(element);
  return folded;
}

static ASTNode* NewIteratingElementExpression(Symbol* begin, size_t index,
                                              TypeRecord* iter_type,
                                              SourceLocation location,
                                              Vector* prefix_statements) {
  if (index == 0 || IteratorSupportsRandomAccessAddition(iter_type)) {
    ASTNode* iter = index == 0
                        ? NewIdentifierASTNode(begin, location)
                        : NewBinaryASTNode(
                              AST_OP(plus), TypeRecordCopy(iter_type), location,
                              NewIdentifierASTNode(begin, location),
                              NewIntConstantASTNode(
                                  (int64_t)index,
                                  NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                  location));
    return NewUnaryASTNode(AST_OP(contents), NULL, location, iter);
  }
  Symbol* iter = SyntaxNewTemporary(&compiler->syntax, TypeRecordCopy(iter_type));
  ASTNode* iter_decl = NewVariableDeclarationASTNode(
      iter,
      NewSemanticInitExpression(
          iter,
          NewExpressionInitializerASTNode(NewIdentifierASTNode(begin, location),
                                          location),
          location),
      location);
  VectorAppend(prefix_statements, iter_decl);
  for (size_t step = 0; step < index; step++) {
    ASTNode* advance = NewUnaryASTNode(AST_OP(preinc), TypeRecordCopy(iter_type),
                                       location,
                                       NewIdentifierASTNode(iter, location));
    VectorAppend(prefix_statements,
                 NewExpressionStatementASTNode(advance, location));
  }
  return NewUnaryASTNode(AST_OP(contents), NULL, location,
                         NewIdentifierASTNode(iter, location));
}

static ASTNode* MaterializeIteratingExpansion(ExpansionStatementASTNode* node,
                                              Map* symbol_map,
                                              Map* pack_symbol_map,
                                              const ExpansionLabels* labels) {
  ASTNode* init_expr = node->initializer;
  if (init_expr == NULL || init_expr->type == NULL) {
    SemanticError((ASTNode*)node,
                  "Cannot determine type for expansion statement initializer");
    return (ASTNode*)node;
  }

  Symbol* hidden = NULL;
  ASTNode* hidden_decl =
      SemanticCreateHiddenReferenceBinding(init_expr, node->base.location,
                                           &hidden);
  if (hidden_decl == NULL || hidden == NULL) {
    return (ASTNode*)node;
  }

  ConstEvalContext eval_ctx;
  ConstEvalContextInit(&eval_ctx);
  AnalyzeStatement(hidden_decl);
  if (!ConstexprBindExpansionRangeHidden(
          &eval_ctx, (VariableDeclarationASTNode*)hidden_decl, init_expr)) {
    ConstEvalContextDestruct(&eval_ctx);
    SemanticError((ASTNode*)node,
                  "Expansion statement range is not a constant expression");
    return (ASTNode*)node;
  }

  ASTNode* begin_bound = SyntaxNewRangeForBoundExpr(
      &compiler->syntax, hidden, true, node->base.location);
  begin_bound = SyntaxResolveRangeForIterator(begin_bound);
  ASTNode* end_bound = SyntaxNewRangeForBoundExpr(
      &compiler->syntax, hidden, false, node->base.location);
  end_bound = SyntaxResolveRangeForIterator(end_bound);
  if (begin_bound == NULL || end_bound == NULL) {
    ConstEvalContextDestruct(&eval_ctx);
    SemanticError((ASTNode*)node,
                  "Cannot resolve expansion statement range begin and end");
    return (ASTNode*)node;
  }
  begin_bound = AnalyzeExpression(begin_bound);
  end_bound = AnalyzeExpression(end_bound);
  if (begin_bound == NULL || end_bound == NULL || begin_bound->type == NULL ||
      end_bound->type == NULL) {
    ConstEvalContextDestruct(&eval_ctx);
    SemanticError((ASTNode*)node,
                  "Expansion statement range begin and end are invalid");
    return (ASTNode*)node;
  }

  size_t element_count = 0;
  if (!ComputeConstexprIteratorDistance(&eval_ctx, begin_bound, end_bound,
                                        &element_count, (ASTNode*)node)) {
    ConstEvalContextDestruct(&eval_ctx);
    return (ASTNode*)node;
  }
  ASTNode* constexpr_begin =
      ASTNodeClone(begin_bound, ExpansionIdentityCloneNode, NULL, NULL);

  Symbol* begin = SyntaxNewTemporary(&compiler->syntax, begin_bound->type);
  Symbol* end = SyntaxNewTemporary(&compiler->syntax, end_bound->type);
  ASTNode* begin_decl = NewVariableDeclarationASTNode(
      begin,
      NewSemanticInitExpression(
          begin, NewExpressionInitializerASTNode(begin_bound, node->base.location),
          node->base.location),
      node->base.location);
  ASTNode* end_decl = NewVariableDeclarationASTNode(
      end,
      NewSemanticInitExpression(
          end, NewExpressionInitializerASTNode(end_bound, node->base.location),
          node->base.location),
      node->base.location);
  AnalyzeStatement(begin_decl);
  AnalyzeStatement(end_decl);

  Vector hoisted_static;
  VectorInit(&hoisted_static);
  CollectStaticLocalDeclarations(node->stmt, &hoisted_static);
  Vector* statements = NewVector();
  if (node->init_stmt != NULL) {
    VectorAppend(statements,
                 ASTNodeClone(node->init_stmt, ExpansionIdentityCloneNode, NULL,
                              NULL));
  }
  AppendHoistedStaticLocals(statements, &hoisted_static);
  VectorAppend(statements, hidden_decl);
  VectorAppend(statements, begin_decl);
  VectorAppend(statements, end_decl);

  for (size_t i = 0; i < element_count; i++) {
    Vector prefix;
    VectorInit(&prefix);
    ASTNode* element = NewIteratingElementExpression(
        begin, i, begin_bound->type, node->base.location, &prefix);
    if (ExpansionIterationItemIsConstexpr(node)) {
      ASTNode* folded = FoldConstexprIteratingElement(
          &eval_ctx, constexpr_begin, i, node->base.location);
      if (folded == NULL) {
        SemanticError((ASTNode*)node,
                      "Expansion statement item initializer is not a constant "
                      "expression");
      } else {
        ASTNodeDelete(element);
        element = folded;
      }
    }
    char continue_label[48];
    if (i + 1 < element_count) {
      FormatExpansionContinueLabel(labels, i + 2, continue_label,
                                   sizeof(continue_label));
    } else {
      snprintf(continue_label, sizeof(continue_label), "%s", labels->break_label);
    }
    if (i > 0) {
      char entry_label[48];
      FormatExpansionContinueLabel(labels, i + 1, entry_label, sizeof(entry_label));
      VectorAppend(statements,
                   NewLabelStatement(entry_label, node->base.location));
    }
    VectorAppend(statements,
                 NewExpansionIterationBlock(
                     node, element, symbol_map, pack_symbol_map,
                     node->base.location, continue_label, labels, &prefix,
                     true, true));
    VectorDestruct(&prefix);
  }
  VectorAppend(statements,
               NewLabelStatement(labels->break_label, node->base.location));
  ASTNodeDelete(constexpr_begin);
  ConstEvalContextDestruct(&eval_ctx);
  VectorDestruct(&hoisted_static);
  return FinalizeExpansionCompound(statements, hidden_decl, node->base.location,
                                   labels);
}

ASTNode* CloneExpansionIterationBody(ASTNode* stmt, Map* symbol_map) {
  return CloneExpansionBodyWithFreshLocals(stmt, symbol_map, false);
}

ASTNode* SemanticMaterializeExpansionStatement(
    ExpansionStatementASTNode* node, Map* symbol_map, Map* pack_symbol_map) {
  if (node == NULL) {
    return NULL;
  }
  if (ExpansionStatementIsDependent(node, symbol_map, pack_symbol_map)) {
    return (ASTNode*)node;
  }

  int errors_before_label_check = NumErrors();
  SemanticDiagnoseExpansionEnclosedLabels(node);
  if (NumErrors() != errors_before_label_check) {
    return (ASTNode*)node;
  }
  SemanticMarkExpansionLoopJumps(node);

  ExpansionLabels labels = AllocateExpansionLabels();
  ExpansionMaterializeKind kind = ClassifyExpansionStatement(node);

  switch (kind) {
    case kExpansionMaterializeEnumerating:
      return MaterializeEnumeratingExpansion(node, symbol_map, pack_symbol_map,
                                             &labels);
    case kExpansionMaterializeIterating:
      return MaterializeIteratingExpansion(node, symbol_map, pack_symbol_map,
                                         &labels);
    case kExpansionMaterializeDestructuring:
      return MaterializeDestructuringExpansion(node, symbol_map,
                                               pack_symbol_map, &labels);
  }
  return (ASTNode*)node;
}
