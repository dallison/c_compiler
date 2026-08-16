//
//  reflection_semantics.c
//  c_compiler
//

#include "reflection_semantics.h"

#include "compiler.h"
#include "constexpr.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_semantics.h"
#include "statement_semantics.h"
#include "symbol_table.h"
#include "syntax.h"
#include "type_compare.h"
#include "type_core.h"
#include "type_internal.h"
#include "type_print.h"
#include "type_template.h"
#include "source.h"

#include <limits.h>
#include <string.h>

static Vector* MetaTemplateArgumentsVector(TypeRecord* type);

static ReflectionValue* MetaEvaluateReflectionArg(ASTNode* node);
static void EvaluateAnnotationAttribute(Attribute* attr);

static bool SymbolIsStructuredBinding(Symbol* symbol) {
  return symbol != NULL && symbol->structured_binding_pack_size != -2;
}

static bool SymbolIsStaticStorageObject(Symbol* symbol) {
  if (symbol == NULL || symbol->flags.is_argument ||
      symbol->flags.is_template_parameter) {
    return false;
  }
  return StorageIs(symbol->storage, STO(static)) ||
         (!symbol->flags.is_block_scope && !symbol->flags.is_local &&
          !StorageIs(symbol->storage, STO(extern)) &&
          symbol->namespace_ != NULL);
}

ReflectionEntityKind ReflectionKindForSymbol(Symbol* symbol) {
  if (symbol == NULL) {
    return kReflectionInvalid;
  }
  if (symbol->flags.is_concept) {
    return kReflectionConcept;
  }
  if (symbol->alias_template != NULL) {
    return kReflectionAliasTemplate;
  }
  if (symbol->variable_template != NULL) {
    return kReflectionVariableTemplate;
  }
  if (symbol->flags.is_template && symbol->type != NULL &&
      TypeIsFunction(symbol->type)) {
    return kReflectionFunctionTemplate;
  }
  if (symbol->flags.is_template && symbol->type != NULL &&
      TypeIsStructOrUnion(symbol->type)) {
    return kReflectionClassTemplate;
  }
  if (symbol->flags.is_template) {
    return kReflectionTemplate;
  }
  if (symbol->flags.is_argument) {
    return kReflectionFunctionParameter;
  }
  if (SymbolIsStructuredBinding(symbol)) {
    return kReflectionStructuredBinding;
  }
  if (StorageIs(symbol->storage, STO(typedef))) {
    if (symbol->type != NULL && TypeIsStructOrUnion(symbol->type) &&
        symbol->type->info.struct_info != NULL &&
        (symbol->type->info.struct_info->tag_symbol == symbol ||
         (symbol->type->info.struct_info->tag_name != NULL &&
          symbol->name.value != NULL &&
          strcmp(symbol->type->info.struct_info->tag_name->value,
                 symbol->name.value) == 0))) {
      return kReflectionType;
    }
    if (symbol->type != NULL && TypeIsEnum(symbol->type) &&
        symbol->type->info.enum_info != NULL &&
        (symbol->type->info.enum_info->tag_symbol == symbol ||
         (symbol->type->info.enum_info->tag_name != NULL &&
          symbol->name.value != NULL &&
          strcmp(symbol->type->info.enum_info->tag_name->value,
                 symbol->name.value) == 0))) {
      return kReflectionType;
    }
    return kReflectionTypeAlias;
  }
  if (symbol->type != NULL && TypeIsFunction(symbol->type)) {
    return kReflectionFunction;
  }
  if (symbol->flags.value_set && symbol->storage == STO(implicit) &&
      !symbol->flags.is_constexpr) {
    return kReflectionEnumerator;
  }
  if (SymbolIsStaticStorageObject(symbol)) {
    return kReflectionObject;
  }
  return kReflectionVariable;
}

static bool ReflectionOperandIsInvalidEntity(Symbol* symbol,
                                             ASTNode* diagnostic) {
  if (symbol == NULL) {
    return false;
  }
  Symbol* scope_symbol =
      SyntaxFindTopScopeSymbol(&compiler->syntax, &symbol->name);
  if (scope_symbol == NULL) {
    scope_symbol = NamespaceFindSymbolInScope(
        compiler->syntax.current_namespace, &symbol->name);
  }
  if (scope_symbol == NULL) {
    scope_symbol = FindGlobalSymbol(&symbol->name);
  }
  if (scope_symbol != NULL && scope_symbol->flags.is_using_alias &&
      !StorageIs(scope_symbol->storage, STO(typedef))) {
    SemanticError(diagnostic, "A using-declarator cannot be reflected");
    return true;
  }
  if (symbol->flags.is_using_alias &&
      !StorageIs(symbol->storage, STO(typedef))) {
    SemanticError(diagnostic, "A using-declarator cannot be reflected");
    return true;
  }
  if (symbol->flags.is_template_parameter) {
    return false;
  }
  if (symbol->flags.is_overloaded ||
      (symbol->overload_next != NULL && TypeIsFunction(symbol->type))) {
    SemanticError(diagnostic, "An overloaded entity cannot be reflected");
    return true;
  }
  if (symbol->type != NULL && (symbol->type->type & kTypeAuto) != 0) {
    SemanticError(diagnostic, "A placeholder type cannot be reflected");
    return true;
  }
  if (symbol->flags.invented && symbol->type != NULL &&
      TypeIsFunction(symbol->type) &&
      symbol->type->info.function.cxx_member_owner != NULL &&
      symbol->type->info.function.cxx_member_owner->tag_symbol != NULL &&
      symbol->type->info.function.cxx_member_owner->tag_symbol->flags.invented) {
    SemanticError(diagnostic, "A lambda capture cannot be reflected");
    return true;
  }
  return false;
}

static bool ReflectionOperandIsDependent(ASTNode* expression) {
  if (expression == NULL) {
    return false;
  }
  if (expression->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)expression)->symbol;
    if (symbol != NULL && symbol->flags.is_template_parameter) {
      return true;
    }
  }
  return (expression->flags &
          (kASTReferencesParameterPack | kASTDependentFunctorCall)) != 0 ||
         expression->type == NULL || TypeIsUnknown(expression->type) ||
         TypeContainsTemplateParameter(expression->type);
}

ReflectionValue* SemanticReflectionValueFromExpression(ASTNode* expression) {
  if (expression == NULL) {
    return NULL;
  }
  if (expression->op == AST_OP(reflection_constant)) {
    return ((ReflectionASTNode*)expression)->value;
  }
  if (expression->op == AST_OP(reflect)) {
    ReflectionASTNode* reflection = (ReflectionASTNode*)expression;
    if (reflection->value != NULL) {
      return reflection->value;
    }
  }
  if (expression->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)expression)->symbol;
    if (symbol != NULL && TypeIsReflection(symbol->type) &&
        symbol->flags.value_set && symbol->value.other != NULL) {
      return (ReflectionValue*)symbol->value.other;
    }
    if (symbol != NULL && TypeIsReflection(symbol->type) &&
        symbol->constexpr_initializer != NULL) {
      ASTNode* initializer =
          ConstexprInitializerExpression(symbol->constexpr_initializer);
      initializer = AnalyzeExpression(initializer);
      ReflectionValue* value =
          initializer != expression
              ? SemanticReflectionValueFromExpression(initializer)
              : NULL;
      if (value != NULL) {
        symbol->value.other = value;
        symbol->flags.value_set = true;
        return value;
      }
    }
  }
  return NULL;
}

ASTNode* SemanticAnalyzeReflection(ReflectionASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  if (node->base.op == AST_OP(reflection_constant) && node->value != NULL) {
    node->base.flags |= kASTAnalyzed;
    return (ASTNode*)node;
  }

  ReflectionValue* value = NULL;
  switch (node->operand_kind) {
    case kReflectionOperandGlobalNamespace:
    case kReflectionOperandNamespace:
      value = ReflectionCreateNamespace(node->namespace_, node->base.location);
      break;
    case kReflectionOperandNamespaceAlias:
      value = ReflectionCreateNamespaceAlias(node->namespace_,
                                             node->base.location);
      break;
    case kReflectionOperandType:
      if (node->operand_type == NULL ||
          TypeContainsTemplateParameter(node->operand_type)) {
        node->base.flags |= kASTAnalyzed;
        return (ASTNode*)node;
      }
      value = ReflectionCreateType(node->operand_type, NULL,
                                   node->base.location);
      if (value != NULL && node->operand_type != NULL &&
          node->operand_type->template_arguments != NULL) {
        for (size_t i = 0; i < node->operand_type->template_arguments->length;
             i++) {
          VectorAppend(
              &value->substituted_arguments,
              TemplateArgumentCopy(
                  node->operand_type->template_arguments->value.p[i]));
        }
      } else if (value != NULL) {
        Vector* template_args = MetaTemplateArgumentsVector(node->operand_type);
        if (template_args != NULL) {
          for (size_t i = 0; i < template_args->length; i++) {
            VectorAppend(&value->substituted_arguments,
                         TemplateArgumentCopy(template_args->value.p[i]));
          }
        }
      }
      break;
    case kReflectionOperandExpression:
      if (node->operand == NULL) {
        SemanticError((ASTNode*)node, "Invalid reflection operand");
        value = ReflectionCreateInvalid(node->base.location);
        break;
      }
      if (node->operand->op == AST_OP(identifier)) {
        IdentifierASTNode* id = (IdentifierASTNode*)node->operand;
        Symbol* symbol = id->symbol;
        if (symbol == NULL) {
          node->base.flags |= kASTAnalyzed;
          return (ASTNode*)node;
        }
        if (ReflectionOperandIsInvalidEntity(symbol, (ASTNode*)node)) {
          value = ReflectionCreateInvalid(node->base.location);
          break;
        }
        if (symbol->flags.is_template_type_parameter) {
          if (node->operand->type == NULL ||
              TypeContainsTemplateParameter(node->operand->type)) {
            node->base.flags |= kASTAnalyzed;
            return (ASTNode*)node;
          }
          value = ReflectionCreateType(node->operand->type, NULL,
                                       node->base.location);
          break;
        }
        if (symbol->type != NULL &&
            TypeContainsTemplateParameter(symbol->type)) {
          node->base.flags |= kASTAnalyzed;
          return (ASTNode*)node;
        }
        if (symbol->flags.is_template_parameter) {
          node->base.flags |= kASTAnalyzed;
          return (ASTNode*)node;
        }
        ReflectionEntityKind kind = ReflectionKindForSymbol(symbol);
        if (kind == kReflectionFunctionParameter) {
          value = ReflectionCreateParameter(
              symbol, (size_t)symbol->value.arg_number, node->base.location);
          break;
        }
        value = kind == kReflectionType || kind == kReflectionTypeAlias
                    ? ReflectionCreateType(
                          symbol->type,
                          kind == kReflectionTypeAlias ? symbol : NULL,
                          node->base.location)
                    : ReflectionCreateSymbol(kind, symbol,
                                             node->base.location);
      } else if (node->operand->op == AST_OP(structmember)) {
        StructMemberASTNode* member =
            (StructMemberASTNode*)node->operand;
        Struct* parent =
            member->owner_type != NULL &&
                    TypeIsStructOrUnion(member->owner_type)
                ? member->owner_type->info.struct_info
                : member->member != NULL &&
                          member->member->symbol != NULL &&
                          member->member->symbol->type != NULL &&
                          TypeIsFunction(member->member->symbol->type)
                      ? member->member->symbol->type->info.function
                            .cxx_member_owner
                      : NULL;
        value = ReflectionCreateMember(member->member, parent,
                                       node->base.location);
      } else {
        SemanticError((ASTNode*)node,
                      "Reflection operand does not name an entity");
        value = ReflectionCreateInvalid(node->base.location);
      }
      break;
    case kReflectionOperandValue:
      value = node->value;
      break;
  }

  node->value = value;
  node->operand_kind = kReflectionOperandValue;
  node->base.op = AST_OP(reflection_constant);
  ASTNodeSetType((ASTNode*)node,
                 NewTypeRecordWithSize(kTypeReflection, kQualPlain));
  node->base.value_category = kValueCategoryPrvalue;
  node->base.flags |= kASTAnalyzed;
  return (ASTNode*)node;
}
static ASTNode* MaterializeReflectedEntity(ReflectionValue* value,
                                           SpliceContext context,
                                           SourceLocation location) {
  if (value == NULL || value->kind == kReflectionInvalid) {
    return NULL;
  }
  switch (value->kind) {
    case kReflectionVariable:
    case kReflectionObject:
    case kReflectionStructuredBinding:
    case kReflectionFunction:
    case kReflectionEnumerator:
    case kReflectionFunctionTemplate:
    case kReflectionClassTemplate:
    case kReflectionVariableTemplate:
    case kReflectionAliasTemplate:
    case kReflectionConcept:
    case kReflectionTemplate:
    case kReflectionFunctionParameter:
      return value->symbol != NULL
                 ? NewIdentifierASTNode(value->symbol, location)
                 : NULL;
    case kReflectionDataMember:
    case kReflectionClassMember:
    case kReflectionUnnamedBitField:
    case kReflectionDataMemberDescription:
      if (value->member == NULL) {
        return NULL;
      }
      if (context == kSpliceAddressed) {
        ASTNode* member = NewStructMemberASTNode(value->member, location);
        return NewUnaryASTNode(AST_OP(member_ptr), NULL, location, member);
      }
      return NewStructMemberASTNode(value->member, location);
    default:
      return NULL;
  }
}

