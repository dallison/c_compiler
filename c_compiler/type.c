//
//  type.c
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "type.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include "dstring.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "symbol_table.h"
#include "syntax.h"
#include "compiler.h"
#include "errors.h"
#include "debug.h"

static int next_type_id = 0;

static struct {
  Type type;
  int size;
} fixed_type_sizes[] = {
    {kTypeChar, 1},   {kTypeShort, 2},
    {kTypeLongLong, 8}, {kTypeFloat, 4}, {kTypeDouble, 8}, {kTypeLongDouble, 8},
    {kTypeVoid, 0},     {kTypeBool, 1},  {kTypeEnum, 4},   {kTypeImplicit, 0},
};

static int FixedSize(Type type) {
  for (int i = 0; fixed_type_sizes[i].type != kTypeImplicit; i++) {
    if ((type & fixed_type_sizes[i].type) != 0) {
       return fixed_type_sizes[i].size;
    }
  }
  return -1;
}


// The size of a pointer depends on the machine architecture.
int SizeofPointer(void) {
  return compiler->pointer_size;
}

// The size of a int depends on the machine architecture.
int SizeofInt(void) {
  return compiler->int_size;
}

// The size of a int depends on the machine architecture.
int SizeofLong(void) {
  return compiler->long_size;
}

// Mapping of type to its size in bytes.
static struct {
  Type type;
  int (*func)(void);
} type_sizes[] = {
    {kTypeInt, SizeofInt},    {kTypeLong, SizeofLong},
    {kTypeImplicit, 0},
};


// What is the size in bytes of the given type?  Uses the mapping
// above.  Returns the size or zero if the type isn't known.
int SizeofType(Type type) {
  int size = FixedSize(type);
  if (size >= 0) {
    return size;
  }
  for (int i = 0; type_sizes[i].type != kTypeImplicit; i++) {
    if ((type & type_sizes[i].type) != 0) {
       return type_sizes[i].func();
    }
  }
  return SizeofPointer();
}



TypeRecord* NewTypeRecord(Type type, Qualifiers quals) {
  TypeRecord* record = malloc(sizeof(TypeRecord));
  record->id = next_type_id++;
  record->type = type;
  record->qualifiers = quals;
  record->size = 0;
  record->refs = 0;
  record->next = NULL;
  record->declarator = kDeclPrimitive;
  memset(&record->info, 0, sizeof(record->info));
  return record;
}

// Deletes a TypeRecord with regard to the reference count.  The
// reference count is decremented and if it goes to zero the
// record can be deleted.  When deleting it, the record pointed
// to by the 'next' field is first deleted (using the same function)
// and then the memory is freed.
void TypeRecordDelete(TypeRecord* record) {
  if (record == NULL) {
    return;
  }
  TypeRecordDecRef(record);
  if (record->refs == 0) {
    if (record->next != NULL) {
      TypeRecordDelete(record->next);
      record->next = NULL;
    }
    // Delete type-specific info if refs goes to zero.
    if (TypeIsStructOrUnion(record)) {
      if (--record->info.struct_info->refs == 0) {
        StructDelete(record->info.struct_info);
      }
    } else if (TypeIsEnum(record)) {
      if (--record->info.enum_info->refs == 0) {
        EnumDelete(record->info.enum_info);
      }
    } else if (TypeIsFunction(record)) {
      VectorDestructWithContents(&record->info.function.prototype,
                                (VectorElementDestructor)SymbolDestruct);
    } else if (TypeIsVLA(record)) {
      // Delete the AST containing the size.
      ASTNodeDelete(record->info.array.size.vla.size);
    }
    free(record);
  }
}

int TypeRecordAlignment(TypeRecord* record) {
  switch (record->declarator) {
    case kDeclArray:
      return TypeRecordAlignment(record->next);
    case kDeclPointer:
      return SizeofPointer();
    case kDeclFunction:
      return SizeofPointer();
      break;
    case kDeclPrimitive:
      if (TypeIsStructOrUnion(record)) {
        return SizeofPointer();
      }
      return SizeofType(record->type);
  }
}

void TypeRecordIncRef(TypeRecord* record) {
  if (record == NULL) {
    return;
  }
  record->refs++;
}

void TypeRecordDecRef(TypeRecord* record) {
  if (record == NULL) {
    return;
  }
  record->refs--;
}

TypeRecord* TypeRecordCalculateSize(TypeRecord* record) {
  if (record == NULL) {
    return NULL;
  }
  TypeRecordCalculateSize(record->next);
  if (record->size == 0) {
    switch (record->declarator) {
      case kDeclArray:
        if (!record->info.array.is_vla) {
          record->size = record->info.array.size.fixed * record->next->size;
        }
        break;
      case kDeclPointer:
        record->size = SizeofPointer();
        break;
      case kDeclFunction:
        record->size = SizeofPointer();
        break;
      case kDeclPrimitive:
        if (TypeIsStructOrUnion(record)) {
          assert(record->info.struct_info != NULL);
          record->size = record->info.struct_info->size;
        } else {
          record->size = SizeofType(record->type);
        }
        break;
    }
  }
  return record;
}

TypeRecord* NewTypeRecordWithSize(Type type, Qualifiers quals) {
  return TypeRecordCalculateSize(NewTypeRecord(type, quals));
}

// Join two type records through the next field.  This increments
// the reference count on the one pointed to.
void TypeRecordChain(TypeRecord* from, TypeRecord* to) {
  TypeRecordIncRef(to);
  if (to == NULL) {
    printf("");
  }
  from->next = to;
}

static ASTNode* CloneVLAExpr(ASTNode* node, void* data) {
  return node;
}

