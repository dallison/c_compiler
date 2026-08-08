//
//  member_pointer.c
//  c_compiler
//
//  Platform-compatible C++ pointer-to-member representation and helpers.
//

#include "member_pointer.h"

#include <string.h>

#include "ast.h"
#include "compiler.h"
#include "expr_semantics.h"
#include "semantics.h"
#include "syntax.h"
#include "symbol.h"
#include "type_class_internal.h"
#include "type_compare.h"
#include "type_core.h"

MemberPointerABI MemberPointerTargetABI(void) {
  if (compiler == NULL || compiler->target_name == NULL) {
    return kMemberPointerABIItanium;
  }
  if (StringEqual(compiler->target_name, "arm") ||
      StringEqual(compiler->target_name, "armv7") ||
      StringEqual(compiler->target_name, "armv7-a") ||
      StringEqual(compiler->target_name, "arm32")) {
    return kMemberPointerABIArmEabi;
  }
  if (StringEqual(compiler->target_name, "6502") ||
      StringEqual(compiler->target_name, "65c02")) {
    return kMemberPointerABI6502;
  }
  if (StringEqual(compiler->target_name, "p-code") ||
      StringEqual(compiler->target_name, "pcode")) {
    return kMemberPointerABIPCode;
  }
  return kMemberPointerABIItanium;
}

bool TypeIsMemberDataPointer(TypeRecord* type) {
  if (!TypeIsMemberPointer(type)) {
    return false;
  }
  TypeRecord* pointee = TypeMemberPointerPointeeType(type);
  return pointee != NULL && !TypeIsFunction(pointee);
}

bool TypeIsMemberFunctionPointer(TypeRecord* type) {
  if (!TypeIsMemberPointer(type)) {
    return false;
  }
  TypeRecord* pointee = TypeMemberPointerPointeeType(type);
  return pointee != NULL && TypeIsFunction(pointee);
}

bool MemberPointerUsesPairLayout(TypeRecord* type) {
  return TypeIsMemberFunctionPointer(type);
}

bool TypeIsMemberPointerScalar(TypeRecord* type) {
  return TypeIsMemberPointer(type) && !MemberPointerUsesPairLayout(type);
}

bool TypeIsMemberPointerAggregate(TypeRecord* type) {
  return TypeIsMemberPointer(type) && MemberPointerUsesPairLayout(type);
}

Struct* TypeMemberPointerClass(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  for (TypeRecord* cur = type; cur != NULL; cur = cur->next) {
    if (cur->declarator == kDeclMemberPointer) {
      return cur->info.struct_info;
    }
  }
  return NULL;
}

TypeRecord* TypeMemberPointerPointeeType(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  for (TypeRecord* cur = type; cur != NULL; cur = cur->next) {
    if (cur->declarator == kDeclMemberPointer) {
      return cur->next;
    }
  }
  return NULL;
}

int MemberPointerNullDataOffset(void) {
  return -1;
}

int MemberPointerNullFunctionPtr(void) {
  return 0;
}

int MemberPointerSize(TypeRecord* type) {
  if (!TypeIsMemberPointer(type)) {
    return compiler != NULL ? compiler->pointer_size : 8;
  }
  int word = compiler != NULL ? compiler->pointer_size : 8;
  return MemberPointerUsesPairLayout(type) ? 2 * word : word;
}

int MemberPointerPtrFieldOffset(TypeRecord* type) {
  (void)type;
  return 0;
}

int MemberPointerAdjFieldOffset(TypeRecord* type) {
  int word = compiler != NULL ? compiler->pointer_size : 8;
  if (!MemberPointerUsesPairLayout(type)) {
    return -1;
  }
  return word;
}

static TypeRecord* MemberPointerClassTypeRecord(Struct* class_info) {
  if (class_info == NULL) {
    return NULL;
  }
  TypeRecord* type =
      NewTypeRecord(class_info->is_union ? kTypeUnion : kTypeStruct, kQualPlain);
  type->info.struct_info = class_info;
  return type;
}

static bool MemberPointerPointeeTypesCompatible(TypeRecord* from,
                                              TypeRecord* to) {
  if (from == NULL || to == NULL) {
    return false;
  }
  if (TypeEqual(from, to)) {
    return true;
  }
  if (TypeIsFunction(from) && TypeIsFunction(to)) {
    if (!from->info.function.is_noexcept ||
        to->info.function.is_noexcept) {
      return false;
    }
    return TypeEqualIgnoringFunctionNoexcept(from, to);
  }
  Qualifiers cv = kQualConst | kQualVolatile;
  if ((from->qualifiers & cv & ~to->qualifiers) != 0) {
    return false;
  }
  return TypeEqualIgnoringTopLevelQualifierMask(from, to, cv);
}

static bool MemberPointerIsVirtualFunctionEncoding(TypeRecord* type,
                                                   const MemberPointerValue* value) {
  if (!TypeIsMemberFunctionPointer(type) || value == NULL) {
    return false;
  }
  if (MemberPointerTargetABI() == kMemberPointerABIArmEabi) {
    return (value->adj & 1) != 0;
  }
  return (value->ptr & 1) != 0;
}

bool MemberPointerEncodeFromMember(StructMember* member, Struct* class_info,
                                   MemberPointerValue* out) {
  if (out == NULL || member == NULL || class_info == NULL ||
      member->symbol == NULL) {
    return false;
  }
  memset(out, 0, sizeof(*out));
  MemberPointerABI abi = MemberPointerTargetABI();

  if (member->is_member_function && TypeIsFunction(member->symbol->type)) {
    TypeRecord* func = member->symbol->type;
    int this_adj = CXXBaseOffsetForMember(class_info, member);
    bool is_virtual = func->info.function.is_virtual;
    if (abi == kMemberPointerABIArmEabi) {
      if (is_virtual) {
        int vindex = func->info.function.virtual_index;
        out->ptr = (int64_t)(vindex * (int)compiler->pointer_size);
        out->adj = ((int64_t)this_adj << 1) | 1;
      } else {
        out->fn_symbol = member->symbol;
        out->adj = (int64_t)this_adj << 1;
      }
      return true;
    }
    if (is_virtual) {
      int vindex = func->info.function.virtual_index;
      out->ptr = (int64_t)(vindex * (int)compiler->pointer_size + 1);
      out->adj = this_adj;
    } else {
      out->fn_symbol = member->symbol;
      out->adj = this_adj;
    }
    return true;
  }

  out->ptr = member->byte_offset;
  if (abi == kMemberPointerABIArmEabi) {
    TypeRecord* from = MemberPointerClassTypeRecord(class_info);
    CXXBaseAdjustment adjustment;
    if (TypeBaseAdjustment(from, MemberPointerClassTypeRecord(class_info),
                           /*public_only=*/true, &adjustment) &&
        adjustment.kind == kCXXBaseAdjustmentVirtual) {
      out->adj = ((int64_t)adjustment.byte_offset << 1) | 1;
    }
    TypeRecordDelete(from);
  }
  return true;
}

