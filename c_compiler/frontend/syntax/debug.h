//
//  debug.h
//  c_compiler_library
//
//  Created by David Allison on 4/5/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef debug_h
#define debug_h

#include "symbol.h"
#include "type.h"
#include "dwarf_defs.h"
#include "vector.h"
#include "map.h"
#include "set.h"
#include "buffer.h"

struct DIE;
struct DebugBuilder;

typedef struct {
  DW_AT id;
  DW_FORM form;
  int size;
  // If set, a DW_FORM(exprloc) is DW_OP_addr followed by this symbol.
  const char* addr_symbol;
  
  union {
    uint8_t data1;      // Also ref1.
    uint16_t data2;     // Also ref2.
    uint32_t data4;     // Also ref4.
    uint64_t data8;     // Also ref8.
    uint8_t sdata[8];   // Also ref_sdata.
    uint8_t udata[8];   // Also ref_udata.
    uint64_t address;
    Buffer block;
    String string;
    bool flag;
    int64_t offset;
    struct DIE* die;
  } v;
} DebugAttributeValue;

DebugAttributeValue* NewDebugAttributeValue(DW_AT id, DW_FORM form);
void DebugAttributeValueDestruct(DebugAttributeValue* v);
void DebugAttributeValueDelete(DebugAttributeValue* v);

// An abbreviation is a set of attribute id and form pairs held
// in a buffer.  The buffer is used as the key for the map and the
// abbreviation contains a pointer back to its key.
typedef struct {
  int num;            // Abbreviation number.
  Buffer* signature;  // Attribue id/form pairs.
} DebugAbbreviation;

typedef struct DebugBuilder {
  Map tags;             // Struct/enum tag vs DIE.
  Map abbreviation_map;    // Key: Buffer: signature, value: DebugAbbreviation*.
  Vector abbreviations;
  Buffer string_table;
  int next_abbrev_number;
  Vector all_dies;
  Vector top_dies;
  
  // Main compile_unit DIE.
  struct DIE* compile_unit;
  String filename;
  const char* producer;
  String dir;
  
  // Output handler.
  FILE* fp;
  struct AsmModule* module;
  Buffer bytes;
} DebugBuilder;

void DebugBuilderInit(DebugBuilder* builder, const char* filename, const char* producer,
                    const char* dir);
void DebugBuilderDestruct(DebugBuilder* builder);
void DebugBuilderEmitAbbreviations(DebugBuilder* builder);
void DebugBuilderEmitDebugInfo(DebugBuilder* builder);

struct DIE;

typedef struct {
  void (*destructor)(struct DIE*);
  void (*builder)(DebugBuilder*, struct DIE*);
  void (*printer)(struct DIE*, int level);
  void (*emitter)(DebugBuilder*, struct DIE*);
} DIEVirtuals;

typedef struct DIE {
  int id;               // ID for DIE, starting at 0.
  DW_TAG tag;
  DIEVirtuals* virtuals;
  DebugAbbreviation* abbrev;
  Vector attr_values;
  bool has_children;
  bool emitted;
} DIE;

typedef struct {
  DIE die;
  String name;
} NamedDIE;

typedef struct {
  DIE die;
  bool (*func)(struct TypeRecord*);
  const char* name;
  DW_ATE encoding;
  int byte_size;
} BaseTypeDIE;

typedef struct {
  DIE die;
  int upper_bound;
  DIE* type;
} SubrangeDIE;

typedef struct {
  DIE die;
  DIE* subrange;
  DIE* subtype;
} ArrayTypeDIE;

typedef struct {
  DIE die;
  DIE* subtype;
} PointerTypeDIE;

typedef struct {
  DIE die;
  DIE* member_type;
  DIE* containing_type;
  int byte_size;
} PtrToMemberTypeDIE;

typedef struct LexicalScopeDIE {
  DIE die;
  Vector variables;       // Vector of VariableDIE*.
  Vector lexical_scopes;  // Vector of LexicalScope*.
  struct LexicalScopeDIE* parent;
  String low_pc_label;
  String high_pc_expr;
} LexicalScopeDIE;

typedef struct {
  NamedDIE die;
  DIE* subtype;
  LexicalScopeDIE* top_scope;
  const char* linkage_name;
  bool is_external;
} FunctionTypeDIE;

typedef struct {
  DIE die;
  DIE* subtype;
} ConstVolatileDIE;

typedef enum {
  kLocationUnknown,
  kLocationInRegister,
  kLocationOnStack,
  kLocationStatic,
} VarLocationType;

typedef struct {
  VarLocationType type;
  union {
    int reg;
    int stack_offset;
    const char* symbol_name;
  } v;
} VariableLocation;

typedef struct {
  NamedDIE die;
  DIE* type;
  bool is_external;
  bool is_declaration;
  VariableLocation location;
} VariableDIE;

void VariableDIESetRegister(DIE* die, int reg);
void VariableDIESetStackOffset(DIE* die, int offset);
void VariableDIESetStatic(DIE* die, const char* symbol);

typedef struct {
  NamedDIE die;
  DIE* type;
  // TODO type
 } ConstantDIE;

typedef struct {
  VariableDIE var;
  int byte_offset;
  bool is_bit_field;
  int bit_offset;
  int bit_size;
} MemberDIE;

typedef struct {
  NamedDIE die;
  int64_t value;
} EnumeratorDIE;

typedef struct {
  NamedDIE die;
  int byte_size;
  Vector members;
} StructDIE;

typedef struct {
  NamedDIE die;
  Vector enumerators;
  int byte_size;
} EnumDIE;

void DIEDestruct(DIE* die);
DIE* BuildDebugInfo(DebugBuilder* builder, Symbol* sym, DW_TAG tag);
DIE* BuildTypeDebugInfo(DebugBuilder* builder, TypeRecord* type);
void DIEPrint(DIE* die, int level);
void BuildDebugInfoAfterCodegen(DebugBuilder* builder, Symbol* sym);

#endif /* debug_h */
