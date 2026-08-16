//
//  reflection_meta_traits.c
//  c_compiler
//
//  Compile-time evaluation of std::meta type-trait mirrors marked
//  [[davecc::meta_intrinsic]].
//

#include "reflection_meta_traits.h"

#include "reflection_semantics.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "binary_tree.h"
#include "compiler.h"
#include "constexpr.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_semantics.h"
#include "reflection.h"
#include "reflection_semantics.h"
#include "symbol_table.h"
#include "type_core.h"
#include "type_compare.h"
#include "type_core.h"
#include "type_class_internal.h"
#include "type_inheritance.h"
#include "type_member.h"
#include "type_special_member.h"
#include "type_template.h"
#include "type_class_internal.h"
#include "type_parse.h"
#include "type_traits_semantics.h"

#include "lex.h"
#include "source.h"

static bool MetaTraitEvaluateIntegralSymbol(Symbol* sym, size_t* value_out) {
  if (sym == NULL) {
    return false;
  }
  if (sym->flags.value_set) {
    if (value_out != NULL) {
      *value_out = (size_t)sym->value.ivalue;
    }
    return true;
  }
  if (sym->constexpr_initializer != NULL) {
    ASTNode* init =
        ConstexprInitializerExpression(sym->constexpr_initializer);
    int64_t ivalue = 0;
    if (EvaluateIntegerExpression(init, &ivalue)) {
      if (value_out != NULL) {
        *value_out = (size_t)ivalue;
      }
      return true;
    }
  }
  ConstEvalContext ctx;
  ConstEvalContextInit(&ctx);
  ASTNode* id = NewIdentifierASTNode(sym, sym->location);
  int64_t ivalue = 0;
  bool ok = EvaluateIntegerExpressionInContext(&ctx, id, &ivalue);
  ConstEvalContextDestruct(&ctx);
  ASTNodeDelete(id);
  if (ok && value_out != NULL) {
    *value_out = (size_t)ivalue;
  }
  return ok;
}

static bool MetaTraitEvaluateStructIntegralMember(Struct* str,
                                                  const char* member_name,
                                                  size_t* value_out) {
  if (str == NULL || member_name == NULL) {
    return false;
  }
  StructMember* member = FindStructMemberByName(str, member_name);
  if (member == NULL || member->symbol == NULL) {
    return false;
  }
  return MetaTraitEvaluateIntegralSymbol(member->symbol, value_out);
}

static Vector* MetaTraitFindTemplateArguments(TypeRecord* type);

static bool MetaTraitTemplateArgumentIsConcrete(TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->template_parameter_index >= 0) {
    return false;
  }
  if (arg->kind == kTemplateParameterType && arg->type != NULL &&
      (TypeContainsTemplateParameter(arg->type) ||
       TypeIsTemplateParameterPlaceholder(arg->type, NULL))) {
    return false;
  }
  if (arg->kind == kTemplateParameterNonType && arg->type != NULL &&
      TypeContainsTemplateParameter(arg->type) &&
      arg->reflection_value == NULL && arg->value_symbol == NULL) {
    return false;
  }
  return arg->type != NULL || arg->reflection_value != NULL;
}

static bool MetaTraitTemplateArgumentsAreConcrete(Vector* args) {
  if (args == NULL || args->length == 0) {
    return false;
  }
  for (size_t i = 0; i < args->length; i++) {
    if (!MetaTraitTemplateArgumentIsConcrete(args->value.p[i])) {
      return false;
    }
  }
  return true;
}

static Symbol* MetaTraitPrimaryTemplateSymbol(TypeRecord* query) {
  if (query == NULL) {
    return NULL;
  }
  if (query->template_origin != NULL) {
    return query->template_origin;
  }
  if (TypeIsStructOrUnion(query) && query->info.struct_info != NULL &&
      query->info.struct_info->tag_symbol != NULL) {
    Symbol* tag = query->info.struct_info->tag_symbol;
    if (tag->type != NULL && tag->type->template_origin != NULL) {
      return tag->type->template_origin;
    }
    if (tag->flags.is_template) {
      return tag;
    }
  }
  return NULL;
}

static TypeRecord* MetaTraitBuildTemplateIdType(TypeRecord* query, Vector* args) {
  if (query == NULL || args == NULL || args->length == 0) {
    return NULL;
  }
  Symbol* primary = MetaTraitPrimaryTemplateSymbol(query);
  if (primary == NULL || primary->type == NULL) {
    return NULL;
  }
  TypeRecord* template_id = TypeRecordCopy(primary->type);
  template_id->template_origin = primary;
  if (template_id->template_arguments != NULL) {
    VectorDeleteWithContents(template_id->template_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  template_id->template_arguments = TemplateArgumentVectorCopy(args);
  template_id->qualifiers |= query->qualifiers;
  return TypeRecordCalculateSize(template_id);
}

static TypeRecord* MetaTraitRecoverTemplateIdType(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  TypeRecord* query = TypeIsReference(type) ? type->next : type;
  if (query == NULL) {
    return TypeRecordCopy(type);
  }
  if (query->template_arguments != NULL &&
      query->template_arguments->length > 0) {
    return TypeRecordCopy(type);
  }
  Vector* args = MetaTraitFindTemplateArguments(type);
  if (args != NULL && args->length > 0) {
    TypeRecord* rebuilt = MetaTraitBuildTemplateIdType(query, args);
    if (rebuilt != NULL) {
      if (TypeIsReference(type)) {
        TypeRecord* ref = TypeRecordCopy(type);
        TypeRecordDelete(ref->next);
        ref->next = rebuilt;
        TypeRecordDelete(type);
        return TypeRecordCalculateSize(ref);
      }
      TypeRecordDelete(type);
      return rebuilt;
    }
  }
  if (TypeIsStructOrUnion(query) && query->info.struct_info != NULL &&
      query->info.struct_info->tag_symbol != NULL &&
      query->info.struct_info->tag_symbol->type != NULL) {
    TypeRecord* tag_type = query->info.struct_info->tag_symbol->type;
    if (tag_type->template_arguments != NULL &&
        tag_type->template_arguments->length > 0) {
      TypeRecord* recovered = TypeRecordCopy(tag_type);
      recovered->qualifiers |= query->qualifiers;
      return TypeRecordCalculateSize(recovered);
    }
  }
  return TypeRecordCopy(type);
}

static TypeRecord* MetaTraitReparseReflectedTypeFromSource(SourceLocation location);
static bool MetaTraitNeedsTemplateArgumentRecovery(TypeRecord* query);
static void MetaTraitRestoreCompilerTemplateState(size_t saved_declarations,
                                                  size_t saved_reflections);

static TypeRecord* MetaTraitRecoverQueryType(TypeRecord* type,
                                             ReflectionValue* value) {
  if (type == NULL) {
    return NULL;
  }
  TypeRecord* query = TypeIsReference(type) ? type->next : type;
  Vector* args = NULL;
  if (value != NULL && value->substituted_arguments.length > 0) {
    args = &value->substituted_arguments;
  } else {
    args = MetaTraitFindTemplateArguments(type);
  }
  if (args != NULL && args->length > 0 && query != NULL) {
    TypeRecord* rebuilt = MetaTraitBuildTemplateIdType(query, args);
    if (rebuilt != NULL) {
      if (TypeIsReference(type)) {
        TypeRecord* ref = TypeRecordCopy(type);
        TypeRecordDelete(ref->next);
        ref->next = rebuilt;
        return TypeRecordCalculateSize(ref);
      }
      return rebuilt;
    }
  }
  if (value != NULL && MetaTraitNeedsTemplateArgumentRecovery(query)) {
    TypeRecord* reparsed =
        MetaTraitReparseReflectedTypeFromSource(value->location);
    if (reparsed != NULL) {
      TypeRecord* recovered = MetaTraitRecoverQueryType(reparsed, NULL);
      TypeRecordDelete(reparsed);
      return recovered;
    }
  }
  return MetaTraitRecoverTemplateIdType(type);
}

static size_t MetaTraitTypeTextLength(const char* text) {
  if (text == NULL) {
    return 0;
  }
  size_t angle_depth = 0;
  size_t paren_depth = 0;
  size_t bracket_depth = 0;
  for (size_t i = 0; text[i] != '\0'; i++) {
    switch (text[i]) {
      case '<':
        angle_depth++;
        break;
      case '>':
        if (angle_depth > 0) {
          angle_depth--;
        }
        break;
      case '(':
        paren_depth++;
        break;
      case ')':
        if (angle_depth == 0 && paren_depth == 0 && bracket_depth == 0) {
          return i;
        }
        if (paren_depth > 0) {
          paren_depth--;
        }
        break;
      case '[':
        bracket_depth++;
        break;
      case ']':
        if (bracket_depth > 0) {
          bracket_depth--;
        }
        break;
      case ',':
      case ';':
        if (angle_depth == 0 && paren_depth == 0 && bracket_depth == 0) {
          return i;
        }
        break;
      default:
        break;
    }
  }
  size_t length = strlen(text);
  while (length > 0 &&
         (text[length - 1] == ' ' || text[length - 1] == '\t' ||
          text[length - 1] == '\n' || text[length - 1] == '\r')) {
    length--;
  }
  return length;
}

static char* MetaTraitReadSourceLine(const char* filename, int lineno) {
  if (filename == NULL || lineno <= 0) {
    return NULL;
  }
  FILE* file = fopen(filename, "r");
  if (file == NULL) {
    return NULL;
  }
  char buffer[4096];
  char* line = NULL;
  int current_line = 0;
  while (fgets(buffer, sizeof(buffer), file) != NULL) {
    current_line++;
    if (current_line != lineno) {
      continue;
    }
    size_t length = strlen(buffer);
    line = malloc(length + 1);
    if (line != NULL) {
      memcpy(line, buffer, length + 1);
    }
    break;
  }
  fclose(file);
  return line;
}

static TypeRecord* MetaTraitReparseReflectedTypeFromSource(SourceLocation location) {
  if (location == SOURCE_LOCATION_MISSING ||
      location == SOURCE_LOCATION_COMMAND_LINE ||
      compiler->syntax.lex == NULL) {
    return NULL;
  }
  const char* filename = NULL;
  int lineno = 0;
  int start = 0;
  int end = 0;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);
  if (filename == NULL || lineno <= 0) {
    return NULL;
  }
  char* line = MetaTraitReadSourceLine(filename, lineno);
  if (line == NULL) {
    return NULL;
  }
  size_t line_len = strlen(line);
  size_t type_start = (size_t)start;
  if (type_start + 2 <= line_len && line[type_start] == '^' &&
      line[type_start + 1] == '^') {
    type_start += 2;
  }
  while (type_start < line_len &&
         (line[type_start] == ' ' || line[type_start] == '\t')) {
    type_start++;
  }
  if (type_start >= line_len) {
    free(line);
    return NULL;
  }
  size_t type_len = MetaTraitTypeTextLength(line + type_start);
  if (type_len == 0) {
    free(line);
    return NULL;
  }
  String* type_string = NewStringWithLength(line + type_start, type_len);
  free(line);
  if (type_string == NULL) {
    return NULL;
  }

  SyntaxOpenScope(&compiler->syntax);
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  Lex lex;
  if (!LexInitFromString(&lex, filename, type_string,
                         compiler->syntax.lex->preprocessor)) {
    StringDelete(type_string);
    DiagnosticSuppressEnd();
    DiagnosticErrorTrapEnd(saved_trap);
    SyntaxCloseScope(&compiler->syntax);
    return NULL;
  }
  TypeParser parser;
  TypeParserInit(&parser, &lex, &compiler->syntax, STO(implicit),
                 compiler->syntax.context);
  TypeRecord* type = TypeParserParseType(&parser, true);
  TypeParserDestruct(&parser);
  LexDestruct(&lex);
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  SyntaxCloseScope(&compiler->syntax);
  if (type == NULL) {
    return NULL;
  }
  return TypeRecordCalculateSize(type);
}

static String* MetaTraitReflectionTypeTextFromLocation(SourceLocation location) {
  if (location == SOURCE_LOCATION_MISSING ||
      location == SOURCE_LOCATION_COMMAND_LINE) {
    return NULL;
  }
  const char* filename = NULL;
  int lineno = 0;
  int start = 0;
  int end = 0;
  (void)end;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);
  if (filename == NULL || lineno <= 0) {
    return NULL;
  }
  char* line = MetaTraitReadSourceLine(filename, lineno);
  if (line == NULL) {
    return NULL;
  }
  size_t line_len = strlen(line);
  size_t type_start = (size_t)start;
  if (type_start + 2 <= line_len && line[type_start] == '^' &&
      line[type_start + 1] == '^') {
    type_start += 2;
  }
  while (type_start < line_len &&
         (line[type_start] == ' ' || line[type_start] == '\t')) {
    type_start++;
  }
  if (type_start >= line_len) {
    free(line);
    return NULL;
  }
  size_t type_len = MetaTraitTypeTextLength(line + type_start);
  if (type_len == 0) {
    free(line);
    return NULL;
  }
  String* type_string = NewStringWithLength(line + type_start, type_len);
  free(line);
  return type_string;
}

static TypeRecord* MetaTraitParseFundamentalTypeName(const char* text,
                                                     size_t length) {
  while (length > 0 && isspace((unsigned char)*text)) {
    text++;
    length--;
  }
  while (length > 0 && isspace((unsigned char)text[length - 1])) {
    length--;
  }
  if (length == 0) {
    return NULL;
  }
  struct {
    const char* name;
    Type kind;
  } kFundamentals[] = {
      {"char", kTypeChar},   {"int", kTypeInt},     {"long", kTypeLong},
      {"short", kTypeShort}, {"float", kTypeFloat}, {"double", kTypeDouble},
      {"bool", kTypeBool},   {"void", kTypeVoid},
  };
  for (size_t i = 0; i < sizeof(kFundamentals) / sizeof(kFundamentals[0]); i++) {
    size_t name_len = strlen(kFundamentals[i].name);
    if (length == name_len &&
        memcmp(text, kFundamentals[i].name, name_len) == 0) {
      return TypeRecordCalculateSize(
          NewTypeRecordWithSize(kFundamentals[i].kind, kQualPlain));
    }
  }
  return NULL;
}

static TypeRecord* MetaTraitParseTypeFromSourceText(const char* filename,
                                                    const char* text,
                                                    size_t length) {
  if (filename == NULL || text == NULL || length == 0 ||
      compiler->syntax.lex == NULL) {
    return NULL;
  }
  TypeRecord* fundamental = MetaTraitParseFundamentalTypeName(text, length);
  if (fundamental != NULL) {
    return fundamental;
  }
  String* type_string = NewStringWithLength(text, length);
  if (type_string == NULL) {
    return NULL;
  }
  size_t saved_declarations = compiler->declaration_asts.length;
  size_t saved_reflections = compiler->reflection_values.length;
  SyntaxOpenScope(&compiler->syntax);
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  Lex lex;
  TypeRecord* type = NULL;
  if (LexInitFromString(&lex, filename, type_string,
                        compiler->syntax.lex->preprocessor)) {
    TypeParser parser;
    TypeParserInit(&parser, &lex, &compiler->syntax, STO(implicit),
                   compiler->syntax.context);
    type = TypeParserParseType(&parser, true);
    TypeParserDestruct(&parser);
    LexDestruct(&lex);
  } else {
    StringDelete(type_string);
  }
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  SyntaxCloseScope(&compiler->syntax);
  MetaTraitRestoreCompilerTemplateState(saved_declarations, saved_reflections);
  if (type == NULL) {
    return NULL;
  }
  return TypeRecordCalculateSize(type);
}

static TypeRecord* MetaTraitBuildTemplateIdFromReflectionSource(
    SourceLocation location, TypeRecord* query) {
  if (query == NULL) {
    return NULL;
  }
  Symbol* primary = MetaTraitPrimaryTemplateSymbol(query);
  if (primary == NULL || primary->type == NULL) {
    return NULL;
  }
  String* type_text = MetaTraitReflectionTypeTextFromLocation(location);
  if (type_text == NULL || type_text->value == NULL) {
    StringDelete(type_text);
    return NULL;
  }
  const char* filename = NULL;
  int lineno = 0;
  int start = 0;
  int end = 0;
  (void)lineno;
  (void)start;
  (void)end;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);
  const char* text = type_text->value;
  const char* lt = strchr(text, '<');
  if (lt == NULL) {
    StringDelete(type_text);
    return NULL;
  }
  size_t inner_start = (size_t)(lt - text) + 1;
  size_t angle_depth = 1;
  size_t inner_end = inner_start;
  for (; text[inner_end] != '\0'; inner_end++) {
    switch (text[inner_end]) {
      case '<':
        angle_depth++;
        break;
      case '>':
        angle_depth--;
        if (angle_depth == 0) {
          goto found_close;
        }
        break;
      default:
        break;
    }
  }
  StringDelete(type_text);
  return NULL;