bool MemberPointerEncodeNull(TypeRecord* type, MemberPointerValue* out) {
  if (out == NULL || type == NULL) {
    return false;
  }
  memset(out, 0, sizeof(*out));
  if (TypeIsMemberFunctionPointer(type)) {
    out->ptr = MemberPointerNullFunctionPtr();
    return true;
  }
  out->ptr = MemberPointerNullDataOffset();
  return true;
}

bool MemberPointerValueIsNull(TypeRecord* type, const MemberPointerValue* value) {
  if (type == NULL || value == NULL) {
    return true;
  }
  if (value->fn_symbol != NULL) {
    return false;
  }
  if (TypeIsMemberFunctionPointer(type)) {
    return value->ptr == MemberPointerNullFunctionPtr() && value->adj == 0;
  }
  return value->ptr == MemberPointerNullDataOffset();
}

bool MemberPointerValuesEqual(TypeRecord* type, const MemberPointerValue* a,
                              const MemberPointerValue* b) {
  if (type == NULL || a == NULL || b == NULL) {
    return a == b;
  }
  if (MemberPointerUsesPairLayout(type)) {
    return a->ptr == b->ptr && a->adj == b->adj &&
           a->fn_symbol == b->fn_symbol;
  }
  return a->ptr == b->ptr;
}

static bool MemberPointerClassRelationship(TypeRecord* from_class_rec,
                                           TypeRecord* to_class_rec,
                                           bool is_cast,
                                           CXXBaseAdjustment* adjustment) {
  // A pointer to a member of a base class converts implicitly to the
  // corresponding pointer to member of an unambiguous public derived class.
  // TypeBaseAdjustment takes (derived, base), opposite to the member-pointer
  // conversion's source/target spelling.
  if (TypeBaseAdjustment(to_class_rec, from_class_rec,
                         /*public_only=*/true, adjustment)) {
    return true;
  }
  if (is_cast &&
      TypeBaseAdjustment(from_class_rec, to_class_rec,
                         /*public_only=*/false, adjustment)) {
    return true;
  }
  return false;
}

static int MemberPointerConversionSign(TypeRecord* from_class_rec,
                                       TypeRecord* to_class_rec) {
  CXXBaseAdjustment adjustment;
  if (TypeBaseAdjustment(to_class_rec, from_class_rec, /*public_only=*/true,
                         &adjustment)) {
    return +1;
  }
  if (TypeBaseAdjustment(from_class_rec, to_class_rec, /*public_only=*/false,
                         &adjustment)) {
    return -1;
  }
  return 0;
}

bool MemberPointerCanConvert(TypeRecord* from, TypeRecord* to, bool is_cast,
                             bool* rejects_virtual_base) {
  if (rejects_virtual_base != NULL) {
    *rejects_virtual_base = false;
  }
  if (from == NULL || to == NULL || !TypeIsMemberPointer(from) ||
      !TypeIsMemberPointer(to)) {
    return false;
  }
  Struct* from_class = TypeMemberPointerClass(from);
  Struct* to_class = TypeMemberPointerClass(to);
  if (from_class == NULL || to_class == NULL) {
    return false;
  }
  if (!MemberPointerPointeeTypesCompatible(TypeMemberPointerPointeeType(from),
                                           TypeMemberPointerPointeeType(to))) {
    return false;
  }
  if (from_class == to_class) {
    return true;
  }
  TypeRecord* from_class_rec = MemberPointerClassTypeRecord(from_class);
  TypeRecord* to_class_rec = MemberPointerClassTypeRecord(to_class);
  CXXBaseAdjustment adjustment;
  bool ok = MemberPointerClassRelationship(from_class_rec, to_class_rec, is_cast,
                                           &adjustment);
  TypeRecordDelete(from_class_rec);
  TypeRecordDelete(to_class_rec);
  if (!ok) {
    return false;
  }
  if (adjustment.kind == kCXXBaseAdjustmentVirtual) {
    if (rejects_virtual_base != NULL) {
      *rejects_virtual_base = true;
    }
    return false;
  }
  return true;
}

bool MemberPointerConvertValue(TypeRecord* from_type, TypeRecord* to_type,
                               const MemberPointerValue* from,
                               MemberPointerValue* to) {
  if (from == NULL || to == NULL || from_type == NULL || to_type == NULL) {
    return false;
  }
  if (MemberPointerValueIsNull(from_type, from)) {
    return MemberPointerEncodeNull(to_type, to);
  }
  *to = *from;
  Struct* from_class = TypeMemberPointerClass(from_type);
  Struct* to_class = TypeMemberPointerClass(to_type);
  if (from_class == NULL || to_class == NULL || from_class == to_class) {
    return true;
  }
  TypeRecord* from_class_rec = MemberPointerClassTypeRecord(from_class);
  TypeRecord* to_class_rec = MemberPointerClassTypeRecord(to_class);
  int sign = MemberPointerConversionSign(from_class_rec, to_class_rec);
  CXXBaseAdjustment adjustment;
  bool have_adjustment = false;
  if (sign < 0) {
    have_adjustment =
        TypeBaseAdjustment(from_class_rec, to_class_rec, /*public_only=*/true,
                           &adjustment);
  } else if (sign > 0) {
    have_adjustment =
        TypeBaseAdjustment(to_class_rec, from_class_rec, /*public_only=*/false,
                           &adjustment);
  }
  TypeRecordDelete(from_class_rec);
  TypeRecordDelete(to_class_rec);
  if (!have_adjustment || adjustment.kind == kCXXBaseAdjustmentVirtual) {
    return false;
  }
  int delta = sign * adjustment.byte_offset;
  if (TypeIsMemberFunctionPointer(to_type)) {
    if (MemberPointerTargetABI() == kMemberPointerABIArmEabi) {
      to->adj += (int64_t)delta << 1;
    } else {
      to->adj += delta;
    }
    return true;
  }
  if (MemberPointerTargetABI() == kMemberPointerABIArmEabi && (to->adj & 1) != 0) {
    return false;
  }
  to->ptr += delta;
  return true;
}