// Copy a type record and chain it to its existing next,
// incrementing the ref count.
TypeRecord* TypeRecordCopy(TypeRecord* record) {
  TypeRecord* r = malloc(sizeof(TypeRecord));
  memcpy(r, record, sizeof(TypeRecord));
  r->id = next_type_id;
  r->refs = 0;  // No refs to this yet.
  if (r->next != NULL) {
    TypeRecordIncRef(r->next);  // Another ref to next.
  }
  // Increment ref counts for type-specific objects.
  if (TypeIsStructOrUnion(record)) {
    record->info.struct_info->refs++;
  } else if (TypeIsEnum(record)) {
    record->info.enum_info->refs++;
  } else if (TypeIsVLA(record)) {
    // Copy the expression AST.
    r->info.array.size.vla.size =
      ASTNodeClone(record->info.array.size.vla.size, CloneVLAExpr, NULL, NULL);
  }
  return r;
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

TypeRecord* NewPointerTo(Qualifiers quals, TypeRecord* type) {
  TypeRecord* ptr = NewPointerTypeRecord(quals);
  TypeRecordChain(ptr, type);
  return ptr;
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
  t->size = 0;  // Don't know yet.
  return t;
}

TypeRecord* NewBasicArrayTypeRecord(Qualifiers quals, int size, bool is_flexible) {
  TypeRecord* t = NewArrayTypeRecord(quals, false);
  t->info.array.size.fixed = size;
  t->info.array.is_flexible = is_flexible;
  return t;
}

TypeRecord* NewFunctionTypeRecord() {
  TypeRecord* t = NewTypeRecord(kTypeImplicit, kQualPlain);
  t->declarator = kDeclFunction;
  t->size = 0;
  t->info.function.symbol = NULL;
  t->info.function.varargs = false;
  t->info.function.unknown_args = false;
  t->info.function.definition = false;
  t->info.function.is_constructor = false;
  t->info.function.is_destructor = false;
  t->info.function.old_style = false;
  t->info.function.is_inline = false;
  t->info.function.body = NULL;
  VectorInit(&t->info.function.prototype);
  return t;
}

TypeRecord* NewSizeTypeRecord() {
  if (compiler->pointer_size == 8) {
    return NewTypeRecord(kTypeLongLong | kTypeUnsigned, kQualPlain);
  }
  return NewTypeRecord(kTypeInt | kTypeUnsigned, kQualPlain);
}

StructMember* NewStructMember(Symbol* symbol) {
  StructMember* mem = malloc(sizeof(StructMember));
  mem->symbol = symbol;
  mem->byte_offset = 0;
  mem->bit_offset = 0;
  mem->bit_size = 0;
  return mem;
}

void StructMemberDelete(StructMember* member) {
  SymbolDelete(member->symbol);
  free(member);
}

bool StructMemberIsBitField(StructMember* member) {
  return member->bit_size > 0;
}

static int CompareStructMember(const void* a, const void* b) {
  const MapKeyValue* key1 = (const MapKeyValue*)a;
  const MapKeyValue* key2 = (const MapKeyValue*)b;
  return StringCompareString(key1->key.p, key2->key.p);
}

Struct* NewStruct(bool is_union) {
  Struct* s = malloc(sizeof(Struct));
  s->refs = 1;
  VectorInit(&s->members);
  MapInit(&s->symbol_table, CompareStructMember);
  s->is_union = is_union;
  s->next_offset = 0;
  s->current_offset = 0;
  s->size = 0;
  s->next_bit_pos = 65;
  return s;
}

void StructDelete(Struct* s) {
  VectorDestructWithContents(&s->members,
                             (VectorElementDestructor)StructMemberDelete);
  MapDestruct(&s->symbol_table);
  free(s);
}

Symbol* NewEnumConstant(const char* name, int value) {
  TypeRecord* int_type = NewTypeRecord(kTypeInt, kQualConst);
  Symbol* c = NewSymbol(name, int_type, STO(implicit));
  c->value.ivalue = value;
  return c;
}

Enum* NewEnum() {
  Enum* e = malloc(sizeof(Enum));
  e->refs = 1;
  VectorInit(&e->constants);
  e->next_value = 0;
  return e;
}

void EnumDelete(Enum* e) {
  VectorDestruct(&e->constants);
  free(e);
}

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
    {kTypeEnum, "enum"},          {kTypeImplicit, ""},
};

// Converts a type to a string and appends it to result.
static void TypeToString(Type type, String* result) {
  if ((type & kTypeSigned) != 0) {
    StringAppend(result, "signed ");
  }
  if ((type & kTypeUnsigned) != 0) {
    StringAppend(result, "unsigned ");
  }
  for (int i = 0; type_names[i].type != kTypeImplicit; i++) {
    if ((type & type_names[i].type) != 0) {
      StringAppend(result, type_names[i].name);
      StringAppend(result, " ");
    }
  }
}

