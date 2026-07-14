//
//  type_print.c
//  c_compiler
//

#include "type_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>

#include <assert.h>
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

// Mapping of type to its name.
static struct {
  Type type;
  const char* name;
} type_names[] = {
    {kTypeChar, "char"},          {kTypeShort, "short"},
    {kTypeLong, "long"},          {kTypeLongLong, "long long"},
    {kTypeInt, "int"},            {kTypeFloat, "float"},
    {kTypeDouble, "double"},      {kTypeLongDouble, "long double"},
    {kTypeStruct, "struct"},      {kTypeUnion, "union"},
    {kTypeVoid, "void"},          {kTypeBool, "bool"},
    {kTypeEnum, "enum"},          {kTypeNullPointer, "std::nullptr_t"},
    {kTypeImplicit, ""},
};

void TypeToString(Type type, String* result) {
  const char* separator = "";
  if ((type & kTypeSigned) != 0) {
    StringAppend(result, "signed");
    separator = " ";
  }
  if ((type & kTypeUnsigned) != 0) {
    StringAppend(result, separator);
    StringAppend(result, "unsigned");
    separator = " ";
  }
  for (int i = 0; type_names[i].type != kTypeImplicit; i++) {
    if ((type & type_names[i].type) != 0) {
      StringAppend(result, separator);
      StringAppend(result, type_names[i].name);
      separator = " ";
    }
  }
}

// Converts qualifiers to string and appends them to result.
void QualifiersToString(Qualifiers quals, String* result) {
  const char* separator = "";
  if ((quals & kQualConst) != 0) {
    StringAppend(result, "const");
    separator = " ";
  }
  if ((quals & kQualVolatile) != 0) {
    StringAppend(result, separator);
    StringAppend(result, "volatile");
    separator = " ";
  }
  if ((quals & kQualRestrict) != 0) {
    StringAppend(result, separator);
    StringAppend(result, "restrict");
  }
}

// Prints a type record to standard output.
// This is very verbose output, intended for debugging to display
// all the information needed.  The 'with_function_body' parameter
// says whether the function body (the statements) are also printed.
void TypeRecordPrintDetails(TypeRecord* record, bool with_function_body, FILE* fp) {
  String str = {0};
  bool print_newline = false;

  while (record != NULL) {
    StringClear(&str);
    if (record->declarator == kDeclPointer) {
      fprintf(fp, "pointer to ");
    } else if (record->declarator == kDeclReference) {
      fprintf(fp, "reference to ");
    } else if (record->declarator == kDeclRValueReference) {
      fprintf(fp, "rvalue reference to ");
    }
    QualifiersToString(record->qualifiers, &str);
    if (record->qualifiers != 0) {
      StringAppend(&str, " ");
    }
    Type printable_type = record->type;
    if (CompilerIsCXX()) {
      printable_type &= ~kTypeStruct;
    }
    TypeToString(printable_type, &str);
    if (TypeIsStructOrUnion(record) && record->info.struct_info != NULL &&
        record->info.struct_info->tag_name != NULL) {
      if (printable_type != kTypeImplicit) {
        StringAppend(&str, " ");
      }
      StringAppendString(&str, record->info.struct_info->tag_name);
    }
    fprintf(fp, "%s", str.value);

    if (record->declarator == kDeclArray) {
      if (record->info.array.is_vla) {
        fprintf(fp, " variable length array ");
      } else {
        fprintf(fp, " array of size %d ", record->info.array.size.fixed);
      }
    } else if (record->declarator == kDeclFunction) {
      fprintf(fp, " function (");
      const char* sep = "";
      size_t nformals = record->info.function.prototype.length;
      for (size_t i = 0; i < nformals; i++) {
        Symbol* formal = (Symbol*)record->info.function.prototype.value.p[i];
        fprintf(fp, "%s", sep);
        sep = ", ";
        SymbolPrint(formal, fp);
      }
      if (record->info.function.varargs) {
        fprintf(fp, "%s...", sep);
      }
      if (with_function_body) {
        fprintf(fp, ") {");
        CompoundStatementASTNode* body = (CompoundStatementASTNode*)
              record->info.function.body;
        size_t num_statements = body->statements->length;
        if (num_statements > 0) {
          fprintf(fp, "\n");
          print_newline = true;
        }
        for (size_t i = 0; i < num_statements; i++) {
          ASTNode* stmt = (ASTNode*)body->statements->value.p[i];
          ASTNodePrintTree(stmt, 2, fp);
        }
        fprintf(fp, "} returning ");
      } else {
        fprintf(fp, ") returning ");
      }
    }
    record = record->next;
  }
  if (print_newline) {
    fprintf(fp, "\n");
  }
  StringDestruct(&str);
}