TypeRecord* SemanticMaterializeReflectedType(ReflectionValue* value,
                                             SourceLocation location) {
  (void)location;
  if (value == NULL) {
    return NULL;
  }
  switch (value->kind) {
    case kReflectionType:
    case kReflectionTypeAlias:
    case kReflectionBase:
      return ReflectionValueType(value);
    case kReflectionConcept:
    case kReflectionClassTemplate:
      return value->symbol != NULL && value->symbol->type != NULL
                 ? TypeRecordCopy(value->symbol->type)
                 : NULL;
    default:
      return NULL;
  }
}

Symbol* SemanticMaterializeReflectedTemplate(ReflectionValue* value,
                                             SourceLocation location) {
  (void)location;
  if (value == NULL || value->symbol == NULL ||
      !ReflectionKindIsTemplate(value->kind)) {
    return NULL;
  }
  return value->symbol;
}

static bool SpliceContextRequiresAccessibleEntity(SpliceContext context) {
  return context == kSpliceMember || context == kSpliceAddressed ||
         context == kSpliceExpression;
}

static bool SpliceAccessAllowed(StructMember* member, Struct* parent_class,
                                SpliceContext context) {
  if (member == NULL || !SpliceContextRequiresAccessibleEntity(context)) {
    return true;
  }
  if (member->access == kAccessPublic) {
    return true;
  }
  if (compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function) &&
      compiler->current_function->info.function.cxx_member_owner != NULL &&
      parent_class != NULL &&
      compiler->current_function->info.function.cxx_member_owner ==
          parent_class) {
    return true;
  }
  return false;
}

static void SemanticDiagnoseInvalidSpliceContext(SpliceASTNode* node) {
  if (node == NULL || node->reflection == NULL) {
    return;
  }
  if (node->reflection->op == AST_OP(reflect) ||
      node->reflection->op == AST_OP(reflection_constant)) {
    ReflectionValue* value =
        SemanticReflectionValueFromExpression(node->reflection);
    if (value == NULL || value->symbol == NULL) {
      return;
    }
    Symbol* symbol = value->symbol;
    if (TypeIsFunction(symbol->type) &&
        symbol->type->info.function.is_constructor) {
      SemanticError((ASTNode*)node,
                    "A constructor cannot be spliced in this context");
    }
    if (TypeIsFunction(symbol->type) &&
        symbol->type->info.function.is_destructor) {
      SemanticError((ASTNode*)node,
                    "A destructor cannot be spliced in this context");
    }
    if (symbol->default_argument != NULL) {
      SemanticError((ASTNode*)node,
                    "A default argument cannot be spliced");
    }
  }
}

TypeRecord* SemanticResolveDependentSpliceType(TypeRecord* type,
                                               ASTNode* diagnostic) {
  if (type == NULL || type->dependent_splice_expr == NULL) {
    return type;
  }
  if (compiler->syntax.current_template_parameter_count > 0 &&
      type->template_parameter_index >= 0) {
    return type;
  }
  ASTNode* splice_expr = type->dependent_splice_expr;
  if (splice_expr->op != AST_OP(splice)) {
    return type;
  }
  SpliceASTNode* splice = (SpliceASTNode*)splice_expr;
  splice->context = kSpliceType;
  splice->reflection = AnalyzeExpression(splice->reflection);
  ReflectionValue* reflection =
      SemanticReflectionValueFromExpression(splice->reflection);
  if (reflection != NULL) {
    TypeRecord* resolved =
        SemanticMaterializeReflectedType(reflection, splice->base.location);
    if (resolved != NULL) {
      Qualifiers quals = type->qualifiers;
      ASTNodeDelete(splice_expr);
      TypeRecordDelete(type);
      resolved->qualifiers |= quals;
      return TypeRecordCalculateSize(resolved);
    }
    SemanticError(diagnostic != NULL ? diagnostic : splice_expr,
                  "Type splice operand does not reflect a type");
  } else if (!ReflectionOperandIsDependent(splice->reflection)) {
    SemanticError(diagnostic != NULL ? diagnostic : splice_expr,
                  "Splice operand is not a constant reflection");
  }
  return type;
}

void SemanticResolveStructDependentSplices(Struct* str, ASTNode* diagnostic) {
  if (str == NULL) {
    return;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL) {
      continue;
    }
    base->type = SemanticResolveDependentSpliceType(base->type, diagnostic);
  }
}

Symbol* SemanticTemplateSymbolFromReflection(ASTNode* reflection,
                                             SourceLocation location) {
  if (reflection == NULL) {
    return NULL;
  }
  ASTNode* analyzed = AnalyzeExpression(reflection);
  ReflectionValue* value = SemanticReflectionValueFromExpression(analyzed);
  if (value == NULL) {
    return NULL;
  }
  Symbol* templ = SemanticMaterializeReflectedTemplate(value, location);
  if (templ == NULL) {
    SemanticError(analyzed, "Template splice operand does not reflect a template");
  }
  return templ;
}

Namespace* SemanticMaterializeReflectedNamespace(ReflectionValue* value) {
  if (value == NULL) {
    return NULL;
  }
  if (value->kind == kReflectionNamespace ||
      value->kind == kReflectionGlobalNamespace) {
    return value->namespace_;
  }
  if (value->kind == kReflectionNamespaceAlias) {
    return value->namespace_alias_target;
  }
  return NULL;
}

ASTNode* SemanticAnalyzeSplice(SpliceASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  SemanticDiagnoseInvalidSpliceContext(node);
  node->reflection = AnalyzeExpression(node->reflection);
  ReflectionValue* value =
      SemanticReflectionValueFromExpression(node->reflection);
  if (value == NULL) {
    if (!ReflectionOperandIsDependent(node->reflection)) {
      SemanticError((ASTNode*)node,
                    "Splice operand is not a constant reflection");
    }
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                         kQualPlain));
    node->base.flags |= kASTAnalyzed;
    return (ASTNode*)node;
  }

  if (node->context == kSpliceType || node->context == kSpliceBase) {
    TypeRecord* type = SemanticMaterializeReflectedType(value, node->base.location);
    if (type == NULL) {
      SemanticError((ASTNode*)node,
                    "Type splice operand does not reflect a type");
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt, kQualPlain));
    } else {
      ASTNodeSetType((ASTNode*)node, type);
      TypeRecordDelete(type);
    }
    node->base.flags |= kASTAnalyzed;
    return (ASTNode*)node;
  }

  if (node->context == kSpliceTemplate) {
    Symbol* templ = SemanticMaterializeReflectedTemplate(value, node->base.location);
    if (templ == NULL) {
      SemanticError((ASTNode*)node,
                    "Template splice operand does not reflect a template");
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt, kQualPlain));
      node->base.flags |= kASTAnalyzed;
      return (ASTNode*)node;
    }
    return AnalyzeExpression(NewIdentifierASTNode(templ, node->base.location));
  }

  ASTNode* result =
      MaterializeReflectedEntity(value, node->context, node->base.location);
  if (result == NULL) {
    SemanticError((ASTNode*)node,
                  "Reflection value cannot be used in this splice context");
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    node->base.flags |= kASTAnalyzed;
    return (ASTNode*)node;
  }
  return AnalyzeExpression(result);
}