// Converts qualifiers to string and appends them to result.
static void QualifiersToString(Qualifiers quals, String* result) {
  if ((quals & kQualConst) != 0) {
    StringAppend(result, "const ");
  }
  if ((quals & kQualVolatile) != 0) {
    StringAppend(result, "volatile ");
  }
  if ((quals & kQualRestrict) != 0) {
    StringAppend(result, "restrict ");
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
    if (record->declarator == kDeclPointer) {
      fprintf(fp, "pointer to ");
    }
    QualifiersToString(record->qualifiers, &str);
    TypeToString(record->type, &str);
    fprintf(fp, "%s", str.value);

    if (record->declarator == kDeclArray) {
      if (record->info.array.is_vla) {
        fprintf(fp, "variable length array ");
      } else {
        fprintf(fp, "array of size %d ", record->info.array.size.fixed);
      }
    } else if (record->declarator == kDeclFunction) {
      fprintf(fp, "function (");
      const char* sep = "";
      size_t nformals = record->info.function.prototype.length;
      for (size_t i = 0; i < nformals; i++) {
        Symbol* formal = (Symbol*)record->info.function.prototype.value.p[i];
        fprintf(fp, "%s", sep);
        sep = ",";
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
          ASTNodePrint(stmt, 2, fp);
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

// Convert a type record to a string in C syntax.  Not verbose.
void TypeRecordToString(TypeRecord* type, String* result) {
  switch (type->declarator) {
    case kDeclPrimitive:
      QualifiersToString(type->qualifiers, result);
      TypeToString(type->type, result);
      if (TypeIsStructOrUnion(type)) {
        StringAppendString(result, type->info.struct_info->tag_name);
      } else if (TypeIsEnum(type)) {
        StringAppendString(result, type->info.enum_info->tag_name);
      }
      break;

    case kDeclPointer:
      TypeRecordToString(type->next, result);
      if (type->next->declarator == kDeclArray ||
          type->next->declarator == kDeclFunction) {
        StringAppend(result, "(*");
        QualifiersToString(type->qualifiers, result);
        StringAppendChar(result, ')');
      } else {
        StringAppendChar(result, '*');
        QualifiersToString(type->qualifiers, result);
      }
      break;

    case kDeclArray:
      TypeRecordToString(type->next, result);
      StringPrintf(result, "[%d]", type->info.array.size.fixed);
      break;

    case kDeclFunction: {
      TypeRecordToString(type->next, result);
      StringAppendChar(result, '(');
      const char* sep = "";
      size_t nformals = type->info.function.prototype.length;
      for (size_t i = 0; i < nformals; i++) {
        Symbol* formal = (Symbol*)type->info.function.prototype.value.p[i];
        StringAppend(result, sep);
        TypeRecordToString(formal->type, result);
        sep = ",";
      }
      StringAppendChar(result, ')');
      break;
    }
  }
}

//
// The type parser: a recursive descent parser for the C language's complex
// type system.
//
void TypeParserInit(TypeParser* parser, Lex* lex, struct Syntax* syntax,
                    Storage storage, ParserContext context) {
  parser->lex = lex;
  parser->syntax = syntax;
  parser->storage = storage;
  VectorInit(&parser->stack);
  parser->symbol = NULL;
  parser->found_void = false;
  parser->dimension_count = 0;
  parser->is_inline = false;
  parser->context = context;
}

void TypeParserReset(TypeParser* parser) {
  parser->symbol = NULL;
  parser->storage = STO(implicit);
  parser->found_void = false;
  parser->dimension_count = 0;
  VectorDestruct(&parser->stack);
  VectorInit(&parser->stack);
}

// Mapping for token vs type for parsing a type specifier.
static struct {
  Token token;
  Type type;
} type_map[] = {
    {TOK(char), kTypeChar},         {TOK(int), kTypeInt},
    {TOK(short), kTypeShort},       {TOK(long), kTypeLong},
    {TOK(float), kTypeFloat},       {TOK(double), kTypeDouble},
    {TOK(struct), kTypeStruct},     {TOK(union), kTypeUnion},
    {TOK(enum), kTypeEnum},         {TOK(void), kTypeVoid},
    {TOK(bool), kTypeBool},         {TOK(signed), kTypeSigned},
    {TOK(unsigned), kTypeUnsigned}, {TOK(bad), kTypeImplicit},
};



// Parse a type-specifier.  This might also be a typedef reference which
// contains a full TypeRecord.
static PartialTypeSpecifier ParseTypeSpecifier(TypeParser* parser) {
  PartialTypeSpecifier result;
  result.error = false;
  
  Type type = kTypeImplicit;
  Qualifiers quals = kQualPlain;
  Lex* lex = parser->lex;
  TypeRecord* type_record = NULL;

  Token tok = lex->current_token;
  bool found = false;
  
  if (parser->found_void) {
    // Special handling for already-consumed void type.  This can
    // happen inside a function prototype.
    type |= kTypeVoid;
    parser->found_void = false;
    found = true;
  } else {
    // Check for known type.
    for (int i = 0; type_map[i].token != TOK(bad); i++) {
      if (type_map[i].token == tok) {
        // First check for a typedef name reference.
        LexNextToken(lex);
        
        type |= type_map[i].type;
        found = true;
        break;
      }
    }
  }

  // If we didn't find a known type, look for qualifiers and typedef
  // name.
  if (!found) {
    if (LexMatch(lex, TOK(const))) {
      quals |= kQualConst;
    } else if (LexMatch(lex, TOK(volatile))) {
      quals |= kQualVolatile;
    } else if (LexMatch(lex, TOK(restrict))) {
      quals |= kQualRestrict;
    } else if (tok == TOK(identifier)) {
      // Identifier.  If this is a known typedef name consume it
      // and keep the type.
      String typedef_name;
      StringInit(&typedef_name, lex->spelling.value);
      Symbol* symbol = SyntaxFindSymbol(parser->syntax, &typedef_name);
      if (symbol != NULL) {
        // Reference to a typedef?
        if (StorageIs(symbol->storage, STO(typedef))) {
          LexNextToken(lex);
          type_record = TypeRecordCopy(symbol->type);
          type |= type_record->type;
        }
      }
      StringDestruct(&typedef_name);
    }
  }

  // Check for struct, union or enum and parse it if necessary.
  if (type_record == NULL &&
      (type & (kTypeStruct | kTypeUnion | kTypeEnum)) != 0) {
    
    Symbol* tag;
    TypeParser composite_parser;
    TypeParserInit(&composite_parser, parser->lex, parser->syntax,
                   STO(implicit), kParsingStructOrUnion);
    
    if ((type & (kTypeStruct | kTypeUnion)) != 0) {
      bool is_union = (type & kTypeUnion) != 0;
      tag = TypeParserParseStruct(&composite_parser, is_union);
    } else {
      tag = TypeParserParseEnum(&composite_parser);
    }
    if (tag != NULL) {
      // We need to copy the type record because it is held in the
      // struct tag and we need to apply our qualifiers to it for this
      // type definition.  For example, this might be:
      //   const struct Foo;
      // and the type record will be the one inside the symbol for 'Foo'
      type_record = TypeRecordCopy(tag->type);
      type |= type_record->type;
    }
  }

  result.type = type;
  result.quals = quals;
  result.type_record = type_record;
  return result;
}

// This is a list of all the valid type combinations.
// These come from the C99 spec, section 6.7.2.
static Type valid_types[] = {
  kTypeVoid,
  
  kTypeChar,
  kTypeChar | kTypeSigned,
  kTypeChar | kTypeUnsigned,
  
  kTypeShort,
  kTypeShort | kTypeSigned,
  kTypeShort | kTypeInt,
  kTypeShort | kTypeSigned | kTypeInt,
  kTypeShort | kTypeUnsigned,
  kTypeShort | kTypeUnsigned | kTypeInt,
  
  kTypeInt,
  kTypeInt | kTypeSigned,
  kTypeSigned,
  kTypeUnsigned,
  kTypeUnsigned | kTypeInt,
  
  kTypeLong,
  kTypeLong | kTypeInt,
  kTypeLong | kTypeSigned,
  kTypeLong | kTypeSigned | kTypeInt,
  kTypeLong | kTypeUnsigned,
  kTypeLong | kTypeUnsigned | kTypeInt,
  
  kTypeLongLong,
  kTypeLongLong | kTypeSigned,
  kTypeLongLong | kTypeInt,
  kTypeLongLong | kTypeSigned | kTypeInt,
  kTypeLongLong | kTypeUnsigned,
  kTypeLongLong | kTypeUnsigned | kTypeInt,
  
  kTypeFloat,
  
  kTypeDouble,
  kTypeDouble | kTypeLong,
  
  kTypeBool,
  
  kTypeStruct,
  
  kTypeUnion,
  
  kTypeEnum,
};

#define NUM_VALID_TYPES (sizeof(valid_types)/sizeof(valid_types[0]))

static bool IsValidType(Type t) {
  for (size_t i = 0; i < NUM_VALID_TYPES; i++) {
    if (t == valid_types[i]) {
      return true;
    }
  }
  return false;
}

static bool IsValidQualiferCombo(Qualifiers q1, Qualifiers q2) {
  return (q1 & q2) == 0;
}

static void TypeComboError1(Syntax* syntax, Type t1, Type t2) {
  String error;
  StringInit(&error, "");
  TypeToString(t1, &error);
  if (t2 != kTypeImplicit) {
    StringAppend(&error, "and ");
    TypeToString(t2, &error);
  }
  SyntaxError(syntax, "Invalid type combination; can't combine %s",
              error.value);
  StringDestruct(&error);
}

static void TypeComboError2(Syntax* syntax, TypeRecord* t1, Type t2) {
  String error;
  StringInit(&error, "defined type ");
  TypeRecordToString(t1, &error);
  StringAppend(&error, "and ");
  TypeToString(t2, &error);
  SyntaxError(syntax, "Invalid type combination; can't combine %s",
              error.value);
  StringDestruct(&error);
}

static void TypeComboError3(Syntax* syntax, TypeRecord* t1, TypeRecord* t2) {
  String error;
  StringInit(&error, "defined type ");
  TypeRecordToString(t1, &error);
  StringAppend(&error, "and defined type ");
  TypeRecordToString(t2, &error);
  SyntaxError(syntax, "Invalid type combination; can't combine %s",
              error.value);
  StringDestruct(&error);
}

static void QualifierComboError(Syntax* syntax, Qualifiers q1, Qualifiers q2) {
  String error;
  StringInit(&error, "");
  QualifiersToString(q1, &error);
  StringAppend(&error, "and ");
  QualifiersToString(q2, &error);
  SyntaxError(syntax, "Invalid type combination; can't combine %s",
              error.value);
  StringDestruct(&error);
}

// Type specifiers can be split into pieces.  For example you could
// have:
//
// int extern unsigned foo;
//
// Where the 'int' and 'unsigned' are split by a storage specifier.
// This function combines two type specifiers if it can and issues
// errors and warnings as necessary.
static PartialTypeSpecifier CombineTypeSpecifiers(Syntax* syntax,
                                                PartialTypeSpecifier* t1,
                                                PartialTypeSpecifier* t2) {
  PartialTypeSpecifier result = {0};
  // Check for a valid type.  You can't combine types that contain the
  // same bits:
  // e.g. short short
  // However, we need to handle 'long long'.
  if ((t1->type & kTypeLong) != 0 && (t2->type & kTypeLong) != 0) {
    t1->type &= ~kTypeLong;
    t1->type |= kTypeLongLong;
    t2->type = kTypeImplicit;
  }
  result.type = t1->type | t2->type;
  
  bool type_ok = result.type == kTypeImplicit ||
              (t1->type & t2->type) == 0;
  if (type_ok) {
    type_ok = IsValidType(result.type);
  }
  if (!type_ok) {
    TypeComboError1(syntax, t1->type, t2->type);
    result.error = true;
  }
  
  // Convert 'long double' to kTypeLongDouble.
  if ((result.type & (kTypeLong | kTypeDouble)) == (kTypeLong | kTypeDouble)) {
    result.type &= ~(kTypeLong | kTypeDouble);
    result.type |= kTypeLongDouble;
  }
  
  // Can't combine qualifiers if they are the same.
  // e.g. const const
  if (!IsValidQualiferCombo(t1->quals, t2->quals)) {
    QualifierComboError(syntax, t1->quals, t2->quals);
    result.error = true;
  }
  result.quals = t1->quals | t2->quals;
  result.type_record = NULL;

  if (result.error) {
    return result;
  }
  if (t1->type_record != NULL || t2->type_record != NULL) {
    // Either t1->typedef_record or t2->typedef_record is non-NULL.
    // Put the non-NULL one in t1 to avoid code duplication.
    if (t1->type_record == NULL) {
      PartialTypeSpecifier* tmp = t1;
      t1 = t2;
      t2 = tmp;
    }
    if (t2->type_record != NULL) {
      TypeComboError3(syntax, t1->type_record, t2->type_record);
      result.error = true;
    } else if (t2->type != kTypeImplicit) {
      TypeComboError2(syntax, t1->type_record, t2->type);
      result.error = true;
    }
    result.type = t1->type;
    if (!IsValidQualiferCombo(t1->quals, t2->quals)) {
      QualifierComboError(syntax, t1->quals, t2->quals);
    }
    result.quals = t1->quals | t2->quals;
    result.type_record = t1->type_record;
  }
  return result;
}

PartialTypeSpecifier TypeParserParseAndCombineTypes(TypeParser* parser,
                                                   PartialTypeSpecifier* prev) {
  PartialTypeSpecifier curr = ParseTypeSpecifier(parser);
  if (prev->type == kTypeImplicit && prev->quals == kQualPlain) {
    return curr;
  }
  
  return CombineTypeSpecifiers(parser->syntax, prev, &curr);
}

// Given a ParseTypeSpecifier, build a TypeRecord.
TypeRecord* TypeParserBuildTypeRecord(TypeParser* parser, PartialTypeSpecifier* type) {
  if (type->error) {
    return NewTypeRecord(kTypeInt, kQualPlain);
  }
  if (type->type_record == NULL) {
    if (type->type == kTypeImplicit) {
      return NULL;
    }
    return NewTypeRecord(type->type, type->quals);
  } else {
    // Add qualifiers to typedef copy.
    type->type_record->qualifiers |= type->quals;
    return type->type_record;
  }
}

TypeRecord* TypeParserParseType(TypeParser* parser, bool needed) {
  Syntax* syntax = parser->syntax;
  
  PartialTypeSpecifier type_specifier = {
    .type = kTypeImplicit,
    .quals = kQualPlain,
    .type_record = NULL,
    .error = false };
  
  while (parser->found_void || SyntaxLookingAtType(syntax)) {
    PartialTypeSpecifier new_type_specifier = ParseTypeSpecifier(parser);
    if (type_specifier.type == kTypeImplicit && type_specifier.quals == kQualPlain) {
      type_specifier = new_type_specifier;
    } else {
      type_specifier = CombineTypeSpecifiers(parser->syntax, &type_specifier, &new_type_specifier);
    }
  }
  // No type?
  if (type_specifier.type == kTypeImplicit) {
    if (needed) {
      SyntaxError(parser->syntax, "Type expected");
      SyntaxRecover(parser->syntax, TC(semicolon) | TC(type));
      return NewTypeRecord(kTypeInt, kQualPlain);
    }
    return NULL;
  }
  return TypeParserBuildTypeRecord(parser, &type_specifier);
}

Symbol* TypeParserParseDeclarator(TypeParser* parser, TypeRecord* base_type) {
  if (base_type == NULL) {
    return NULL;
  }
  
  VectorClear(&parser->stack);
  parser->symbol = NULL;
  parser->base_type = base_type;
  TypeParserParsePointer(parser);

  // Join all the type records together in reverse order.
  size_t i = parser->stack.length;
  TypeRecord* t = parser->base_type;
  while (i > 0) {
    TypeRecord* record = (TypeRecord*)parser->stack.value.p[i - 1];
    TypeRecordChain(record, t);
    record->type = t->type;
    t = record;
    i--;
  }

  // Calculate the size of t, now that we have the complete chain.
  TypeRecordCalculateSize(t);

  if (parser->symbol != NULL) {
    SymbolSetType(parser->symbol, t);
  } else {
    // Invent a fake symbol.
    parser->symbol = NewSymbol(SyntaxFakeName(parser->syntax), t, STO(auto));
    parser->symbol->flags.invented = true;
  }
  return parser->symbol;
}

static Qualifiers ParseQualifiers(TypeParser* parser) {
  Qualifiers quals = kQualPlain;
  int num_consts = 0;
  int num_volatiles = 0;
  int num_restricts = 0;
  while (!LexEof(parser->lex)) {
    if (LexMatch(parser->lex, TOK(const))) {
      quals |= kQualConst;
      num_consts++;
    } else if (LexMatch(parser->lex, TOK(volatile))) {
      num_volatiles++;
      quals |= kQualVolatile;
    } else if (LexMatch(parser->lex, TOK(restrict))) {
      num_restricts++;
      quals |= kQualRestrict;
    } else {
      break;
    }
  }
  if (num_consts > 1 || num_volatiles > 1 || num_restricts > 1) {
    SyntaxError(parser->syntax, "Invalid pointer qualifier declaration");
  }
  return quals;
}

void TypeParserParsePointer(TypeParser* parser) {
  if (LexMatch(parser->lex, TOK(star))) {
    Qualifiers quals = ParseQualifiers(parser);
    TypeParserParsePointer(parser);
    TypeRecord* p = NewPointerTypeRecord(quals);
    VectorAppend(&parser->stack, p);
  } else {
    TypeParserParseFuncOrArray(parser);
  }
}

// Check that a formal argument name is not already in the list
// of formals.  Returns true if name is OK.
static bool CheckFormalName(Vector* formals, String* name) {
  if (name->length == 0) {
    // Empty name is OK.
    return true;
  }
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* formal = (Symbol*)formals->value.p[i];
    if (StringEqualString(&formal->name, name)) {
      return false;
    }
  }
  return true;
}

// Parse a formal argument declaration.  Takes ownership of
// formal.
static void ParseFormalArgument(TypeParser* proto_parser,
                                TypeRecord* func,
                                Symbol* formal,
                                int arg_number) {
  if (CheckFormalName(&func->info.function.prototype, &formal->name)) {
    // Function arguments are pointer to functions.
    if (TypeIsFunction(formal->type)) {
      TypeRecord* func_ptr = NewPointerTypeRecord(kQualPlain);
      TypeRecordChain(func_ptr, formal->type);
      SymbolSetType(formal, func_ptr);
    } else if (TypeIsArray(formal->type)) {
      // For an array, convert the type record to a pointer.
      TypeRecord* ptr = TypeRecordCopy(formal->type);
      ptr->declarator = kDeclPointer;
      SymbolSetType(formal, ptr);
    }
    VectorAppend(&func->info.function.prototype, formal);
    formal->flags.is_defined = true;
    formal->flags.is_argument = true;
    formal->value.arg_number = arg_number;
    if (proto_parser->syntax->local_symbol_stack != NULL) {
      InsertLocalSymbol(proto_parser->syntax->local_symbol_stack,
                      formal);
    }

  } else {
    SyntaxError(proto_parser->syntax, "Duplicate function argument '%s'",
                formal->name.value);
    SymbolDelete(formal);
  }
}


// Prototype style.  C still allows old-style K&R code.
typedef enum  {
  kStyleUnknown,
  kStyleOld,
  kStyleNew
} PrototypeStyle;

static PrototypeStyle ParseFunctionParameter(TypeParser* proto_parser, TypeRecord* func,
                                             PrototypeStyle style,
                                             int arg_number) {
  if (proto_parser->found_void ||
        SyntaxLookingAtType(proto_parser->syntax)) {
    TypeRecord* type = TypeParserParseType(proto_parser, true);
    if (style == kStyleUnknown) {
      style = kStyleNew;
    }
    if (style == kStyleOld) {
      SyntaxError(proto_parser->syntax,
                  "Cannot mix function prototype with old-style function args");
    }

    Symbol* formal = TypeParserParseDeclarator(proto_parser, type);
    assert(formal != NULL);
    ParseFormalArgument(proto_parser, func, formal, arg_number);
  } else {
    // Possible old-style function decl, identifiers only.
    if (LexLookingAt(proto_parser->lex, TOK(identifier))) {
      if (style == kStyleUnknown) {
        style = kStyleOld;
      }
      if (style == kStyleNew) {
        SyntaxError(proto_parser->syntax, "Type expected for function arg");
        SyntaxRecover(proto_parser->syntax, TC(closebra));
      } else {
        TypeRecord* unknown = NewTypeRecord(kTypeInt, kQualPlain);
        Symbol* formal = NewSymbol(proto_parser->lex->spelling.value,
                                   unknown, STO(auto));
        LexNextToken(proto_parser->lex);
        ParseFormalArgument(proto_parser, func, formal, arg_number);
      }
    } else {
      SyntaxError(proto_parser->syntax,
                  "Expected type or identifier in function prototype");
      SyntaxRecover(proto_parser->syntax, TC(closebra));
    }
  }
  return style;
}

// Parse a function prototype, old or new style.
static void ParseFunctionPrototype(TypeParser* proto_parser, TypeRecord* func) {
  bool void_args = false;
  FunctionInfo* info = &func->info.function;
  PrototypeStyle style = kStyleUnknown;
  int arg_number = 0;
  
  info->old_style = false;

  while (!LexLookingAt(proto_parser->lex, TOK(rparen))) {
    if (LexMatch(proto_parser->lex, TOK(ellipsis))) {
      // ... must be the last argument in the prototype.
      info->varargs = true;
      if (!LexLookingAt(proto_parser->lex, TOK(rparen))) {
        SyntaxError(proto_parser->syntax,
                    "... must be at the end of a function prototype");
        SyntaxRecover(proto_parser->syntax, TC(closebra));
      }
      break;
    }
    // Check for (void).
    if (arg_number == 0 && LexMatch(proto_parser->lex, TOK(void))) {
      proto_parser->found_void = true;
      // Look for close paren; meaning (void).
      if (LexLookingAt(proto_parser->lex, TOK(rparen))) {
        // "void" means that there are no arguments.
        void_args = true;
        break;
      }
    }
    
    // The keyword 'register' is allowed here but we ignore it, except
    // to set the type as new style.
    if (LexMatch(proto_parser->lex, TOK(register))) {
      style = kStyleNew;
    }
    
    // Parse the formal argument's type, if it has one.
    // Otherwise it's a possible old-style function.
    style = ParseFunctionParameter(proto_parser, func, style, arg_number);
    arg_number++;
    if (!LexMatch(proto_parser->lex, TOK(comma))) {
      break;
    }
  }
  if (style == kStyleOld) {
    info->old_style = true;
  }

  // If we were not told (void) and there are no formal args then the C
  // language says that this is a variable arguments function.
  if (info->prototype.length == 0 && !void_args) {
    info->unknown_args = true;
  }
}

static void ParseFunctionDecl(TypeParser* parser) {
  TypeParser proto_parser;
  TypeParserInit(&proto_parser, parser->lex, parser->syntax, STO(auto), kParsingPrototype);
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_inline = parser->is_inline;
  if (parser->symbol != NULL) {
    func->info.function.symbol = parser->symbol;
  }
  
  ParseFunctionPrototype(&proto_parser, func);

  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));
  VectorAppend(&parser->stack, func);
}
 
static void ParseArrayDecl(TypeParser* parser) {
  bool is_static = false;
  Qualifiers quals = kQualPlain;
  // An array decl can have:
  // [static quals ...]
  // [quals static ...];
  if (LexMatch(parser->lex, TOK(static))) {
    is_static = true;
    quals = ParseQualifiers(parser);
  } else {
    quals = ParseQualifiers(parser);
    is_static = LexMatch(parser->lex, TOK(static));
  }

  // These are only allowed inside a function prototype.
  if (parser->context != kParsingPrototype) {
    if (is_static || quals != kQualPlain) {
      SyntaxError(parser->syntax, "static or qualifiers used in array declarator outside function prototype");
    }
  }
  
  parser->dimension_count++;
  TypeRecord* p = NewArrayTypeRecord(quals, is_static);
  VectorAppend(&parser->stack, p);

  bool is_vla = false;
  bool found_star = false;
  SourceLocation location = parser->lex->current_token_location;
  if (LexMatch(parser->lex, TOK(rsquare))) {
    if (parser->dimension_count != 1) {
      SyntaxError(parser->syntax,
             "Array dimension required after first dimension");
    }
    // No size expression present.
    if (parser->context != kParsingPrototype) {
      p->info.array.is_flexible = true;
    }
    return;
  }
  
  // There is something in the [...]
  if (LexMatch(parser->lex, TOK(star))) {
    // Unfortunately * can be a unary operator and part of an expression
    // so we need to look at the next token to see if it's a close square
    // bracket.  We've already consumed the *.
    found_star = true;
  }
  if (found_star && LexLookingAt(parser->lex, TOK(rsquare))) {
    if (parser->context != kParsingPrototype) {
      SyntaxError(parser->syntax, "VLA placeholder '*' is only valid in a function prototype");
    } else {
      is_vla = true;
      p->info.array.is_placeholder_vla = true;
    }
  } else {
    // Size expression is present.  If it's constant we have a
    // regular fixed size array, otherwise it's a VLA.
    ASTNode* size_expr =
        SyntaxParseSingleExpression(parser->syntax, TC(closebra));
    if (found_star) {
      // There was a * before the expression, this means contents.
      size_expr = NewUnaryASTNode(AST_OP(contents), NULL, location, size_expr);
    }
    size_expr = AnalyzeExpression(size_expr);
    bool delete_expr = true;
    int64_t size;
    bool ok = EvaluateIntegerExpression(size_expr, &size);
    if (!ok) {
      // VLA.
      if (!TypeIsIntegral(size_expr->type)) {
        SyntaxError(parser->syntax, "Variable length array size must be integral");
      }
      if (parser->context != kParsingBlockScope && parser->context != kParsingPrototype) {
        SyntaxError(parser->syntax, "Variable length array is only allowed inside a function");
      } else if (StorageIs(parser->storage, STO(extern)|STO(static))) {
        SyntaxError(parser->syntax, "Variable length array cannot be static or extern");
      } else {
        p->info.array.size.vla.size = size_expr;
        delete_expr = false;      // Hold on to expression.
        is_vla = true;
      }
    } else {
      if (size <= 0) {
        SyntaxError(parser->syntax, "Array with negative or zero size");
        size = 1;
      }
      p->info.array.size.fixed = (int)size;
    }
    if (delete_expr) {
      ASTNodeDelete(size_expr);
    }
  }
  p->info.array.is_vla = is_vla;
  
  if (!LexMatch(parser->lex, TOK(rsquare))) {
    LexError(parser->lex, "Missing ]");
  }
}

void TypeParserParseFuncOrArray(TypeParser* parser) {
  TypeParserParseBase(parser);
  while (LexLookingAt(parser->lex, TOK(lparen)) ||
         LexLookingAt(parser->lex, TOK(lsquare))) {
    // Check for function prototype declaration.
    if (LexMatch(parser->lex, TOK(lparen))) {
      ParseFunctionDecl(parser);
    } else if (LexMatch(parser->lex, TOK(lsquare))) {
      ParseArrayDecl(parser);
    }
  }
}


void TypeParserParseBase(TypeParser* parser) {
  if (LexMatch(parser->lex, TOK(lparen))) {
    TypeParserParsePointer(parser);
    if (!LexMatch(parser->lex, TOK(rparen))) {
      LexError(parser->lex, "Missing close parenthesis in declaration");
    }
  } else {
    if (LexLookingAt(parser->lex, TOK(identifier))) {
      SourceLocation location = parser->lex->current_token_location;
      String* name = &parser->lex->spelling;
      LexNextToken(parser->lex);
      parser->symbol =
          NewSymbol(name->value, parser->base_type, parser->storage);
      parser->symbol->location = location;
    }
  }
}

static void PrintStructMember(const MapKeyValue* kv) {
  String* name = kv->key.p;
  printf("%s", name->value);
}

StructMember* FindStructMember(Struct* str, String* name) {
//  MapPrint(&str->symbol_table, PrintStructMember);
//  printf("\n");
  return MapFindPointerKey(&str->symbol_table, name);
}

static bool CheckStructMember(Struct* str, String* name) {
  return FindStructMember(str, name) == NULL;
}

static void AlignNextOffset(Struct* str, TypeRecord* type) {
  int alignment = TypeRecordAlignment(type);
  str->next_offset = (str->next_offset + (alignment - 1)) & ~(alignment - 1);
  str->next_bit_pos = 65;
  str->current_offset = str->next_offset;
}

// Parse a bitfield.  We are just after the : in the member definition.
// We will parse a constant integer expression and calculate the bit position,
// byte position and bit width for the current member, moving on to the next
// word if the it doesn't fit.
static void ParseBitField(TypeParser* parser, bool is_union, Struct* str,
                          Symbol* member_symbol, StructMember* member) {
  char error[256];
  ASTNode* width_node =
      SyntaxParseSingleExpression(parser->syntax, TC(semicolon));
  if (width_node == NULL) {
    snprintf(error, sizeof(error), "constant expression needed");
    goto error;
  }
  int64_t bit_width = 0;
  if (!EvaluateIntegerExpression(width_node, &bit_width)) {
    ASTNodeDelete(width_node);
    snprintf(error, sizeof(error), "constant expression needed");
    goto error;
  }
  ASTNodeDelete(width_node);
  if (!TypeIsIntegral(member_symbol->type)) {
    snprintf(error, sizeof(error),
             "only integer types can be used for bitfields");
    goto error;
  }
  int word_width = member_symbol->type->size * 8;
  if (bit_width <= 0 || bit_width > word_width) {
    snprintf(error, sizeof(error),
             "width of %lld is out of bounds for type of size %d bite",
             bit_width, word_width);
    goto error;
  }
  int width = (int)bit_width;
  if (str->next_bit_pos + width > word_width) {
    // No room in current word for bit field (or first bit field).  We align
    // to the next boundary based on the bitfield type and add a new word
    // (of the appropriate type) to the struct.
    AlignNextOffset(
        str, member_symbol->type);  // Will set current_offset and next_offset.
    member->byte_offset = str->next_offset;
    member->index = str->members.length - 1;
    str->next_bit_pos = 0;
    if (!is_union) {
      str->next_offset += member_symbol->type->size;
      str->size = str->next_offset;
    } else {
      if (member_symbol->type->size > str->size) {
        str->size = member_symbol->type->size;
      }
    }
  } else {
    // There is room in the current word for the bitfield.
    member->byte_offset = str->current_offset;
  }
  member->bit_size = width;
  member->bit_offset = str->next_bit_pos;
  if (!is_union) {
    str->next_bit_pos += width;
  }
  return;

error:
  // If we get here we have an error.
  SyntaxError(parser->syntax, "Invalid bitfield; %s", error);
}

static void ParseStructMembers(TypeParser* parser, Struct* str, bool is_union) {
  while (!LexLookingAt(parser->lex, TOK(rbrace))) {
    TypeRecord* member_type = TypeParserParseType(parser, true);
    while (!LexEof(parser->lex)) {
      Symbol* member_symbol = TypeParserParseDeclarator(parser, member_type);
      if (member_symbol == NULL) {
        SyntaxError(parser->syntax, "Invalid type for struct member");
      } else if (!CheckStructMember(str, &member_symbol->name)) {
        SyntaxError(parser->syntax, "Duplicate struct/union member %s",
                    member_symbol->name.value);
        SymbolDelete(member_symbol);
      } else {
        StructMember* member = NewStructMember(member_symbol);

        // Add the member to the struct/union.
        VectorAppend(&str->members, member);
        MapKeyValue kv;
        kv.key.p = &member->symbol->name;
        kv.value.p = member;
        MapInsert(&str->symbol_table, kv);
        
        // Check for bitfield.
        if (LexMatch(parser->lex, TOK(colon))) {
          ParseBitField(parser, is_union, str, member_symbol, member);
        } else {
          // Regular member, align the member to the appropriate boundary.
          AlignNextOffset(str, member_symbol->type);
          member->byte_offset = str->next_offset;
          member->index = str->members.length - 1;

          // Update the struct offset and size based on the
          // member that was inserted.
          if (!is_union) {
            str->next_offset += member_symbol->type->size;
            str->size = str->next_offset;
          } else {
            // The size of a union is the maximum size of its members.
            if (member_symbol->type->size > str->size) {
              str->size = member_symbol->type->size;
            }
          }
        }
      }
      if (!LexMatch(parser->lex, TOK(comma))) {
        break;
      }
    }
  
    SyntaxNeedSemicolon(parser->syntax, TC(type));
  }
}

static void CheckFlexibleArrays(TypeParser* parser, Struct* str, bool is_union) {
  // Check the constraints for flexible arrays.
  // 1. Flexible array cannot be the only member
  // 2. Flexible array must be at the end of the struct.
  // 3. No flexible arrays in unions.
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (TypeIsArray(member->symbol->type)) {
      if (member->symbol->type->info.array.is_flexible) {
        if (is_union) {
          SyntaxError(parser->syntax, "No flexible arrays allowed in unions");
          break;
        }
        if (str->members.length == 1) {
          SyntaxError(parser->syntax,
                      "Flexible array '%s' cannot be the only "
                      "member in a struct",
                      member->symbol->name.value);
          break;
        }
        if (i != str->members.length - 1) {
          SyntaxError(parser->syntax,
                      "Flexible array '%s' needs to be "
                      "the last member in a struct",
                      member->symbol->name.value);
          break;
        }
      }
    }
  }
}