// Basic type record printer without function body.  Also pretty verbose.
void TypeRecordPrint(TypeRecord* record, FILE* fp) {
  TypeRecordPrintDetails(record, false, fp);
}

static void AppendReadableTypeName(String* result, String* name) {
  if (name == NULL) {
    return;
  }
  for (size_t i = 0; i < name->length && name->value[i] != '\0';) {
    if (i + 4 < name->length && name->value[i] == '$' &&
        name->value[i + 1] == 'S' && name->value[i + 2] == '0' &&
        name->value[i + 3] == 'x' &&
        isxdigit((unsigned char)name->value[i + 4])) {
      i += 4;
      while (i < name->length &&
             isxdigit((unsigned char)name->value[i])) {
        i++;
      }
      continue;
    }
    if (i + 2 < name->length && name->value[i] == '$' &&
        (name->value[i + 1] == 'T' || name->value[i + 1] == 'N') &&
        isdigit((unsigned char)name->value[i + 2])) {
      i++;
      continue;
    }
    StringAppendChar(result, name->value[i++]);
  }
}

static String* TemplateParameterDisplayName(Vector* parameters, int index) {
  if (parameters == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < parameters->length; i++) {
    TemplateParameter* parameter = parameters->value.p[i];
    if (parameter != NULL && parameter->index == index &&
        parameter->name.length != 0) {
      return &parameter->name;
    }
  }
  return NULL;
}

static void TypeRecordToStringWithTemplateParameters(TypeRecord* type,
                                                     Vector* parameters,
                                                     String* result) {
  if (type == NULL) {
    StringAppend(result, "<invalid>");
    return;
  }
  switch (type->declarator) {
    case kDeclPrimitive:
      if (TypeIsUnknown(type) && type->template_parameter_index >= 0) {
        QualifiersToString(type->qualifiers, result);
        if (type->qualifiers != 0) {
          StringAppend(result, " ");
        }
        String* parameter_name = TemplateParameterDisplayName(
            parameters, type->template_parameter_index);
        if (parameter_name != NULL) {
          StringAppendString(result, parameter_name);
        } else if (type->template_parameter_name != NULL &&
                   type->template_parameter_name->length != 0) {
          StringAppendString(result, type->template_parameter_name);
        } else {
          StringPrintf(result, "T%d", type->template_parameter_index);
        }
        if (type->dependent_member_name != NULL) {
          StringAppend(result, "::");
          StringAppendString(result, type->dependent_member_name);
        }
        break;
      }
      QualifiersToString(type->qualifiers, result);
      if (type->qualifiers != 0) {
        StringAppend(result, " ");
      }
      Type printable_type = type->type;
      if (CompilerIsCXX()) {
        printable_type &= ~kTypeStruct;
      }
      TypeToString(printable_type, result);
      if (TypeIsStructOrUnion(type)) {
        if (printable_type != kTypeImplicit) {
          StringAppend(result, " ");
        }
        AppendReadableTypeName(result, type->info.struct_info->tag_name);
      } else if (TypeIsEnum(type)) {
        StringAppend(result, " ");
        StringAppendString(result, type->info.enum_info->tag_name);
      }
      break;

    case kDeclPointer:
      TypeRecordToStringWithTemplateParameters(type->next, parameters, result);
      if (type->next != NULL &&
          (type->next->declarator == kDeclArray ||
           type->next->declarator == kDeclFunction)) {
        StringAppend(result, "(*");
        if (type->qualifiers != 0) {
          StringAppend(result, " ");
        }
        QualifiersToString(type->qualifiers, result);
        StringAppendChar(result, ')');
      } else {
        StringAppendChar(result, '*');
        if (type->qualifiers != 0) {
          StringAppend(result, " ");
        }
        QualifiersToString(type->qualifiers, result);
      }
      break;

    case kDeclMemberPointer:
      if (type->info.struct_info != NULL &&
          type->info.struct_info->tag_name != NULL) {
        AppendReadableTypeName(result, type->info.struct_info->tag_name);
      } else {
        StringAppend(result, "<class>");
      }
      StringAppend(result, "::*");
      if (type->next != NULL) {
        if (type->next->declarator == kDeclFunction) {
          StringAppendChar(result, '(');
          TypeRecordToStringWithTemplateParameters(type->next, parameters,
                                                   result);
          StringAppendChar(result, ')');
        } else {
          StringAppend(result, " ");
          TypeRecordToStringWithTemplateParameters(type->next, parameters,
                                                   result);
        }
      }
      break;

    case kDeclReference:
    case kDeclRValueReference:
      TypeRecordToStringWithTemplateParameters(type->next, parameters, result);
      StringAppend(result,
                   type->declarator == kDeclRValueReference ? "&&" : "&");
      break;

    case kDeclArray:
      TypeRecordToStringWithTemplateParameters(type->next, parameters, result);
      StringPrintf(result, "[%d]", type->info.array.size.fixed);
      break;

    case kDeclFunction: {
      TypeRecordToStringWithTemplateParameters(type->next, parameters, result);
      StringAppendChar(result, '(');
      const char* sep = "";
      size_t nformals = type->info.function.prototype.length;
      for (size_t i = 0; i < nformals; i++) {
        Symbol* formal = (Symbol*)type->info.function.prototype.value.p[i];
        StringAppend(result, sep);
        TypeRecordToStringWithTemplateParameters(formal->type, parameters,
                                                 result);
        sep = ",";
      }
      StringAppendChar(result, ')');
      break;
    }
  }
}