ASTNode* SemanticAnalyzeSpliceQualified(SpliceQualifiedASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  node->reflection = AnalyzeExpression(node->reflection);
  ReflectionValue* value =
      SemanticReflectionValueFromExpression(node->reflection);
  if (value == NULL) {
    if (!ReflectionOperandIsDependent(node->reflection)) {
      SemanticError((ASTNode*)node,
                    "Splice operand is not a constant reflection");
    }
    node->suffix = AnalyzeExpression(node->suffix);
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
    node->base.flags |= kASTAnalyzed;
    return (ASTNode*)node;
  }
  Namespace* namespace_ = SemanticMaterializeReflectedNamespace(value);
  if (namespace_ == NULL) {
    SemanticError((ASTNode*)node,
                  "Qualified splice requires a namespace reflection");
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    node->base.flags |= kASTAnalyzed;
    return (ASTNode*)node;
  }
  if (node->suffix != NULL && node->suffix->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->suffix;
    if (id->symbol != NULL &&
        (id->symbol->flags.invented || id->symbol->flags.is_forward_declared)) {
      NamespaceInlineSymbolLookup lookup =
          NamespaceResolveSymbolInInlineSet(namespace_, &id->symbol->name);
      if (lookup.status == kInlineLookupUnique) {
        id->symbol = lookup.symbol;
      }
    }
  }
  Syntax* syntax = &compiler->syntax;
  Namespace* saved = syntax->current_namespace;
  syntax->current_namespace = namespace_;
  node->suffix = AnalyzeExpression(node->suffix);
  syntax->current_namespace = saved;
  if (node->suffix != NULL) {
    return node->suffix;
  }
  ASTNodeSetType((ASTNode*)node,
                 NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
  node->base.flags |= kASTAnalyzed;
  return (ASTNode*)node;
}
ASTNode* SemanticAnalyzeAddressedSplice(UnaryASTNode* address) {
  if (address == NULL || address->sub == NULL ||
      address->sub->op != AST_OP(splice)) {
    return (ASTNode*)address;
  }
  SpliceASTNode* splice = (SpliceASTNode*)address->sub;
  splice->context = kSpliceAddressed;
  splice->reflection = AnalyzeExpression(splice->reflection);
  ReflectionValue* value =
      SemanticReflectionValueFromExpression(splice->reflection);
  if (value != NULL && value->kind == kReflectionDataMember) {
    ASTNode* result =
        MaterializeReflectedEntity(value, kSpliceAddressed,
                                   address->base.location);
    return result != NULL ? AnalyzeExpression(result) : (ASTNode*)address;
  }
  if (value == NULL &&
      !ReflectionOperandIsDependent(splice->reflection)) {
    SemanticError((ASTNode*)splice,
                  "Splice operand is not a constant reflection");
  }
  return (ASTNode*)address;
}

bool SemanticLowerMemberSplice(BinaryASTNode* access) {
  if (access == NULL || access->right == NULL ||
      access->right->op != AST_OP(splice)) {
    return false;
  }
  SpliceASTNode* splice = (SpliceASTNode*)access->right;
  splice->context = kSpliceMember;
  splice->reflection = AnalyzeExpression(splice->reflection);
  ReflectionValue* value =
      SemanticReflectionValueFromExpression(splice->reflection);
  if (value == NULL) {
    if (!ReflectionOperandIsDependent(splice->reflection)) {
      SemanticError((ASTNode*)splice,
                    "Splice operand is not a constant reflection");
    }
    ASTNodeSetType((ASTNode*)access,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                         kQualPlain));
    access->base.flags |= kASTAnalyzed;
    return true;
  }
  if (value->kind != kReflectionDataMember &&
      !(value->kind == kReflectionFunction && value->member != NULL) &&
      !(value->kind == kReflectionVariable && value->member != NULL)) {
    SemanticError((ASTNode*)splice,
                  "Member splice operand does not reflect a member");
    ASTNodeSetType((ASTNode*)access,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    access->base.flags |= kASTAnalyzed;
    return true;
  }
  if (!SpliceAccessAllowed(value->member, value->parent_class, kSpliceMember)) {
    SemanticError((ASTNode*)splice,
                  "Spliced member is not accessible in this context");
    ASTNodeSetType((ASTNode*)access,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    access->base.flags |= kASTAnalyzed;
    return true;
  }
  access->right = NewStructMemberASTNode(value->member,
                                         splice->base.location);
  access->right->parent = (ASTNode*)access;
  access->right->child_id = 1;
  return false;
}

ASTNode* SemanticAnalyzeReflectionComparison(BinaryASTNode* comparison) {
  comparison->left = AnalyzeExpression(comparison->left);
  comparison->right = AnalyzeExpression(comparison->right);
  ReflectionValue* left =
      SemanticReflectionValueFromExpression(comparison->left);
  ReflectionValue* right =
      SemanticReflectionValueFromExpression(comparison->right);
  if (left == NULL || right == NULL) {
    ASTNodeSetType((ASTNode*)comparison,
                   NewTypeRecordWithSize(kTypeBool, kQualPlain));
    comparison->base.flags |= kASTAnalyzed;
    return (ASTNode*)comparison;
  }
  bool equal = ReflectionValueEqual(left, right);
  if (comparison->base.op == AST_OP(noteq)) {
    equal = !equal;
  }
  return NewIntConstantASTNode(
      equal ? 1 : 0,
      NewTypeRecordWithSize(kTypeBool, kQualConst),
      comparison->base.location);
}

static Symbol* MetaCallSymbol(VectorASTNode* call) {
  if (call == NULL || call->left == NULL) {
    return NULL;
  }
  Symbol* symbol = NULL;
  if (call->left->op == AST_OP(identifier)) {
    symbol = ((IdentifierASTNode*)call->left)->symbol;
  } else if (call->left->op == AST_OP(structmember)) {
    StructMemberASTNode* member = (StructMemberASTNode*)call->left;
    symbol = member->member != NULL ? member->member->symbol : NULL;
  } else if (call->left->op == AST_OP(dot) ||
             call->left->op == AST_OP(arrow)) {
    ASTNode* right = ((BinaryASTNode*)call->left)->right;
    if (right != NULL && right->op == AST_OP(structmember)) {
      StructMemberASTNode* member = (StructMemberASTNode*)right;
      symbol = member->member != NULL ? member->member->symbol : NULL;
    }
  }
  for (size_t depth = 0;
       symbol != NULL && symbol->alias_target != NULL && depth < 64;
       depth++) {
    symbol = symbol->alias_target;
  }
  if (symbol == NULL ||
      !SymbolHasAttribute(symbol, "meta_intrinsic")) {
    return NULL;
  }
  return symbol;
}

typedef enum {
  kMetaUnknown,
  kMetaAccessContextCurrent,
  kMetaAlignmentOf,
  kMetaAnnotationsOf,
  kMetaAnnotationsOfWithType,
  kMetaBasesOf,
  kMetaBitSizeOf,
  kMetaConstantOf,
  kMetaCurrentClass,
  kMetaCurrentFunction,
  kMetaCurrentNamespace,
  kMetaDealias,
  kMetaDisplayStringOf,
  kMetaEnumeratorsOf,
  kMetaHasAutomaticStorageDuration,
  kMetaHasCLanguageLinkage,
  kMetaHasDefaultArgument,
  kMetaHasDefaultMemberInitializer,
  kMetaHasExternalLinkage,
  kMetaHasIdentifier,
  kMetaHasInaccessibleBases,
  kMetaHasInaccessibleNonstaticDataMembers,
  kMetaHasInaccessibleSubobjects,
  kMetaHasInternalLinkage,
  kMetaHasLinkage,
  kMetaHasModuleLinkage,
  kMetaHasParent,
  kMetaHasStaticStorageDuration,
  kMetaHasTemplateArguments,
  kMetaHasThreadStorageDuration,
  kMetaIdentifierOf,
  kMetaIsAccessible,
  kMetaIsAliasTemplate,
  kMetaIsAnnotation,
  kMetaIsAssignment,
  kMetaIsBase,
  kMetaIsBitField,
  kMetaIsClassMember,
  kMetaIsClassTemplate,
  kMetaIsCompleteType,
  kMetaIsConcept,
  kMetaIsConst,
  kMetaIsConstructor,
  kMetaIsConstructorTemplate,
  kMetaIsConversionFunction,
  kMetaIsConversionFunctionTemplate,
  kMetaIsCopyAssignment,
  kMetaIsCopyConstructor,
  kMetaIsDataMember,
  kMetaIsDefaultConstructor,
  kMetaIsDefaulted,
  kMetaIsDeleted,
  kMetaIsDestructor,
  kMetaIsEnumerableType,
  kMetaIsEnumerator,
  kMetaIsExplicit,
  kMetaIsExplicitObjectParameter,
  kMetaIsFinal,
  kMetaIsFunction,
  kMetaIsFunctionParameter,
  kMetaIsFunctionTemplate,
  kMetaIsInvalid,
  kMetaIsLiteralOperator,
  kMetaIsLiteralOperatorTemplate,
  kMetaIsLvalueReferenceQualified,
  kMetaIsMoveAssignment,
  kMetaIsMoveConstructor,
  kMetaIsMutableMember,
  kMetaIsNamespace,
  kMetaIsNamespaceAlias,
  kMetaIsNamespaceMember,
  kMetaIsNoexcept,
  kMetaIsObject,
  kMetaIsOperatorFunction,
  kMetaIsOperatorFunctionTemplate,
  kMetaIsOverride,
  kMetaIsPrivate,
  kMetaIsProtected,
  kMetaIsPublic,
  kMetaIsPureVirtual,
  kMetaIsRvalueReferenceQualified,
  kMetaIsSpecialMemberFunction,
  kMetaIsStaticMember,
  kMetaIsStructuredBinding,
  kMetaIsTemplate,
  kMetaIsType,
  kMetaIsTypeAlias,
  kMetaIsUserDeclared,
  kMetaIsUserProvided,
  kMetaIsValue,
  kMetaIsVariable,
  kMetaIsVariableTemplate,
  kMetaIsVarargFunction,
  kMetaIsVirtual,
  kMetaIsVolatile,
  kMetaMembersOf,
  kMetaNonstaticDataMembersOf,
  kMetaObjectOf,
  kMetaOffsetOf,
  kMetaOperatorOf,
  kMetaParametersOf,
  kMetaParentOf,
  kMetaReturnTypeOf,
  kMetaSizeOf,
  kMetaSourceLocationOf,
  kMetaStaticDataMembersOf,
  kMetaSubobjectsOf,
  kMetaSymbolOf,
  kMetaTemplateArgumentsOf,
  kMetaTemplateOf,
  kMetaTypeOf,
  kMetaU8DisplayStringOf,
  kMetaU8IdentifierOf,
  kMetaU8SymbolOf,
  kMetaUnchecked,
  kMetaUnprivileged,
  kMetaVariableOf,
  kMetaVia,
} MetaOperation;

typedef struct {
  const char* name;
  MetaOperation operation;
} MetaOperationEntry;

// Keep this table sorted by name for MetaOperationForName's binary search.
static const MetaOperationEntry kMetaOperations[] = {
    {"alignment_of", kMetaAlignmentOf},
    {"annotations_of", kMetaAnnotationsOf},
    {"annotations_of_with_type", kMetaAnnotationsOfWithType},
    {"bases_of", kMetaBasesOf},
    {"bit_size_of", kMetaBitSizeOf},
    {"constant_of", kMetaConstantOf},
    {"current", kMetaAccessContextCurrent},
    {"current_class", kMetaCurrentClass},
    {"current_function", kMetaCurrentFunction},
    {"current_namespace", kMetaCurrentNamespace},
    {"dealias", kMetaDealias},
    {"display_string_of", kMetaDisplayStringOf},
    {"enumerators_of", kMetaEnumeratorsOf},
    {"has_automatic_storage_duration", kMetaHasAutomaticStorageDuration},
    {"has_c_language_linkage", kMetaHasCLanguageLinkage},
    {"has_default_argument", kMetaHasDefaultArgument},
    {"has_default_member_initializer", kMetaHasDefaultMemberInitializer},
    {"has_external_linkage", kMetaHasExternalLinkage},
    {"has_identifier", kMetaHasIdentifier},
    {"has_inaccessible_bases", kMetaHasInaccessibleBases},
    {"has_inaccessible_nonstatic_data_members",
     kMetaHasInaccessibleNonstaticDataMembers},
    {"has_inaccessible_subobjects", kMetaHasInaccessibleSubobjects},
    {"has_internal_linkage", kMetaHasInternalLinkage},
    {"has_linkage", kMetaHasLinkage},
    {"has_module_linkage", kMetaHasModuleLinkage},
    {"has_parent", kMetaHasParent},
    {"has_static_storage_duration", kMetaHasStaticStorageDuration},
    {"has_template_arguments", kMetaHasTemplateArguments},
    {"has_thread_storage_duration", kMetaHasThreadStorageDuration},
    {"identifier_of", kMetaIdentifierOf},
    {"is_accessible", kMetaIsAccessible},
    {"is_alias_template", kMetaIsAliasTemplate},
    {"is_annotation", kMetaIsAnnotation},
    {"is_assignment", kMetaIsAssignment},
    {"is_base", kMetaIsBase},
    {"is_bit_field", kMetaIsBitField},
    {"is_class_member", kMetaIsClassMember},
    {"is_class_template", kMetaIsClassTemplate},
    {"is_complete_type", kMetaIsCompleteType},
    {"is_concept", kMetaIsConcept},
    {"is_const", kMetaIsConst},
    {"is_constructor", kMetaIsConstructor},
    {"is_constructor_template", kMetaIsConstructorTemplate},
    {"is_conversion_function", kMetaIsConversionFunction},
    {"is_conversion_function_template", kMetaIsConversionFunctionTemplate},
    {"is_copy_assignment", kMetaIsCopyAssignment},
    {"is_copy_constructor", kMetaIsCopyConstructor},
    {"is_data_member", kMetaIsDataMember},
    {"is_default_constructor", kMetaIsDefaultConstructor},
    {"is_defaulted", kMetaIsDefaulted},
    {"is_deleted", kMetaIsDeleted},
    {"is_destructor", kMetaIsDestructor},
    {"is_enumerable_type", kMetaIsEnumerableType},
    {"is_enumerator", kMetaIsEnumerator},
    {"is_explicit", kMetaIsExplicit},
    {"is_explicit_object_parameter", kMetaIsExplicitObjectParameter},
    {"is_final", kMetaIsFinal},
    {"is_function", kMetaIsFunction},
    {"is_function_parameter", kMetaIsFunctionParameter},
    {"is_function_template", kMetaIsFunctionTemplate},
    {"is_invalid", kMetaIsInvalid},
    {"is_literal_operator", kMetaIsLiteralOperator},
    {"is_literal_operator_template", kMetaIsLiteralOperatorTemplate},
    {"is_lvalue_reference_qualified", kMetaIsLvalueReferenceQualified},
    {"is_move_assignment", kMetaIsMoveAssignment},
    {"is_move_constructor", kMetaIsMoveConstructor},
    {"is_mutable_member", kMetaIsMutableMember},
    {"is_namespace", kMetaIsNamespace},
    {"is_namespace_alias", kMetaIsNamespaceAlias},
    {"is_namespace_member", kMetaIsNamespaceMember},
    {"is_noexcept", kMetaIsNoexcept},
    {"is_object", kMetaIsObject},
    {"is_operator_function", kMetaIsOperatorFunction},
    {"is_operator_function_template", kMetaIsOperatorFunctionTemplate},
    {"is_override", kMetaIsOverride},
    {"is_private", kMetaIsPrivate},
    {"is_protected", kMetaIsProtected},
    {"is_public", kMetaIsPublic},
    {"is_pure_virtual", kMetaIsPureVirtual},
    {"is_rvalue_reference_qualified", kMetaIsRvalueReferenceQualified},
    {"is_special_member_function", kMetaIsSpecialMemberFunction},
    {"is_static_member", kMetaIsStaticMember},
    {"is_structured_binding", kMetaIsStructuredBinding},
    {"is_template", kMetaIsTemplate},
    {"is_type", kMetaIsType},
    {"is_type_alias", kMetaIsTypeAlias},
    {"is_user_declared", kMetaIsUserDeclared},
    {"is_user_provided", kMetaIsUserProvided},
    {"is_value", kMetaIsValue},
    {"is_vararg_function", kMetaIsVarargFunction},
    {"is_variable", kMetaIsVariable},
    {"is_variable_template", kMetaIsVariableTemplate},
    {"is_virtual", kMetaIsVirtual},
    {"is_volatile", kMetaIsVolatile},
    {"members_of", kMetaMembersOf},
    {"nonstatic_data_members_of", kMetaNonstaticDataMembersOf},
    {"object_of", kMetaObjectOf},
    {"offset_of", kMetaOffsetOf},
    {"operator_of", kMetaOperatorOf},
    {"parameters_of", kMetaParametersOf},
    {"parent_of", kMetaParentOf},
    {"return_type_of", kMetaReturnTypeOf},
    {"size_of", kMetaSizeOf},
    {"source_location_of", kMetaSourceLocationOf},
    {"static_data_members_of", kMetaStaticDataMembersOf},
    {"subobjects_of", kMetaSubobjectsOf},
    {"symbol_of", kMetaSymbolOf},
    {"template_arguments_of", kMetaTemplateArgumentsOf},
    {"template_of", kMetaTemplateOf},
    {"type_of", kMetaTypeOf},
    {"u8display_string_of", kMetaU8DisplayStringOf},
    {"u8identifier_of", kMetaU8IdentifierOf},
    {"u8symbol_of", kMetaU8SymbolOf},
    {"unchecked", kMetaUnchecked},
    {"unprivileged", kMetaUnprivileged},
    {"variable_of", kMetaVariableOf},
    {"via", kMetaVia},
};

static MetaOperation MetaOperationForName(const char* name) {
  size_t begin = 0;
  size_t end = sizeof(kMetaOperations) / sizeof(kMetaOperations[0]);
  while (begin < end) {
    size_t middle = begin + (end - begin) / 2;
    int comparison = strcmp(name, kMetaOperations[middle].name);
    if (comparison == 0) {
      return kMetaOperations[middle].operation;
    }
    if (comparison < 0) {
      end = middle;
    } else {
      begin = middle + 1;
    }
  }
  return kMetaUnknown;
}

static ASTNode* NewMetaBool(bool value, SourceLocation location) {
  return NewIntConstantASTNode(
      value ? 1 : 0,
      NewTypeRecordWithSize(kTypeBool, kQualConst), location);
}

static ASTNode* NewMetaString(const char* text, SourceLocation location) {
  String* contents = NewString(text != NULL ? text : "");
  TypeRecord* array = NewBasicArrayTypeRecord(
      kQualPlain, (int)contents->length + 1, false);
  TypeRecordChain(
      array, NewTypeRecordWithSize(kTypeChar, kQualConst));
  TypeRecordCalculateSize(array);
  return NewStringConstantASTNode(contents, array, location);
}

static ASTNode* NewMetaU8String(const char* text, SourceLocation location) {
  String* contents = NewString(text != NULL ? text : "");
  TypeRecord* array = NewBasicArrayTypeRecord(
      kQualPlain, (int)contents->length + 1, false);
  TypeRecordChain(
      array, NewTypeRecordWithSize(kTypeChar8, kQualConst));
  TypeRecordCalculateSize(array);
  return NewStringConstantASTNode(contents, array, location);
}

static ASTNode* MetaThrowException(VectorASTNode* call, Symbol* function,
                                   const char* message) {
  Namespace* std_namespace = NamespaceFindStdNamespace();
  if (std_namespace == NULL) {
    return NULL;
  }
  String meta_name;
  StringInit(&meta_name, "meta");
  Namespace* meta_namespace =
      NamespaceFindDirectChild(std_namespace, &meta_name);
  StringDestruct(&meta_name);
  if (meta_namespace == NULL) {
    return NULL;
  }
  String exception_name;
  StringInit(&exception_name, "exception");
  Symbol* exception =
      NamespaceFindSymbolInScope(meta_namespace, &exception_name);
  StringDestruct(&exception_name);
  if (exception == NULL || exception->type == NULL ||
      !TypeIsStructOrUnion(exception->type)) {
    return NULL;
  }

  Vector* arguments = NewVector();
  VectorAppend(arguments,
               NewMetaString(message != NULL ? message : "invalid reflection",
                             call->base.location));
  ReflectionEntityKind function_kind = ReflectionKindForSymbol(function);
  VectorAppend(arguments, NewReflectionConstantASTNode(
                              ReflectionCreateSymbol(function_kind, function,
                                                     call->base.location),
                              call->base.location));
  ASTNode* type_id =
      NewIdentifierASTNode(exception, call->base.location);
  ASTNodeSetType(type_id, TypeRecordCopy(exception->type));
  VectorASTNode* construction = (VectorASTNode*)NewVectorASTNode(
      AST_OP(call), TypeRecordCopy(exception->type), call->base.location,
      type_id, arguments);
  construction->base.flags |= kASTCXXFunctionalConstruction;
  ASTNode* object = AnalyzeExpression((ASTNode*)construction);
  if (object == NULL) {
    return NULL;
  }
  return AnalyzeExpression(NewThrowASTNode(object, call->base.location));
}

static TypeRecord* MetaCallReturnType(VectorASTNode* call) {
  return call != NULL && call->left != NULL && call->left->type != NULL &&
                 TypeIsFunction(call->left->type)
             ? call->left->type->next
             : NULL;
}

static ASTNode* MetaLowerToReturnType(VectorASTNode* call, ASTNode* expr) {
  TypeRecord* return_type = MetaCallReturnType(call);
  if (return_type == NULL || expr == NULL ||
      expr->op != AST_OP(braced_init)) {
    return expr;
  }
  return LowerCXXBracedInitToTarget(expr, return_type);
}

static Vector* MetaTemplateArgumentsVector(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  Vector* arguments = TypeSpecializationTemplateArguments(type);
  if (arguments != NULL) {
    return arguments;
  }
  type = TypeMaterializeClassTemplateSpecialization(&compiler->syntax, type);
  if (type == NULL) {
    return NULL;
  }
  return TypeSpecializationTemplateArguments(type);
}

static ASTNode* MetaMaterializeConstexprObjectResult(VectorASTNode* call,
                                                     ASTNode* initializer) {
  TypeRecord* return_type = MetaCallReturnType(call);
  SourceLocation location = call->base.location;
  if (return_type == NULL || initializer == NULL) {
    return initializer;
  }
  initializer = AnalyzeExpression(initializer);
  if (initializer == NULL) {
    SemanticError((ASTNode*)call,
                  "Reflection result is not a constant expression");
    return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeBool, kQualConst),
                                 location);
  }
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, TypeRecordCopy(return_type));
  temp->location = location;
  temp->flags.is_constexpr = true;
  ASTNode* init = NewExpressionInitializerASTNode(initializer, location);
  if (!ConstexprEvaluateObjectConstantForSymbol(temp, init)) {
    SemanticError((ASTNode*)call,
                  "Reflection result is not a constant expression");
    return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeBool, kQualConst),
                                 location);
  }
  temp->constexpr_initializer = init;
  ASTNode* value = NewIdentifierASTNode(temp, location);
  ASTNodeSetType(value, TypeRecordCopy(return_type));
  value->value_category = kValueCategoryPrvalue;
  return value;
}