static void CheckTagType(TypeParser* parser, Symbol* old,
                         bool is_union, bool is_enum) {
  bool error = false;
  if (TypeIsEnum(old->type)) {
    error = !is_enum;
  } else {
    // Tag is a struct or union.
    Struct* str = old->type->info.struct_info;
    error = str->is_union != is_union;
  }
  if (error) {
    SyntaxError(parser->syntax,
              "Tag %s declared with different tag type",
              old->name.value);
  }
}

static Symbol* ParseStructBody(TypeParser* parser, String* tag_name, bool is_union) {
  // We have a struct body.
  // First check that this is not a duplicate definition.
  Struct* str = NULL;
  bool empty_tag_name = tag_name->length == 0;
  if (empty_tag_name) {
    StringSet(tag_name, SyntaxFakeName(parser->syntax));
  }
  Symbol* tag = SyntaxFindTopScopeTag(parser->syntax, tag_name);
  if (tag != NULL) {
    if (!tag->flags.is_forward_declared) {
      SyntaxError(parser->syntax, "Duplicate definition of struct/union %s",
                   tag_name->value);
    } else {
      CheckTagType(parser, tag, is_union, false);
    }
    str = tag->type->info.struct_info;
  } else {
    // Tag doesn't exist in the, create one.
    str = NewStruct(is_union);
    TypeRecord* type = NewTypeRecord(is_union ? kTypeUnion : kTypeStruct,
                                      kQualPlain);
    type->info.struct_info = str;
    tag = NewSymbol(tag_name->value, type, STO(implicit));
    str->tag_name = &tag->name;
    if (empty_tag_name) {
      tag->flags.invented = true;
    }
    SyntaxAddTag(parser->syntax, tag);
  }

  // Note in the symbol that this tag is now defined and not
  // forward declared.
  tag->flags.is_forward_declared = false;
  tag->flags.is_defined = true;

  // Now 'tag' will be the struct tag pointer
  // and 'str' will be a pointer to the Struct information.
  ParseStructMembers(parser, str, is_union);
  
  // Round the size of the struct to the next 8 byte boundary.
  str->size = (str->size + 7) & ~7;
  SyntaxNeedBracket(parser->syntax, TOK(rbrace), TC(exprsep));
   
  CheckFlexibleArrays(parser, str, is_union);
  
  return tag;
}