static bool MemberPointerValueFromMemberNode(struct ASTNode* node,
                                             MemberPointerValue* out) {
  if (node == NULL || out == NULL) {
    return false;
  }
  if (node->op == AST_OP(member_ptr)) {
    UnaryASTNode* unary = (UnaryASTNode*)node;
    if (unary->sub == NULL || unary->sub->op != AST_OP(structmember)) {
      return false;
    }
    StructMemberASTNode* member_node = (StructMemberASTNode*)unary->sub;
    StructMember* member = member_node->member;
    Struct* class_info = TypeMemberPointerClass(node->type);
    return MemberPointerEncodeFromMember(member, class_info, out);
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->symbol == NULL || !TypeIsMemberPointer(id->symbol->type) ||
        (!id->symbol->flags.is_constexpr && !TypeIsConst(id->symbol->type))) {
      return false;
    }
    // value is a union.  Member-pointer initializers retain the referenced
    // member in value.other, so never reinterpret that pointer as ivalue.
    if (id->symbol->value.other != NULL) {
      StructMember* member = (StructMember*)id->symbol->value.other;
      Struct* class_info = TypeMemberPointerClass(id->symbol->type);
      return MemberPointerEncodeFromMember(member, class_info, out);
    }
    if (!id->symbol->flags.value_set) {
      return false;
    }
    out->ptr = id->symbol->value.ivalue;
    out->adj = 0;
    out->fn_symbol = NULL;
    return true;
  }
  if (node->op == AST_OP(number) && TypeIsMemberPointer(node->type)) {
    out->ptr = ((ConstantASTNode*)node)->value.ivalue;
    out->adj = 0;
    out->fn_symbol = NULL;
    return true;
  }
  return false;
}

StructMember* MemberPointerReferencedMember(ASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  if (node->op == AST_OP(expr_init)) {
    return MemberPointerReferencedMember(
        ((ExpressionInitializerASTNode*)node)->expr);
  }
  if (node->op == AST_OP(cast)) {
    return MemberPointerReferencedMember(((CastASTNode*)node)->expr);
  }
  if (node->op != AST_OP(member_ptr)) {
    return NULL;
  }
  UnaryASTNode* unary = (UnaryASTNode*)node;
  if (unary->sub == NULL || unary->sub->op != AST_OP(structmember)) {
    return NULL;
  }
  return ((StructMemberASTNode*)unary->sub)->member;
}

bool MemberPointerTryEvaluateConstant(struct ASTNode* node, TypeRecord* type,
                                      MemberPointerValue* out) {
  if (node == NULL || type == NULL || out == NULL) {
    return false;
  }
  if (node->op == AST_OP(expr_init)) {
    return MemberPointerTryEvaluateConstant(
        ((ExpressionInitializerASTNode*)node)->expr, type, out);
  }
  if (MemberPointerValueFromMemberNode(node, out)) {
    return true;
  }
  if (node->op == AST_OP(cast)) {
    CastASTNode* cast = (CastASTNode*)node;
    TypeRecord* from_type =
        cast->expr != NULL ? cast->expr->type : cast->base.type;
    MemberPointerValue inner;
    if (!MemberPointerTryEvaluateConstant(cast->expr, from_type, &inner)) {
      return false;
    }
    return MemberPointerConvertValue(from_type, type, &inner, out);
  }
  return false;
}

static void MemberPointerAppendWordInitializer(Vector* initializers, int offset,
                                               int64_t value) {
  Initializer* init = malloc(sizeof(Initializer));
  memset(init, 0, sizeof(*init));
  init->offset = offset;
  int word = compiler->pointer_size;
  if (word == 8) {
    init->type = kInitTypeLong;
    init->value._long = (uint64_t)value;
  } else if (word == 4) {
    init->type = kInitTypeWord;
    init->value.word = (uint32_t)value;
  } else if (word == 2) {
    init->type = kInitTypeHalf;
    init->value.half = (uint16_t)value;
  } else {
    init->type = kInitTypeByte;
    init->value.byte = (uint8_t)value;
  }
  VectorAppend(initializers, init);
}

static void MemberPointerAppendSymbolInitializer(Vector* initializers,
                                                   int offset, Symbol* symbol) {
  Initializer* init = malloc(sizeof(Initializer));
  memset(init, 0, sizeof(*init));
  init->offset = offset;
  init->type = kInitTypeSymbol;
  init->value.symbol = symbol;
  VectorAppend(initializers, init);
}

void MemberPointerEmitStaticInitializers(TypeRecord* type,
                                         const MemberPointerValue* value,
                                         int dest_offset,
                                         Vector* initializers) {
  if (type == NULL || value == NULL || initializers == NULL) {
    return;
  }
  if (value->fn_symbol != NULL) {
    MemberPointerAppendSymbolInitializer(initializers, dest_offset,
                                         value->fn_symbol);
  } else {
    MemberPointerAppendWordInitializer(initializers, dest_offset, value->ptr);
  }
  if (MemberPointerUsesPairLayout(type)) {
    MemberPointerAppendWordInitializer(
        initializers, dest_offset + MemberPointerAdjFieldOffset(type),
        value->adj);
  }
}