static ASTNode* NewMetaStringViewResult(VectorASTNode* call, const char* text,
                                        bool u8) {
  const char* contents = text != NULL ? text : "";
  size_t length = strlen(contents);
  SourceLocation location = call->base.location;
  ASTNode* literal = u8 ? NewMetaU8String(contents, location)
                        : NewMetaString(contents, location);
  literal = AnalyzeExpression(literal);
  TypeRecord* char_type = u8 ? NewTypeRecordWithSize(kTypeChar8, kQualConst)
                             : NewTypeRecordWithSize(kTypeChar, kQualConst);
  TypeRecord* pointer_type = NewPointerTypeRecord(kQualPlain);
  TypeRecordChain(pointer_type, char_type);
  ASTNode* pointer =
      NewCastASTNode(TypeRecordCopy(pointer_type), location, literal);
  pointer = AnalyzeExpression(pointer);
  Vector* actuals = NewVector();
  VectorAppend(actuals, pointer);
  VectorAppend(actuals,
               NewIntConstantASTNode((int64_t)length, NewSizeTypeRecord(),
                                     location));
  TypeRecord* return_type = MetaCallReturnType(call);
  if (return_type == NULL || return_type->info.struct_info == NULL ||
      return_type->info.struct_info->tag_symbol == NULL) {
    ASTNode* braced = NewBracedInitializerASTNode(actuals, NULL, location);
    return MetaMaterializeConstexprObjectResult(
        call, MetaLowerToReturnType(call, braced));
  }
  TypeRecord* plain = TypeRecordCopy(return_type);
  plain->qualifiers = kQualPlain;
  ASTNode* type_id =
      NewIdentifierASTNode(return_type->info.struct_info->tag_symbol, location);
  ASTNodeSetType(type_id, plain);
  VectorASTNode* construction = (VectorASTNode*)NewVectorASTNode(
      AST_OP(call), TypeRecordCopy(return_type), location, type_id, actuals);
  construction->base.flags |= kASTCXXFunctionalConstruction;
  ASTNode* constructed = AnalyzeExpression((ASTNode*)construction);
  if (constructed == NULL) {
    SemanticError((ASTNode*)call,
                  "Reflection string_view result is not a constant expression");
    return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeBool, kQualConst),
                                 location);
  }
  return MetaMaterializeConstexprObjectResult(call, constructed);
}

static ReflectionValue* MetaReflectionFromObjectSlot(ASTNode* node,
                                                     size_t slot) {
  if (node == NULL) {
    return NULL;
  }
  if (node->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)node;
    if (braced->initializers != NULL && slot < braced->initializers->length) {
      return MetaEvaluateReflectionArg(braced->initializers->value.p[slot]);
    }
  }
  int64_t ivalue = 0;
  if (ConstexprEvaluateObjectSlotInteger(node, slot, &ivalue) && ivalue != 0) {
    return (ReflectionValue*)(intptr_t)ivalue;
  }
  return NULL;
}

static ASTNode* NewMetaSize(size_t value, SourceLocation location) {
  return NewIntConstantASTNode((int64_t)value, NewSizeTypeRecord(), location);
}

static ASTNode* NewMetaOperator(int64_t value, SourceLocation location) {
  return NewIntConstantASTNode(
      value, NewTypeRecordWithSize(kTypeInt, kQualConst), location);
}

static ReflectionValue* MetaEvaluateReflectionArg(ASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  ReflectionValue* value = SemanticReflectionValueFromExpression(node);
  if (value == NULL) {
    ConstEvalContext context;
    ConstEvalContextInit(&context);
    value = ConstexprEvaluateReflectionExpression(&context, node);
    ConstEvalContextDestruct(&context);
  }
  return value;
}

typedef struct {
  bool unchecked;
  ReflectionValue* scope;
  Struct* access_class;
  ReflectionValue* designating_reflection;
  Struct* designating_class;
} MetaAccessContext;

static Struct* MetaStructFromReflection(const ReflectionValue* value);

static Struct* MetaAccessClassFromScope(const ReflectionValue* scope) {
  Struct* cls = MetaStructFromReflection(scope);
  if (cls != NULL) {
    return cls;
  }
  if (scope != NULL && scope->symbol != NULL &&
      TypeIsFunction(scope->symbol->type)) {
    return scope->symbol->type->info.function.cxx_member_owner;
  }
  return NULL;
}

static MetaAccessContext MetaParseAccessContext(ASTNode* node) {
  MetaAccessContext context = {false, NULL, NULL, NULL, NULL};
  if (node == NULL) {
    return context;
  }
  int64_t include_private = 0;
  if (ConstexprEvaluateObjectSlotInteger(node, 0, &include_private) &&
      include_private != 0) {
    context.unchecked = true;
  }
  context.designating_reflection = MetaReflectionFromObjectSlot(node, 1);
  context.designating_class =
      MetaStructFromReflection(context.designating_reflection);
  context.scope = MetaReflectionFromObjectSlot(node, 2);
  context.access_class = MetaAccessClassFromScope(context.scope);
  return context;
}

static MetaAccessContext MetaAccessContextFromCall(VectorASTNode* call,
                                                   size_t arg_index) {
  MetaAccessContext context = {false, NULL, NULL, NULL, NULL};
  if (call == NULL || call->children == NULL ||
      call->children->length <= arg_index) {
    return context;
  }
  return MetaParseAccessContext(call->children->value.p[arg_index]);
}

static FunctionInfo* MetaFunctionInfo(const ReflectionValue* value) {
  if (value == NULL || value->symbol == NULL ||
      !TypeIsFunction(value->symbol->type)) {
    return NULL;
  }
  return &value->symbol->type->info.function;
}

static bool MetaSymbolIsConversionFunction(Symbol* symbol) {
  if (symbol == NULL || !TypeIsFunction(symbol->type) ||
      symbol->type->next == NULL) {
    return false;
  }
  String expected;
  StringInit(&expected, NULL);
  ConversionOperatorName(symbol->type->next, &expected);
  bool matches = expected.value != NULL &&
                 strcmp(expected.value, symbol->name.value) == 0;
  StringDestruct(&expected);
  return matches;
}

static bool MetaSymbolIsOperatorFunction(Symbol* symbol) {
  return symbol != NULL && symbol->name.value != NULL &&
         strncmp(symbol->name.value, "operator", 8) == 0 &&
         !MetaSymbolIsConversionFunction(symbol);
}

static bool MetaSymbolIsLiteralOperator(Symbol* symbol) {
  return symbol != NULL && symbol->name.value != NULL &&
         strncmp(symbol->name.value, "operator\"\"", 10) == 0;
}

static TypeRecord* MetaEntityType(const ReflectionValue* value) {
  if (value == NULL) {
    return NULL;
  }
  if (value->reflected_type != NULL) {
    return value->reflected_type;
  }
  if (value->symbol != NULL) {
    return value->symbol->type;
  }
  return NULL;
}

static Struct* MetaStructFromReflection(const ReflectionValue* value) {
  TypeRecord* type = MetaEntityType(value);
  if (type != NULL && TypeIsStructOrUnion(type) &&
      type->info.struct_info != NULL) {
    return type->info.struct_info;
  }
  return NULL;
}

static SourceLocation MetaEntityLocation(const ReflectionValue* value) {
  if (value == NULL) {
    return SOURCE_LOCATION_MISSING;
  }
  if (value->location != SOURCE_LOCATION_MISSING) {
    return value->location;
  }
  if (value->symbol != NULL) {
    return value->symbol->location;
  }
  return SOURCE_LOCATION_MISSING;
}

static void MetaFormatDisplayString(const ReflectionValue* value, String* out) {
  StringDestruct(out);
  StringInit(out, NULL);
  if (value == NULL) {
    return;
  }
  TypeRecord* type = MetaEntityType(value);
  if (type != NULL &&
      (value->kind == kReflectionType || value->kind == kReflectionTypeAlias ||
       value->reflected_type != NULL)) {
    TypeRecordToString(type, out);
    return;
  }
  const char* identifier = ReflectionValueIdentifier(value);
  if (identifier != NULL) {
    StringAppend(out, identifier);
  }
}

static CXXAccess ReflectionAccess(const ReflectionValue* value) {
  if (value != NULL && value->member != NULL) {
    return value->member->access;
  }
  if (value != NULL && value->kind == kReflectionBase &&
      value->parent_class != NULL &&
      value->base_index < value->parent_class->bases.length) {
    CXXBaseSpecifier* base =
        value->parent_class->bases.value.p[value->base_index];
    return base != NULL ? base->access : kAccessPublic;
  }
  return kAccessPublic;
}

static bool MetaAccessContextIncludesProtectedBase(const MetaAccessContext* ctx,
                                                   Struct* owner) {
  if (ctx == NULL || owner == NULL || ctx->access_class == NULL) {
    return false;
  }
  if (ctx->access_class == owner) {
    return true;
  }
  if (owner->tag_symbol != NULL && owner->tag_symbol->type != NULL &&
      ctx->access_class->tag_symbol != NULL &&
      ctx->access_class->tag_symbol->type != NULL) {
    return TypeIsDerivedFrom(ctx->access_class->tag_symbol->type,
                             owner->tag_symbol->type);
  }
  return false;
}

static bool MetaClassIsOrDerivesFrom(Struct* derived, Struct* base) {
  if (derived == NULL || base == NULL) {
    return false;
  }
  if (derived == base) {
    return true;
  }
  return derived->tag_symbol != NULL && derived->tag_symbol->type != NULL &&
         base->tag_symbol != NULL && base->tag_symbol->type != NULL &&
         TypeIsDerivedFrom(derived->tag_symbol->type, base->tag_symbol->type);
}

static bool MetaMemberAccessible(Struct* owner, StructMember* member,
                                 const MetaAccessContext* ctx) {
  if (member == NULL) {
    return false;
  }
  Struct* designating =
      ctx != NULL && ctx->designating_class != NULL
          ? ctx->designating_class
          : owner;
  if (!MetaClassIsOrDerivesFrom(designating, owner)) {
    return false;
  }
  if (ctx != NULL && ctx->unchecked) {
    return true;
  }
  if (member->access == kAccessPublic) {
    return true;
  }
  if (ctx != NULL && ctx->access_class == owner) {
    return true;
  }
  if (member->access == kAccessProtected &&
      MetaAccessContextIncludesProtectedBase(ctx, owner)) {
    return ctx->access_class == owner ||
           MetaClassIsOrDerivesFrom(designating, ctx->access_class);
  }
  return false;
}