// Parse a struct.  The 'struct' or 'union' keyword has been
// consumed and the current token will be the follower.  This may
// be a tag name or an open brace, or semicolon.  Don't consume
// a semicolon at the end of the struct.
Symbol* TypeParserParseStruct(TypeParser* parser, bool is_union) {
  if (LexLookingAt(parser->lex, TOK(semicolon))) {
    // Don't consume the semicolon.
    return NULL;
  }

  // Read the tag name if there is one.
  String tag_name = {0};
  if (LexLookingAt(parser->lex, TOK(identifier))) {
    // Struct tag is present.
    StringSetString(&tag_name, &parser->lex->spelling);
    LexNextToken(parser->lex);
  }
  Symbol* tag = NULL;
  if (LexMatch(parser->lex, TOK(lbrace))) {
    tag = ParseStructBody(parser, &tag_name, is_union);
  } else {
    // No open brace, this is a reference to an existing struct or the
    // creation of a new one.
    if (tag_name.length == 0) {
      // No tag name, nothing to do.
      return NULL;
    }
    tag = SyntaxFindTag(parser->syntax, &tag_name);
    if (tag == NULL) {
      // New tag.
      Struct* str = NewStruct(is_union);
      TypeRecord* type = NewTypeRecord(kTypeStruct, kQualPlain);
      type->info.struct_info = str;
      tag = NewSymbol(tag_name.value, type, STO(implicit));
      tag->flags.is_forward_declared = true;
      str->tag_name = &tag->name;
      SyntaxAddTag(parser->syntax, tag);
    } else {
      // Tag already exists, make sure it's the same tag type.
      CheckTagType(parser, tag, is_union, false);
    }
  }
  return tag;
}