// Convert a type record to a string in C syntax.  Not verbose.
void TypeRecordToString(TypeRecord* type, String* result) {
  TypeRecordToStringWithTemplateParameters(type, NULL, result);
}

static bool FunctionFormalIsImplicitThis(TypeRecord* func, size_t index) {
  if (func == NULL || index >= func->info.function.prototype.length) {
    return false;
  }
  Symbol* formal = func->info.function.prototype.value.p[index];
  return formal != NULL && StringEqual(&formal->name, "this");
}

static void TrimTrailingSpaces(String* out) {
  while (out->length > 0 && out->value[out->length - 1] == ' ') {
    out->value[--out->length] = '\0';
  }
}

static void AppendFunctionDisplayName(TypeRecord* func, String* out) {
  Symbol* symbol = func != NULL ? func->info.function.symbol : NULL;
  if (symbol == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  Namespace* ns = symbol->namespace_;
  if (ns == NULL && owner != NULL && owner->tag_symbol != NULL) {
    ns = owner->tag_symbol->namespace_;
  }
  if (ns != NULL && ns != compiler->global_namespace &&
      ns->qualified_name.length != 0) {
    StringAppendString(out, &ns->qualified_name);
    StringAppend(out, "::");
  }
  if (owner != NULL && owner->tag_name != NULL) {
    StringAppendString(out, owner->tag_name);
    StringAppend(out, "::");
  }
  if (func->info.function.is_constructor && owner != NULL &&
      owner->tag_name != NULL) {
    StringAppendString(out, owner->tag_name);
  } else if (func->info.function.is_destructor && owner != NULL &&
             owner->tag_name != NULL) {
    StringAppendChar(out, '~');
    StringAppendString(out, owner->tag_name);
  } else {
    StringAppendString(out, &symbol->name);
  }
}

static void AppendFunctionParameterList(TypeRecord* func, String* out) {
  StringAppendChar(out, '(');
  const char* sep = "";
  bool wrote_parameter = false;
  size_t start = FunctionFormalIsImplicitThis(func, 0) ? 1 : 0;
  for (size_t i = start; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal == NULL) {
      continue;
    }
    StringAppend(out, sep);
    TypeRecordToStringWithTemplateParameters(
        formal->type, &func->info.function.template_parameters, out);
    sep = ", ";
    wrote_parameter = true;
  }
  if (!wrote_parameter) {
    StringAppend(out, "void");
  }
  StringAppendChar(out, ')');
}

void TypeRecordFunctionPrettyName(TypeRecord* func, String* result) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.symbol == NULL) {
    return;
  }
  if (!func->info.function.is_constructor &&
      !func->info.function.is_destructor) {
    TypeRecordToStringWithTemplateParameters(
        func->next, &func->info.function.template_parameters, result);
    TrimTrailingSpaces(result);
    StringAppendChar(result, ' ');
  }
  AppendFunctionDisplayName(func, result);
  AppendFunctionParameterList(func, result);
  if (func->info.function.is_const_member) {
    StringAppend(result, " const");
  }
  if (func->info.function.is_volatile_member) {
    StringAppend(result, " volatile");
  }
  if (func->info.function.ref_qualifier == kCXXRefQualifierLValue) {
    StringAppend(result, " &");
  } else if (func->info.function.ref_qualifier == kCXXRefQualifierRValue) {
    StringAppend(result, " &&");
  }
}

void SymbolFunctionPrettyName(Symbol* symbol, String* result) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    if (symbol != NULL) {
      StringAppendString(result, &symbol->name);
    }
    return;
  }
  TypeRecordFunctionPrettyName(symbol->type, result);
}