found_close:;
  Vector* args = NewVector();
  size_t segment_start = inner_start;
  size_t depth_angle = 0;
  size_t depth_paren = 0;
  size_t depth_bracket = 0;
  for (size_t i = inner_start; i < inner_end; i++) {
    switch (text[i]) {
      case '<':
        depth_angle++;
        break;
      case '>':
        if (depth_angle > 0) {
          depth_angle--;
        }
        break;
      case '(':
        depth_paren++;
        break;
      case ')':
        if (depth_paren > 0) {
          depth_paren--;
        }
        break;
      case '[':
        depth_bracket++;
        break;
      case ']':
        if (depth_bracket > 0) {
          depth_bracket--;
        }
        break;
      case ',':
        if (depth_angle == 0 && depth_paren == 0 && depth_bracket == 0) {
          size_t seg_start = segment_start;
          while (seg_start < i &&
                 (text[seg_start] == ' ' || text[seg_start] == '\t')) {
            seg_start++;
          }
          TypeRecord* arg_type = MetaTraitParseTypeFromSourceText(
              filename, text + seg_start, i - seg_start);
          if (arg_type != NULL) {
            VectorAppend(args, NewTypeTemplateArgument(arg_type));
          }
          segment_start = i + 1;
        }
        break;
      default:
        break;
    }
  }
  while (segment_start < inner_end &&
         (text[segment_start] == ' ' || text[segment_start] == '\t')) {
    segment_start++;
  }
  if (segment_start < inner_end) {
    TypeRecord* arg_type = MetaTraitParseTypeFromSourceText(
        filename, text + segment_start, inner_end - segment_start);
    if (arg_type != NULL) {
      VectorAppend(args, NewTypeTemplateArgument(arg_type));
    }
  }
  StringDelete(type_text);
  if (args->length == 0) {
    VectorDeleteWithContents(args, (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  TypeRecord* rebuilt = MetaTraitBuildTemplateIdType(query, args);
  VectorDeleteWithContents(args, (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return rebuilt;
}

static bool MetaTraitNeedsTemplateArgumentRecovery(TypeRecord* query) {
  if (query == NULL) {
    return false;
  }
  if (query->template_arguments != NULL &&
      query->template_arguments->length > 0) {
    return false;
  }
  if (!TypeIsStructOrUnion(query) || query->info.struct_info == NULL) {
    return false;
  }
  Struct* str = query->info.struct_info;
  if (str->is_template && str->members.length == 0) {
    return true;
  }
  if (str->tag_symbol != NULL && str->tag_symbol->flags.is_template &&
      str->members.length == 0) {
    return true;
  }
  return MetaTraitPrimaryTemplateSymbol(query) != NULL &&
         MetaTraitFindTemplateArguments(query) == NULL &&
         str->members.length == 0;
}

static bool MetaTraitTypeIsConcreteForQuery(TypeRecord* type) {
  return type != NULL && !TypeContainsTemplateParameter(type) &&
         !TypeIsTemplateParameterPlaceholder(type, NULL);
}

static TypeRecord* MetaTraitCopyConcreteType(TypeRecord* type) {
  if (!MetaTraitTypeIsConcreteForQuery(type)) {
    return NULL;
  }
  return TypeRecordCalculateSize(TypeRecordCopy(type));
}

static TypeRecord* MetaTraitRecoverConcreteQueryType(TypeRecord* object_type,
                                                     ReflectionValue* value) {
  TypeRecord* recovered = MetaTraitRecoverQueryType(object_type, value);
  if (recovered == NULL) {
    return NULL;
  }
  Vector* args = MetaTraitFindTemplateArguments(recovered);
  if (args != NULL && MetaTraitTemplateArgumentsAreConcrete(args)) {
    return recovered;
  }
  if (value != NULL && value->substituted_arguments.length > 0 &&
      MetaTraitTemplateArgumentsAreConcrete(&value->substituted_arguments)) {
    TypeRecord* query = TypeIsReference(recovered) ? recovered->next : recovered;
    TypeRecord* rebuilt =
        MetaTraitBuildTemplateIdType(query, &value->substituted_arguments);
    TypeRecordDelete(recovered);
    if (rebuilt != NULL) {
      if (object_type != NULL && TypeIsReference(object_type)) {
        TypeRecord* ref = TypeRecordCopy(object_type);
        TypeRecordDelete(ref->next);
        ref->next = rebuilt;
        return TypeRecordCalculateSize(ref);
      }
      return rebuilt;
    }
  }
  TypeRecord* seed =
      value != NULL && value->reflected_type != NULL ? value->reflected_type
                                                     : object_type;
  if (value != NULL && value->location != SOURCE_LOCATION_MISSING &&
      value->location != SOURCE_LOCATION_COMMAND_LINE && seed != NULL) {
    TypeRecord* from_source =
        MetaTraitBuildTemplateIdFromReflectionSource(value->location, seed);
    if (from_source != NULL) {
      TypeRecordDelete(recovered);
      return from_source;
    }
  }
  return recovered;
}

static TypeRecord* MetaTraitMaterializeQueryType(TypeRecord* object_type);
static bool MetaTraitIsRecursiveTupleStruct(Struct* str);
static size_t MetaTraitRecursiveTupleSizeFromStruct(Struct* str);
static TypeRecord* MetaTraitRecursiveTupleElementType(Struct* str, size_t index);
static size_t MetaTraitStructTupleLikeSize(Struct* str);
static TypeRecord* MetaTraitStructTupleElementType(Struct* str, size_t index);

typedef enum {
  kMetaTraitUnknown,
  kMetaTraitAddConst,
  kMetaTraitAddCv,
  kMetaTraitAddLvalueReference,
  kMetaTraitAddPointer,
  kMetaTraitAddRvalueReference,
  kMetaTraitAddVolatile,
  kMetaTraitApplyResult,
  kMetaTraitCommonReference,
  kMetaTraitCommonType,
  kMetaTraitDecay,
  kMetaTraitExtent,
  kMetaTraitHasUniqueObjectRepresentations,
  kMetaTraitHasVirtualDestructor,
  kMetaTraitInvokeResult,
  kMetaTraitIsAbstractType,
  kMetaTraitIsAggregateType,
  kMetaTraitIsApplicableType,
  kMetaTraitIsArithmeticType,
  kMetaTraitIsArrayType,
  kMetaTraitIsAssignableType,
  kMetaTraitIsBaseOfType,
  kMetaTraitIsBoundedArrayType,
  kMetaTraitIsClassType,
  kMetaTraitIsCompoundType,
  kMetaTraitIsCompleteType,
  kMetaTraitIsConstType,
  kMetaTraitIsConstructibleType,
  kMetaTraitIsConvertibleType,
  kMetaTraitIsCopyAssignableType,
  kMetaTraitIsCopyConstructibleType,
  kMetaTraitIsDefaultConstructibleType,
  kMetaTraitIsDestructibleType,
  kMetaTraitIsEmptyType,
  kMetaTraitIsEnumType,
  kMetaTraitIsFinalType,
  kMetaTraitIsFloatingPointType,
  kMetaTraitIsFunctionType,
  kMetaTraitIsFundamentalType,
  kMetaTraitIsImplicitLifetimeType,
  kMetaTraitIsIntegralType,
  kMetaTraitIsInvocableRType,
  kMetaTraitIsInvocableType,
  kMetaTraitIsLayoutCompatibleType,
  kMetaTraitIsLvalueReferenceType,
  kMetaTraitIsMemberFunctionPointerType,
  kMetaTraitIsMemberObjectPointerType,
  kMetaTraitIsMemberPointerType,
  kMetaTraitIsMoveAssignableType,
  kMetaTraitIsMoveConstructibleType,
  kMetaTraitIsNothrowApplicableType,
  kMetaTraitIsNothrowAssignableType,
  kMetaTraitIsNothrowConstructibleType,
  kMetaTraitIsNothrowConvertibleType,
  kMetaTraitIsNothrowCopyAssignableType,
  kMetaTraitIsNothrowCopyConstructibleType,
  kMetaTraitIsNothrowDefaultConstructibleType,
  kMetaTraitIsNothrowDestructibleType,
  kMetaTraitIsNothrowInvocableRType,
  kMetaTraitIsNothrowInvocableType,
  kMetaTraitIsNothrowMoveAssignableType,
  kMetaTraitIsNothrowMoveConstructibleType,
  kMetaTraitIsNothrowSwappableType,
  kMetaTraitIsNothrowSwappableWithType,
  kMetaTraitIsNullPointerType,
  kMetaTraitIsObjectType,
  kMetaTraitIsPointerInterconvertibleBaseOfType,
  kMetaTraitIsPointerType,
  kMetaTraitIsPolymorphicType,
  kMetaTraitIsReflectionType,
  kMetaTraitIsReferenceType,
  kMetaTraitIsRvalueReferenceType,
  kMetaTraitIsSameType,
  kMetaTraitIsScalarType,
  kMetaTraitIsScopedEnumType,
  kMetaTraitIsSignedType,
  kMetaTraitIsStandardLayoutType,
  kMetaTraitIsStructuralType,
  kMetaTraitIsSwappableType,
  kMetaTraitIsSwappableWithType,
  kMetaTraitIsTriviallyAssignablType,
  kMetaTraitIsTriviallyConstructibleType,
  kMetaTraitIsTriviallyCopyAssignableType,
  kMetaTraitIsTriviallyCopyConstructibleType,
  kMetaTraitIsTriviallyDefaultConstructibleType,
  kMetaTraitIsTriviallyDestructibleType,
  kMetaTraitIsTriviallyMoveAssignableType,
  kMetaTraitIsTriviallyMoveConstructibleType,
  kMetaTraitIsTriviallyCopyableType,
  kMetaTraitIsUnboundedArrayType,
  kMetaTraitIsUnionType,
  kMetaTraitIsUnsignedType,
  kMetaTraitIsVirtualBaseOfType,
  kMetaTraitIsVoidType,
  kMetaTraitIsVolatileType,
  kMetaTraitMakeSigned,
  kMetaTraitMakeUnsigned,
  kMetaTraitRank,
  kMetaTraitReferenceConstructsFromTemporary,
  kMetaTraitReferenceConvertsFromTemporary,
  kMetaTraitRemoveAllExtents,
  kMetaTraitRemoveConst,
  kMetaTraitRemoveCv,
  kMetaTraitRemoveCvref,
  kMetaTraitRemoveExtent,
  kMetaTraitRemovePointer,
  kMetaTraitRemoveReference,
  kMetaTraitRemoveVolatile,
  kMetaTraitTupleElement,
  kMetaTraitTupleSize,
  kMetaTraitTypeOrder,
  kMetaTraitUnderlyingType,
  kMetaTraitUnwrapReference,
  kMetaTraitUnwrapRefDecay,
  kMetaTraitVariantAlternative,
  kMetaTraitVariantSize,
} MetaTraitOperation;

static TypeRecord* MetaTraitMaterializeQueryType(TypeRecord* object_type);

typedef struct {
  const char* name;
  MetaTraitOperation operation;
} MetaTraitOperationEntry;

// Sorted by name for binary search.
static const MetaTraitOperationEntry kMetaTraitOperations[] = {
    {"add_const", kMetaTraitAddConst},
    {"add_cv", kMetaTraitAddCv},
    {"add_lvalue_reference", kMetaTraitAddLvalueReference},
    {"add_pointer", kMetaTraitAddPointer},
    {"add_rvalue_reference", kMetaTraitAddRvalueReference},
    {"add_volatile", kMetaTraitAddVolatile},
    {"apply_result", kMetaTraitApplyResult},
    {"common_reference", kMetaTraitCommonReference},
    {"common_type", kMetaTraitCommonType},
    {"decay", kMetaTraitDecay},
    {"extent", kMetaTraitExtent},
    {"has_unique_object_representations", kMetaTraitHasUniqueObjectRepresentations},
    {"has_virtual_destructor", kMetaTraitHasVirtualDestructor},
    {"invoke_result", kMetaTraitInvokeResult},
    {"is_abstract_type", kMetaTraitIsAbstractType},
    {"is_aggregate_type", kMetaTraitIsAggregateType},
    {"is_applicable_type", kMetaTraitIsApplicableType},
    {"is_arithmetic_type", kMetaTraitIsArithmeticType},
    {"is_array_type", kMetaTraitIsArrayType},
    {"is_assignable_type", kMetaTraitIsAssignableType},
    {"is_base_of_type", kMetaTraitIsBaseOfType},
    {"is_bounded_array_type", kMetaTraitIsBoundedArrayType},
    {"is_class_type", kMetaTraitIsClassType},
    {"is_complete_type", kMetaTraitIsCompleteType},
    {"is_compound_type", kMetaTraitIsCompoundType},
    {"is_const_type", kMetaTraitIsConstType},
    {"is_constructible_type", kMetaTraitIsConstructibleType},
    {"is_convertible_type", kMetaTraitIsConvertibleType},
    {"is_copy_assignable_type", kMetaTraitIsCopyAssignableType},
    {"is_copy_constructible_type", kMetaTraitIsCopyConstructibleType},
    {"is_default_constructible_type", kMetaTraitIsDefaultConstructibleType},
    {"is_destructible_type", kMetaTraitIsDestructibleType},
    {"is_empty_type", kMetaTraitIsEmptyType},
    {"is_enum_type", kMetaTraitIsEnumType},
    {"is_final_type", kMetaTraitIsFinalType},
    {"is_floating_point_type", kMetaTraitIsFloatingPointType},
    {"is_function_type", kMetaTraitIsFunctionType},
    {"is_fundamental_type", kMetaTraitIsFundamentalType},
    {"is_implicit_lifetime_type", kMetaTraitIsImplicitLifetimeType},
    {"is_integral_type", kMetaTraitIsIntegralType},
    {"is_invocable_r_type", kMetaTraitIsInvocableRType},
    {"is_invocable_type", kMetaTraitIsInvocableType},
    {"is_layout_compatible_type", kMetaTraitIsLayoutCompatibleType},
    {"is_lvalue_reference_type", kMetaTraitIsLvalueReferenceType},
    {"is_member_function_pointer_type", kMetaTraitIsMemberFunctionPointerType},
    {"is_member_object_pointer_type", kMetaTraitIsMemberObjectPointerType},
    {"is_member_pointer_type", kMetaTraitIsMemberPointerType},
    {"is_move_assignable_type", kMetaTraitIsMoveAssignableType},
    {"is_move_constructible_type", kMetaTraitIsMoveConstructibleType},
    {"is_nothrow_applicable_type", kMetaTraitIsNothrowApplicableType},
    {"is_nothrow_assignable_type", kMetaTraitIsNothrowAssignableType},
    {"is_nothrow_constructible_type", kMetaTraitIsNothrowConstructibleType},
    {"is_nothrow_convertible_type", kMetaTraitIsNothrowConvertibleType},
    {"is_nothrow_copy_assignable_type", kMetaTraitIsNothrowCopyAssignableType},
    {"is_nothrow_copy_constructible_type", kMetaTraitIsNothrowCopyConstructibleType},
    {"is_nothrow_default_constructible_type",
     kMetaTraitIsNothrowDefaultConstructibleType},
    {"is_nothrow_destructible_type", kMetaTraitIsNothrowDestructibleType},
    {"is_nothrow_invocable_r_type", kMetaTraitIsNothrowInvocableRType},
    {"is_nothrow_invocable_type", kMetaTraitIsNothrowInvocableType},
    {"is_nothrow_move_assignable_type", kMetaTraitIsNothrowMoveAssignableType},
    {"is_nothrow_move_constructible_type",
     kMetaTraitIsNothrowMoveConstructibleType},
    {"is_nothrow_swappable_type", kMetaTraitIsNothrowSwappableType},
    {"is_nothrow_swappable_with_type", kMetaTraitIsNothrowSwappableWithType},
    {"is_null_pointer_type", kMetaTraitIsNullPointerType},
    {"is_object_type", kMetaTraitIsObjectType},
    {"is_pointer_interconvertible_base_of_type",
     kMetaTraitIsPointerInterconvertibleBaseOfType},
    {"is_pointer_type", kMetaTraitIsPointerType},
    {"is_polymorphic_type", kMetaTraitIsPolymorphicType},
    {"is_reference_type", kMetaTraitIsReferenceType},
    {"is_reflection_type", kMetaTraitIsReflectionType},
    {"is_rvalue_reference_type", kMetaTraitIsRvalueReferenceType},
    {"is_same_type", kMetaTraitIsSameType},
    {"is_scalar_type", kMetaTraitIsScalarType},
    {"is_scoped_enum_type", kMetaTraitIsScopedEnumType},
    {"is_signed_type", kMetaTraitIsSignedType},
    {"is_standard_layout_type", kMetaTraitIsStandardLayoutType},
    {"is_structural_type", kMetaTraitIsStructuralType},
    {"is_swappable_type", kMetaTraitIsSwappableType},
    {"is_swappable_with_type", kMetaTraitIsSwappableWithType},
    {"is_trivially_assignable_type", kMetaTraitIsTriviallyAssignablType},
    {"is_trivially_constructible_type", kMetaTraitIsTriviallyConstructibleType},
    {"is_trivially_copy_assignable_type",
     kMetaTraitIsTriviallyCopyAssignableType},
    {"is_trivially_copy_constructible_type",
     kMetaTraitIsTriviallyCopyConstructibleType},
    {"is_trivially_copyable_type", kMetaTraitIsTriviallyCopyableType},
    {"is_trivially_default_constructible_type",
     kMetaTraitIsTriviallyDefaultConstructibleType},
    {"is_trivially_destructible_type", kMetaTraitIsTriviallyDestructibleType},
    {"is_trivially_move_assignable_type",
     kMetaTraitIsTriviallyMoveAssignableType},
    {"is_trivially_move_constructible_type",
     kMetaTraitIsTriviallyMoveConstructibleType},
    {"is_unbounded_array_type", kMetaTraitIsUnboundedArrayType},
    {"is_union_type", kMetaTraitIsUnionType},
    {"is_unsigned_type", kMetaTraitIsUnsignedType},
    {"is_virtual_base_of_type", kMetaTraitIsVirtualBaseOfType},
    {"is_void_type", kMetaTraitIsVoidType},
    {"is_volatile_type", kMetaTraitIsVolatileType},
    {"make_signed", kMetaTraitMakeSigned},
    {"make_unsigned", kMetaTraitMakeUnsigned},
    {"rank", kMetaTraitRank},
    {"reference_constructs_from_temporary",
     kMetaTraitReferenceConstructsFromTemporary},
    {"reference_converts_from_temporary",
     kMetaTraitReferenceConvertsFromTemporary},
    {"remove_all_extents", kMetaTraitRemoveAllExtents},
    {"remove_const", kMetaTraitRemoveConst},
    {"remove_cv", kMetaTraitRemoveCv},
    {"remove_cvref", kMetaTraitRemoveCvref},
    {"remove_extent", kMetaTraitRemoveExtent},
    {"remove_pointer", kMetaTraitRemovePointer},
    {"remove_reference", kMetaTraitRemoveReference},
    {"remove_volatile", kMetaTraitRemoveVolatile},
    {"tuple_element", kMetaTraitTupleElement},
    {"tuple_size", kMetaTraitTupleSize},
    {"type_order", kMetaTraitTypeOrder},
    {"underlying_type", kMetaTraitUnderlyingType},
    {"unwrap_ref_decay", kMetaTraitUnwrapRefDecay},
    {"unwrap_reference", kMetaTraitUnwrapReference},
    {"variant_alternative", kMetaTraitVariantAlternative},
    {"variant_size", kMetaTraitVariantSize},
};

static MetaTraitOperation MetaTraitOperationForName(const char* name) {
  size_t begin = 0;
  size_t end = sizeof(kMetaTraitOperations) / sizeof(kMetaTraitOperations[0]);
  while (begin < end) {
    size_t middle = begin + (end - begin) / 2;
    int comparison = strcmp(name, kMetaTraitOperations[middle].name);
    if (comparison == 0) {
      return kMetaTraitOperations[middle].operation;
    }
    if (comparison < 0) {
      end = middle;
    } else {
      begin = middle + 1;
    }
  }
  return kMetaTraitUnknown;
}

static Symbol* MetaTraitCallSymbol(VectorASTNode* call) {
  if (call == NULL || call->left == NULL ||
      call->left->op != AST_OP(identifier)) {
    return NULL;
  }
  Symbol* symbol = ((IdentifierASTNode*)call->left)->symbol;
  for (size_t depth = 0;
       symbol != NULL && symbol->alias_target != NULL && depth < 64;
       depth++) {
    symbol = symbol->alias_target;
  }
  if (symbol == NULL || !SymbolHasAttribute(symbol, "meta_intrinsic")) {
    return NULL;
  }
  return symbol;
}

static bool MetaTraitArgIsDependent(ASTNode* expression) {
  if (expression == NULL) {
    return true;
  }
  if (expression->op == AST_OP(reflection_constant)) {
    return false;
  }
  if (expression->op == AST_OP(reflect)) {
    ReflectionASTNode* reflection = (ReflectionASTNode*)expression;
    if (reflection->value != NULL) {
      return false;
    }
    if (reflection->operand_kind == kReflectionOperandType &&
        reflection->operand_type != NULL &&
        !TypeContainsTemplateParameter(reflection->operand_type) &&
        !TypeIsUnknown(reflection->operand_type)) {
      return false;
    }
  }
  if (expression->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)expression)->symbol;
    if (symbol != NULL && symbol->flags.is_template_parameter) {
      if (TypeIsReflection(symbol->type) && symbol->flags.value_set &&
          symbol->value.other != NULL) {
        return false;
      }
      return true;
    }
  }
  return (expression->flags &
          (kASTReferencesParameterPack | kASTDependentFunctorCall)) != 0 ||
         TypeContainsTemplateParameter(expression->type);
}

static ASTNode* MetaTraitUnwrapInitExpression(ASTNode* expression) {
  if (expression == NULL) {
    return NULL;
  }
  if (expression->op == AST_OP(expr_init)) {
    return MetaTraitUnwrapInitExpression(
        ((ExpressionInitializerASTNode*)expression)->expr);
  }
  if (expression->op == AST_OP(designated_init)) {
    return MetaTraitUnwrapInitExpression(
        ((DesignatedInitializerASTNode*)expression)->init);
  }
  return expression;
}

static ReflectionValue* MetaTraitReflectionFromTemplateArgument(
    TemplateArgument* argument, SourceLocation location) {
  if (argument == NULL) {
    return NULL;
  }
  switch (argument->kind) {
    case kTemplateParameterType:
      if (argument->reflection_value != NULL) {
        return argument->reflection_value;
      }
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
      if (argument->type != NULL && TypeIsReflection(argument->type)) {
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

static int MetaTraitTemplateParameterIndex(Symbol* param) {
  if (param == NULL) {
    return -1;
  }
  if (param->template_parameter_index >= 0) {
    return param->template_parameter_index;
  }
  return param->dependent_value_template_parameter_index;
}

static bool MetaTraitPrototypeContainsParameter(TypeRecord* function_type,
                                                Symbol* param) {
  if (function_type == NULL || param == NULL ||
      !TypeIsFunction(function_type)) {
    return false;
  }
  int index = MetaTraitTemplateParameterIndex(param);
  if (index >= 0 &&
      function_type->info.function.template_parameters.length > 0 &&
      (size_t)index < function_type->info.function.template_parameters.length) {
    return true;
  }
  for (size_t i = 0; i < function_type->info.function.prototype.length; i++) {
    Symbol* formal = function_type->info.function.prototype.value.p[i];
    if (formal == param) {
      return true;
    }
    if (index >= 0 && formal != NULL &&
        MetaTraitTemplateParameterIndex(formal) == index) {
      return true;
    }
  }
  return false;
}

typedef struct MetaTraitParameterSearch {
  Symbol* param;
  TypeRecord* found;
} MetaTraitParameterSearch;

static void MetaTraitSearchSymbolForParameter(BinaryTreeNode* node, int depth,
                                              void* data) {
  (void)depth;
  MetaTraitParameterSearch* search = data;
  if (search == NULL || search->found != NULL || node == NULL) {
    return;
  }
  for (Symbol* symbol = ((SymbolNode*)node)->symbol; symbol != NULL;
       symbol = symbol->overload_next) {
    if (symbol->type == NULL || !TypeIsFunction(symbol->type)) {
      continue;
    }
    TypeRecord* function_type = symbol->type;
    if (function_type->info.function.template_origin != NULL &&
        MetaTraitPrototypeContainsParameter(function_type, search->param) &&
        function_type->template_arguments != NULL) {
      search->found = function_type;
      return;
    }
    if (MetaTraitPrototypeContainsParameter(function_type, search->param) &&
        function_type->template_arguments != NULL) {
      search->found = function_type;
      return;
    }
    for (size_t i = 0;
         i < function_type->info.function.template_instantiations.length; i++) {
      Symbol* instantiation =
          function_type->info.function.template_instantiations.value.p[i];
      if (instantiation == NULL || instantiation->type == NULL) {
        continue;
      }
      TypeRecord* inst_type = instantiation->type;
      if (MetaTraitPrototypeContainsParameter(inst_type, search->param) &&
          inst_type->template_arguments != NULL) {
        search->found = inst_type;
        return;
      }
    }
  }
}

static void MetaTraitSearchTemplateInstantiations(BinaryTreeNode* node,
                                                  int depth, void* data) {
  (void)depth;
  MetaTraitParameterSearch* search = data;
  int param_index = MetaTraitTemplateParameterIndex(
      search != NULL ? search->param : NULL);
  if (search == NULL || search->param == NULL || param_index < 0) {
    return;
  }
  size_t index = (size_t)param_index;
  for (Symbol* symbol = ((SymbolNode*)node)->symbol; symbol != NULL;
       symbol = symbol->overload_next) {
    if (symbol->type == NULL || !TypeIsFunction(symbol->type) ||
        !symbol->flags.is_template) {
      continue;
    }
    for (size_t i = 0;
         i < symbol->type->info.function.template_instantiations.length; i++) {
      Symbol* instantiation =
          symbol->type->info.function.template_instantiations.value.p[i];
      if (instantiation == NULL || instantiation->type == NULL ||
          instantiation->type->template_arguments == NULL ||
          index >= instantiation->type->template_arguments->length) {
        continue;
      }
      TemplateArgument* arg =
          instantiation->type->template_arguments->value.p[index];
      if (MetaTraitReflectionFromTemplateArgument(arg,
                                                  SOURCE_LOCATION_MISSING) !=
          NULL) {
        search->found = instantiation->type;
        return;
      }
    }
  }
}

static void MetaTraitSearchNamespaceForParameter(Namespace* ns,
                                                 MetaTraitParameterSearch* search) {
  if (ns == NULL || search == NULL || search->found != NULL) {
    return;
  }
  BinaryTreeTraverse(&ns->symbol_table, MetaTraitSearchSymbolForParameter,
                     search);
  if (search->found == NULL) {
    BinaryTreeTraverse(&ns->symbol_table, MetaTraitSearchTemplateInstantiations,
                       search);
  }
  if (search->found != NULL) {
    return;
  }
  for (size_t i = 0; i < ns->children.length && search->found == NULL; i++) {
    MetaTraitSearchNamespaceForParameter(ns->children.value.p[i], search);
  }
  if (search->found == NULL && ns->anonymous_child != NULL) {
    MetaTraitSearchNamespaceForParameter(ns->anonymous_child, search);
  }
}

static TypeRecord* MetaTraitFunctionTypeWithBoundReflectionArg(
    TypeRecord* function_type, size_t index) {
  if (function_type == NULL || function_type->template_arguments == NULL ||
      index >= function_type->template_arguments->length) {
    return NULL;
  }
  TemplateArgument* arg = function_type->template_arguments->value.p[index];
  if (MetaTraitReflectionFromTemplateArgument(arg, SOURCE_LOCATION_MISSING) ==
      NULL) {
    return NULL;
  }
  return function_type;
}

static TypeRecord* MetaTraitFunctionTypeForParameterSymbol(Symbol* param) {
  if (param == NULL) {
    return NULL;
  }
  int index = MetaTraitTemplateParameterIndex(param);
  TypeRecord* contexts[16];
  size_t context_count = 0;
  if (compiler->current_function != NULL &&
      context_count < sizeof(contexts) / sizeof(contexts[0])) {
    contexts[context_count++] = compiler->current_function;
  }
  for (size_t i = 0;
       i < compiler->functions_being_analyzed.length &&
       context_count < sizeof(contexts) / sizeof(contexts[0]);
       i++) {
    contexts[context_count++] =
        compiler->functions_being_analyzed.value.p[i];
  }
  if (index >= 0) {
    size_t arg_index = (size_t)index;
    for (size_t i = context_count; i-- > 0;) {
      TypeRecord* bound =
          MetaTraitFunctionTypeWithBoundReflectionArg(contexts[i], arg_index);
      if (bound != NULL) {
        return bound;
      }
    }
    for (size_t i = context_count; i-- > 0;) {
      TypeRecord* function_type = contexts[i];
      if (function_type->template_arguments != NULL &&
          arg_index < function_type->template_arguments->length) {
        return function_type;
      }
    }
  }
  for (size_t i = context_count; i-- > 0;) {
    TypeRecord* function_type = contexts[i];
    if (MetaTraitPrototypeContainsParameter(function_type, param) &&
        function_type->template_arguments != NULL) {
      if (index >= 0) {
        TypeRecord* bound = MetaTraitFunctionTypeWithBoundReflectionArg(
            function_type, (size_t)index);
        if (bound != NULL) {
          return bound;
        }
      }
      return function_type;
    }
  }
  for (size_t i = context_count; i-- > 0;) {
    TypeRecord* function_type = contexts[i];
    for (size_t j = 0; j < function_type->info.function.prototype.length; j++) {
      Symbol* formal = function_type->info.function.prototype.value.p[j];
      if (formal == param && function_type->template_arguments != NULL) {
        if (index >= 0) {
          TypeRecord* bound = MetaTraitFunctionTypeWithBoundReflectionArg(
              function_type, (size_t)index);
          if (bound != NULL) {
            return bound;
          }
        }
        return function_type;
      }
    }
    if (TypeIsFunction(function_type) &&
        function_type->info.function.template_instantiations.length > 0 &&
        index >= 0) {
      size_t arg_index = (size_t)index;
      for (size_t j = 0;
           j < function_type->info.function.template_instantiations.length; j++) {
        Symbol* instantiation =
            function_type->info.function.template_instantiations.value.p[j];
        if (instantiation == NULL || instantiation->type == NULL) {
          continue;
        }
        TypeRecord* bound = MetaTraitFunctionTypeWithBoundReflectionArg(
            instantiation->type, arg_index);
        if (bound != NULL) {
          return bound;
        }
      }
    }
  }
  MetaTraitParameterSearch search = {.param = param, .found = NULL};
  MetaTraitSearchNamespaceForParameter(compiler->global_namespace, &search);
  return search.found;
}

static ReflectionValue* MetaTraitReflectionFromTemplateParameterSymbol(
    Symbol* symbol, SourceLocation location) {
  if (symbol == NULL) {
    return NULL;
  }
  int index = MetaTraitTemplateParameterIndex(symbol);
  if (index < 0) {
    return NULL;
  }
  TypeRecord* function_type = MetaTraitFunctionTypeForParameterSymbol(symbol);
  if (function_type != NULL && function_type->template_arguments != NULL &&
      (size_t)index < function_type->template_arguments->length) {
    TemplateArgument* arg = function_type->template_arguments->value.p[index];
    ReflectionValue* reflection =
        MetaTraitReflectionFromTemplateArgument(arg, location);
    if (reflection != NULL) {
      return reflection;
    }
    ASTNode* materialized =
        TemplateArgumentMaterializeExpression(arg, location);
    if (materialized != NULL) {
      reflection = SemanticReflectionValueFromExpression(materialized);
      if (reflection == NULL) {
        ConstEvalContext context;
        ConstEvalContextInit(&context);
        reflection =
            ConstexprEvaluateReflectionExpression(&context, materialized);
        ConstEvalContextDestruct(&context);
      }
      ASTNodeDelete(materialized);
      if (reflection != NULL) {
        return reflection;
      }
    }
  }
  if (symbol->flags.is_template_parameter && TypeIsReflection(symbol->type)) {
    MetaTraitParameterSearch search = {.param = symbol, .found = NULL};
    MetaTraitSearchNamespaceForParameter(compiler->global_namespace, &search);
    if (search.found != NULL &&
        search.found->template_arguments != NULL &&
        (size_t)index < search.found->template_arguments->length) {
      TemplateArgument* arg = search.found->template_arguments->value.p[index];
      ReflectionValue* reflection =
          MetaTraitReflectionFromTemplateArgument(arg, location);
      if (reflection != NULL) {
        return reflection;
      }
      ASTNode* materialized =
          TemplateArgumentMaterializeExpression(arg, location);
      if (materialized != NULL) {
        reflection = SemanticReflectionValueFromExpression(materialized);
        ASTNodeDelete(materialized);
        if (reflection != NULL) {
          return reflection;
        }
      }
    }
  }
  return NULL;
}

static ReflectionValue* MetaTraitReflectionFromExpression(ASTNode* expression,
                                                          bool* dependent) {
  if (dependent != NULL) {
    *dependent = false;
  }
  if (expression == NULL) {
    if (dependent != NULL) {
      *dependent = true;
    }
    return NULL;
  }
  expression = MetaTraitUnwrapInitExpression(expression);
  if ((expression->flags & kASTAnalyzed) == 0) {
    expression = AnalyzeExpression(expression);
  }
  if (expression == NULL) {
    if (dependent != NULL) {
      *dependent = true;
    }
    return NULL;
  }
  if (expression->op == AST_OP(reflection_constant)) {
    return ((ReflectionASTNode*)expression)->value;
  }
  if (expression->op == AST_OP(reflect)) {
    ReflectionASTNode* reflect = (ReflectionASTNode*)expression;
    if (reflect->value != NULL) {
      return reflect->value;
    }
  }
  if (expression->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)expression)->symbol;
    if (symbol != NULL && TypeIsReflection(symbol->type) &&
        symbol->flags.value_set && symbol->value.other != NULL) {
      return (ReflectionValue*)symbol->value.other;
    }
    ReflectionValue* from_param =
        MetaTraitReflectionFromTemplateParameterSymbol(symbol,
                                                       expression->location);
    if (from_param != NULL) {
      return from_param;
    }
    ReflectionValue* from_symbol =
        SemanticReflectionValueFromExpression(expression);
    if (from_symbol != NULL) {
      return from_symbol;
    }
  }
  ReflectionValue* value =
      SemanticReflectionValueFromExpression(expression);
  if (value == NULL) {
    ConstEvalContext context;
    ConstEvalContextInit(&context);
    value = ConstexprEvaluateReflectionExpression(&context, expression);
    ConstEvalContextDestruct(&context);
  }
  if (value != NULL) {
    return value;
  }
  if (MetaTraitArgIsDependent(expression)) {
    if (dependent != NULL) {
      *dependent = true;
    }
  }
  return NULL;
}

static TypeRecord* MetaTraitTypeFromReflection(ReflectionValue* value,
                                             SourceLocation location,
                                             bool* dependent) {
  TypeRecord* type =
      SemanticMaterializeReflectedType(value, location);
  if (type == NULL) {
    return NULL;
  }
  if (dependent != NULL &&
      (TypeContainsTemplateParameter(type) || TypeIsUnknown(type))) {
    *dependent = true;
    TypeRecordDelete(type);
    return NULL;
  }
  TypeRecord* recovered = MetaTraitRecoverQueryType(type, value);
  TypeRecordDelete(type);
  return recovered;
}

static TypeRecord* MetaTraitTypeFromCallExpression(ASTNode* expression,
                                                   SourceLocation location,
                                                   bool* dependent) {
  if (dependent != NULL) {
    *dependent = false;
  }
  expression = MetaTraitUnwrapInitExpression(expression);
  if (expression == NULL) {
    if (dependent != NULL) {
      *dependent = true;
    }
    return NULL;
  }
  if ((expression->flags & kASTAnalyzed) == 0) {
    expression = AnalyzeExpression(expression);
  }
  if (expression == NULL) {
    if (dependent != NULL) {
      *dependent = true;
    }
    return NULL;
  }
  if (expression->op == AST_OP(reflection_constant)) {
    ReflectionValue* value = ((ReflectionASTNode*)expression)->value;
    return MetaTraitTypeFromReflection(value, location, dependent);
  }
  if (expression->op == AST_OP(reflect)) {
    ReflectionASTNode* reflect = (ReflectionASTNode*)expression;
    if (reflect->value != NULL) {
      return MetaTraitTypeFromReflection(reflect->value, location, dependent);
    }
    if (reflect->operand_kind == kReflectionOperandType &&
        reflect->operand_type != NULL) {
      if (TypeContainsTemplateParameter(reflect->operand_type) ||
          TypeIsUnknown(reflect->operand_type)) {
        if (dependent != NULL) {
          *dependent = true;
        }
        return NULL;
      }
      TypeRecord* operand =
          TypeRecordCalculateSize(TypeRecordCloneSpine(reflect->operand_type));
      return MetaTraitRecoverQueryType(operand, reflect->value);
    }
  }
  bool arg_dependent = false;
  ReflectionValue* value =
      MetaTraitReflectionFromExpression(expression, &arg_dependent);
  if (arg_dependent) {
    if (dependent != NULL) {
      *dependent = true;
    }
    return NULL;
  }
  return MetaTraitTypeFromReflection(value, location, dependent);
}

static void MetaTraitAppendTypeFromExpression(ASTNode* expression,
                                              Vector* types, bool* dependent,
                                              SourceLocation location) {
  TypeRecord* type =
      MetaTraitTypeFromCallExpression(expression, location, dependent);
  if (type != NULL) {
    VectorAppend(types, type);
  }
}

static void MetaTraitAppendTypesFromBracedInit(
    BracedInitializerASTNode* init, Vector* types, bool* dependent,
    SourceLocation location) {
  if (init == NULL || init->initializers == NULL) {
    return;
  }
  for (size_t i = 0; i < init->initializers->length; i++) {
    ASTNode* expression = MetaTraitUnwrapInitExpression(
        init->initializers->value.p[i]);
    if (expression != NULL && expression->op != AST_OP(braced_init) &&
        expression->op != AST_OP(designated_init) &&
        expression->op != AST_OP(expr_init) &&
        (expression->flags & kASTAnalyzed) == 0) {
      expression = AnalyzeExpression(expression);
    }
    MetaTraitAppendTypeFromExpression(expression, types, dependent, location);
  }
}

static bool MetaTraitVectorElementType(TypeRecord* vector_type,
                                       TypeRecord** element_out) {
  TypeRecord* element_type = NULL;
  TypeRecord* materialized = MetaTraitMaterializeQueryType(vector_type);
  if (materialized == NULL || materialized->template_arguments == NULL ||
      materialized->template_arguments->length == 0) {
    TypeRecordDelete(materialized);
    return false;
  }
  TemplateArgument* arg = materialized->template_arguments->value.p[0];
  if (arg == NULL || arg->type == NULL || !TypeIsReflection(arg->type)) {
    TypeRecordDelete(materialized);
    return false;
  }
  element_type = TypeRecordCopy(arg->type);
  TypeRecordDelete(materialized);
  if (element_out != NULL) {
    *element_out = element_type;
  } else {
    TypeRecordDelete(element_type);
  }
  return true;
}

static void MetaTraitAppendTypesFromVectorCall(VectorASTNode* call, Vector* types,
                                               bool* dependent,
                                               SourceLocation location) {
  if (call == NULL || call->base.type == NULL ||
      !MetaTraitVectorElementType(call->base.type, NULL)) {
    return;
  }
  if (call->children == NULL) {
    return;
  }
  for (size_t i = 0; i < call->children->length; i++) {
    ASTNode* arg =
        MetaTraitUnwrapInitExpression(call->children->value.p[i]);
    if (arg == NULL) {
      continue;
    }
    if (arg->op == AST_OP(braced_init)) {
      MetaTraitAppendTypesFromBracedInit((BracedInitializerASTNode*)arg, types,
                                         dependent, location);
      return;
    }
    if (arg->op == AST_OP(call) &&
        MetaTraitVectorElementType(arg->type, NULL)) {
      MetaTraitAppendTypesFromVectorCall((VectorASTNode*)arg, types, dependent,
                                         location);
      return;
    }
    MetaTraitAppendTypeFromExpression(arg, types, dependent, location);
  }
}

static void MetaTraitAppendTypesFromVectorIdentifier(IdentifierASTNode* id,
                                                     Vector* types,
                                                     bool* dependent,
                                                     SourceLocation location) {
  if (id == NULL || id->symbol == NULL || id->symbol->type == NULL ||
      !MetaTraitVectorElementType(id->symbol->type, NULL)) {
    return;
  }
  if (id->symbol->constexpr_initializer != NULL) {
    ASTNode* init =
        ConstexprInitializerExpression(id->symbol->constexpr_initializer);
    init = MetaTraitUnwrapInitExpression(init);
    if (init != NULL && init->op == AST_OP(call)) {
      MetaTraitAppendTypesFromVectorCall((VectorASTNode*)init, types, dependent,
                                         location);
    }
  }
}

static void MetaTraitAppendTypesFromRangeArgument(ASTNode* range, Vector* types,
                                                  bool* dependent,
                                                  SourceLocation location) {
  range = MetaTraitUnwrapInitExpression(range);
  if (range == NULL) {
    return;
  }
  if (range->op == AST_OP(braced_init)) {
    MetaTraitAppendTypesFromBracedInit((BracedInitializerASTNode*)range, types,
                                       dependent, location);
    return;
  }
  if (range->op == AST_OP(call)) {
    MetaTraitAppendTypesFromVectorCall((VectorASTNode*)range, types, dependent,
                                       location);
    if (types->length > 0) {
      return;
    }
  }
  if (range->op == AST_OP(identifier)) {
    size_t before = types->length;
    MetaTraitAppendTypesFromVectorIdentifier((IdentifierASTNode*)range, types,
                                             dependent, location);
    if (types->length > before) {
      return;
    }
  }
  MetaTraitAppendTypeFromExpression(range, types, dependent, location);
}

static Vector* MetaTraitCollectTypeVector(VectorASTNode* call, size_t fixed_count,
                                          bool range_from_last) {
  Vector* types = NewVector();
  bool dependent = false;
  if (call == NULL || call->children == NULL) {
    return types;
  }
  size_t child_count = call->children->length;
  size_t fixed_end = fixed_count;
  if (range_from_last && child_count > fixed_count) {
    fixed_end = child_count - 1;
  }
  for (size_t i = 0; i < fixed_end && i < child_count; i++) {
    MetaTraitAppendTypeFromExpression(call->children->value.p[i], types,
                                      &dependent, call->base.location);
  }
  if (range_from_last && child_count > fixed_count) {
    ASTNode* range = call->children->value.p[child_count - 1];
    MetaTraitAppendTypesFromRangeArgument(range, types, &dependent,
                                          call->base.location);
  } else if (!range_from_last && fixed_count == 0) {
    for (size_t i = 0; i < child_count; i++) {
      MetaTraitAppendTypesFromRangeArgument(call->children->value.p[i], types,
                                          &dependent, call->base.location);
    }
  }
  if (dependent) {
    VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                             /*free_element=*/false);
    return NULL;
  }
  return types;
}

static TypeRecord* MetaTraitSingleType(VectorASTNode* call, bool* dependent) {
  if (call == NULL || call->children == NULL || call->children->length == 0) {
    if (dependent != NULL) {
      *dependent = true;
    }
    return NULL;
  }
  return MetaTraitTypeFromCallExpression(call->children->value.p[0],
                                         call->base.location, dependent);
}

static TypeRecord* MetaTraitPairTypes(VectorASTNode* call, TypeRecord** second,
                                      bool* dependent) {
  *second = NULL;
  if (call == NULL || call->children == NULL || call->children->length < 2) {
    if (dependent != NULL) {
      *dependent = true;
    }
    return NULL;
  }
  TypeRecord* first =
      MetaTraitTypeFromCallExpression(call->children->value.p[0],
                                      call->base.location, dependent);
  *second = MetaTraitTypeFromCallExpression(call->children->value.p[1],
                                            call->base.location, dependent);
  return first;
}

static ASTNode* MetaTraitBool(bool value, SourceLocation location) {
  return NewIntConstantASTNode(value ? 1 : 0,
                               NewTypeRecordWithSize(kTypeBool, kQualConst),
                               location);
}

static ASTNode* MetaTraitSizeT(size_t value, SourceLocation location) {
  return NewIntConstantASTNode((int64_t)value, NewSizeTypeRecord(), location);
}

static ASTNode* MetaTraitTypeReflection(TypeRecord* type,
                                        SourceLocation location) {
  if (type == NULL) {
    return NULL;
  }
  ReflectionValue* reflection =
      ReflectionCreateType(type, NULL, location);
  return NewReflectionConstantASTNode(reflection, location);
}

static ASTNode* MetaTraitPreserveDependent(VectorASTNode* call,
                                           Symbol* function) {
  ASTNodeSetType((ASTNode*)call,
                 function != NULL && function->type != NULL ? function->type->next
                                                            : NULL);
  call->base.flags |= kASTAnalyzed | kASTDependentFunctorCall;
  return (ASTNode*)call;
}

static bool MetaTraitTypeEqual(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  TypeRecord* plain_left =
      TypeRecordCalculateSize(TypeRecordCloneSpine(left));
  TypeRecord* plain_right =
      TypeRecordCalculateSize(TypeRecordCloneSpine(right));
  if (plain_left == NULL || plain_right == NULL) {
    TypeRecordDelete(plain_left);
    TypeRecordDelete(plain_right);
    return false;
  }
  for (TypeRecord* cur = plain_left; cur != NULL; cur = cur->next) {
    cur->qualifiers = kQualPlain;
  }
  for (TypeRecord* cur = plain_right; cur != NULL; cur = cur->next) {
    cur->qualifiers = kQualPlain;
  }
  bool equal = TypeEqual(plain_left, plain_right);
  TypeRecordDelete(plain_left);
  TypeRecordDelete(plain_right);
  return equal;
}

static TypeRecord* MetaTraitOwnedCopy(TypeRecord* type) {
  return type != NULL ? TypeRecordCalculateSize(TypeRecordCloneSpine(type))
                      : NULL;
}

static TypeRecord* MetaTraitRemoveTopQual(TypeRecord* type, Qualifiers mask) {
  TypeRecord* copy = MetaTraitOwnedCopy(type);
  if (copy != NULL) {
    copy->qualifiers &= ~mask;
  }
  return copy;
}

static TypeRecord* MetaTraitAddTopQual(TypeRecord* type, Qualifiers mask) {
  TypeRecord* copy = MetaTraitOwnedCopy(type);
  if (copy != NULL) {
    copy->qualifiers |= mask;
  }
  return copy;
}

static TypeRecord* MetaTraitRemoveReference(TypeRecord* type) {
  TypeRecord* copy = MetaTraitOwnedCopy(type);
  while (copy != NULL && TypeIsReference(copy)) {
    copy = copy->next;
  }
  return copy != NULL ? TypeRecordCalculateSize(TypeRecordCloneSpine(copy))
                      : NULL;
}

static TypeRecord* MetaTraitRemoveCv(TypeRecord* type) {
  TypeRecord* copy = MetaTraitOwnedCopy(type);
  if (copy != NULL) {
    for (TypeRecord* cur = copy; cur != NULL; cur = cur->next) {
      cur->qualifiers &= ~(kQualConst | kQualVolatile);
    }
  }
  return copy;
}

static TypeRecord* MetaTraitAddReference(TypeRecord* type, bool rvalue) {
  TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, rvalue);
  TypeRecord* inner = MetaTraitOwnedCopy(type);
  if (inner == NULL) {
    TypeRecordDelete(ref);
    return NULL;
  }
  TypeRecordChain(ref, inner);
  ref->type = inner->type;
  return TypeRecordCalculateSize(ref);
}

static TypeRecord* MetaTraitRemoveExtentOnce(TypeRecord* type) {
  TypeRecord* copy = MetaTraitOwnedCopy(type);
  if (copy == NULL || !TypeIsArray(copy)) {
    TypeRecordDelete(copy);
    return NULL;
  }
  TypeRecord* element = TypeRecordCopy(copy->next);
  if (element != NULL) {
    element->qualifiers |= copy->qualifiers;
  }
  TypeRecordDelete(copy);
  return TypeRecordCalculateSize(element);
}

static TypeRecord* MetaTraitRemoveAllExtents(TypeRecord* type) {
  TypeRecord* copy = MetaTraitOwnedCopy(type);
  while (copy != NULL && TypeIsArray(copy)) {
    TypeRecord* next = copy->next;
    copy->next = NULL;
    TypeRecordDelete(copy);
    copy = next != NULL ? TypeRecordCalculateSize(TypeRecordCloneSpine(next))
                        : NULL;
  }
  return copy;
}

static TypeRecord* MetaTraitRemovePointerOnce(TypeRecord* type) {
  TypeRecord* copy = MetaTraitOwnedCopy(type);
  if (copy == NULL || !TypeIsPointer(copy) || copy->next == NULL) {
    TypeRecordDelete(copy);
    return NULL;
  }
  TypeRecord* pointee = TypeRecordCopy(copy->next);
  if (pointee != NULL) {
    pointee->qualifiers |= copy->qualifiers;
  }
  TypeRecordDelete(copy);
  return TypeRecordCalculateSize(pointee);
}

static TypeRecord* MetaTraitAddPointer(TypeRecord* type) {
  TypeRecord* decayed = MetaTraitRemoveReference(type);
  decayed = decayed != NULL ? MetaTraitRemoveCv(decayed) : NULL;
  if (decayed == NULL) {
    return NULL;
  }
  if (TypeIsArray(decayed)) {
    TypeRecord* element = MetaTraitRemoveExtentOnce(decayed);
    TypeRecordDelete(decayed);
    decayed = element;
  }
  if (decayed != NULL && TypeIsFunction(decayed)) {
    TypeRecord* fn_ptr = NewPointerTo(kQualPlain, decayed);
    return TypeRecordCalculateSize(fn_ptr);
  }
  TypeRecord* pointer = NewPointerTo(kQualPlain, decayed);
  TypeRecordDelete(decayed);
  return TypeRecordCalculateSize(pointer);
}

static TypeRecord* MetaTraitDecay(TypeRecord* type) {
  TypeRecord* result = MetaTraitRemoveReference(type);
  if (result == NULL) {
    return NULL;
  }
  if (TypeIsArray(result)) {
    TypeRecord* pointer = MetaTraitAddPointer(result);
    TypeRecordDelete(result);
    return pointer;
  }
  if (TypeIsFunction(result)) {
    TypeRecord* pointer = MetaTraitAddPointer(result);
    TypeRecordDelete(result);
    return pointer;
  }
  return MetaTraitRemoveCv(result);
}

static TypeRecord* MetaTraitMakeSigned(TypeRecord* type) {
  type = MetaTraitRemoveCv(type);
  if (type == NULL || !TypeIsIntegral(type) || TypeIsBool(type)) {
    TypeRecordDelete(type);
    return NULL;
  }
  type->qualifiers = kQualPlain;
  if (TypeIsUnsigned(type)) {
    type->type &= ~kTypeUnsigned;
  }
  if (TypeIsChar(type) && !TypeIsSigned(type) && !TypeIsUnsigned(type)) {
    type->type |= kTypeSigned;
  }
  return TypeRecordCalculateSize(type);
}

static TypeRecord* MetaTraitMakeUnsigned(TypeRecord* type) {
  type = MetaTraitRemoveCv(type);
  if (type == NULL || !TypeIsIntegral(type) || TypeIsBool(type)) {
    TypeRecordDelete(type);
    return NULL;
  }
  type->qualifiers = kQualPlain;
  if (!TypeIsUnsigned(type)) {
    type->type |= kTypeUnsigned;
  }
  return TypeRecordCalculateSize(type);
}

static bool MetaTraitIsMemberPointer(TypeRecord* type) {
  type = MetaTraitOwnedCopy(type);
  if (type == NULL) {
    return false;
  }
  for (TypeRecord* cur = type; cur != NULL; cur = cur->next) {
    if (cur->declarator == kDeclMemberPointer) {
      TypeRecordDelete(type);
      return true;
    }
  }
  TypeRecordDelete(type);
  return false;
}

static bool MetaTraitIsMemberFunctionPointer(TypeRecord* type) {
  type = MetaTraitOwnedCopy(type);
  if (type == NULL) {
    return false;
  }
  for (TypeRecord* cur = type; cur != NULL; cur = cur->next) {
    if (cur->declarator == kDeclMemberPointer) {
      bool result = cur->next != NULL && TypeIsFunction(cur->next);
      TypeRecordDelete(type);
      return result;
    }
  }
  TypeRecordDelete(type);
  return false;
}

static bool MetaTraitIsClassType(TypeRecord* type) {
  return type != NULL && TypeIsStructOrUnion(type) &&
         (type->type & kTypeUnion) == 0 && type->info.struct_info != NULL;
}

static bool MetaTraitIsUnionType(TypeRecord* type) {
  return type != NULL && TypeIsStructOrUnion(type) &&
         (type->type & kTypeUnion) != 0;
}

static bool MetaTraitIsPolymorphic(TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return false;
  }
  Struct* str = type->info.struct_info;
  return str->vptr_member != NULL || str->virtual_members.length > 0;
}