static void ParseEnumConstants(TypeParser* parser, Enum* e) {
  while (!LexLookingAt(parser->lex, TOK(rbrace))) {
    if (LexLookingAt(parser->lex, TOK(identifier))) {
      String const_name;
      StringInit(&const_name, parser->lex->spelling.value);
      LexNextToken(parser->lex);
      if (LexMatch(parser->lex, TOK(equal))) {
        ASTNode* value =
            SyntaxParseSingleExpression(parser->syntax, TC(semicolon));
        value = AnalyzeExpression(value);
        int64_t next_value = e->next_value;
        if (!EvaluateIntegerExpression(value, &next_value)) {
          SyntaxError(parser->syntax,
                      "Constant integer expression required for value of "
                      "enum constant %s",
                      const_name.value);
          next_value = e->next_value;
        }
        e->next_value = (int32_t)next_value;
        ASTNodeDelete(value);
      }
      Symbol* ec = NewEnumConstant(const_name.value, e->next_value);
      e->next_value++;
      StringDestruct(&const_name);

      // Insert the constant as a symbol in the current scope.
      bool ok = SyntaxAddSymbol(parser->syntax, ec);
      if (!ok) {
        SyntaxError(parser->syntax,
                    "Enum constant %s is already defined in this scope",
                    ec->name.value);
        SymbolDelete(ec);
      } else {
        VectorAppend(&e->constants, ec);
      }
    }
    if (!LexMatch(parser->lex, TOK(comma))) {
      break;
    }
  }
}