static bool MetaBaseAccessible(Struct* owner, CXXBaseSpecifier* base,
                               const MetaAccessContext* ctx) {
  if (base == NULL) {
    return false;
  }
  Struct* designating =
      ctx != NULL && ctx->designating_class != NULL
          ? ctx->designating_class
          : owner;
  if (!MetaClassIsOrDerivesFrom(designating, owner)) {
    return false;
  }
  if (ctx != NULL && ctx->unchecked) {
    return true;
  }
  if (base->access == kAccessPublic) {
    return true;
  }
  if (ctx != NULL && ctx->access_class == owner) {
    return true;
  }
  if (base->access == kAccessProtected &&
      MetaAccessContextIncludesProtectedBase(ctx, owner)) {
    return true;
  }
  return false;
}

static bool MetaEntityAccessible(const ReflectionValue* value,
                                 const MetaAccessContext* ctx) {
  if (value == NULL || value->kind == kReflectionInvalid) {
    return false;
  }
  if (value->member != NULL) {
    return MetaMemberAccessible(value->parent_class, value->member, ctx);
  }
  if (value->kind == kReflectionBase) {
    CXXBaseSpecifier* base = NULL;
    if (value->parent_class != NULL &&
        value->base_index < value->parent_class->bases.length) {
      base = value->parent_class->bases.value.p[value->base_index];
    }
    return MetaBaseAccessible(value->parent_class, base, ctx);
  }
  return true;
}

static bool MetaTypeIsComplete(TypeRecord* type) {
  if (type == NULL || TypeContainsTemplateParameter(type)) {
    return false;
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* str = type->info.struct_info;
    return str->tag_symbol != NULL && !str->tag_symbol->flags.is_forward_declared &&
           str->size > 0;
  }
  if (TypeIsEnum(type) && type->info.enum_info != NULL) {
    Enum* enumeration = type->info.enum_info;
    return enumeration->tag_symbol != NULL &&
           !enumeration->tag_symbol->flags.is_forward_declared;
  }
  return type->size > 0;
}

static bool MetaHasExternalLinkage(Symbol* symbol) {
  return symbol != NULL && !symbol->flags.is_block_scope &&
         symbol->cxx_linkage == kCXXLinkageExternal &&
         !StorageIs(symbol->storage, STO(static));
}

static bool MetaHasInternalLinkage(Symbol* symbol) {
  if (symbol == NULL) {
    return false;
  }
  if (symbol->cxx_linkage == kCXXLinkageInternal) {
    return true;
  }
  return StorageIs(symbol->storage, STO(static)) && !symbol->flags.is_block_scope &&
         symbol->namespace_ == NULL &&
         (symbol->type == NULL || !TypeIsFunction(symbol->type) ||
          symbol->type->info.function.cxx_member_owner == NULL);
}

static bool MetaHasModuleLinkage(Symbol* symbol) {
  return symbol != NULL && symbol->cxx_linkage == kCXXLinkageModule;
}

static bool MetaHasAutomaticStorageDuration(Symbol* symbol) {
  return symbol != NULL && symbol->flags.is_local &&
         !StorageIs(symbol->storage, STO(static)) &&
         !StorageIs(symbol->storage, STO(extern));
}

static bool MetaHasStaticStorageDuration(Symbol* symbol) {
  return SymbolIsStaticStorageObject(symbol) ||
         (symbol != NULL && symbol->type != NULL && TypeIsFunction(symbol->type));
}

static bool MetaHasThreadStorageDuration(Symbol* symbol) {
  return symbol != NULL && StorageIs(symbol->storage, STO(thread));
}

static int MetaOperatorEnumForName(const char* name) {
  if (name == NULL) {
    return -1;
  }
  static const struct {
    const char* name;
    int value;
  } kOperators[] = {
      {"operator new", 0},
      {"operator delete", 1},
      {"operator new[]", 2},
      {"operator delete[]", 3},
      {"operator co_await", 4},
      {"operator()", 5},
      {"operator[]", 6},
      {"operator->", 7},
      {"operator->*", 8},
      {"operator~", 9},
      {"operator!", 10},
      {"operator+", 11},
      {"operator-", 12},
      {"operator*", 13},
      {"operator/", 14},
      {"operator%", 15},
      {"operator^", 16},
      {"operator&", 17},
      {"operator=", 18},
      {"operator|", 19},
      {"operator+=", 20},
      {"operator-=", 21},
      {"operator*=", 22},
      {"operator/=", 23},
      {"operator%=", 24},
      {"operator^=", 25},
      {"operator&=", 26},
      {"operator|=", 27},
      {"operator==", 28},
      {"operator!=", 29},
      {"operator<", 30},
      {"operator>", 31},
      {"operator<=", 32},
      {"operator>=", 33},
      {"operator<=>", 34},
      {"operator&&", 35},
      {"operator||", 36},
      {"operator<<", 37},
      {"operator>>", 38},
      {"operator<<=", 39},
      {"operator>>=", 40},
      {"operator++", 41},
      {"operator--", 42},
      {"operator,", 43},
  };
  for (size_t i = 0; i < sizeof(kOperators) / sizeof(kOperators[0]); i++) {
    if (strcmp(name, kOperators[i].name) == 0) {
      return kOperators[i].value;
    }
  }
  return -1;
}

static const char* MetaOperatorSymbolForEnum(int op) {
  static const struct {
    int value;
    const char* symbol;
  } kOperatorSymbols[] = {
      {0, " new"},       {1, " delete"},    {2, " new[]"},
      {3, " delete[]"},  {4, " co_await"},  {5, "()"},
      {6, "[]"},         {7, "->"},        {8, "->*"},
      {9, "~"},          {10, "!"},        {11, "+"},
      {12, "-"},         {13, "*"},        {14, "/"},
      {15, "%"},         {16, "^"},        {17, "&"},
      {18, "="},         {19, "|"},        {20, "+="},
      {21, "-="},        {22, "*="},       {23, "/="},
      {24, "%="},        {25, "^="},       {26, "&="},
      {27, "|="},        {28, "=="},       {29, "!="},
      {30, "<"},         {31, ">"},        {32, "<="},
      {33, ">="},        {34, "<=>"},      {35, "&&"},
      {36, "||"},        {37, "<<"},       {38, ">>"},
      {39, "<<="},       {40, ">>="},      {41, "++"},
      {42, "--"},        {43, ","},
  };
  for (size_t i = 0; i < sizeof(kOperatorSymbols) / sizeof(kOperatorSymbols[0]);
       i++) {
    if (kOperatorSymbols[i].value == op) {
      return kOperatorSymbols[i].symbol;
    }
  }
  return "";
}

static ReflectionValue* MetaTypeOf(ReflectionValue* value,
                                   SourceLocation location) {
  if (value == NULL) {
    return NULL;
  }
  if (value->kind == kReflectionType) {
    return value;
  }
  TypeRecord* type = MetaEntityType(value);
  return type != NULL ? ReflectionCreateType(type, NULL, location) : NULL;
}

static ReflectionValue* MetaParentOf(ReflectionValue* value,
                                     SourceLocation location) {
  if (value == NULL) {
    return NULL;
  }
  if (value->parent_class != NULL && value->parent_class->tag_symbol != NULL) {
    return ReflectionCreateType(value->parent_class->tag_symbol->type, NULL,
                                location);
  }
  if (value->symbol != NULL && value->symbol->type != NULL &&
      TypeIsFunction(value->symbol->type) &&
      value->symbol->type->info.function.cxx_member_owner != NULL) {
    Struct* owner = value->symbol->type->info.function.cxx_member_owner;
    return owner->tag_symbol != NULL
               ? ReflectionCreateType(owner->tag_symbol->type, NULL, location)
               : NULL;
  }
  if (value->symbol != NULL && value->symbol->namespace_ != NULL) {
    return ReflectionCreateNamespace(value->symbol->namespace_, location);
  }
  Namespace* namespace_ =
      value->namespace_ != NULL
          ? value->namespace_
          : value->symbol != NULL ? value->symbol->namespace_ : NULL;
  if ((value->kind == kReflectionNamespace ||
       value->kind == kReflectionGlobalNamespace) &&
      namespace_ != NULL) {
    namespace_ = namespace_->parent;
  }
  if (value->reflected_type != NULL && TypeIsStructOrUnion(value->reflected_type) &&
      value->reflected_type->info.struct_info != NULL &&
      value->reflected_type->info.struct_info->lexical_parent != NULL) {
    Struct* parent = value->reflected_type->info.struct_info->lexical_parent;
    return parent->tag_symbol != NULL
               ? ReflectionCreateType(parent->tag_symbol->type, NULL, location)
               : NULL;
  }
  return namespace_ != NULL
             ? ReflectionCreateNamespace(namespace_, location)
             : NULL;
}

static ReflectionValue* MetaTemplateOf(ReflectionValue* value,
                                       SourceLocation location) {
  if (value == NULL) {
    return NULL;
  }
  TypeRecord* type = MetaEntityType(value);
  Symbol* origin = NULL;
  if (type != NULL && type->template_origin != NULL) {
    origin = type->template_origin;
  } else if (value->symbol != NULL &&
             value->symbol->type != NULL &&
             value->symbol->type->template_origin != NULL) {
    origin = value->symbol->type->template_origin;
  } else if (value->symbol != NULL && value->symbol->flags.is_template) {
    origin = value->symbol;
  }
  if (origin == NULL) {
    return NULL;
  }
  ReflectionEntityKind kind = ReflectionKindForSymbol(origin);
  return kind == kReflectionType || kind == kReflectionTypeAlias
             ? ReflectionCreateType(origin->type, kind == kReflectionTypeAlias
                                                         ? origin
                                                         : NULL,
                                    location)
             : ReflectionCreateSymbol(kind, origin, location);
}

static ReflectionValue* MetaReflectionFromTemplateArgument(
    TemplateArgument* argument, SourceLocation location) {
  if (argument == NULL) {
    return NULL;
  }
  switch (argument->kind) {
    case kTemplateParameterType:
      return argument->type != NULL
                 ? ReflectionCreateType(argument->type, NULL, location)
                 : NULL;
    case kTemplateParameterNonType:
      if (argument->reflection_value != NULL) {
        return argument->reflection_value;
      }
      if (argument->value_symbol != NULL) {
        ReflectionEntityKind kind =
            ReflectionKindForSymbol(argument->value_symbol);
        return ReflectionCreateSymbol(kind, argument->value_symbol, location);
      }
      if (argument->type != NULL) {
        return ReflectionCreateScalar(
            argument->type, argument->int_value, 0.0,
            argument->value_kind == kTemplateValueNull, location);
      }
      return NULL;
    case kTemplateParameterTemplate:
      if (argument->template_symbol != NULL) {
        ReflectionEntityKind kind =
            ReflectionKindForSymbol(argument->template_symbol);
        return ReflectionCreateSymbol(kind, argument->template_symbol, location);
      }
      return NULL;
    default:
      return NULL;
  }
}

static void MetaAppendTemplateArgument(TemplateArgument* argument,
                                       SourceLocation location, Vector* values) {
  if (argument == NULL) {
    return;
  }
  if (argument->pack_arguments != NULL) {
    for (size_t i = 0; i < argument->pack_arguments->length; i++) {
      MetaAppendTemplateArgument(argument->pack_arguments->value.p[i], location,
                                 values);
    }
    return;
  }
  ReflectionValue* reflected =
      MetaReflectionFromTemplateArgument(argument, location);
  if (reflected != NULL) {
    VectorAppend(values, reflected);
  }
}

static Vector* MetaTemplateArgumentsOf(TypeRecord* type, ASTNode* operand,
                                       ReflectionValue* value,
                                       SourceLocation location) {
  Vector* values = NewVector();
  if (operand != NULL && operand->op == AST_OP(reflect)) {
    ReflectionASTNode* reflection = (ReflectionASTNode*)operand;
    if (reflection->operand_type != NULL) {
      type = TypeMaterializeClassTemplateSpecialization(
          &compiler->syntax, reflection->operand_type);
    }
  } else if (value != NULL && value->reflected_type != NULL) {
    type = TypeMaterializeClassTemplateSpecialization(
        &compiler->syntax, value->reflected_type);
  }
  Vector* arguments = MetaTemplateArgumentsVector(type);
  if (arguments == NULL && value != NULL &&
      value->substituted_arguments.length > 0) {
    arguments = &value->substituted_arguments;
  }
  if (arguments == NULL) {
    return values;
  }
  for (size_t i = 0; i < arguments->length; i++) {
    MetaAppendTemplateArgument(arguments->value.p[i], location, values);
  }
  return values;
}

static Vector* MetaParametersOf(ReflectionValue* value, SourceLocation location) {
  Vector* values = NewVector();
  FunctionInfo* function = MetaFunctionInfo(value);
  if (function == NULL) {
    return values;
  }
  for (size_t i = 0; i < function->prototype.length; i++) {
    Symbol* parameter = function->prototype.value.p[i];
    if (parameter != NULL) {
      VectorAppend(values, ReflectionCreateParameter(parameter, i, location));
    }
  }
  return values;
}