static bool MetaTraitIsEmpty(TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return false;
  }
  Struct* str = type->info.struct_info;
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member != NULL && !member->is_static && !member->is_member_function) {
      return false;
    }
  }
  return true;
}

static bool MetaTraitIsVirtualBaseOf(Struct* derived, TypeRecord* base_type) {
  if (derived == NULL || base_type == NULL) {
    return false;
  }
  for (size_t i = 0; i < derived->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = derived->virtual_bases.value.p[i];
    if (base != NULL && base->type != NULL &&
        MetaTraitTypeEqual(base->type, base_type)) {
      return true;
    }
  }
  for (size_t i = 0; i < derived->bases.length; i++) {
    CXXBaseSpecifier* base = derived->bases.value.p[i];
    if (base != NULL && base->is_virtual && base->type != NULL &&
        MetaTraitTypeEqual(base->type, base_type)) {
      return true;
    }
  }
  return false;
}

static bool MetaTraitIsReferenceWrapper(TypeRecord* type, TypeRecord** wrapped) {
  if (wrapped != NULL) {
    *wrapped = NULL;
  }
  type = CXXTypeTraitMaterializeType(&compiler->syntax, type);
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->template_arguments == NULL ||
      type->template_arguments->length != 1) {
    TypeRecordDelete(type);
    return false;
  }
  Symbol* origin = type->template_origin;
  if (origin == NULL && type->info.struct_info != NULL &&
      type->info.struct_info->tag_symbol != NULL &&
      type->info.struct_info->tag_symbol->type != NULL) {
    origin = type->info.struct_info->tag_symbol->type->template_origin;
  }
  if (origin == NULL || origin->name.value == NULL ||
      strcmp(origin->name.value, "reference_wrapper") != 0) {
    TypeRecordDelete(type);
    return false;
  }
  TemplateArgument* arg = type->template_arguments->value.p[0];
  if (arg == NULL || arg->kind != kTemplateParameterType || arg->type == NULL) {
    TypeRecordDelete(type);
    return false;
  }
  if (wrapped != NULL) {
    *wrapped = TypeRecordCopy(arg->type);
  }
  TypeRecordDelete(type);
  return true;
}