static Symbol* ParseEnumBody(TypeParser* parser, String* tag_name) {
  // We have an enum body.
  // First check that this is not a duplicate definition.
  Enum* e = NULL;
  bool empty_tag_name = tag_name->length == 0;
  if (empty_tag_name) {
    StringSet(tag_name, SyntaxFakeName(parser->syntax));
  }
  Symbol* tag = SyntaxFindTopScopeTag(parser->syntax, tag_name);
  if (tag != NULL) {
    if (!tag->flags.is_forward_declared) {
      SyntaxError(parser->syntax, "Duplicate definition of enum %s",
                  tag_name->value);
    } else {
      CheckTagType(parser, tag, false, true);
    }
    e = tag->type->info.enum_info;
  } else {
    // Tag doesn't exist, create one.
    e = NewEnum();
    TypeRecord* type = NewTypeRecord(kTypeEnum, kQualPlain);
    type->info.enum_info = e;
    tag = NewSymbol(tag_name->value, type, STO(implicit));
    e->tag_name = &tag->name;
    if (empty_tag_name) {
      tag->flags.invented = true;
    }
    SyntaxAddTag(parser->syntax, tag);
  }

  // Note in the symbol that this tag is now defined and not
  // forward declared.
  tag->flags.is_forward_declared = false;
  tag->flags.is_defined = true;

  // Now 'tag' will be the struct tag pointer
  // and 'e' will be a pointer to the Enum information.
  ParseEnumConstants(parser, e);

  SyntaxNeedBracket(parser->syntax, TOK(rbrace), TC(expr));
  return tag;
}

// Parse an enum definition or reference.
Symbol* TypeParserParseEnum(TypeParser* parser) {
  if (LexLookingAt(parser->lex, TOK(semicolon))) {
    // Don't consume the semicolon.
    return NULL;
  }

  // Read the tag name if there is one.
  String tag_name = {0};
  if (LexLookingAt(parser->lex, TOK(identifier))) {
    // Struct tag is present.
    StringSetString(&tag_name, &parser->lex->spelling);
    LexNextToken(parser->lex);
  }
  Symbol* tag = NULL;
  if (LexMatch(parser->lex, TOK(lbrace))) {
    tag = ParseEnumBody(parser, &tag_name);
  } else {
    // No open brace, this is a reference to an existing enum or the
    // creation of a new one.
    if (tag_name.length == 0) {
      // No tag name, nothing to do.
      return NULL;
    }
    tag = SyntaxFindTag(parser->syntax, &tag_name);
    if (tag == NULL) {
      // New tag.
      Enum* e = NewEnum();
      TypeRecord* type = NewTypeRecord(kTypeEnum, kQualPlain);
      type->info.enum_info = e;
      tag = NewSymbol(tag_name.value, type, STO(implicit));
      tag->flags.is_forward_declared = true;
      e->tag_name = &tag->name;
      SyntaxAddTag(parser->syntax, tag);
    } else {
      // Tag already exists, make sure it's the same tag type.
      CheckTagType(parser, tag, false, true);
    }
  }
  return tag;
}