static ReflectionValue* MetaReturnTypeOf(ReflectionValue* value,
                                         SourceLocation location) {
  TypeRecord* type = MetaEntityType(value);
  if (type == NULL || !TypeIsFunction(type) || type->next == NULL) {
    return NULL;
  }
  return ReflectionCreateType(type->next, NULL, location);
}

static ReflectionValue* MetaObjectOf(ReflectionValue* value,
                                     SourceLocation location) {
  if (value == NULL) {
    return NULL;
  }
  switch (value->kind) {
    case kReflectionObject:
    case kReflectionVariable:
    case kReflectionDataMember:
    case kReflectionStructuredBinding:
      return value;
    default:
      break;
  }
  if (value->member != NULL && value->member->is_static &&
      value->symbol != NULL && !TypeIsFunction(value->symbol->type)) {
    return value;
  }
  (void)location;
  return NULL;
}

static ReflectionValue* MetaConstantOf(ReflectionValue* value,
                                       SourceLocation location) {
  if (value == NULL) {
    return NULL;
  }
  if (value->kind == kReflectionValue ||
      value->kind == kReflectionEnumerator) {
    return value;
  }
  if (value->symbol != NULL) {
    if (value->symbol->flags.is_constexpr && value->symbol->flags.value_set) {
      return value;
    }
    if (value->symbol->flags.is_template_parameter &&
        value->symbol->type != NULL) {
      return ReflectionCreateScalar(value->symbol->type,
                                    value->symbol->value.ivalue, 0.0, false,
                                    location);
    }
  }
  (void)location;
  return NULL;
}

static ReflectionValue* MetaVariableOf(ReflectionValue* value,
                                         SourceLocation location) {
  if (value == NULL) {
    return NULL;
  }
  if (value->kind == kReflectionDataMember ||
      value->kind == kReflectionVariable ||
      value->kind == kReflectionObject) {
    return value;
  }
  (void)location;
  return NULL;
}

static ReflectionValue* MetaCurrentFunction(SourceLocation location) {
  if (compiler->current_function == NULL ||
      !TypeIsFunction(compiler->current_function) ||
      compiler->current_function->info.function.symbol == NULL) {
    return ReflectionCreateInvalid(location);
  }
  return ReflectionCreateSymbol(
      kReflectionFunction, compiler->current_function->info.function.symbol,
      location);
}

static ReflectionValue* MetaCurrentClass(SourceLocation location) {
  Struct* owner = NULL;
  if (compiler->current_class_access_context != NULL) {
    owner = compiler->current_class_access_context;
  } else if (compiler->current_function != NULL &&
             TypeIsFunction(compiler->current_function) &&
             compiler->current_function->info.function.cxx_member_owner !=
                 NULL) {
    owner = compiler->current_function->info.function.cxx_member_owner;
  }
  if (owner == NULL || owner->tag_symbol == NULL) {
    return ReflectionCreateInvalid(location);
  }
  return ReflectionCreateType(owner->tag_symbol->type, NULL, location);
}

static ReflectionValue* MetaCurrentNamespace(SourceLocation location) {
  Namespace* namespace_ = compiler->syntax.current_namespace;
  if (namespace_ == NULL) {
    namespace_ = compiler->global_namespace;
  }
  return namespace_ != NULL ? ReflectionCreateNamespace(namespace_, location)
                            : ReflectionCreateInvalid(location);
}

static ReflectionValue* MetaCurrentScope(SourceLocation location) {
  if (compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function) &&
      compiler->current_function->info.function.symbol != NULL) {
    return MetaCurrentFunction(location);
  }
  ReflectionValue* current_class = MetaCurrentClass(location);
  if (current_class != NULL &&
      current_class->kind != kReflectionInvalid) {
    return current_class;
  }
  return MetaCurrentNamespace(location);
}

static ASTNode* MetaAccessContextExpression(VectorASTNode* call,
                                            ReflectionValue* scope,
                                            ReflectionValue* designating_class) {
  Vector* fields = NewVector();
  bool unchecked = scope == NULL || scope->kind == kReflectionInvalid;
  VectorAppend(fields,
               NewMetaBool(unchecked, call->base.location));
  VectorAppend(fields, NewReflectionConstantASTNode(
                           designating_class != NULL
                               ? designating_class
                               : ReflectionCreateInvalid(call->base.location),
                           call->base.location));
  VectorAppend(fields, NewReflectionConstantASTNode(
                           scope != NULL ? scope
                                         : ReflectionCreateInvalid(
                                               call->base.location),
                           call->base.location));
  ASTNode* aggregate =
      NewBracedInitializerASTNode(fields, NULL, call->base.location);
  Struct* saved_access_context = compiler->current_class_access_context;
  TypeRecord* return_type = MetaCallReturnType(call);
  if (return_type != NULL && TypeIsStructOrUnion(return_type)) {
    compiler->current_class_access_context = return_type->info.struct_info;
  }
  compiler->immediate_function_context_depth++;
  ASTNode* result = MetaMaterializeConstexprObjectResult(
      call, MetaLowerToReturnType(call, aggregate));
  compiler->immediate_function_context_depth--;
  compiler->current_class_access_context = saved_access_context;
  return result;
}

static void MetaCollectAnnotationsFromAttributes(Vector* attributes,
                                                 Vector* out,
                                                 ReflectionValue* type_filter,
                                                 SourceLocation location) {
  if (attributes == NULL) {
    return;
  }
  for (size_t i = 0; i < attributes->length; i++) {
    Attribute* attr = attributes->value.p[i];
    if (attr == NULL || attr->name.value == NULL ||
        strcmp(attr->name.value, "annotation") != 0) {
      continue;
    }
    if (attr->annotation_value == NULL) {
      EvaluateAnnotationAttribute(attr);
    }
    if (attr->annotation_value == NULL) {
      continue;
    }
    if (type_filter != NULL) {
      ReflectionValue* annotation_type =
          MetaTypeOf(attr->annotation_value, location);
      if (annotation_type == NULL ||
          !ReflectionValueEqual(annotation_type, type_filter)) {
        continue;
      }
    }
    VectorAppend(out, ReflectionCreateAnnotation(attr, location));
  }
}

static Vector* MetaCollectAnnotations(ReflectionValue* value,
                                      ReflectionValue* type_filter,
                                      SourceLocation location) {
  Vector* values = NewVector();
  if (value == NULL) {
    return values;
  }
  if (value->annotation != NULL) {
    VectorAppend(values, value);
    return values;
  }
  if (value->symbol != NULL) {
    MetaCollectAnnotationsFromAttributes(&value->symbol->attributes, values,
                                         type_filter, location);
  }
  if (value->member != NULL && value->member->symbol != NULL) {
    MetaCollectAnnotationsFromAttributes(&value->member->symbol->attributes,
                                         values, type_filter, location);
  }
  if (value->reflected_type != NULL && TypeIsStructOrUnion(value->reflected_type) &&
      value->reflected_type->info.struct_info != NULL &&
      value->reflected_type->info.struct_info->tag_symbol != NULL) {
    MetaCollectAnnotationsFromAttributes(
        &value->reflected_type->info.struct_info->tag_symbol->attributes, values,
        type_filter, location);
  }
  return values;
}

static bool MetaCollectRangeMember(Struct* owner, StructMember* member,
                                   MetaOperation operation,
                                   const MetaAccessContext* ctx,
                                   SourceLocation location, Vector* values) {
  if (member == NULL || member->symbol == NULL ||
      !MetaMemberAccessible(owner, member, ctx)) {
    return false;
  }
  if (operation == kMetaNonstaticDataMembersOf &&
      (member->is_static || TypeIsFunction(member->symbol->type))) {
    return false;
  }
  if (operation == kMetaStaticDataMembersOf &&
      (!member->is_static || TypeIsFunction(member->symbol->type))) {
    return false;
  }
  if (operation == kMetaSubobjectsOf &&
      (member->is_static || TypeIsFunction(member->symbol->type))) {
    return false;
  }
  VectorAppend(values, ReflectionCreateMember(member, owner, location));
  return true;
}

static void MetaCollectDirectSubobjects(Struct* owner,
                                        const MetaAccessContext* ctx,
                                        SourceLocation location,
                                        Vector* values) {
  if (owner == NULL) {
    return;
  }
  for (size_t i = 0; i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    if (base == NULL || !MetaBaseAccessible(owner, base, ctx)) {
      continue;
    }
    VectorAppend(values, ReflectionCreateBase(owner, i, location));
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    MetaCollectRangeMember(owner, member, kMetaSubobjectsOf, ctx, location,
                           values);
  }
}

static Vector* MetaCollectSubobjects(TypeRecord* type, SourceLocation location,
                                     const MetaAccessContext* ctx) {
  Vector* values = NewVector();
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return values;
  }
  MetaCollectDirectSubobjects(type->info.struct_info, ctx, location, values);
  return values;
}

static Vector* MetaCollectRange(MetaOperation operation, ReflectionValue* value,
                                SourceLocation location,
                                const MetaAccessContext* ctx,
                                VectorASTNode* call) {
  Vector* values = NewVector();
  if (value == NULL) {
    return values;
  }
  TypeRecord* type = value->reflected_type != NULL ? value->reflected_type
                                                   : MetaEntityType(value);
  if (operation == kMetaParametersOf) {
    VectorDelete(values);
    return MetaParametersOf(value, location);
  }
  if (operation == kMetaTemplateArgumentsOf) {
    VectorDelete(values);
    return MetaTemplateArgumentsOf(type,
                                   call != NULL && call->children != NULL &&
                                           call->children->length > 0
                                       ? call->children->value.p[0]
                                       : NULL,
                                   value, location);
  }
  if (operation == kMetaSubobjectsOf) {
    VectorDelete(values);
    return MetaCollectSubobjects(type, location, ctx);
  }
  if (operation == kMetaAnnotationsOf) {
    VectorDelete(values);
    return MetaCollectAnnotations(value, NULL, location);
  }
  if (type == NULL) {
    return values;
  }
  if (operation == kMetaEnumeratorsOf) {
    if (!TypeIsEnum(type) || type->info.enum_info == NULL) {
      return values;
    }
    Enum* enumeration = type->info.enum_info;
    for (size_t i = 0; i < enumeration->constants.length; i++) {
      Symbol* enumerator = enumeration->constants.value.p[i];
      VectorAppend(values,
                   ReflectionCreateSymbol(kReflectionEnumerator, enumerator,
                                          location));
    }
    return values;
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return values;
  }
  Struct* owner = type->info.struct_info;
  if (operation == kMetaBasesOf) {
    for (size_t i = 0; i < owner->bases.length; i++) {
      CXXBaseSpecifier* base = owner->bases.value.p[i];
      if (base != NULL && MetaBaseAccessible(owner, base, ctx)) {
        VectorAppend(values, ReflectionCreateBase(owner, i, location));
      }
    }
    return values;
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    MetaCollectRangeMember(owner, owner->members.value.p[i], operation, ctx,
                           location, values);
  }
  return values;
}

static bool MetaHasInaccessibleNonstaticDataMembers(TypeRecord* type,
                                                    const MetaAccessContext* ctx) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return false;
  }
  Struct* owner = type->info.struct_info;
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        TypeIsFunction(member->symbol->type)) {
      continue;
    }
    if (!MetaMemberAccessible(owner, member, ctx)) {
      return true;
    }
  }
  return false;
}

static bool MetaHasInaccessibleBases(TypeRecord* type,
                                     const MetaAccessContext* ctx) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return false;
  }
  Struct* owner = type->info.struct_info;
  for (size_t i = 0; i < owner->bases.length; i++) {
    if (!MetaBaseAccessible(owner, owner->bases.value.p[i], ctx)) {
      return true;
    }
  }
  return false;
}

static ASTNode* MetaMemberOffsetExpression(SourceLocation location,
                                           int byte_offset, int bit_offset) {
  TypeRecord* ptrdiff_type =
      NewTypeRecordWithSize(kTypeLong, kQualPlain);
  Vector* fields = NewVector();
  VectorAppend(fields,
               NewIntConstantASTNode(byte_offset, ptrdiff_type, location));
  VectorAppend(fields,
               NewIntConstantASTNode(bit_offset, ptrdiff_type, location));
  return NewBracedInitializerASTNode(fields, NULL, location);
}

static ASTNode* MetaSourceLocationExpression(SourceLocation location,
                                             SourceLocation entity_location) {
  int fileno = 0;
  int lineno = 0;
  int colno = 0;
  SourceLocationNumbers(entity_location, &fileno, &lineno, &colno);
  const char* filename = "";
  int start = 0;
  int end = 0;
  DecodeSourceLocation(entity_location, &filename, &lineno, &start, &end);
  (void)fileno;
  Vector* fields = NewVector();
  TypeRecord* u32 = NewTypeRecordWithSize(kTypeInt | kTypeUnsigned, kQualPlain);
  VectorAppend(fields,
               NewIntConstantASTNode(lineno, u32, location));
  VectorAppend(fields,
               NewIntConstantASTNode(colno > 0 ? colno : start + 1, u32, location));
  VectorAppend(fields, NewMetaString(filename != NULL ? filename : "", location));
  VectorAppend(fields, NewMetaString("", location));
  return NewBracedInitializerASTNode(fields, NULL, location);
}