static TypeRecord* MetaTraitUnwrapReference(TypeRecord* type) {
  TypeRecord* wrapped = NULL;
  if (MetaTraitIsReferenceWrapper(type, &wrapped)) {
    return wrapped;
  }
  return MetaTraitOwnedCopy(type);
}

static TypeRecord* MetaTraitUnwrapRefDecay(TypeRecord* type) {
  TypeRecord* unwrapped = MetaTraitUnwrapReference(type);
  TypeRecord* decayed = MetaTraitDecay(unwrapped);
  TypeRecordDelete(unwrapped);
  return decayed;
}

static TypeRecord* MetaTraitUnderlyingType(TypeRecord* type) {
  if (type == NULL || !TypeIsEnum(type) || type->info.enum_info == NULL) {
    return NULL;
  }
  Enum* enumeration = type->info.enum_info;
  Type underlying = enumeration->has_fixed_underlying
                        ? enumeration->fixed_underlying_type
                        : kTypeInt;
  TypeRecord* result = NewTypeRecordWithSize(underlying, kQualPlain);
  if (enumeration->has_fixed_underlying) {
    result->size = enumeration->fixed_underlying_size;
  }
  return TypeRecordCalculateSize(result);
}

static size_t MetaTraitArrayRank(TypeRecord* type) {
  size_t rank = 0;
  for (TypeRecord* cur = type; cur != NULL && TypeIsArray(cur); cur = cur->next) {
    rank++;
  }
  return rank;
}