//
// Type inference functions.
//

bool TypeIsInt(TypeRecord* type);
bool TypeIsChar(TypeRecord* type);
bool TypeIsShort(TypeRecord* type);
bool TypeIsLong(TypeRecord* type);
bool TypeIsLongLong(TypeRecord* type);
bool TypeIsUnsignedInt(TypeRecord* type);
bool TypeIsUnsignedChar(TypeRecord* type);
bool TypeIsUnsignedShort(TypeRecord* type);
bool TypeIsUnsignedLong(TypeRecord* type);
bool TypeIsUnsignedLongLong(TypeRecord* type);
bool TypeIsFloat(TypeRecord* type);
bool TypeIsDouble(TypeRecord* type);
bool TypeIsLongDouble(TypeRecord* type);
bool TypeIsBool(TypeRecord* type);
bool TypeIsVoid(TypeRecord* type);

bool TypeIsPointer(TypeRecord* type);
bool TypeIsPrimitive(TypeRecord* type);
bool TypeIsPointerOrArray(TypeRecord* type);
bool TypeIsIntegral(TypeRecord* type);
bool TypeIsFloatingPoint(TypeRecord* type);
bool TypeIsFunction(TypeRecord* type);
bool TypeIsFunctionDefinition(TypeRecord* type);
bool TypeIsFunctionPointer(TypeRecord* type);
bool TypeIsStructOrUnionPointer(TypeRecord* type);
bool TypeIsFunctionReturningStructOrUnion(TypeRecord* type);
bool TypeIsVoidFunction(TypeRecord* type);

bool TypeIsPointerToSameType(TypeRecord* ptr1, TypeRecord* ptr2);
bool TypeIsStructOrUnion(TypeRecord* type);
bool TypeIsScalar(TypeRecord* type);
bool TypeIsVoidPointer(TypeRecord* type);
bool TypeIsArray(TypeRecord* type);
bool TypeIsConst(TypeRecord* type);
bool TypeIsVolatile(TypeRecord* type);
bool TypeIsEnum(TypeRecord* type);
bool TypeIsUnsigned(TypeRecord* type);
bool TypeIsSigned(TypeRecord* type);


bool TypeIsIntConstant(TypeRecord* type);
bool TypeIsFloatingPointConstant(TypeRecord* type);
bool TypeIsUnknown(TypeRecord* type);
bool TypeIsFixedArray(TypeRecord* type);
bool TypeIsVLA(TypeRecord* type);

static bool FunctionPrototypesEqual(FunctionInfo* a, FunctionInfo* b) {
  if (a->prototype.length != b->prototype.length) {
    return false;
  }
  for (size_t i = 0; i < a->prototype.length; i++) {
    Symbol* s1 = a->prototype.value.p[i];
    Symbol* s2 = b->prototype.value.p[i];
    if (!TypeEqual(s1->type, s2->type)) {
      return false;
    }
  }
  return true;
}

bool TypeEqual(TypeRecord* t1, TypeRecord* t2) {
  if (t1->declarator != t2->declarator) {
    return false;
  }
  switch (t1->declarator) {
    case kDeclArray:
      if (!TypeEqual(t1->next, t2->next)) {
        return false;
      }
      return t1->info.array.size.fixed == t2->info.array.size.fixed;
    case kDeclPointer:
      return TypeEqual(t1->next, t2->next);

    case kDeclFunction:
      if (!TypeEqual(t1->next, t2->next)) {
        return false;
      }
      return FunctionPrototypesEqual(&t1->info.function, &t2->info.function);
    case kDeclPrimitive:
      return t1->type == t2->type && t1->qualifiers == t2->qualifiers;
  }
}

bool TypeAssignmentCompatible(TypeRecord* from, TypeRecord* to) {
  if (TypeEqual(to, from)) {
    return true;
  }
  // A pointer can be assigned to a const pointer of the same type.
  if (TypeIsPointerOrArray(to)) {
    int to_quals = to->next->qualifiers & ~kQualConst;
    int from_quals = from->next->qualifiers & ~kQualConst;
    if (to_quals == from_quals) {
      return true;
    }
  }
  return false;
}

bool TypeEqualIgnoringSign(TypeRecord* t1, TypeRecord* t2) {
  if (t1->declarator != t2->declarator) {
    return false;
  }
  switch (t1->declarator) {
    case kDeclArray:
      if (!TypeEqual(t1->next, t2->next)) {
        return false;
      }
      return t1->info.array.size.fixed == t2->info.array.size.fixed;
    case kDeclPointer:
      return TypeEqual(t1->next, t2->next);

    case kDeclFunction:
      if (!TypeEqual(t1->next, t2->next)) {
        return false;
      }
      return FunctionPrototypesEqual(&t1->info.function, &t2->info.function);
    case kDeclPrimitive: {
      Type a = t1->type & ~(kTypeUnsigned | kTypeSigned);
      Type b = t2->type & ~(kTypeUnsigned | kTypeSigned);
      return a == b;
    }
  }
}

static void FunctionPrototypesDetails(SourceLocation location, FunctionInfo* a, FunctionInfo* b) {
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);
  if (a->prototype.length != b->prototype.length) {
    ReportNote(filename, lineno, "Different number of arguments: %zd vs %zd",
               a->prototype.length, b->prototype.length);
    return;
  }
  for (size_t i = 0; i < a->prototype.length; i++) {
    Symbol* s1 = a->prototype.value.p[i];
    Symbol* s2 = b->prototype.value.p[i];
    if (!TypeEqual(s1->type, s2->type)) {
      TypeErrorDetails(location, s1->type, s2->type);
      ReportNote(filename, lineno, "  for argument #%zd", i+1);
    }
  }
}

void TypeErrorDetails(SourceLocation location, TypeRecord* t1, TypeRecord* t2) {
  String error1 = {0};
  String error2 = {0};
  TypeRecordToString(t1, &error1);
  TypeRecordToString(t2, &error2);

  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);
    
  if (t1->declarator != t2->declarator) {
    ReportNote(filename, lineno, "Declarators '%s' and '%s' are different",
               error1.value, error2.value);
    return;
  }
  switch (t1->declarator) {
    case kDeclArray:
    case kDeclPointer:
      ReportNote(filename, lineno, "Declaration of '%s' and '%s' are different",
                 error1.value, error2.value);
      TypeErrorDetails(location, t1->next, t2->next);
      break;

    case kDeclFunction:
      ReportNote(filename, lineno, "Declaration of '%s' and '%s' are different",
                 error1.value, error2.value);
      TypeErrorDetails(location, t1->next, t2->next);
      return FunctionPrototypesDetails(location, &t1->info.function, &t2->info.function);
      
    case kDeclPrimitive:
      if (t1->type != t2->type || t1->qualifiers != t2->qualifiers) {
        ReportNote(filename, lineno, "Types '%s' and '%s' are different",
                   error1.value, error2.value);

      }
  }
  StringDestruct(&error1);
  StringDestruct(&error1);
}