static ASTNode* MetaRangeExpression(VectorASTNode* call, Vector* values) {
  Vector* array_values = NewVector();
  for (size_t i = 0; i < values->length; i++) {
    ReflectionValue* value = values->value.p[i];
    VectorAppend(array_values,
                 NewReflectionConstantASTNode(value, call->base.location));
  }
  ASTNode* array = NewBracedInitializerASTNode(
      array_values, NULL, call->base.location);
  Vector* aggregate_values = NewVector();
  VectorAppend(aggregate_values,
               NewIntConstantASTNode((int64_t)values->length, NewSizeTypeRecord(),
                                     call->base.location));
  VectorAppend(aggregate_values, array);
  VectorDelete(values);
  ASTNode* aggregate = NewBracedInitializerASTNode(
      aggregate_values, NULL, call->base.location);
  TypeRecord* return_type =
      call->left != NULL && call->left->type != NULL &&
              TypeIsFunction(call->left->type)
          ? call->left->type->next
          : NULL;
  if (return_type == NULL) {
    return aggregate;
  }
  aggregate = LowerCXXBracedInitToTarget(aggregate, return_type);
  return MetaMaterializeConstexprObjectResult(call, aggregate);
}

static bool MetaEvaluatePredicate(MetaOperation operation,
                                  const ReflectionValue* value,
                                  bool* result) {
  FunctionInfo* function = MetaFunctionInfo(value);
  Symbol* symbol = value != NULL ? value->symbol : NULL;
  TypeRecord* type = MetaEntityType(value);
  switch (operation) {
    case kMetaIsInvalid:
      *result = value != NULL && value->kind == kReflectionInvalid;
      return true;
    case kMetaIsNamespace:
      *result = value != NULL &&
                (value->kind == kReflectionNamespace ||
                 value->kind == kReflectionGlobalNamespace);
      return true;
    case kMetaIsNamespaceAlias:
      *result = value != NULL && value->kind == kReflectionNamespaceAlias;
      return true;
    case kMetaIsType:
      *result = value != NULL &&
                (value->kind == kReflectionType ||
                 value->kind == kReflectionTypeAlias ||
                 value->kind == kReflectionBase ||
                 value->kind == kReflectionConcept);
      return true;
    case kMetaIsTypeAlias:
      *result = value != NULL && value->kind == kReflectionTypeAlias;
      return true;
    case kMetaIsVariable:
      *result = value != NULL && value->kind == kReflectionVariable;
      return true;
    case kMetaIsObject:
      *result = value != NULL &&
                (value->kind == kReflectionObject ||
                 value->kind == kReflectionVariable);
      return true;
    case kMetaIsValue:
      *result = value != NULL &&
                (value->kind == kReflectionValue ||
                 value->kind == kReflectionEnumerator);
      return true;
    case kMetaIsFunction:
      *result = value != NULL && value->kind == kReflectionFunction;
      return true;
    case kMetaIsDataMember:
      *result = value != NULL && value->kind == kReflectionDataMember;
      return true;
    case kMetaIsEnumerator:
      *result = value != NULL && value->kind == kReflectionEnumerator;
      return true;
    case kMetaIsAnnotation:
      *result = value != NULL && value->kind == kReflectionAnnotation;
      return true;
    case kMetaIsStructuredBinding:
      *result = value != NULL && value->kind == kReflectionStructuredBinding;
      return true;
    case kMetaIsFunctionParameter:
      *result = value != NULL && value->kind == kReflectionFunctionParameter;
      return true;
    case kMetaIsBase:
      *result = value != NULL && value->kind == kReflectionBase;
      return true;
    case kMetaIsConcept:
      *result = value != NULL && value->kind == kReflectionConcept;
      return true;
    case kMetaIsTemplate:
      *result = value != NULL &&
                (ReflectionKindIsTemplate(value->kind) ||
                 value->kind == kReflectionConcept);
      return true;
    case kMetaIsFunctionTemplate:
      *result = value != NULL && value->kind == kReflectionFunctionTemplate;
      return true;
    case kMetaIsVariableTemplate:
      *result = value != NULL && value->kind == kReflectionVariableTemplate;
      return true;
    case kMetaIsClassTemplate:
      *result = value != NULL && value->kind == kReflectionClassTemplate;
      return true;
    case kMetaIsAliasTemplate:
      *result = value != NULL && value->kind == kReflectionAliasTemplate;
      return true;
    case kMetaIsClassMember:
      *result = value != NULL &&
                (value->member != NULL ||
                 (function != NULL && function->cxx_member_owner != NULL));
      return true;
    case kMetaIsNamespaceMember:
      *result = symbol != NULL && symbol->namespace_ != NULL &&
                value->member == NULL &&
                (function == NULL || function->cxx_member_owner == NULL);
      return true;
    case kMetaIsStaticMember:
      *result = value != NULL &&
                ((value->member != NULL && value->member->is_static) ||
                 (function != NULL && function->cxx_member_owner != NULL &&
                  symbol != NULL && value->member != NULL &&
                  value->member->is_static));
      return true;
    case kMetaIsPublic:
      *result = ReflectionAccess(value) == kAccessPublic;
      return true;
    case kMetaIsProtected:
      *result = ReflectionAccess(value) == kAccessProtected;
      return true;
    case kMetaIsPrivate:
      *result = ReflectionAccess(value) == kAccessPrivate;
      return true;
    case kMetaHasIdentifier:
      *result = ReflectionValueIdentifier(value) != NULL;
      return true;
    case kMetaHasParent:
      *result = MetaParentOf((ReflectionValue*)value, SOURCE_LOCATION_MISSING) !=
                NULL;
      return true;
    case kMetaHasTemplateArguments: {
      const Vector* arguments = MetaTemplateArgumentsVector(type);
      if ((arguments == NULL || arguments->length == 0) && value != NULL &&
          value->substituted_arguments.length > 0) {
        arguments = &value->substituted_arguments;
      }
      *result = arguments != NULL && arguments->length > 0;
      return true;
    }
    case kMetaHasDefaultMemberInitializer:
      *result = value != NULL && value->member != NULL &&
                value->member->default_initializer != NULL;
      return true;
    case kMetaHasDefaultArgument:
      *result = symbol != NULL && symbol->default_argument != NULL;
      return true;
    case kMetaIsBitField:
      *result = value != NULL && value->member != NULL &&
                value->member->bit_size > 0;
      return true;
    case kMetaIsMutableMember:
      *result = value != NULL && value->member != NULL &&
                value->member->is_mutable;
      return true;
    case kMetaIsConst:
      *result = type != NULL && TypeIsConst(type);
      return true;
    case kMetaIsVolatile:
      *result = type != NULL && TypeIsVolatile(type);
      return true;
    case kMetaIsCompleteType:
      *result = MetaTypeIsComplete(type);
      return true;
    case kMetaIsEnumerableType:
      *result = type != NULL && TypeIsEnum(type);
      return true;
    case kMetaHasExternalLinkage:
      *result = MetaHasExternalLinkage(symbol);
      return true;
    case kMetaHasInternalLinkage:
      *result = MetaHasInternalLinkage(symbol);
      return true;
    case kMetaHasModuleLinkage:
      *result = MetaHasModuleLinkage(symbol);
      return true;
    case kMetaHasCLanguageLinkage:
      *result = symbol != NULL && symbol->flags.is_c_linkage;
      return true;
    case kMetaHasLinkage:
      *result = symbol != NULL &&
                (MetaHasExternalLinkage(symbol) ||
                 MetaHasInternalLinkage(symbol) || MetaHasModuleLinkage(symbol));
      return true;
    case kMetaHasStaticStorageDuration:
      *result = MetaHasStaticStorageDuration(symbol);
      return true;
    case kMetaHasAutomaticStorageDuration:
      *result = MetaHasAutomaticStorageDuration(symbol);
      return true;
    case kMetaHasThreadStorageDuration:
      *result = MetaHasThreadStorageDuration(symbol);
      return true;
    case kMetaIsVirtual:
      *result = function != NULL && function->is_virtual;
      return true;
    case kMetaIsPureVirtual:
      *result = function != NULL && function->is_pure_virtual;
      return true;
    case kMetaIsOverride:
      *result = function != NULL && function->is_override;
      return true;
    case kMetaIsFinal:
      *result = (function != NULL && function->is_final) ||
                (type != NULL && TypeIsStructOrUnion(type) &&
                 type->info.struct_info != NULL &&
                 type->info.struct_info->is_final);
      return true;
    case kMetaIsDeleted:
      *result = function != NULL && function->is_deleted;
      return true;
    case kMetaIsDefaulted:
      *result = function != NULL && function->is_defaulted;
      return true;
    case kMetaIsUserProvided:
      *result = function != NULL && function->is_user_provided;
      return true;
    case kMetaIsUserDeclared:
      *result = function != NULL && function->is_user_declared;
      return true;
    case kMetaIsExplicit:
      *result = function != NULL && function->is_explicit;
      return true;
    case kMetaIsNoexcept:
      *result = function != NULL && function->is_noexcept;
      return true;
    case kMetaIsLvalueReferenceQualified:
      *result = function != NULL &&
                function->ref_qualifier == kCXXRefQualifierLValue;
      return true;
    case kMetaIsRvalueReferenceQualified:
      *result = function != NULL &&
                function->ref_qualifier == kCXXRefQualifierRValue;
      return true;
    case kMetaIsExplicitObjectParameter:
      *result = function != NULL && function->has_explicit_object_parameter;
      return true;
    case kMetaIsVarargFunction:
      *result = function != NULL && function->varargs;
      return true;
    case kMetaIsConstructor:
      *result = function != NULL && function->is_constructor;
      return true;
    case kMetaIsDestructor:
      *result = function != NULL && function->is_destructor;
      return true;
    case kMetaIsDefaultConstructor:
      *result = function != NULL &&
                function->cxx_special_member_kind ==
                    kCXXSpecialMemberDefaultConstructor;
      return true;
    case kMetaIsCopyConstructor:
      *result = function != NULL &&
                function->cxx_special_member_kind ==
                    kCXXSpecialMemberCopyConstructor;
      return true;
    case kMetaIsMoveConstructor:
      *result = function != NULL &&
                function->cxx_special_member_kind ==
                    kCXXSpecialMemberMoveConstructor;
      return true;
    case kMetaIsAssignment:
      *result = function != NULL &&
                (function->cxx_special_member_kind ==
                     kCXXSpecialMemberCopyAssignment ||
                 function->cxx_special_member_kind ==
                     kCXXSpecialMemberMoveAssignment);
      return true;
    case kMetaIsCopyAssignment:
      *result = function != NULL &&
                function->cxx_special_member_kind ==
                    kCXXSpecialMemberCopyAssignment;
      return true;
    case kMetaIsMoveAssignment:
      *result = function != NULL &&
                function->cxx_special_member_kind ==
                    kCXXSpecialMemberMoveAssignment;
      return true;
    case kMetaIsSpecialMemberFunction:
      *result = function != NULL &&
                function->cxx_special_member_kind != kCXXSpecialMemberNone;
      return true;
    case kMetaIsConversionFunction:
      *result = MetaSymbolIsConversionFunction(symbol);
      return true;
    case kMetaIsOperatorFunction:
      *result = MetaSymbolIsOperatorFunction(symbol);
      return true;
    case kMetaIsLiteralOperator:
      *result = MetaSymbolIsLiteralOperator(symbol);
      return true;
    case kMetaIsOperatorFunctionTemplate:
      *result = symbol != NULL && symbol->flags.is_template &&
                MetaSymbolIsOperatorFunction(symbol);
      return true;
    case kMetaIsLiteralOperatorTemplate:
      *result = symbol != NULL &&
                SyntaxIsCXXNumericLiteralOperatorTemplate(symbol);
      return true;
    case kMetaIsConversionFunctionTemplate:
      *result = symbol != NULL && symbol->flags.is_template &&
                MetaSymbolIsConversionFunction(symbol);
      return true;
    case kMetaIsConstructorTemplate:
      *result = symbol != NULL && symbol->flags.is_template &&
                function != NULL && function->is_constructor;
      return true;
    default:
      return false;
  }
}

static bool MetaOperationIsContextQuery(MetaOperation operation) {
  return operation == kMetaAccessContextCurrent ||
         operation == kMetaCurrentFunction ||
         operation == kMetaCurrentClass ||
         operation == kMetaCurrentNamespace ||
         operation == kMetaUnchecked ||
         operation == kMetaUnprivileged;
}

static bool MetaOperationIsRangeQuery(MetaOperation operation) {
  return operation == kMetaMembersOf ||
         operation == kMetaNonstaticDataMembersOf ||
         operation == kMetaStaticDataMembersOf ||
         operation == kMetaSubobjectsOf ||
         operation == kMetaBasesOf ||
         operation == kMetaEnumeratorsOf ||
         operation == kMetaParametersOf ||
         operation == kMetaTemplateArgumentsOf ||
         operation == kMetaAnnotationsOf ||
         operation == kMetaAnnotationsOfWithType;
}

static bool MetaRangeOperandIsValid(MetaOperation operation,
                                    ReflectionValue* value) {
  if (value == NULL || value->kind == kReflectionInvalid) {
    return false;
  }
  TypeRecord* type = MetaEntityType(value);
  switch (operation) {
    case kMetaMembersOf:
      return value->kind == kReflectionNamespace ||
             value->kind == kReflectionGlobalNamespace ||
             (type != NULL && TypeIsStructOrUnion(type) &&
              MetaTypeIsComplete(type));
    case kMetaBasesOf:
    case kMetaStaticDataMembersOf:
    case kMetaNonstaticDataMembersOf:
    case kMetaSubobjectsOf:
      return type != NULL && TypeIsStructOrUnion(type) &&
             MetaTypeIsComplete(type);
    case kMetaEnumeratorsOf:
      return type != NULL && TypeIsEnum(type) && MetaTypeIsComplete(type);
    case kMetaParametersOf:
      return MetaFunctionInfo(value) != NULL;
    case kMetaTemplateArgumentsOf: {
      Vector* arguments = MetaTemplateArgumentsVector(type);
      return arguments != NULL || value->substituted_arguments.length > 0;
    }
    case kMetaAnnotationsOf:
    case kMetaAnnotationsOfWithType:
      return true;
    default:
      return false;
  }
}