static TypeRecord* MemberPointerWordType(void) {
  if (compiler->pointer_size == 8) {
    return NewTypeRecordWithSize(kTypeLongLong, kQualPlain);
  }
  if (compiler->pointer_size == 4) {
    return NewTypeRecordWithSize(kTypeLong, kQualPlain);
  }
  return NewTypeRecordWithSize(kTypeShort, kQualPlain);
}

static TypeRecord* MemberPointerCharPointerType(void) {
  return NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeChar, kQualPlain));
}

ASTNode* MemberPointerMaterializePrvalue(struct Syntax* syntax,
                                         SourceLocation location,
                                         ASTNode* member_ptr,
                                         TypeRecord* type) {
  (void)syntax;
  if (member_ptr->value_category == kValueCategoryLvalue) {
    return member_ptr;
  }
  Symbol* tmp = SyntaxNewTemporary(&compiler->syntax, type);
  ASTNode* id = NewIdentifierASTNode(tmp, location);
  ASTNode* init =
      NewBinaryASTNode(AST_OP(assign), type, location, id, member_ptr);
  init = AnalyzeExpression(init);
  ASTNode* comma = NewBinaryASTNode(AST_OP(comma), type, location, init, id);
  return AnalyzeExpression(comma);
}

static ASTNode* MemberPointerLoadField(ASTNode* member_ptr, TypeRecord* type,
                                       int field_offset,
                                       SourceLocation location) {
  ASTNode* base =
      MemberPointerMaterializePrvalue(&compiler->syntax, location, member_ptr,
                                     type);
  if (!MemberPointerUsesPairLayout(type) && field_offset == 0) {
    ASTNode* word = NewCastASTNode(MemberPointerWordType(), location, base);
    return AnalyzeExpression(word);
  }
  ASTNode* addr = NewUnaryASTNode(AST_OP(address), NULL, location, base);
  addr = AnalyzeExpression(addr);
  addr = NewCastASTNode(MemberPointerCharPointerType(), location, addr);
  addr = AnalyzeExpression(addr);
  ASTNode* offset =
      NewIntConstantASTNode(field_offset,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  addr = NewBinaryASTNode(AST_OP(plus), addr->type, location, addr, offset);
  addr = AnalyzeExpression(addr);
  TypeRecord* word_ptr_type =
      NewPointerTo(kQualPlain, MemberPointerWordType());
  ASTNode* word_addr = NewCastASTNode(word_ptr_type, location, addr);
  word_addr = AnalyzeExpression(word_addr);
  ASTNode* load = NewUnaryASTNode(AST_OP(contents), MemberPointerWordType(),
                                  location, word_addr);
  return AnalyzeExpression(load);
}

static ASTNode* MemberPointerAddByteOffset(ASTNode* address, ASTNode* offset,
                                           SourceLocation location) {
  TypeRecord* result_type = TypeRecordCopy(address->type);
  ASTNode* byte_address =
      NewCastASTNode(MemberPointerCharPointerType(), location, address);
  byte_address = AnalyzeExpression(byte_address);
  ASTNode* adjusted = NewBinaryASTNode(AST_OP(plus), byte_address->type,
                                       location, byte_address, offset);
  adjusted = AnalyzeExpression(adjusted);
  adjusted = NewCastASTNode(result_type, location, adjusted);
  return AnalyzeExpression(adjusted);
}

static ASTNode* MemberPointerIdentityClone(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static ASTNode* MemberPointerAdjustReceiverToOwner(
    ASTNode* object_addr, TypeRecord* member_ptr_type,
    SourceLocation location) {
  if (object_addr == NULL || object_addr->type == NULL ||
      !TypeIsPointer(object_addr->type)) {
    return object_addr;
  }
  TypeRecord* object_type = object_addr->type->next;
  Struct* object_class =
      TypeIsStructOrUnion(object_type) ? object_type->info.struct_info : NULL;
  Struct* owner_class = TypeMemberPointerClass(member_ptr_type);
  if (object_class == NULL || owner_class == NULL ||
      object_class == owner_class) {
    return object_addr;
  }

  TypeRecord* owner_type = MemberPointerClassTypeRecord(owner_class);
  CXXBaseAdjustment adjustment;
  bool found = TypeBaseAdjustment(object_type, owner_type,
                                  /*public_only=*/true, &adjustment);
  TypeRecordDelete(owner_type);
  if (!found) {
    return object_addr;
  }

  if (adjustment.kind != kCXXBaseAdjustmentVirtual) {
    if (adjustment.byte_offset == 0) {
      return object_addr;
    }
    ASTNode* offset = NewIntConstantASTNode(
        adjustment.byte_offset, NewSizeTypeRecord(), location);
    return MemberPointerAddByteOffset(object_addr, offset, location);
  }

  // A virtual-base lookup needs the receiver both to load __vbptr and to form
  // the adjusted address.  Evaluate the original receiver exactly once before
  // using those two references.
  Symbol* temporary =
      SyntaxNewTemporary(&compiler->syntax, object_addr->type);
  ASTNode* destination = NewIdentifierASTNode(temporary, location);
  ASTNode* assign = NewBinaryASTNode(AST_OP(assign), object_addr->type, location,
                                     destination, object_addr);
  assign = AnalyzeExpression(assign);

  ASTNode* receiver_for_lookup = NewIdentifierASTNode(temporary, location);
  receiver_for_lookup = AnalyzeExpression(receiver_for_lookup);
  ASTNode* vbptr_name =
      NewStringConstantASTNode(NewString("__vbptr"), NULL, location);
  ASTNode* vbptr = NewBinaryASTNode(AST_OP(arrow), NULL, location,
                                    receiver_for_lookup, vbptr_name);
  vbptr = AnalyzeExpression(vbptr);
  ASTNode* index = NewIntConstantASTNode(
      adjustment.vbtable_index,
      NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  ASTNode* runtime_offset =
      NewBinaryASTNode(AST_OP(subscript), NULL, location, vbptr, index);
  runtime_offset = AnalyzeExpression(runtime_offset);
  if (adjustment.byte_offset != 0) {
    runtime_offset = NewBinaryASTNode(
        AST_OP(plus), runtime_offset->type, location, runtime_offset,
        NewIntConstantASTNode(adjustment.byte_offset, NewSizeTypeRecord(),
                              location));
    runtime_offset = AnalyzeExpression(runtime_offset);
  }

  ASTNode* receiver_for_adjustment =
      NewIdentifierASTNode(temporary, location);
  receiver_for_adjustment = AnalyzeExpression(receiver_for_adjustment);
  ASTNode* adjusted = MemberPointerAddByteOffset(
      receiver_for_adjustment, runtime_offset, location);
  ASTNode* result = NewBinaryASTNode(AST_OP(comma), adjusted->type, location,
                                     assign, adjusted);
  return AnalyzeExpression(result);
}

static ASTNode* MemberPointerAdjustObjectAddress(ASTNode* object_addr,
                                                ASTNode* member_ptr,
                                                TypeRecord* member_ptr_type,
                                                SourceLocation location) {
  if (TypeIsMemberDataPointer(member_ptr_type)) {
    // The ARM C++ ABI changes only the member-function-pointer discriminator.
    // Data member pointers retain the generic ABI's single ptrdiff_t offset,
    // so there is no second adjustment word to load.
    return object_addr;
  }

  if (!MemberPointerUsesPairLayout(member_ptr_type)) {
    return object_addr;
  }

  ASTNode* adj = MemberPointerLoadField(
      member_ptr, member_ptr_type, MemberPointerAdjFieldOffset(member_ptr_type),
      location);
  if (MemberPointerTargetABI() == kMemberPointerABIArmEabi) {
    adj = NewBinaryASTNode(
        AST_OP(rshiftl), NewTypeRecordWithSize(kTypeInt, kQualPlain), location,
        adj,
        NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location));
    adj = AnalyzeExpression(adj);
  }
  return MemberPointerAddByteOffset(object_addr, adj, location);
}

ASTNode* MemberPointerApplyDataAccess(struct Syntax* syntax,
                                      SourceLocation location,
                                      ASTNode* receiver,
                                      bool receiver_is_pointer,
                                      ASTNode* member_ptr,
                                      TypeRecord* result_type) {
  (void)syntax;
  TypeRecord* receiver_type = receiver->type;
  if (receiver_is_pointer) {
    if (receiver_type == NULL || !TypeIsPointer(receiver_type)) {
      SemanticError(receiver, "Left operand of ->* is not a pointer");
      return NewIntConstantASTNode(0, result_type, location);
    }
    receiver = NewUnaryASTNode(AST_OP(contents), receiver_type->next, location,
                               receiver);
    receiver = AnalyzeExpression(receiver);
    receiver_type = receiver->type;
  } else if (receiver_type != NULL && TypeIsReference(receiver_type)) {
    receiver_type = receiver_type->next;
  }

  ASTNode* object_addr = receiver;
  if (!TypeIsPointer(receiver->type)) {
    object_addr = NewUnaryASTNode(AST_OP(address), NULL, location, receiver);
    object_addr = AnalyzeExpression(object_addr);
  }
  object_addr = MemberPointerAdjustReceiverToOwner(
      object_addr, member_ptr->type, location);

  MemberPointerValue pm_value;
  bool have_const =
      MemberPointerTryEvaluateConstant(member_ptr, member_ptr->type, &pm_value);

  ASTNode* member_addr;
  if (have_const) {
    ASTNode* byte_addr = MemberPointerAdjustObjectAddress(
        object_addr, member_ptr, member_ptr->type, location);
    if (pm_value.ptr != MemberPointerNullDataOffset()) {
      ASTNode* char_ptr = NewCastASTNode(MemberPointerCharPointerType(), location,
                                         byte_addr);
      char_ptr = AnalyzeExpression(char_ptr);
      byte_addr = NewBinaryASTNode(
          AST_OP(plus), char_ptr->type, location, char_ptr,
          NewIntConstantASTNode(pm_value.ptr,
                                NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                location));
      byte_addr = AnalyzeExpression(byte_addr);
    }
    TypeRecord* member_ptr_type =
        NewPointerTo(kQualPlain, TypeRecordCopy(result_type));
    member_addr = NewCastASTNode(member_ptr_type, location, byte_addr);
    member_addr = AnalyzeExpression(member_addr);
  } else {
    ASTNode* base_addr = MemberPointerAdjustObjectAddress(
        object_addr, member_ptr, member_ptr->type, location);
    ASTNode* data_off =
        MemberPointerLoadField(member_ptr, member_ptr->type, 0, location);
    ASTNode* char_base =
        NewCastASTNode(MemberPointerCharPointerType(), location, base_addr);
    char_base = AnalyzeExpression(char_base);
    member_addr = NewBinaryASTNode(AST_OP(plus), char_base->type, location,
                                   char_base, data_off);
    member_addr = AnalyzeExpression(member_addr);
    TypeRecord* member_ptr_type =
        NewPointerTo(kQualPlain, TypeRecordCopy(result_type));
    member_addr = NewCastASTNode(member_ptr_type, location, member_addr);
    member_addr = AnalyzeExpression(member_addr);
  }

  ASTNode* load =
      NewUnaryASTNode(AST_OP(contents), result_type, location, member_addr);
  load = AnalyzeExpression(load);
  load->value_category = kValueCategoryLvalue;
  return load;
}

static ASTNode* MemberPointerBuildPtrFieldLoad(ASTNode* member_ptr,
                                               TypeRecord* member_ptr_type,
                                               SourceLocation location) {
  return MemberPointerLoadField(member_ptr, member_ptr_type, 0, location);
}

static ASTNode* MemberPointerBuildAdjFieldLoad(ASTNode* member_ptr,
                                               TypeRecord* member_ptr_type,
                                               SourceLocation location) {
  return MemberPointerLoadField(member_ptr, member_ptr_type,
                                MemberPointerAdjFieldOffset(member_ptr_type),
                                location);
}

static TypeRecord* MemberPointerCopyFunctionType(TypeRecord* function_type) {
  return TypeRecordCopy(function_type);
}

static ASTNode* MemberPointerCloneReceiver(ASTNode* receiver) {
  return ASTNodeClone(receiver, MemberPointerIdentityClone, NULL, NULL);
}

static ASTNode* MemberPointerBuildVirtualCallee(ASTNode* receiver,
                                                bool receiver_is_pointer,
                                                ASTNode* ptr_field,
                                                TypeRecord* fn_type,
                                                SourceLocation location) {
  ASTNode* receiver_clone = MemberPointerCloneReceiver(receiver);
  if (!receiver_is_pointer) {
    TypeRecord* pointer_type = NewPointerTo(kQualPlain, receiver_clone->type);
    ASTNode* address =
        NewUnaryASTNode(AST_OP(address), pointer_type, location, receiver_clone);
    address = AnalyzeExpression(address);
    receiver_clone = address;
  }
  ASTNode* vptr_name =
      NewStringConstantASTNode(NewString("__vptr"), NULL, location);
  ASTNode* vptr =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver_clone, vptr_name);
  vptr = AnalyzeExpression(vptr);

  TypeRecord* function_type = MemberPointerCopyFunctionType(fn_type);
  TypeRecord* function_pointer = NewPointerTo(kQualPlain, function_type);

  if (MemberPointerTargetABI() == kMemberPointerABIArmEabi) {
    ASTNode* char_vptr =
        NewCastASTNode(MemberPointerCharPointerType(), location, vptr);
    char_vptr = AnalyzeExpression(char_vptr);
    ASTNode* slot_addr =
        NewBinaryASTNode(AST_OP(plus), char_vptr->type, location, char_vptr,
                         ptr_field);
    slot_addr = AnalyzeExpression(slot_addr);
    ASTNode* fn_ptr =
        NewUnaryASTNode(AST_OP(contents), function_pointer, location, slot_addr);
    fn_ptr = AnalyzeExpression(fn_ptr);
    return NewUnaryASTNode(AST_OP(contents), function_type, location, fn_ptr);
  }

  ASTNode* one =
      NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  ASTNode* byte_off = NewBinaryASTNode(
      AST_OP(minus), NewTypeRecordWithSize(kTypeInt, kQualPlain), location,
      ptr_field, one);
  byte_off = AnalyzeExpression(byte_off);
  ASTNode* psize = NewIntConstantASTNode(
      (int)compiler->pointer_size, NewTypeRecordWithSize(kTypeInt, kQualPlain),
      location);
  ASTNode* index = NewBinaryASTNode(
      AST_OP(div), NewTypeRecordWithSize(kTypeInt, kQualPlain), location,
      byte_off, psize);
  index = AnalyzeExpression(index);
  ASTNode* slot =
      NewBinaryASTNode(AST_OP(subscript), function_pointer, location, vptr,
                       index);
  slot = AnalyzeExpression(slot);
  return NewUnaryASTNode(AST_OP(contents), function_type, location, slot);
}

static ASTNode* MemberPointerBuildNonvirtualCallee(ASTNode* ptr_field,
                                                   TypeRecord* fn_type,
                                                   ASTNode* object,
                                                   SourceLocation location) {
  TypeRecord* function_type = MemberPointerCopyFunctionType(fn_type);
  TypeRecord* object_type = object != NULL ? object->type : NULL;
  if (object_type != NULL && TypeIsReference(object_type)) {
    object_type = object_type->next;
  }
  if (object_type != NULL) {
    TypeRecord* this_type = NewPointerTo(kQualPlain, TypeRecordCopy(object_type));
    Symbol* this_param = NewSymbol("this", this_type, STO(auto));
    this_param->flags.is_argument = true;
    this_param->value.arg_number = 0;
    if (function_type->info.function.prototype.length == 0) {
      VectorAppend(&function_type->info.function.prototype, this_param);
    } else {
      VectorInsertBefore(&function_type->info.function.prototype, 0, this_param);
      for (size_t i = 1; i < function_type->info.function.prototype.length; i++) {
        Symbol* formal =
            (Symbol*)function_type->info.function.prototype.value.p[i];
        if (formal != NULL) {
          formal->value.arg_number = (int)i;
        }
      }
    }
  }
  TypeRecord* function_pointer = NewPointerTo(kQualPlain, function_type);
  ASTNode* fn_addr =
      NewCastASTNode(function_pointer, location, ptr_field);
  fn_addr = AnalyzeExpression(fn_addr);
  return NewUnaryASTNode(AST_OP(contents), function_type, location, fn_addr);
}

static void MemberPointerRenumberCallChildren(VectorASTNode* node) {
  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* child = (ASTNode*)node->children->value.p[i];
    child->parent = &node->base;
    child->child_id = (int)i;
  }
}