static size_t MetaTraitArrayExtent(TypeRecord* type, size_t index) {
  size_t rank = 0;
  for (TypeRecord* cur = type; cur != NULL && TypeIsArray(cur); cur = cur->next) {
    if (rank == index) {
      if (cur->info.array.is_vla || cur->info.array.is_dependent_bound) {
        return 0;
      }
      if (TypeIsVLA(cur)) {
        return 0;
      }
      return (size_t)cur->info.array.size.fixed;
    }
    rank++;
  }
  return 0;
}

static bool MetaTraitEvaluateBoolKind(CXXTypeTraitKind kind, Vector* types) {
  return CXXTypeTraitEvaluateBool(&compiler->syntax, kind, types);
}

static Symbol* MetaTraitFindStdSymbol(const char* name) {
  Namespace* std_ns = NamespaceFindStdNamespace();
  if (std_ns == NULL) {
    return NULL;
  }
  String symbol_name;
  StringInit(&symbol_name, name);
  NamespaceInlineSymbolLookup result =
      NamespaceResolveSymbolInInlineSet(std_ns, &symbol_name);
  if (result.status == kInlineLookupUnique) {
    StringDestruct(&symbol_name);
    return result.symbol;
  }
  NamespaceInlineTagLookup tag_result =
      NamespaceResolveTagInInlineSet(std_ns, &symbol_name);
  StringDestruct(&symbol_name);
  if (tag_result.status == kInlineLookupUnique) {
    return tag_result.tag;
  }
  return NULL;
}

static void MetaTraitRestoreCompilerTemplateState(size_t saved_declarations,
                                                  size_t saved_reflections) {
  (void)saved_declarations;
  (void)saved_reflections;
}

static TypeRecord* MetaTraitInstantiateClassTemplateScoped(Symbol* trait,
                                                           Vector* args) {
  size_t saved_declarations = compiler->declaration_asts.length;
  size_t saved_reflections = compiler->reflection_values.length;
  SyntaxOpenScope(&compiler->syntax);
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  int saved_speculative_depth =
      compiler->speculative_template_instantiation_depth;
  compiler->speculative_template_instantiation_depth++;
  TypeRecord* result =
      TypeInstantiateClassTemplate(&compiler->syntax, trait, args);
  compiler->speculative_template_instantiation_depth = saved_speculative_depth;
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  SyntaxCloseScope(&compiler->syntax);
  MetaTraitRestoreCompilerTemplateState(saved_declarations, saved_reflections);
  return result;
}

static TypeRecord* MetaTraitMaterializeQueryType(TypeRecord* object_type) {
  TypeRecord* query_type = object_type;
  if (query_type != NULL && TypeIsReference(query_type)) {
    query_type = query_type->next;
  }
  if (query_type == NULL) {
    return NULL;
  }
  if (!TypeIsStructOrUnion(query_type)) {
    return TypeRecordCopy(object_type);
  }
  size_t saved_declarations = compiler->declaration_asts.length;
  size_t saved_reflections = compiler->reflection_values.length;
  SyntaxOpenScope(&compiler->syntax);
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  int saved_speculative_depth =
      compiler->speculative_template_instantiation_depth;
  compiler->speculative_template_instantiation_depth++;
  TypeRecord* materialized =
      CXXTypeTraitMaterializeType(&compiler->syntax, query_type);
  compiler->speculative_template_instantiation_depth = saved_speculative_depth;
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  SyntaxCloseScope(&compiler->syntax);
  MetaTraitRestoreCompilerTemplateState(saved_declarations, saved_reflections);
  if (materialized == NULL) {
    return NULL;
  }
  if (materialized == query_type) {
    return TypeRecordCopy(object_type);
  }
  if (TypeIsReference(object_type)) {
    TypeRecord* ref = TypeRecordCopy(object_type);
    TypeRecordDelete(ref->next);
    ref->next = materialized;
    return TypeRecordCalculateSize(ref);
  }
  return materialized;
}

static Vector* MetaTraitFindTemplateArguments(TypeRecord* type) {
  TypeRecord* query = type;
  if (query != NULL && TypeIsReference(query)) {
    query = query->next;
  }
  Vector* fallback = NULL;
  for (TypeRecord* cur = query; cur != NULL; cur = cur->next) {
    if (cur->template_arguments != NULL &&
        cur->template_arguments->length > 0) {
      if (MetaTraitTemplateArgumentsAreConcrete(cur->template_arguments)) {
        return cur->template_arguments;
      }
      if (fallback == NULL) {
        fallback = cur->template_arguments;
      }
    }
    if (TypeIsStructOrUnion(cur) && cur->info.struct_info != NULL) {
      Struct* str = cur->info.struct_info;
      if (str->tag_symbol != NULL && str->tag_symbol->type != NULL &&
          str->tag_symbol->type->template_arguments != NULL &&
          str->tag_symbol->type->template_arguments->length > 0) {
        Vector* tag_args = str->tag_symbol->type->template_arguments;
        if (MetaTraitTemplateArgumentsAreConcrete(tag_args)) {
          return tag_args;
        }
        if (fallback == NULL) {
          fallback = tag_args;
        }
      }
    }
  }
  return fallback;
}

static bool MetaTraitClassTemplateArgCount(TypeRecord* object_type,
                                           size_t* count_out) {
  Vector* args = MetaTraitFindTemplateArguments(object_type);
  if (args != NULL) {
    size_t count = 0;
    for (size_t i = 0; i < args->length; i++) {
      TemplateArgument* arg = args->value.p[i];
      count += arg != NULL && arg->pack_arguments != NULL
                   ? arg->pack_arguments->length
                   : 1;
    }
    if (count_out != NULL) {
      *count_out = count;
    }
    return true;
  }
  TypeRecord* materialized = MetaTraitMaterializeQueryType(object_type);
  args = MetaTraitFindTemplateArguments(materialized);
  if (args == NULL) {
    TypeRecordDelete(materialized);
    return false;
  }
  if (count_out != NULL) {
    size_t count = 0;
    for (size_t i = 0; i < args->length; i++) {
      TemplateArgument* arg = args->value.p[i];
      count += arg != NULL && arg->pack_arguments != NULL
                   ? arg->pack_arguments->length
                   : 1;
    }
    *count_out = count;
  }
  TypeRecordDelete(materialized);
  return true;
}

static TypeRecord* MetaTraitNamedElementType(TypeRecord* object_type,
                                             size_t index,
                                             const char* const* names,
                                             size_t name_count) {
  if (index >= name_count) {
    return NULL;
  }
  TypeRecord* materialized = MetaTraitMaterializeQueryType(object_type);
  if (materialized == NULL || !TypeIsStructOrUnion(materialized) ||
      materialized->info.struct_info == NULL) {
    TypeRecordDelete(materialized);
    return NULL;
  }
  StructMember* member =
      FindStructMemberByName(materialized->info.struct_info, names[index]);
  TypeRecord* result = NULL;
  if (member != NULL && member->symbol != NULL &&
      member->symbol->type != NULL) {
    result = MetaTraitCopyConcreteType(member->symbol->type);
  }
  TypeRecordDelete(materialized);
  return result;
}

static TypeRecord* MetaTraitClassTemplateArgType(TypeRecord* object_type,
                                                 size_t index) {
  Vector* args = MetaTraitFindTemplateArguments(object_type);
  TypeRecord* materialized = NULL;
  if (args == NULL) {
    materialized = MetaTraitMaterializeQueryType(object_type);
    args = MetaTraitFindTemplateArguments(materialized);
  }
  TemplateArgument* selected = NULL;
  if (args != NULL) {
    size_t flattened_index = 0;
    for (size_t i = 0; i < args->length && selected == NULL; i++) {
      TemplateArgument* arg = args->value.p[i];
      if (arg != NULL && arg->pack_arguments != NULL) {
        if (index >= flattened_index &&
            index < flattened_index + arg->pack_arguments->length) {
          selected = arg->pack_arguments->value.p[index - flattened_index];
        }
        flattened_index += arg->pack_arguments->length;
      } else {
        if (index == flattened_index) {
          selected = arg;
        }
        flattened_index++;
      }
    }
  }
  if (selected != NULL) {
    TemplateArgument* arg = selected;
    TypeRecord* result = NULL;
    if (arg != NULL && MetaTraitTemplateArgumentIsConcrete(arg)) {
      result = TypeRecordCalculateSize(TypeRecordCopy(arg->type));
    }
    if (result != NULL) {
      TypeRecordDelete(materialized);
      return result;
    }
  }
  if (materialized == NULL) {
    materialized = MetaTraitMaterializeQueryType(object_type);
  }
  TypeRecord* result = NULL;
  if (materialized != NULL && TypeIsStructOrUnion(materialized) &&
      materialized->info.struct_info != NULL) {
    result =
        MetaTraitStructTupleElementType(materialized->info.struct_info, index);
  }
  TypeRecordDelete(materialized);
  if (result != NULL) {
    return result;
  }
  static const char* kPairNames[] = {"first", "second"};
  result = MetaTraitNamedElementType(object_type, index, kPairNames, 2);
  return result;
}

static bool MetaTraitIsRecursiveTupleStruct(Struct* str) {
  return str != NULL && FindStructMemberByName(str, "__0") != NULL &&
         FindStructMemberByName(str, "__rest") != NULL;
}