ASTNode* SemanticTryAnalyzeMetaCall(VectorASTNode* call) {
  Symbol* function = MetaCallSymbol(call);
  if (function == NULL) {
    return NULL;
  }
  const char* name = function->name.value;
  if (name == NULL) {
    return NULL;
  }
  MetaOperation operation = MetaOperationForName(name);
  if (operation == kMetaUnknown) {
    return NULL;
  }

  if (MetaOperationIsContextQuery(operation)) {
    ReflectionValue* context_result = NULL;
    switch (operation) {
      case kMetaAccessContextCurrent:
        return MetaAccessContextExpression(
            call, MetaCurrentScope(call->base.location), NULL);
      case kMetaUnchecked:
        return MetaAccessContextExpression(call, NULL, NULL);
      case kMetaUnprivileged:
        return MetaAccessContextExpression(
            call,
            ReflectionCreateNamespace(compiler->global_namespace,
                                      call->base.location),
            NULL);
      case kMetaCurrentFunction:
        context_result = MetaCurrentFunction(call->base.location);
        break;
      case kMetaCurrentClass:
        context_result = MetaCurrentClass(call->base.location);
        break;
      case kMetaCurrentNamespace:
        context_result = MetaCurrentNamespace(call->base.location);
        break;
      default:
        break;
    }
    if (context_result != NULL &&
        context_result->kind == kReflectionInvalid) {
      return MetaThrowException(call, function,
                                "no matching current reflection scope");
    }
    return context_result != NULL
               ? NewReflectionConstantASTNode(context_result, call->base.location)
               : NULL;
  }

  if (operation == kMetaVia) {
    if (call->children == NULL || call->children->length == 0 ||
        call->left == NULL ||
        (call->left->op != AST_OP(dot) &&
         call->left->op != AST_OP(arrow))) {
      return NULL;
    }
    BinaryASTNode* access = (BinaryASTNode*)call->left;
    MetaAccessContext context = MetaParseAccessContext(access->left);
    ReflectionValue* designating =
        MetaEvaluateReflectionArg(call->children->value.p[0]);
    Struct* designating_class = MetaStructFromReflection(designating);
    if (designating == NULL ||
        (designating->kind != kReflectionInvalid &&
         (designating_class == NULL ||
          !MetaTypeIsComplete(MetaEntityType(designating))))) {
      return MetaThrowException(
          call, function,
          "access_context::via requires a null reflection or complete class");
    }
    return MetaAccessContextExpression(call, context.scope, designating);
  }

  if (call->children == NULL || call->children->length == 0) {
    return NULL;
  }

  if (operation == kMetaSymbolOf || operation == kMetaU8SymbolOf) {
    int64_t op_enum = 0;
    if (!EvaluateIntegerExpression(call->children->value.p[0], &op_enum)) {
      ASTNodeSetType((ASTNode*)call, function->type->next);
      call->base.flags |= kASTAnalyzed | kASTDependentFunctorCall;
      return (ASTNode*)call;
    }
    bool u8 = operation == kMetaU8SymbolOf;
    return NewMetaStringViewResult(call, MetaOperatorSymbolForEnum((int)op_enum),
                                   u8);
  }

  ReflectionValue* value = MetaEvaluateReflectionArg(call->children->value.p[0]);
  if (value == NULL) {
    ASTNodeSetType((ASTNode*)call, function->type->next);
    call->base.flags |= kASTAnalyzed | kASTDependentFunctorCall;
    return (ASTNode*)call;
  }

  bool predicate = false;
  if (MetaEvaluatePredicate(operation, value, &predicate)) {
    return NewMetaBool(predicate, call->base.location);
  }

  if (operation == kMetaIsAccessible) {
    MetaAccessContext ctx = MetaAccessContextFromCall(call, 1);
    return NewMetaBool(MetaEntityAccessible(value, &ctx), call->base.location);
  }
  if (operation == kMetaHasInaccessibleNonstaticDataMembers) {
    MetaAccessContext ctx = MetaAccessContextFromCall(call, 1);
    return NewMetaBool(
        MetaHasInaccessibleNonstaticDataMembers(MetaEntityType(value), &ctx),
        call->base.location);
  }
  if (operation == kMetaHasInaccessibleBases) {
    MetaAccessContext ctx = MetaAccessContextFromCall(call, 1);
    return NewMetaBool(MetaHasInaccessibleBases(MetaEntityType(value), &ctx),
                       call->base.location);
  }
  if (operation == kMetaHasInaccessibleSubobjects) {
    MetaAccessContext ctx = MetaAccessContextFromCall(call, 1);
    bool inaccessible =
        MetaHasInaccessibleBases(MetaEntityType(value), &ctx) ||
        MetaHasInaccessibleNonstaticDataMembers(MetaEntityType(value), &ctx);
    return NewMetaBool(inaccessible, call->base.location);
  }

  if (operation == kMetaIdentifierOf || operation == kMetaDisplayStringOf ||
      operation == kMetaU8IdentifierOf || operation == kMetaU8DisplayStringOf) {
    String text;
    StringInit(&text, NULL);
    const char* contents = "";
    if (operation == kMetaIdentifierOf || operation == kMetaU8IdentifierOf) {
      contents = ReflectionValueIdentifier(value);
      if (contents == NULL) {
        StringDestruct(&text);
        return MetaThrowException(call, function,
                                  "reflection does not have an identifier");
      }
    } else {
      MetaFormatDisplayString(value, &text);
      contents = text.value != NULL ? text.value : "";
    }
    bool u8 = operation == kMetaU8IdentifierOf ||
              operation == kMetaU8DisplayStringOf;
    ASTNode* result = NewMetaStringViewResult(call, contents, u8);
    StringDestruct(&text);
    return result;
  }
  if (operation == kMetaOperatorOf) {
    int op = MetaOperatorEnumForName(
        value != NULL && value->symbol != NULL ? value->symbol->name.value
                                               : NULL);
    if (op < 0) {
      return MetaThrowException(
          call, function,
          "reflection does not represent an operator function or template");
    }
    return NewMetaOperator(op, call->base.location);
  }
  if (operation == kMetaSourceLocationOf) {
    ASTNode* aggregate =
        MetaSourceLocationExpression(call->base.location,
                                     MetaEntityLocation(value));
    return MetaLowerToReturnType(call, aggregate);
  }
  if (operation == kMetaOffsetOf) {
    if (value->member == NULL && value->kind != kReflectionBase &&
        value->kind != kReflectionDataMemberDescription) {
      return MetaThrowException(
          call, function,
          "offset_of requires a base or non-static data member reflection");
    }
    int byte_offset = value->member != NULL ? value->member->byte_offset : 0;
    int bit_offset = value->member != NULL ? value->member->bit_offset : 0;
    if (value->kind == kReflectionBase && value->parent_class != NULL &&
        value->base_index < value->parent_class->bases.length) {
      CXXBaseSpecifier* base =
          value->parent_class->bases.value.p[value->base_index];
      if (base != NULL) {
        byte_offset = base->byte_offset;
        bit_offset = 0;
      }
    }
    ASTNode* aggregate =
        MetaMemberOffsetExpression(call->base.location, byte_offset, bit_offset);
    return MetaLowerToReturnType(call, aggregate);
  }
  if (operation == kMetaSizeOf) {
    size_t size = 0;
    TypeRecord* type = MetaEntityType(value);
    if (type == NULL || !MetaTypeIsComplete(
                            TypeIsReference(type) ? type->next : type)) {
      return MetaThrowException(
          call, function,
          "size_of requires an entity with a complete object type");
    }
    if (type != NULL) {
      TypeRecordCalculateSize(type);
      TypeRecord* measured =
          TypeIsReference(type) ? NewPointerTypeRecord(kQualPlain) : NULL;
      if (measured != NULL) {
        TypeRecordChain(measured, TypeRecordCopy(type->next));
        TypeRecordCalculateSize(measured);
        size = (size_t)measured->size;
        TypeRecordDelete(measured);
      } else {
        size = (size_t)type->size;
      }
    }
    return NewMetaSize(size, call->base.location);
  }
  if (operation == kMetaAlignmentOf) {
    size_t alignment = 0;
    TypeRecord* entity_type = MetaEntityType(value);
    if (entity_type == NULL ||
        !MetaTypeIsComplete(TypeIsReference(entity_type)
                                ? entity_type->next
                                : entity_type)) {
      return MetaThrowException(
          call, function,
          "alignment_of requires an entity with a complete object type");
    }
    if (value->member != NULL && value->member->symbol != NULL &&
        value->member->symbol->alignment > 0) {
      alignment = (size_t)value->member->symbol->alignment;
    } else if (entity_type != NULL && TypeIsStructOrUnion(entity_type) &&
               entity_type->info.struct_info != NULL) {
      alignment = (size_t)entity_type->info.struct_info->alignment;
    } else if (entity_type != NULL) {
      TypeRecordCalculateSize(entity_type);
      alignment = (size_t)TypeRecordAlignment(entity_type);
    }
    return NewMetaSize(alignment, call->base.location);
  }
  if (operation == kMetaBitSizeOf) {
    size_t bits = 0;
    if (value->member != NULL && value->member->bit_size > 0) {
      bits = (size_t)value->member->bit_size;
    } else {
      TypeRecord* type = MetaEntityType(value);
      if (type != NULL &&
          MetaTypeIsComplete(TypeIsReference(type) ? type->next : type)) {
        TypeRecordCalculateSize(type);
        bits = (size_t)type->size * CHAR_BIT;
      } else {
        return MetaThrowException(
            call, function,
            "bit_size_of requires an entity with a complete object type");
      }
    }
    return NewMetaSize(bits, call->base.location);
  }

  ReflectionValue* result = NULL;
  switch (operation) {
    case kMetaTypeOf:
      result = MetaTypeOf(value, call->base.location);
      break;
    case kMetaParentOf:
      result = MetaParentOf(value, call->base.location);
      break;
    case kMetaDealias:
      result = value->kind == kReflectionTypeAlias
                   ? ReflectionCreateType(value->reflected_type, NULL,
                                          call->base.location)
                   : value;
      break;
    case kMetaTemplateOf:
      result = MetaTemplateOf(value, call->base.location);
      break;
    case kMetaReturnTypeOf:
      result = MetaReturnTypeOf(value, call->base.location);
      break;
    case kMetaObjectOf:
      result = MetaObjectOf(value, call->base.location);
      break;
    case kMetaConstantOf:
      result = MetaConstantOf(value, call->base.location);
      break;
    case kMetaVariableOf:
      result = MetaVariableOf(value, call->base.location);
      break;
    default:
      break;
  }
  if (result != NULL) {
    return NewReflectionConstantASTNode(result, call->base.location);
  }
  switch (operation) {
    case kMetaTypeOf:
    case kMetaParentOf:
    case kMetaDealias:
    case kMetaTemplateOf:
    case kMetaReturnTypeOf:
    case kMetaObjectOf:
    case kMetaConstantOf:
    case kMetaVariableOf:
      return MetaThrowException(call, function,
                                "reflection query precondition was not met");
    default:
      break;
  }

  if (MetaOperationIsRangeQuery(operation)) {
    if (!MetaRangeOperandIsValid(operation, value)) {
      return MetaThrowException(call, function,
                                "invalid reflection range query operand");
    }
    MetaAccessContext ctx = MetaAccessContextFromCall(call, 1);
    if (operation == kMetaAnnotationsOfWithType) {
      ReflectionValue* filter = call->children->length > 1
                                    ? MetaEvaluateReflectionArg(
                                          call->children->value.p[1])
                                    : NULL;
      return MetaRangeExpression(
          call, MetaCollectAnnotations(value, filter, call->base.location));
    }
    Vector* values =
        MetaCollectRange(operation, value, call->base.location, &ctx, call);
    return MetaRangeExpression(call, values);
  }
  return NULL;
}

static void EvaluateAnnotationAttribute(Attribute* attr) {
  if (attr == NULL || attr->annotation_expr == NULL ||
      attr->annotation_value != NULL) {
    return;
  }
  ASTNode* expression = AnalyzeExpression(attr->annotation_expr);
  ReflectionValue* value = SemanticReflectionValueFromExpression(expression);
  if (value == NULL) {
    ConstEvalContext context;
    ConstEvalContextInit(&context);
    value = ConstexprEvaluateReflectionExpression(&context, expression);
    ConstEvalContextDestruct(&context);
  }
  if (value != NULL) {
    attr->annotation_value = value;
  }
}

void SemanticAttachAnnotationAttributes(Vector* attributes, Symbol* symbol) {
  (void)symbol;
  if (attributes == NULL) {
    return;
  }
  for (size_t i = 0; i < attributes->length; i++) {
    EvaluateAnnotationAttribute(attributes->value.p[i]);
  }
}

ASTNode* SemanticAnalyzeConstevalBlock(ConstevalBlockASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
    SemanticError((ASTNode*)node, "consteval blocks require C++26");
  }
  compiler->immediate_function_context_depth++;
  compiler->constant_evaluation_required_depth++;
  if (node->body != NULL) {
    AnalyzeStatement(node->body);
  }
  compiler->constant_evaluation_required_depth--;
  compiler->immediate_function_context_depth--;
  node->base.flags |= kASTAnalyzed;
  return (ASTNode*)node;
}