bool MemberPointerLowerRuntimeFunctionCall(VectorASTNode* call,
                                           ASTNode* receiver,
                                           bool receiver_is_pointer,
                                           ASTNode* member_ptr,
                                           TypeRecord* member_ptr_type) {
  TypeRecord* fn_type = TypeMemberPointerPointeeType(member_ptr_type);
  if (fn_type == NULL || !TypeIsFunction(fn_type)) {
    return false;
  }
  SourceLocation location = call->base.location;

  // The dotstar/arrowstar node is replaced below.  Detach both operands before
  // deleting that node so the callee and implicit-this expressions do not keep
  // references into the deleted subtree.
  receiver = ASTNodeMove(receiver);
  member_ptr = ASTNodeMove(member_ptr);
  member_ptr = MemberPointerMaterializePrvalue(&compiler->syntax, location,
                                               member_ptr, member_ptr_type);

  ASTNode* object = receiver;
  if (receiver_is_pointer) {
    object = NewUnaryASTNode(AST_OP(contents), receiver->type->next, location,
                             receiver);
    object = AnalyzeExpression(object);
    receiver_is_pointer = false;
  }

  ASTNode* object_addr = object;
  if (!TypeIsPointer(object->type)) {
    object_addr = NewUnaryASTNode(AST_OP(address), NULL, location, object);
    object_addr = AnalyzeExpression(object_addr);
  }
  object_addr = MemberPointerAdjustReceiverToOwner(
      object_addr, member_ptr_type, location);

  ASTNode* ptr_field =
      MemberPointerBuildPtrFieldLoad(member_ptr, member_ptr_type, location);
  ASTNode* adj_field =
      MemberPointerBuildAdjFieldLoad(member_ptr, member_ptr_type, location);

  ASTNode* this_addr = object_addr;
  if (MemberPointerUsesPairLayout(member_ptr_type)) {
    ASTNode* adj = adj_field;
    if (MemberPointerTargetABI() == kMemberPointerABIArmEabi) {
      adj = NewBinaryASTNode(
          AST_OP(rshiftl), NewTypeRecordWithSize(kTypeInt, kQualPlain), location,
          adj_field,
          NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                location));
      adj = AnalyzeExpression(adj);
    }
    this_addr = MemberPointerAddByteOffset(object_addr, adj, location);
  }

  MemberPointerValue pm_value;
  bool have_const =
      MemberPointerTryEvaluateConstant(member_ptr, member_ptr_type, &pm_value);
  bool const_is_virtual = false;
  if (have_const) {
    const_is_virtual =
        MemberPointerIsVirtualFunctionEncoding(member_ptr_type, &pm_value);
  }

  bool receiver_is_polymorphic = false;
  if (object->type != NULL) {
    TypeRecord* receiver_class_type = object->type;
    if (TypeIsReference(receiver_class_type)) {
      receiver_class_type = receiver_class_type->next;
    }
    if (TypeIsStructOrUnion(receiver_class_type) &&
        receiver_class_type->info.struct_info != NULL) {
      receiver_is_polymorphic =
          receiver_class_type->info.struct_info->vtable_symbol != NULL;
    }
  }

  ASTNode* callee = NULL;
  if (have_const && const_is_virtual) {
    callee = MemberPointerBuildVirtualCallee(this_addr, /*receiver_is_pointer=*/true,
                                             ptr_field, fn_type, location);
  } else if (have_const && pm_value.fn_symbol != NULL) {
    callee = NewIdentifierASTNode(pm_value.fn_symbol, location);
    callee = AnalyzeExpression(callee);
  } else if (have_const || !receiver_is_polymorphic) {
    callee = MemberPointerBuildNonvirtualCallee(ptr_field, fn_type, object,
                                                location);
  } else {
    ASTNode* virtual_callee =
        MemberPointerBuildVirtualCallee(this_addr, /*receiver_is_pointer=*/true,
                                        ptr_field, fn_type, location);
    ASTNode* nonvirtual_callee =
        MemberPointerBuildNonvirtualCallee(ptr_field, fn_type, object, location);
    ASTNode* one =
        NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location);
    ASTNode* is_virtual_flag;
    if (MemberPointerTargetABI() == kMemberPointerABIArmEabi) {
      is_virtual_flag = NewBinaryASTNode(
          AST_OP(and), NewTypeRecordWithSize(kTypeInt, kQualPlain), location,
          adj_field, one);
    } else {
      is_virtual_flag = NewBinaryASTNode(
          AST_OP(and), NewTypeRecordWithSize(kTypeInt, kQualPlain), location,
          ptr_field, one);
    }
    is_virtual_flag = AnalyzeExpression(is_virtual_flag);
    ASTNode* colon = NewBinaryASTNode(AST_OP(colon), fn_type, location,
                                      virtual_callee, nonvirtual_callee);
    ASTNode* cond_expr = NewBinaryASTNode(AST_OP(question), fn_type, location,
                                          is_virtual_flag, colon);
    callee = AnalyzeExpression(cond_expr);
  }

  ASTNode* old_left = call->left;
  call->left = callee;
  call->left->parent = &call->base;
  call->left->child_id = 0;
  ASTNodeDelete(old_left);

  Symbol* this_sym = NULL;
  if (fn_type->info.function.prototype.length > 0) {
    Symbol* first = fn_type->info.function.prototype.value.p[0];
    if (first != NULL && StringEqual(&first->name, "this")) {
      this_sym = first;
    }
  }
  TypeRecord* object_type = object->type;
  if (object_type != NULL && TypeIsReference(object_type)) {
    object_type = object_type->next;
  }
  TypeRecord* this_type = this_sym != NULL ? this_sym->type
                        : object_type != NULL
                              ? NewPointerTo(kQualPlain, TypeRecordCopy(object_type))
                              : this_addr->type;
  if (TypeIsReference(this_type)) {
    this_addr = NewUnaryASTNode(AST_OP(contents), this_type->next, location,
                                this_addr);
    this_addr = AnalyzeExpression(this_addr);
  } else if (TypeIsPointer(this_type) && !TypeIsPointer(this_addr->type)) {
    this_addr = NewCastASTNode(this_type, location, this_addr);
    this_addr = AnalyzeExpression(this_addr);
  }

  if (call->children->length == 0) {
    VectorAppend(call->children, this_addr);
  } else {
    VectorInsertBefore(call->children, 0, this_addr);
  }
  MemberPointerRenumberCallChildren(call);
  return true;
}