static size_t MetaTraitRecursiveTupleSizeFromStruct(Struct* str) {
  if (str == NULL) {
    return 0;
  }
  if (str->members.length == 0) {
    return 0;
  }
  if (!MetaTraitIsRecursiveTupleStruct(str)) {
    return 0;
  }
  StructMember* rest_member = FindStructMemberByName(str, "__rest");
  if (rest_member == NULL || rest_member->symbol == NULL ||
      rest_member->symbol->type == NULL) {
    return 1;
  }
  TypeRecord* rest_type =
      MetaTraitMaterializeQueryType(rest_member->symbol->type);
  size_t tail = 0;
  if (rest_type != NULL && TypeIsStructOrUnion(rest_type) &&
      rest_type->info.struct_info != NULL) {
    tail = MetaTraitRecursiveTupleSizeFromStruct(rest_type->info.struct_info);
  }
  TypeRecordDelete(rest_type);
  return 1 + tail;
}

static TypeRecord* MetaTraitRecursiveTupleElementType(Struct* str,
                                                      size_t index) {
  if (str == NULL) {
    return NULL;
  }
  if (str->members.length == 0) {
    return NULL;
  }
  if (!MetaTraitIsRecursiveTupleStruct(str)) {
    return NULL;
  }
  StructMember* head = FindStructMemberByName(str, "__0");
  if (index == 0) {
    if (head == NULL || head->symbol == NULL || head->symbol->type == NULL) {
      return NULL;
    }
    return MetaTraitCopyConcreteType(head->symbol->type);
  }
  StructMember* rest_member = FindStructMemberByName(str, "__rest");
  if (rest_member == NULL || rest_member->symbol == NULL ||
      rest_member->symbol->type == NULL) {
    return NULL;
  }
  TypeRecord* rest_type =
      MetaTraitMaterializeQueryType(rest_member->symbol->type);
  TypeRecord* result = NULL;
  if (rest_type != NULL && TypeIsStructOrUnion(rest_type) &&
      rest_type->info.struct_info != NULL) {
    result = MetaTraitRecursiveTupleElementType(rest_type->info.struct_info,
                                               index - 1);
  }
  TypeRecordDelete(rest_type);
  return result;
}

static size_t MetaTraitStructTupleLikeSize(Struct* str) {
  if (str == NULL) {
    return 0;
  }
  size_t recursive = MetaTraitRecursiveTupleSizeFromStruct(str);
  if (recursive > 0) {
    return recursive;
  }
  if (FindStructMemberByName(str, "first") != NULL &&
      FindStructMemberByName(str, "second") != NULL) {
    return 2;
  }
  return 0;
}

static TypeRecord* MetaTraitStructTupleElementType(Struct* str, size_t index) {
  if (str == NULL) {
    return NULL;
  }
  TypeRecord* recursive = MetaTraitRecursiveTupleElementType(str, index);
  if (recursive != NULL) {
    return recursive;
  }
  static const char* kPairNames[] = {"first", "second"};
  if (index < 2 && FindStructMemberByName(str, kPairNames[index]) != NULL) {
    StructMember* member = FindStructMemberByName(str, kPairNames[index]);
    if (member != NULL && member->symbol != NULL &&
        member->symbol->type != NULL) {
      return MetaTraitCopyConcreteType(member->symbol->type);
    }
  }
  return NULL;
}

static size_t MetaTraitTupleLikeSize(TypeRecord* object_type) {
  size_t direct_count = 0;
  if (MetaTraitClassTemplateArgCount(object_type, &direct_count)) {
    return direct_count;
  }
  TypeRecord* materialized = MetaTraitMaterializeQueryType(object_type);
  if (materialized != NULL && TypeIsStructOrUnion(materialized) &&
      materialized->info.struct_info != NULL) {
    size_t structural =
        MetaTraitStructTupleLikeSize(materialized->info.struct_info);
    if (structural > 0) {
      TypeRecordDelete(materialized);
      return structural;
    }
  }
  TypeRecordDelete(materialized);
  return 0;
}

static bool MetaTraitIntegralConstantValue(TypeRecord* type, size_t* value_out) {
  if (type == NULL) {
    return false;
  }
  Symbol* origin = type->template_origin;
  if (origin == NULL && TypeIsStructOrUnion(type) &&
      type->info.struct_info != NULL &&
      type->info.struct_info->tag_symbol != NULL &&
      type->info.struct_info->tag_symbol->type != NULL) {
    origin = type->info.struct_info->tag_symbol->type->template_origin;
  }
  if (origin != NULL && origin->name.value != NULL &&
      strcmp(origin->name.value, "integral_constant") == 0 &&
      type->template_arguments != NULL &&
      type->template_arguments->length >= 2) {
    TemplateArgument* arg = type->template_arguments->value.p[1];
    if (arg != NULL && arg->kind == kTemplateParameterNonType) {
      if (value_out != NULL) {
        *value_out = (size_t)arg->int_value;
      }
      return true;
    }
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return false;
  }
  StructMember* value =
      FindStructMemberByName(type->info.struct_info, "value");
  if (value != NULL && value->symbol != NULL &&
      MetaTraitEvaluateIntegralSymbol(value->symbol, value_out)) {
    return true;
  }
  for (size_t i = 0; i < type->info.struct_info->bases.length; i++) {
    CXXBaseSpecifier* base = type->info.struct_info->bases.value.p[i];
    if (base != NULL && base->type != NULL &&
        MetaTraitIntegralConstantValue(base->type, value_out)) {
      return true;
    }
  }
  return false;
}

static TypeRecord* MetaTraitTraitStructMemberType(TypeRecord* trait_type,
                                                  const char* member_name) {
  if (trait_type == NULL || !TypeIsStructOrUnion(trait_type) ||
      trait_type->info.struct_info == NULL || member_name == NULL) {
    return NULL;
  }
  StructMember* member =
      FindStructMemberByName(trait_type->info.struct_info, member_name);
  if (member != NULL && member->symbol != NULL) {
    Symbol* sym = member->symbol;
    if (sym->alias_target != NULL && sym->alias_target->type != NULL) {
      return TypeRecordCalculateSize(TypeRecordCopy(sym->alias_target->type));
    }
    if (sym->type != NULL) {
      return TypeRecordCalculateSize(TypeRecordCopy(sym->type));
    }
  }
  for (size_t i = 0; i < trait_type->info.struct_info->bases.length; i++) {
    CXXBaseSpecifier* base = trait_type->info.struct_info->bases.value.p[i];
    if (base == NULL || base->type == NULL) {
      continue;
    }
    TypeRecord* from_base =
        MetaTraitTraitStructMemberType(base->type, member_name);
    if (from_base != NULL) {
      return from_base;
    }
  }
  return NULL;
}

static bool MetaTraitInstantiateVariableSizeTrait(const char* var_name,
                                                  TypeRecord* object_type,
                                                  size_t* size_out) {
  Symbol* var = MetaTraitFindStdSymbol(var_name);
  if (var == NULL || var->variable_template == NULL || object_type == NULL) {
    return false;
  }
  TypeRecord* query_type = MetaTraitRecoverTemplateIdType(object_type);
  if (query_type == NULL) {
    return false;
  }
  if (TypeIsReference(query_type)) {
    TypeRecord* unreferenced = query_type->next;
    TypeRecordDelete(query_type);
    query_type = unreferenced != NULL ? TypeRecordCopy(unreferenced) : NULL;
  }
  if (query_type == NULL) {
    return false;
  }
  Vector* args = NewVector();
  VectorAppend(args, NewTypeTemplateArgument(query_type));
  int64_t value = 0;
  bool ok = TypeInstantiateVariableTemplateConstant(
      &compiler->syntax, var, args, &value);
  VectorDeleteWithContents(args, (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  TypeRecordDelete(query_type);
  if (ok && size_out != NULL) {
    *size_out = (size_t)value;
  }
  return ok;
}

static bool MetaTraitInstantiateSizeTrait(const char* trait_name,
                                          TypeRecord* object_type,
                                          size_t* size_out) {
  if (object_type == NULL) {
    return false;
  }
  size_t direct_count = MetaTraitTupleLikeSize(object_type);
  if (direct_count > 0) {
    if (size_out != NULL) {
      *size_out = direct_count;
    }
    return true;
  }
  if (trait_name != NULL) {
    size_t name_len = strlen(trait_name);
    if (name_len + 2 < 64) {
      char var_name[64];
      memcpy(var_name, trait_name, name_len);
      var_name[name_len] = '_';
      var_name[name_len + 1] = 'v';
      var_name[name_len + 2] = '\0';
      if (MetaTraitInstantiateVariableSizeTrait(var_name, object_type,
                                              size_out)) {
        return true;
      }
    }
  }
  Symbol* trait = MetaTraitFindStdSymbol(trait_name);
  if (trait != NULL && trait->flags.is_template) {
    TypeRecord* query_type = MetaTraitRecoverTemplateIdType(object_type);
    if (query_type != NULL && TypeIsReference(query_type)) {
      TypeRecord* unreferenced = query_type->next;
      TypeRecordDelete(query_type);
      query_type =
          unreferenced != NULL ? TypeRecordCopy(unreferenced) : NULL;
    }
    if (query_type != NULL) {
      Vector* args = NewVector();
      VectorAppend(args, NewTypeTemplateArgument(query_type));
      TypeRecord* trait_type =
          MetaTraitInstantiateClassTemplateScoped(trait, args);
      VectorDeleteWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      TypeRecordDelete(query_type);
      if (trait_type != NULL && TypeIsStructOrUnion(trait_type) &&
          trait_type->info.struct_info != NULL) {
        size_t value = 0;
        if (MetaTraitEvaluateStructIntegralMember(trait_type->info.struct_info,
                                                  "value", &value) ||
            MetaTraitIntegralConstantValue(trait_type, &value)) {
          if (size_out != NULL) {
            *size_out = value;
          }
          TypeRecordDelete(trait_type);
          return true;
        }
        for (size_t i = 0; i < trait_type->info.struct_info->bases.length; i++) {
          CXXBaseSpecifier* base =
              trait_type->info.struct_info->bases.value.p[i];
          if (base != NULL && base->type != NULL &&
              TypeIsStructOrUnion(base->type) &&
              base->type->info.struct_info != NULL &&
              MetaTraitEvaluateStructIntegralMember(base->type->info.struct_info,
                                                    "value", &value)) {
            if (size_out != NULL) {
              *size_out = value;
            }
            TypeRecordDelete(trait_type);
            return true;
          }
        }
      }
      TypeRecordDelete(trait_type);
    }
  }
  return false;
}

static TypeRecord* MetaTraitInstantiateIndexedTypeTrait(const char* trait_name,
                                                        size_t index,
                                                        TypeRecord* object_type,
                                                        ReflectionValue* value) {
  if (object_type == NULL) {
    return NULL;
  }
  TypeRecord* query_type =
      MetaTraitRecoverConcreteQueryType(object_type, value);
  TypeRecord* direct = MetaTraitClassTemplateArgType(query_type, index);
  if (direct != NULL) {
    TypeRecordDelete(query_type);
    return direct;
  }
  Symbol* trait = MetaTraitFindStdSymbol(trait_name);
  if (trait != NULL && trait->flags.is_template) {
    TypeRecord* template_query = MetaTraitRecoverTemplateIdType(query_type);
    if (template_query != NULL && TypeIsReference(template_query)) {
      TypeRecord* unreferenced = template_query->next;
      TypeRecordDelete(template_query);
      template_query =
          unreferenced != NULL ? TypeRecordCopy(unreferenced) : NULL;
    }
    if (template_query != NULL) {
      Vector* args = NewVector();
      TemplateArgument* index_arg =
          NewIntegralTemplateArgument((long long)index);
      index_arg->type = NewSizeTypeRecord();
      VectorAppend(args, index_arg);
      VectorAppend(args, NewTypeTemplateArgument(template_query));
      TypeRecord* trait_type =
          MetaTraitInstantiateClassTemplateScoped(trait, args);
      VectorDeleteWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      TypeRecordDelete(template_query);
      TypeRecord* result = NULL;
      if (trait_type != NULL && TypeIsStructOrUnion(trait_type) &&
          trait_type->info.struct_info != NULL) {
        result = MetaTraitTraitStructMemberType(trait_type, "type");
      }
      TypeRecordDelete(trait_type);
      TypeRecordDelete(query_type);
      if (result != NULL) {
        return result;
      }
    }
  }
  TypeRecordDelete(query_type);
  return NULL;
}

static Vector* MetaTraitTupleElementTypes(TypeRecord* tuple_type) {
  Vector* types = NewVector();
  size_t size = 0;
  if (!MetaTraitInstantiateSizeTrait("tuple_size", tuple_type, &size)) {
    VectorDelete(types);
    return NULL;
  }
  for (size_t i = 0; i < size; i++) {
    TypeRecord* element =
        MetaTraitInstantiateIndexedTypeTrait("tuple_element", i, tuple_type,
                                             NULL);
    if (element == NULL) {
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return NULL;
    }
    VectorAppend(types, element);
  }
  return types;
}

static int MetaTraitTypeOrderValue(TypeRecord* left, TypeRecord* right) {
  if (MetaTraitTypeEqual(left, right)) {
    return 0;
  }
  uintptr_t left_key = (uintptr_t)left;
  uintptr_t right_key = (uintptr_t)right;
  if (left_key < right_key) {
    return -1;
  }
  if (left_key > right_key) {
    return 1;
  }
  return 0;
}

static bool MetaTraitStructHasNonStaticDataMembers(Struct* str) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member != NULL && !member->is_static && !member->is_member_function &&
        !StructMemberIsNestedType(member)) {
      return true;
    }
  }
  return false;
}

static bool MetaTraitIsStandardLayoutTypeImpl(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  if (TypeIsFixedArray(type)) {
    return MetaTraitIsStandardLayoutTypeImpl(type->next);
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return true;
  }
  Struct* str = type->info.struct_info;
  if (str->vptr_member != NULL || str->virtual_members.length > 0 ||
      str->virtual_bases.length > 0) {
    return false;
  }
  bool seen_data_member = false;
  CXXAccess member_access = kAccessPublic;
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->is_static || member->is_member_function ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    if (member->symbol == NULL || member->symbol->type == NULL) {
      return false;
    }
    if (!MetaTraitIsStandardLayoutTypeImpl(member->symbol->type)) {
      return false;
    }
    if (!seen_data_member) {
      member_access = member->access;
      seen_data_member = true;
    } else if (member->access != member_access) {
      return false;
    }
  }
  size_t bases_with_members = 0;
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      return false;
    }
    if (base->is_virtual) {
      return false;
    }
    if (!MetaTraitIsStandardLayoutTypeImpl(base->type)) {
      return false;
    }
    if (MetaTraitStructHasNonStaticDataMembers(base->type->info.struct_info)) {
      bases_with_members++;
    }
  }
  return bases_with_members <= 1;
}

static bool MetaTraitIsStandardLayoutType(TypeRecord* type) {
  type = CXXTypeTraitMaterializeType(&compiler->syntax, type);
  if (type == NULL) {
    return false;
  }
  bool result = MetaTraitIsStandardLayoutTypeImpl(type);
  TypeRecordDelete(type);
  return result;
}

static bool MetaTraitTypesLayoutCompatible(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  if (MetaTraitTypeEqual(left, right)) {
    return true;
  }
  left = CXXTypeTraitMaterializeType(&compiler->syntax, left);
  right = CXXTypeTraitMaterializeType(&compiler->syntax, right);
  if (left == NULL || right == NULL) {
    TypeRecordDelete(left);
    TypeRecordDelete(right);
    return false;
  }
  if (!MetaTraitIsStandardLayoutTypeImpl(left) ||
      !MetaTraitIsStandardLayoutTypeImpl(right)) {
    TypeRecordDelete(left);
    TypeRecordDelete(right);
    return false;
  }
  if (left->size != right->size ||
      TypeRecordAlignment(left) != TypeRecordAlignment(right)) {
    TypeRecordDelete(left);
    TypeRecordDelete(right);
    return false;
  }
  if (!TypeIsStructOrUnion(left) || !TypeIsStructOrUnion(right) ||
      left->info.struct_info == NULL || right->info.struct_info == NULL) {
    bool result = TypeEqualIgnoringSign(left, right);
    TypeRecordDelete(left);
    TypeRecordDelete(right);
    return result;
  }
  Struct* left_str = left->info.struct_info;
  Struct* right_str = right->info.struct_info;
  if (left_str->is_union != right_str->is_union ||
      left_str->members.length != right_str->members.length ||
      left_str->bases.length != right_str->bases.length) {
    TypeRecordDelete(left);
    TypeRecordDelete(right);
    return false;
  }
  bool result = true;
  for (size_t i = 0; i < left_str->members.length && result; i++) {
    StructMember* left_member = left_str->members.value.p[i];
    StructMember* right_member = right_str->members.value.p[i];
    if (left_member == NULL || right_member == NULL ||
        left_member->byte_offset != right_member->byte_offset ||
        left_member->is_static != right_member->is_static ||
        left_member->is_member_function != right_member->is_member_function) {
      result = false;
      break;
    }
    if (left_member->symbol == NULL || right_member->symbol == NULL ||
        left_member->symbol->type == NULL ||
        right_member->symbol->type == NULL) {
      result = false;
      break;
    }
    if (!MetaTraitTypesLayoutCompatible(left_member->symbol->type,
                                      right_member->symbol->type)) {
      result = false;
    }
  }
  TypeRecordDelete(left);
  TypeRecordDelete(right);
  return result;
}