void SymbolFunctionDiagnosticSuffix(Symbol* symbol, String* result) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return;
  }
  String pretty;
  StringInit(&pretty, NULL);
  SymbolFunctionPrettyName(symbol, &pretty);
  if (pretty.length != 0 && !StringEqualString(&pretty, &symbol->name)) {
    StringAppend(result, " (");
    StringAppendString(result, &pretty);
    StringAppendChar(result, ')');
  }
  StringDestruct(&pretty);
}

void SymbolFunctionDiagnosticName(Symbol* symbol, String* result) {
  if (symbol == NULL) {
    StringAppend(result, "<unknown>");
    return;
  }
  StringAppendString(result, &symbol->name);
  SymbolFunctionDiagnosticSuffix(symbol, result);
}

void TypeRecordToTemplateKeyString(TypeRecord* type, String* result) {
  if (type == NULL) {
    StringAppend(result, "<invalid-type>");
    return;
  }
  switch (type->declarator) {
    case kDeclPrimitive:
      if (TypeIsUnknown(type) && type->template_parameter_index >= 0) {
        QualifiersToString(type->qualifiers, result);
        if (type->qualifiers != 0) {
          StringAppend(result, " ");
        }
        StringPrintf(result, "$T%d", type->template_parameter_index);
        if (type->dependent_member_name != NULL) {
          StringAppend(result, "::");
          StringAppendString(result, type->dependent_member_name);
        }
        break;
      }
      QualifiersToString(type->qualifiers, result);
      if (type->qualifiers != 0) {
        StringAppend(result, " ");
      }
      TypeToString(type->type, result);
      if (TypeIsStructOrUnion(type)) {
        Struct* str = type->info.struct_info;
        if (str != NULL && str->tag_name != NULL) {
          StringAppend(result, " ");
          if (str->tag_symbol != NULL && str->tag_symbol->namespace_ != NULL &&
              str->tag_symbol->namespace_ != compiler->global_namespace) {
            StringAppendString(result,
                               &str->tag_symbol->namespace_->qualified_name);
            StringAppend(result, "::");
          }
          if (str->tag_name->value[0] != '<') {
            StringAppendString(result, str->tag_name);
          }
          StringPrintf(result, "$S%p", (void*)str);
        }
      } else if (TypeIsEnum(type)) {
        Enum* e = type->info.enum_info;
        if (e != NULL && e->tag_name != NULL) {
          StringAppendString(result, e->tag_name);
        }
      }
      break;

    case kDeclPointer:
      TypeRecordToTemplateKeyString(type->next, result);
      StringAppendChar(result, '*');
      if (type->qualifiers != 0) {
        StringAppend(result, " ");
      }
      QualifiersToString(type->qualifiers, result);
      break;

    case kDeclMemberPointer: {
      StringAppend(result, "$M");
      Struct* str = type->info.struct_info;
      if (type->template_parameter_index >= 0) {
        StringPrintf(result, "$T%d", type->template_parameter_index);
      } else if (str != NULL) {
        if (str->tag_symbol != NULL && str->tag_symbol->namespace_ != NULL &&
            str->tag_symbol->namespace_ != compiler->global_namespace) {
          StringAppendString(result,
                             &str->tag_symbol->namespace_->qualified_name);
          StringAppend(result, "::");
        }
        if (str->tag_name != NULL) {
          StringAppendString(result, str->tag_name);
        }
        StringPrintf(result, "$S%p", (void*)str);
      } else {
        StringAppend(result, "<class>");
      }
      StringAppendChar(result, '{');
      TypeRecordToTemplateKeyString(type->next, result);
      StringAppendChar(result, '}');
      QualifiersToString(type->qualifiers, result);
      break;
    }

    case kDeclReference:
    case kDeclRValueReference:
      TypeRecordToTemplateKeyString(type->next, result);
      StringAppend(result,
                   type->declarator == kDeclRValueReference ? "&&" : "&");
      break;

    case kDeclArray:
      TypeRecordToTemplateKeyString(type->next, result);
      StringPrintf(result, "[%d]", type->info.array.size.fixed);
      break;

    case kDeclFunction:
      TypeRecordToString(type, result);
      if (type->info.function.is_const_member) {
        StringAppend(result, "$const");
      }
      if (type->info.function.is_volatile_member) {
        StringAppend(result, "$volatile");
      }
      if (type->info.function.ref_qualifier == kCXXRefQualifierLValue) {
        StringAppend(result, "$ref");
      } else if (type->info.function.ref_qualifier ==
                 kCXXRefQualifierRValue) {
        StringAppend(result, "$rref");
      }
      if (type->info.function.is_noexcept) {
        StringAppend(result, "$noexcept");
      }
      break;
  }
}

//
// The type parser: a recursive descent parser for the C language's complex
// type system.
//