ASTNode* MemberPointerBuildConversion(ASTNode* expr, TypeRecord* from_type,
                                      TypeRecord* to_type, bool is_cast) {
  if (expr == NULL || from_type == NULL || to_type == NULL ||
      TypeEqual(from_type, to_type)) {
    return expr;
  }
  MemberPointerValue value;
  if (MemberPointerTryEvaluateConstant(expr, from_type, &value)) {
    MemberPointerValue converted;
    if (!MemberPointerConvertValue(from_type, to_type, &value, &converted)) {
      return expr;
    }
    if (expr->op == AST_OP(member_ptr)) {
      ASTNode* clone =
          ASTNodeClone(expr, MemberPointerIdentityClone, NULL, NULL);
      ASTNodeSetType(clone, to_type);
      return clone;
    }
    if (MemberPointerValueIsNull(from_type, &value)) {
      ASTNodeSetType(expr, to_type);
      return expr;
    }
    // A local/member-pointer identifier may have a known initializer, but its
    // stored runtime value still has the source representation.  Do not merely
    // retag that identifier with the destination type; emit the adjustment
    // below so the converted value is actually written.
  }

  if (!MemberPointerCanConvert(from_type, to_type, is_cast, NULL)) {
    return expr;
  }

  Struct* from_class = TypeMemberPointerClass(from_type);
  Struct* to_class = TypeMemberPointerClass(to_type);
  if (from_class == NULL || to_class == NULL || from_class == to_class) {
    ASTNodeSetType(expr, to_type);
    return expr;
  }

  TypeRecord* from_class_rec = MemberPointerClassTypeRecord(from_class);
  TypeRecord* to_class_rec = MemberPointerClassTypeRecord(to_class);
  int sign = MemberPointerConversionSign(from_class_rec, to_class_rec);
  CXXBaseAdjustment adjustment;
  bool have_adjustment = false;
  if (sign < 0) {
    have_adjustment =
        TypeBaseAdjustment(from_class_rec, to_class_rec, /*public_only=*/true,
                           &adjustment);
  } else if (sign > 0) {
    have_adjustment =
        TypeBaseAdjustment(to_class_rec, from_class_rec, /*public_only=*/false,
                           &adjustment);
  }
  TypeRecordDelete(from_class_rec);
  TypeRecordDelete(to_class_rec);
  if (!have_adjustment) {
    ASTNodeSetType(expr, to_type);
    return expr;
  }

  SourceLocation location = expr->location;
  int delta = sign * adjustment.byte_offset;
  if (delta == 0) {
    ASTNodeSetType(expr, to_type);
    return expr;
  }
  Symbol* temporary = SyntaxNewTemporary(&compiler->syntax, from_type);
  ASTNode* init_destination = NewIdentifierASTNode(temporary, location);
  ASTNode* init = NewBinaryASTNode(AST_OP(assign), from_type, location,
                                   init_destination, expr);
  init = AnalyzeExpression(init);
  if (TypeIsMemberFunctionPointer(to_type)) {
    ASTNode* materialized = NewIdentifierASTNode(temporary, location);
    materialized = AnalyzeExpression(materialized);
    ASTNode* adj = MemberPointerBuildAdjFieldLoad(materialized, from_type,
                                                  location);
    if (MemberPointerTargetABI() == kMemberPointerABIArmEabi) {
      ASTNode* delta_node = NewIntConstantASTNode(
          (int64_t)delta << 1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
          location);
      adj = NewBinaryASTNode(AST_OP(plus), adj->type, location, adj, delta_node);
    } else {
      ASTNode* delta_node = NewIntConstantASTNode(
          delta, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
      adj = NewBinaryASTNode(AST_OP(plus), adj->type, location, adj, delta_node);
    }
    adj = AnalyzeExpression(adj);
    ASTNode* stored_object = NewIdentifierASTNode(temporary, location);
    stored_object = AnalyzeExpression(stored_object);
    ASTNode* addr =
        NewUnaryASTNode(AST_OP(address), NULL, location, stored_object);
    addr = AnalyzeExpression(addr);
    addr = NewCastASTNode(MemberPointerCharPointerType(), location, addr);
    addr = AnalyzeExpression(addr);
    ASTNode* adj_addr = NewBinaryASTNode(
        AST_OP(plus), addr->type, location, addr,
        NewIntConstantASTNode(MemberPointerAdjFieldOffset(from_type),
                              NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location));
    adj_addr = AnalyzeExpression(adj_addr);
    adj_addr = NewCastASTNode(
        NewPointerTo(kQualPlain, MemberPointerWordType()), location, adj_addr);
    adj_addr = AnalyzeExpression(adj_addr);
    ASTNode* store = NewBinaryASTNode(AST_OP(assign), adj->type, location,
                                      NewUnaryASTNode(AST_OP(contents), adj->type,
                                                      location, adj_addr),
                                      adj);
    store = AnalyzeExpression(store);
    ASTNode* converted = NewIdentifierASTNode(temporary, location);
    converted = AnalyzeExpression(converted);
    ASTNodeSetType(converted, to_type);
    ASTNode* update_then_value = NewBinaryASTNode(
        AST_OP(comma), to_type, location, store, converted);
    update_then_value = AnalyzeExpression(update_then_value);
    ASTNode* result = NewBinaryASTNode(AST_OP(comma), to_type, location, init,
                                       update_then_value);
    result = AnalyzeExpression(result);
    ASTNodeSetType(result, to_type);
    return result;
  }

  ASTNode* materialized = NewIdentifierASTNode(temporary, location);
  materialized = AnalyzeExpression(materialized);
  ASTNode* ptr = MemberPointerBuildPtrFieldLoad(materialized, from_type, location);
  ASTNode* delta_node = NewIntConstantASTNode(
      delta, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  ASTNode* adjusted =
      NewBinaryASTNode(AST_OP(plus), ptr->type, location, ptr, delta_node);
  adjusted = AnalyzeExpression(adjusted);

  ASTNode* null_source = NewIdentifierASTNode(temporary, location);
  null_source = AnalyzeExpression(null_source);
  ASTNode* null_ptr =
      MemberPointerBuildPtrFieldLoad(null_source, from_type, location);
  MemberPointerValue null_value;
  MemberPointerEncodeNull(from_type, &null_value);
  ASTNode* null_constant =
      NewIntConstantASTNode(null_value.ptr, null_ptr->type, location);
  ASTNode* is_null = NewBinaryASTNode(AST_OP(equal), NULL, location, null_ptr,
                                      null_constant);
  is_null = AnalyzeExpression(is_null);

  ASTNode* preserved_null = NewIdentifierASTNode(temporary, location);
  preserved_null = AnalyzeExpression(preserved_null);
  ASTNodeSetType(preserved_null, to_type);
  ASTNodeSetType(adjusted, to_type);
  ASTNode* alternatives = NewBinaryASTNode(AST_OP(colon), to_type, location,
                                            preserved_null, adjusted);
  ASTNode* converted = NewBinaryASTNode(AST_OP(question), to_type, location,
                                         is_null, alternatives);
  converted = AnalyzeExpression(converted);
  ASTNode* result =
      NewBinaryASTNode(AST_OP(comma), to_type, location, init, converted);
  result = AnalyzeExpression(result);
  ASTNodeSetType(result, to_type);
  return result;
}