static bool MetaTraitIsImplicitLifetimeTypeImpl(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  if (TypeIsFixedArray(type)) {
    return MetaTraitIsImplicitLifetimeTypeImpl(type->next);
  }
  if (TypeIsScalar(type)) {
    return true;
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return false;
  }
  if (!CXXTypeSpecialMemberIsTrivial(type, kCXXSpecialMemberDestructor)) {
    return false;
  }
  Struct* str = type->info.struct_info;
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !MetaTraitIsImplicitLifetimeTypeImpl(base->type)) {
      return false;
    }
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->is_static || member->is_member_function ||
        StructMemberIsNestedType(member) || member->symbol == NULL ||
        member->symbol->type == NULL) {
      continue;
    }
    if (!MetaTraitIsImplicitLifetimeTypeImpl(member->symbol->type)) {
      return false;
    }
  }
  return true;
}

static bool MetaTraitHasUniqueObjectRepresentationsImpl(TypeRecord* type) {
  if (type == NULL || !CXXTypeIsTriviallyCopyable(type)) {
    return false;
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return true;
  }
  if (!MetaTraitIsStandardLayoutTypeImpl(type)) {
    return false;
  }
  Struct* str = type->info.struct_info;
  size_t member_bits = 0;
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->is_static || member->is_member_function ||
        StructMemberIsNestedType(member) || member->symbol == NULL ||
        member->symbol->type == NULL) {
      continue;
    }
    if (!MetaTraitHasUniqueObjectRepresentationsImpl(member->symbol->type)) {
      return false;
    }
    member_bits += (size_t)member->symbol->type->size * 8;
  }
  return member_bits == (size_t)type->size * 8;
}

static bool MetaTraitIsPointerInterconvertibleBase(TypeRecord* base_type,
                                                   TypeRecord* derived_type) {
  if (base_type == NULL || derived_type == NULL ||
      !MetaTraitIsStandardLayoutTypeImpl(base_type) ||
      !MetaTraitIsStandardLayoutTypeImpl(derived_type) ||
      !TypeIsStructOrUnion(base_type) || !TypeIsStructOrUnion(derived_type) ||
      base_type->info.struct_info == NULL ||
      derived_type->info.struct_info == NULL) {
    return false;
  }
  if (MetaTraitTypeEqual(base_type, derived_type)) {
    return true;
  }
  if (!TypeIsDerivedFrom(derived_type, base_type)) {
    return false;
  }
  int offset = 0;
  return TypeBaseOffset(derived_type, base_type, /*public_only=*/true, &offset) &&
         offset == 0;
}

static bool MetaTraitEvaluateTrivialSpecial(MetaTraitOperation operation,
                                            VectorASTNode* call,
                                            bool* dependent,
                                            SourceLocation location) {
  Vector* types = NULL;
  bool result = false;
  switch (operation) {
    case kMetaTraitIsTriviallyConstructibleType:
      types = MetaTraitCollectTypeVector(call, 1, true);
      if (types != NULL) {
        result = CXXTypeTraitIsTriviallyConstructible(&compiler->syntax, types);
      }
      break;
    case kMetaTraitIsTriviallyDefaultConstructibleType:
      types = MetaTraitCollectTypeVector(call, 1, false);
      if (types != NULL) {
        result = CXXTypeTraitIsTriviallyConstructible(&compiler->syntax, types);
      }
      break;
    case kMetaTraitIsTriviallyCopyConstructibleType:
    case kMetaTraitIsTriviallyMoveConstructibleType: {
      TypeRecord* type = MetaTraitSingleType(call, dependent);
      if (*dependent) {
        return false;
      }
      Vector local;
      VectorInit(&local);
      if (type != NULL) {
        VectorAppend(&local, type);
        if (operation == kMetaTraitIsTriviallyCopyConstructibleType) {
          TypeRecord* plain = MetaTraitRemoveCv(type);
          TypeRecord* const_plain = MetaTraitAddTopQual(plain, kQualConst);
          TypeRecord* const_lref = MetaTraitAddReference(const_plain, false);
          TypeRecordDelete(plain);
          TypeRecordDelete(const_plain);
          VectorAppend(&local, const_lref);
        } else {
          VectorAppend(&local, MetaTraitAddReference(type, true));
        }
      }
      result = CXXTypeTraitIsTriviallyConstructible(&compiler->syntax, &local);
      VectorDestructWithContents(&local, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
      return result;
    }
    case kMetaTraitIsTriviallyAssignablType:
      types = MetaTraitCollectTypeVector(call, 2, false);
      if (types != NULL) {
        result = CXXTypeTraitIsTriviallyAssignable(&compiler->syntax, types);
      }
      break;
    case kMetaTraitIsTriviallyCopyAssignableType:
    case kMetaTraitIsTriviallyMoveAssignableType: {
      TypeRecord* type = MetaTraitSingleType(call, dependent);
      if (*dependent) {
        return false;
      }
      Vector local;
      VectorInit(&local);
      if (type != NULL) {
        VectorAppend(&local, type);
        if (operation == kMetaTraitIsTriviallyCopyAssignableType) {
          TypeRecord* plain = MetaTraitRemoveCv(type);
          TypeRecord* const_plain = MetaTraitAddTopQual(plain, kQualConst);
          TypeRecord* const_lref = MetaTraitAddReference(const_plain, false);
          TypeRecordDelete(plain);
          TypeRecordDelete(const_plain);
          VectorAppend(&local, const_lref);
        } else {
          VectorAppend(&local, MetaTraitAddReference(type, true));
        }
      }
      result = CXXTypeTraitIsTriviallyAssignable(&compiler->syntax, &local);
      VectorDestructWithContents(&local, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
      return result;
    }
    case kMetaTraitIsTriviallyDestructibleType: {
      TypeRecord* type = MetaTraitSingleType(call, dependent);
      if (*dependent) {
        return false;
      }
      result = CXXTypeTraitIsTriviallyDestructible(&compiler->syntax, type);
      TypeRecordDelete(type);
      return result;
    }
    default:
      return false;
  }
  if (types == NULL) {
    return false;
  }
  VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                           /*free_element=*/false);
  return result;
}

static ASTNode* MetaTraitDispatch(MetaTraitOperation operation,
                                  VectorASTNode* call) {
  SourceLocation location = call->base.location;
  bool dependent = false;

  switch (operation) {
    case kMetaTraitIsVoidType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      return MetaTraitBool(type != NULL && TypeIsVoid(type), location);
    }
    case kMetaTraitIsNullPointerType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      return MetaTraitBool(type != NULL && TypeIsNullPointer(type), location);
    }
    case kMetaTraitIsIntegralType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      return MetaTraitBool(type != NULL && TypeIsIntegral(type), location);
    }
    case kMetaTraitIsFloatingPointType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      return MetaTraitBool(type != NULL && TypeIsFloatingPoint(type), location);
    }
    case kMetaTraitIsArrayType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = false;
      if (type != NULL) {
        for (TypeRecord* cur = type; cur != NULL; cur = cur->next) {
          if (TypeIsArray(cur)) {
            result = true;
            break;
          }
        }
      }
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsBoundedArrayType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsFixedArray(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsUnboundedArrayType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsArray(type) && !TypeIsFixedArray(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsPointerType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = false;
      if (type != NULL) {
        for (TypeRecord* cur = type; cur != NULL; cur = cur->next) {
          if (TypeIsPointer(cur)) {
            result = true;
            break;
          }
        }
      }
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsLvalueReferenceType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsReference(type) &&
                    type->declarator == kDeclReference;
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsRvalueReferenceType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsReference(type) &&
                    type->declarator == kDeclRValueReference;
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsReferenceType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsReference(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsMemberPointerType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = MetaTraitIsMemberPointer(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsMemberObjectPointerType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = MetaTraitIsMemberPointer(type) &&
                    !MetaTraitIsMemberFunctionPointer(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsMemberFunctionPointerType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = MetaTraitIsMemberFunctionPointer(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsEnumType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsEnum(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsScopedEnumType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsScopedEnum(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsUnionType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = MetaTraitIsUnionType(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsClassType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = MetaTraitIsClassType(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsFunctionType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsFunction(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsReflectionType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsReflection(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsArithmeticType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL &&
                    (TypeIsIntegral(type) || TypeIsFloatingPoint(type));
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsFundamentalType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsScalar(type) && !TypeIsEnum(type) &&
                    !TypeIsNullPointer(type) && !TypeIsPointer(type) &&
                    !TypeIsMemberPointer(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsObjectType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && !TypeIsReference(type) && !TypeIsVoid(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsScalarType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsScalar(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsCompoundType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && !TypeIsScalar(type) && !TypeIsVoid(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsCompleteType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL &&
                    (TypeIsVoid(type) || type->size > 0 ||
                     (TypeIsArray(type) && !type->info.array.is_dependent_bound));
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsConstType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsConst(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsVolatileType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsVolatile(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsSignedType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsSigned(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsUnsignedType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsUnsigned(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsTriviallyCopyableType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && CXXTypeIsTriviallyCopyable(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsAggregateType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsStructOrUnion(type) &&
                    type->info.struct_info != NULL &&
                    type->info.struct_info->is_aggregate;
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsEmptyType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = MetaTraitIsEmpty(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsPolymorphicType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = MetaTraitIsPolymorphic(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsAbstractType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = TypeIsAbstractClass(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsFinalType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL && TypeIsStructOrUnion(type) &&
                    type->info.struct_info != NULL &&
                    type->info.struct_info->is_final;
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsStructuralType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = type != NULL &&
                    (TypeIsIntegral(type) || TypeIsFloatingPoint(type) ||
                     TypeIsNullPointer(type) || TypeIsEnum(type) ||
                     TypeIsPointer(type) || TypeIsMemberPointer(type));
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsSameType: {
      TypeRecord* second = NULL;
      TypeRecord* first = MetaTraitPairTypes(call, &second, &dependent);
      if (dependent) {
        TypeRecordDelete(first);
        TypeRecordDelete(second);
        return NULL;
      }
      bool result = MetaTraitTypeEqual(first, second);
      TypeRecordDelete(first);
      TypeRecordDelete(second);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsBaseOfType: {
      Vector* types = MetaTraitCollectTypeVector(call, 2, false);
      if (types == NULL) {
        return NULL;
      }
      bool result = MetaTraitEvaluateBoolKind(kCXXTypeTraitIsBaseOf, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsVirtualBaseOfType: {
      TypeRecord* second = NULL;
      TypeRecord* first = MetaTraitPairTypes(call, &second, &dependent);
      if (dependent) {
        TypeRecordDelete(first);
        TypeRecordDelete(second);
        return NULL;
      }
      bool result = false;
      if (first != NULL && second != NULL && TypeIsStructOrUnion(second) &&
          second->info.struct_info != NULL) {
        result = MetaTraitIsVirtualBaseOf(second->info.struct_info, first);
      }
      TypeRecordDelete(first);
      TypeRecordDelete(second);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsConvertibleType: {
      Vector* types = MetaTraitCollectTypeVector(call, 2, false);
      if (types == NULL) {
        return NULL;
      }
      bool result = CXXTypeTraitIsConvertible(&compiler->syntax, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsNothrowConvertibleType: {
      Vector* types = MetaTraitCollectTypeVector(call, 2, false);
      if (types == NULL) {
        return NULL;
      }
      bool result = CXXTypeTraitIsNothrowConvertible(&compiler->syntax, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsStandardLayoutType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = MetaTraitIsStandardLayoutType(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsLayoutCompatibleType: {
      TypeRecord* second = NULL;
      TypeRecord* first = MetaTraitPairTypes(call, &second, &dependent);
      if (dependent) {
        TypeRecordDelete(first);
        TypeRecordDelete(second);
        return NULL;
      }
      bool result = MetaTraitTypesLayoutCompatible(first, second);
      TypeRecordDelete(first);
      TypeRecordDelete(second);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsPointerInterconvertibleBaseOfType: {
      TypeRecord* second = NULL;
      TypeRecord* first = MetaTraitPairTypes(call, &second, &dependent);
      if (dependent) {
        TypeRecordDelete(first);
        TypeRecordDelete(second);
        return NULL;
      }
      first = first != NULL ? CXXTypeTraitMaterializeType(&compiler->syntax, first)
                            : NULL;
      second = second != NULL ? CXXTypeTraitMaterializeType(&compiler->syntax, second)
                              : NULL;
      bool result = MetaTraitIsPointerInterconvertibleBase(first, second);
      TypeRecordDelete(first);
      TypeRecordDelete(second);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsImplicitLifetimeType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      type = type != NULL ? CXXTypeTraitMaterializeType(&compiler->syntax, type)
                          : NULL;
      bool result = MetaTraitIsImplicitLifetimeTypeImpl(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitHasUniqueObjectRepresentations: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      type = type != NULL ? CXXTypeTraitMaterializeType(&compiler->syntax, type)
                          : NULL;
      bool result = MetaTraitHasUniqueObjectRepresentationsImpl(type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitReferenceConstructsFromTemporary: {
      Vector* types = MetaTraitCollectTypeVector(call, 2, false);
      if (types == NULL) {
        return NULL;
      }
      bool result =
          CXXTypeTraitReferenceConstructsFromTemporary(&compiler->syntax, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitReferenceConvertsFromTemporary: {
      Vector* types = MetaTraitCollectTypeVector(call, 2, false);
      if (types == NULL) {
        return NULL;
      }
      bool result =
          CXXTypeTraitReferenceConvertsFromTemporary(&compiler->syntax, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsTriviallyConstructibleType:
    case kMetaTraitIsTriviallyDefaultConstructibleType:
    case kMetaTraitIsTriviallyCopyConstructibleType:
    case kMetaTraitIsTriviallyMoveConstructibleType:
    case kMetaTraitIsTriviallyAssignablType:
    case kMetaTraitIsTriviallyCopyAssignableType:
    case kMetaTraitIsTriviallyMoveAssignableType:
    case kMetaTraitIsTriviallyDestructibleType: {
      bool result = MetaTraitEvaluateTrivialSpecial(operation, call, &dependent,
                                                   location);
      if (dependent) {
        return NULL;
      }
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsConstructibleType: {
      Vector* types = MetaTraitCollectTypeVector(call, 1, true);
      if (types == NULL) {
        return NULL;
      }
      bool result =
          MetaTraitEvaluateBoolKind(kCXXTypeTraitIsConstructible, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsDefaultConstructibleType: {
      Vector* types = MetaTraitCollectTypeVector(call, 1, false);
      if (types == NULL) {
        return NULL;
      }
      bool result =
          MetaTraitEvaluateBoolKind(kCXXTypeTraitIsConstructible, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsCopyConstructibleType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      Vector types;
      VectorInit(&types);
      if (type != NULL) {
        VectorAppend(&types, type);
        TypeRecord* plain = MetaTraitRemoveCv(type);
        TypeRecord* const_plain = MetaTraitAddTopQual(plain, kQualConst);
        TypeRecord* const_lref = MetaTraitAddReference(const_plain, false);
        TypeRecordDelete(plain);
        TypeRecordDelete(const_plain);
        VectorAppend(&types, const_lref);
      }
      bool result =
          MetaTraitEvaluateBoolKind(kCXXTypeTraitIsConstructible, &types);
      VectorDestructWithContents(&types, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsMoveConstructibleType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      Vector types;
      VectorInit(&types);
      if (type != NULL) {
        VectorAppend(&types, type);
        TypeRecord* rref = MetaTraitAddReference(type, true);
        VectorAppend(&types, rref);
      }
      bool result =
          MetaTraitEvaluateBoolKind(kCXXTypeTraitIsConstructible, &types);
      VectorDestructWithContents(&types, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsAssignableType: {
      Vector* types = MetaTraitCollectTypeVector(call, 2, false);
      if (types == NULL) {
        return NULL;
      }
      bool result = MetaTraitEvaluateBoolKind(kCXXTypeTraitIsAssignable, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsCopyAssignableType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      Vector types;
      VectorInit(&types);
      if (type != NULL) {
        VectorAppend(&types, type);
        TypeRecord* plain = MetaTraitRemoveCv(type);
        TypeRecord* const_plain = MetaTraitAddTopQual(plain, kQualConst);
        TypeRecord* const_lref = MetaTraitAddReference(const_plain, false);
        TypeRecordDelete(plain);
        TypeRecordDelete(const_plain);
        VectorAppend(&types, const_lref);
      }
      bool result = MetaTraitEvaluateBoolKind(kCXXTypeTraitIsAssignable, &types);
      VectorDestructWithContents(&types, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsMoveAssignableType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      Vector types;
      VectorInit(&types);
      if (type != NULL) {
        VectorAppend(&types, type);
        TypeRecord* rref = MetaTraitAddReference(type, true);
        VectorAppend(&types, rref);
      }
      bool result = MetaTraitEvaluateBoolKind(kCXXTypeTraitIsAssignable, &types);
      VectorDestructWithContents(&types, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsDestructibleType: {
      Vector* types = MetaTraitCollectTypeVector(call, 1, false);
      if (types == NULL) {
        return NULL;
      }
      bool result =
          MetaTraitEvaluateBoolKind(kCXXTypeTraitIsDestructible, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsSwappableType: {
      Vector* types = MetaTraitCollectTypeVector(call, 1, false);
      if (types == NULL) {
        return NULL;
      }
      bool result = MetaTraitEvaluateBoolKind(kCXXTypeTraitIsSwappable, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsSwappableWithType: {
      Vector* types = MetaTraitCollectTypeVector(call, 2, false);
      if (types == NULL) {
        return NULL;
      }
      bool result =
          MetaTraitEvaluateBoolKind(kCXXTypeTraitIsSwappableWith, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsNothrowConstructibleType: {
      Vector* types = MetaTraitCollectTypeVector(call, 1, true);
      if (types == NULL) {
        return NULL;
      }
      bool result =
          MetaTraitEvaluateBoolKind(kCXXTypeTraitIsNothrowConstructible, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsNothrowDefaultConstructibleType: {
      Vector* types = MetaTraitCollectTypeVector(call, 1, false);
      if (types == NULL) {
        return NULL;
      }
      bool result =
          MetaTraitEvaluateBoolKind(kCXXTypeTraitIsNothrowConstructible, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsNothrowCopyConstructibleType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      Vector types;
      VectorInit(&types);
      if (type != NULL) {
        VectorAppend(&types, type);
        TypeRecord* plain = MetaTraitRemoveCv(type);
        TypeRecord* const_plain = MetaTraitAddTopQual(plain, kQualConst);
        TypeRecord* const_lref = MetaTraitAddReference(const_plain, false);
        TypeRecordDelete(plain);
        TypeRecordDelete(const_plain);
        VectorAppend(&types, const_lref);
      }
      bool result = MetaTraitEvaluateBoolKind(kCXXTypeTraitIsNothrowConstructible,
                                              &types);
      VectorDestructWithContents(&types, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsNothrowMoveConstructibleType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      Vector types;
      VectorInit(&types);
      if (type != NULL) {
        VectorAppend(&types, type);
        TypeRecord* rref = MetaTraitAddReference(type, true);
        VectorAppend(&types, rref);
      }
      bool result = MetaTraitEvaluateBoolKind(kCXXTypeTraitIsNothrowConstructible,
                                              &types);
      VectorDestructWithContents(&types, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsNothrowAssignableType: {
      Vector* types = MetaTraitCollectTypeVector(call, 2, false);
      if (types == NULL) {
        return NULL;
      }
      bool result =
          MetaTraitEvaluateBoolKind(kCXXTypeTraitIsNothrowAssignable, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsNothrowCopyAssignableType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      Vector types;
      VectorInit(&types);
      if (type != NULL) {
        VectorAppend(&types, type);
        TypeRecord* plain = MetaTraitRemoveCv(type);
        TypeRecord* const_plain = MetaTraitAddTopQual(plain, kQualConst);
        TypeRecord* const_lref = MetaTraitAddReference(const_plain, false);
        TypeRecordDelete(plain);
        TypeRecordDelete(const_plain);
        VectorAppend(&types, const_lref);
      }
      bool result =
          MetaTraitEvaluateBoolKind(kCXXTypeTraitIsNothrowAssignable, &types);
      VectorDestructWithContents(&types, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsNothrowMoveAssignableType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      Vector types;
      VectorInit(&types);
      if (type != NULL) {
        VectorAppend(&types, type);
        TypeRecord* rref = MetaTraitAddReference(type, true);
        VectorAppend(&types, rref);
      }
      bool result =
          MetaTraitEvaluateBoolKind(kCXXTypeTraitIsNothrowAssignable, &types);
      VectorDestructWithContents(&types, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsNothrowDestructibleType: {
      Vector* types = MetaTraitCollectTypeVector(call, 1, false);
      if (types == NULL) {
        return NULL;
      }
      bool result =
          MetaTraitEvaluateBoolKind(kCXXTypeTraitIsNothrowDestructible, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsNothrowSwappableType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = CXXTypeTraitIsNothrowSwappable(&compiler->syntax, type);
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsNothrowSwappableWithType: {
      TypeRecord* second = NULL;
      TypeRecord* first = MetaTraitPairTypes(call, &second, &dependent);
      if (dependent) {
        TypeRecordDelete(first);
        TypeRecordDelete(second);
        return NULL;
      }
      bool result =
          CXXTypeTraitIsNothrowSwappableWith(&compiler->syntax, first, second);
      TypeRecordDelete(first);
      TypeRecordDelete(second);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsInvocableType: {
      Vector* types = MetaTraitCollectTypeVector(call, 1, true);
      if (types == NULL) {
        return NULL;
      }
      bool result = MetaTraitEvaluateBoolKind(kCXXTypeTraitIsInvocable, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsNothrowInvocableType: {
      Vector* types = MetaTraitCollectTypeVector(call, 1, true);
      if (types == NULL) {
        return NULL;
      }
      bool result =
          MetaTraitEvaluateBoolKind(kCXXTypeTraitIsNothrowInvocable, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsInvocableRType: {
      Vector* types = MetaTraitCollectTypeVector(call, 2, true);
      if (types == NULL || types->length < 2) {
        VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
        return NULL;
      }
      TypeRecord* result_type = (TypeRecord*)types->value.p[0];
      Vector args;
      VectorInit(&args);
      for (size_t i = 1; i < types->length; i++) {
        VectorAppend(&args, types->value.p[i]);
      }
      bool result = CXXTypeTraitIsInvocableR(&compiler->syntax, result_type, &args,
                                             false);
      VectorDestruct(&args);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitIsNothrowInvocableRType: {
      Vector* types = MetaTraitCollectTypeVector(call, 2, true);
      if (types == NULL || types->length < 2) {
        VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
        return NULL;
      }
      TypeRecord* result_type = (TypeRecord*)types->value.p[0];
      Vector args;
      VectorInit(&args);
      for (size_t i = 1; i < types->length; i++) {
        VectorAppend(&args, types->value.p[i]);
      }
      bool result = CXXTypeTraitIsInvocableR(&compiler->syntax, result_type, &args,
                                             true);
      VectorDestruct(&args);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitHasVirtualDestructor: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      bool result = false;
      if (type != NULL && TypeIsStructOrUnion(type) &&
          type->info.struct_info != NULL) {
        StructMember* dtor =
            FindStructMemberByName(type->info.struct_info, "~");
        if (dtor == NULL) {
          for (size_t i = 0; i < type->info.struct_info->members.length; i++) {
            StructMember* member = type->info.struct_info->members.value.p[i];
            if (member != NULL && member->symbol != NULL &&
                TypeIsFunction(member->symbol->type) &&
                member->symbol->type->info.function.is_destructor) {
              dtor = member;
              break;
            }
          }
        }
        result = dtor != NULL && dtor->symbol != NULL &&
                 TypeIsFunction(dtor->symbol->type) &&
                 dtor->symbol->type->info.function.is_virtual;
      }
      TypeRecordDelete(type);
      return MetaTraitBool(result, location);
    }
    case kMetaTraitRank: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      size_t value = MetaTraitArrayRank(type);
      TypeRecordDelete(type);
      return MetaTraitSizeT(value, location);
    }
    case kMetaTraitExtent: {
      if (call->children == NULL || call->children->length == 0) {
        return NULL;
      }
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      size_t index = 0;
      if (call->children->length > 1) {
        int64_t index_value = 0;
        if (!EvaluateIntegerExpression(call->children->value.p[1],
                                     &index_value) ||
            index_value < 0) {
          TypeRecordDelete(type);
          return MetaTraitSizeT(0, location);
        }
        index = (size_t)index_value;
      }
      size_t value = MetaTraitArrayExtent(type, index);
      TypeRecordDelete(type);
      return MetaTraitSizeT(value, location);
    }
    case kMetaTraitRemoveConst: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitRemoveTopQual(type, kQualConst);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitRemoveVolatile: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitRemoveTopQual(type, kQualVolatile);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitRemoveCv: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitRemoveTopQual(type, kQualConst | kQualVolatile);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitAddConst: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitAddTopQual(type, kQualConst);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitAddVolatile: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitAddTopQual(type, kQualVolatile);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitAddCv: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result =
          MetaTraitAddTopQual(type, kQualConst | kQualVolatile);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitRemoveReference: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitRemoveReference(type);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitAddLvalueReference: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitAddReference(type, false);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitAddRvalueReference: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitAddReference(type, true);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitRemoveExtent: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitRemoveExtentOnce(type);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitRemoveAllExtents: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitRemoveAllExtents(type);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitRemovePointer: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitRemovePointerOnce(type);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitAddPointer: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitAddPointer(type);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitRemoveCvref: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitRemoveCv(MetaTraitRemoveReference(type));
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitDecay: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitDecay(type);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitMakeSigned: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitMakeSigned(type);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitMakeUnsigned: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitMakeUnsigned(type);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitCommonType: {
      Vector* types = MetaTraitCollectTypeVector(call, 0, false);
      if (types == NULL) {
        return NULL;
      }
      TypeRecord* result = NULL;
      if (types->length == 1) {
        result = MetaTraitDecay((TypeRecord*)types->value.p[0]);
      } else {
        result = CXXTypeTraitCommonTypeFold(&compiler->syntax, types);
        if (result != NULL) {
          TypeRecord* decayed = MetaTraitDecay(result);
          TypeRecordDelete(result);
          result = decayed;
        }
      }
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitCommonReference: {
      Vector* types = MetaTraitCollectTypeVector(call, 0, false);
      if (types == NULL) {
        return NULL;
      }
      TypeRecord* result = CXXTypeTraitCommonReference(&compiler->syntax, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitUnderlyingType: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitUnderlyingType(type);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitInvokeResult: {
      Vector* types = MetaTraitCollectTypeVector(call, 1, true);
      if (types == NULL) {
        return NULL;
      }
      TypeRecord* result = CXXTypeTraitInvokeResultType(&compiler->syntax, types);
      VectorDeleteWithContents(types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitUnwrapReference: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitUnwrapReference(type);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitUnwrapRefDecay: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitUnwrapRefDecay(type);
      TypeRecordDelete(type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitTupleSize: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      size_t size = 0;
      bool ok = MetaTraitClassTemplateArgCount(type, &size);
      if (!ok) {
        ok = MetaTraitInstantiateSizeTrait("tuple_size", type, &size);
      }
      TypeRecordDelete(type);
      return ok ? MetaTraitSizeT(size, location) : MetaTraitSizeT(0, location);
    }
    case kMetaTraitVariantSize: {
      TypeRecord* type = MetaTraitSingleType(call, &dependent);
      if (dependent) {
        return NULL;
      }
      size_t size = 0;
      bool ok = MetaTraitInstantiateSizeTrait("variant_size", type, &size);
      TypeRecordDelete(type);
      return ok ? MetaTraitSizeT(size, location) : MetaTraitSizeT(0, location);
    }
    case kMetaTraitTupleElement: {
      if (call->children == NULL || call->children->length < 2) {
        return NULL;
      }
      int64_t index_value = 0;
      if (!EvaluateIntegerExpression(call->children->value.p[0], &index_value) ||
          index_value < 0) {
        return NULL;
      }
      bool arg_dependent = false;
      ASTNode* tuple_expr = call->children->value.p[1];
      ReflectionValue* tuple_reflection =
          MetaTraitReflectionFromExpression(tuple_expr, &arg_dependent);
      TypeRecord* tuple_type =
          MetaTraitTypeFromCallExpression(tuple_expr, location, &arg_dependent);
      if (arg_dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitInstantiateIndexedTypeTrait(
          "tuple_element", (size_t)index_value, tuple_type, tuple_reflection);
      TypeRecordDelete(tuple_type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitVariantAlternative: {
      if (call->children == NULL || call->children->length < 2) {
        return NULL;
      }
      int64_t index_value = 0;
      if (!EvaluateIntegerExpression(call->children->value.p[0], &index_value) ||
          index_value < 0) {
        return NULL;
      }
      bool arg_dependent = false;
      ASTNode* variant_expr = call->children->value.p[1];
      ReflectionValue* variant_reflection =
          MetaTraitReflectionFromExpression(variant_expr, &arg_dependent);
      TypeRecord* variant_type =
          MetaTraitTypeFromCallExpression(variant_expr, location, &arg_dependent);
      if (arg_dependent) {
        return NULL;
      }
      TypeRecord* result = MetaTraitInstantiateIndexedTypeTrait(
          "variant_alternative", (size_t)index_value, variant_type,
          variant_reflection);
      TypeRecordDelete(variant_type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitIsApplicableType:
    case kMetaTraitIsNothrowApplicableType:
    case kMetaTraitApplyResult: {
      TypeRecord* tuple_type = NULL;
      TypeRecord* fn_type = MetaTraitPairTypes(call, &tuple_type, &dependent);
      if (dependent) {
        TypeRecordDelete(fn_type);
        TypeRecordDelete(tuple_type);
        return NULL;
      }
      Vector* arg_types = MetaTraitTupleElementTypes(tuple_type);
      if (arg_types == NULL) {
        TypeRecordDelete(fn_type);
        TypeRecordDelete(tuple_type);
        return MetaTraitBool(false, location);
      }
      Vector invoke_types;
      VectorInit(&invoke_types);
      VectorAppend(&invoke_types, fn_type);
      for (size_t i = 0; i < arg_types->length; i++) {
        VectorAppend(&invoke_types, arg_types->value.p[i]);
      }
      if (operation == kMetaTraitIsApplicableType) {
        bool result =
            MetaTraitEvaluateBoolKind(kCXXTypeTraitIsInvocable, &invoke_types);
        VectorDestruct(&invoke_types);
        VectorDeleteWithContents(arg_types, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
        TypeRecordDelete(tuple_type);
        return MetaTraitBool(result, location);
      }
      if (operation == kMetaTraitIsNothrowApplicableType) {
        bool result = MetaTraitEvaluateBoolKind(kCXXTypeTraitIsNothrowInvocable,
                                                &invoke_types);
        VectorDestruct(&invoke_types);
        VectorDeleteWithContents(arg_types, (VectorElementDestructor)TypeRecordDelete,
                                 /*free_element=*/false);
        TypeRecordDelete(tuple_type);
        return MetaTraitBool(result, location);
      }
      TypeRecord* result =
          CXXTypeTraitInvokeResultType(&compiler->syntax, &invoke_types);
      VectorDestruct(&invoke_types);
      VectorDeleteWithContents(arg_types, (VectorElementDestructor)TypeRecordDelete,
                               /*free_element=*/false);
      TypeRecordDelete(tuple_type);
      return MetaTraitTypeReflection(result, location);
    }
    case kMetaTraitTypeOrder: {
      TypeRecord* second = NULL;
      TypeRecord* first = MetaTraitPairTypes(call, &second, &dependent);
      if (dependent) {
        TypeRecordDelete(first);
        TypeRecordDelete(second);
        return NULL;
      }
      int order = MetaTraitTypeOrderValue(first, second);
      TypeRecordDelete(first);
      TypeRecordDelete(second);
      return MetaTraitSizeT((size_t)(int64_t)order, location);
    }
    case kMetaTraitUnknown:
      return NULL;
  }
  return NULL;
}

ASTNode* SemanticTryAnalyzeMetaTraitCall(VectorASTNode* call) {
  Symbol* function = MetaTraitCallSymbol(call);
  if (function == NULL || function->name.value == NULL) {
    return NULL;
  }
  MetaTraitOperation operation = MetaTraitOperationForName(function->name.value);
  if (operation == kMetaTraitUnknown) {
    return NULL;
  }
  ASTNode* result = MetaTraitDispatch(operation, call);
  if (result != NULL) {
    return result;
  }
  return MetaTraitPreserveDependent(call, function);
}
