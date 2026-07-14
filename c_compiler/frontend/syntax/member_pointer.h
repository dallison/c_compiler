//
//  member_pointer.h
//  c_compiler
//
//  Platform-compatible C++ pointer-to-member representation and helpers.
//

#ifndef member_pointer_h
#define member_pointer_h

#include "type_defs.h"
#include "source.h"
#include "ast.h"

// ABI families for pointer-to-member values.
typedef enum {
  kMemberPointerABIItanium,  // x86_64, AArch64, RISC-V
  kMemberPointerABIArmEabi,  // ARM32
  kMemberPointerABIPCode,    // DaveCC p-code (documented Itanium-like 64-bit)
  kMemberPointerABI6502,     // DaveCC 6502 (documented 16-bit word layout)
} MemberPointerABI;

// Encoded pointer-to-member value (ptr/adj fields in the target ABI sense).
typedef struct MemberPointerValue {
  int64_t ptr;
  int64_t adj;
  struct Symbol* fn_symbol;  // Non-virtual member function; relocatable address.
} MemberPointerValue;

MemberPointerABI MemberPointerTargetABI(void);

bool TypeIsMemberDataPointer(TypeRecord* type);
bool TypeIsMemberFunctionPointer(TypeRecord* type);
bool TypeIsMemberPointerScalar(TypeRecord* type);
bool TypeIsMemberPointerAggregate(TypeRecord* type);
Struct* TypeMemberPointerClass(TypeRecord* type);
TypeRecord* TypeMemberPointerPointeeType(TypeRecord* type);

int MemberPointerSize(TypeRecord* type);
int MemberPointerPtrFieldOffset(TypeRecord* type);
int MemberPointerAdjFieldOffset(TypeRecord* type);
int MemberPointerNullDataOffset(void);
int MemberPointerNullFunctionPtr(void);

bool MemberPointerUsesPairLayout(TypeRecord* type);

bool MemberPointerEncodeFromMember(StructMember* member, Struct* class_info,
                                   MemberPointerValue* out);
bool MemberPointerEncodeNull(TypeRecord* type, MemberPointerValue* out);
bool MemberPointerValueIsNull(TypeRecord* type, const MemberPointerValue* value);
bool MemberPointerValuesEqual(TypeRecord* type, const MemberPointerValue* a,
                              const MemberPointerValue* b);

bool MemberPointerCanConvert(TypeRecord* from, TypeRecord* to, bool is_cast,
                             bool* rejects_virtual_base);
bool MemberPointerConvertValue(TypeRecord* from_type, TypeRecord* to_type,
                               const MemberPointerValue* from,
                               MemberPointerValue* to);

bool MemberPointerTryEvaluateConstant(ASTNode* node, TypeRecord* type,
                                      MemberPointerValue* out);
StructMember* MemberPointerReferencedMember(ASTNode* node);

void MemberPointerEmitStaticInitializers(TypeRecord* type,
                                         const MemberPointerValue* value,
                                         int dest_offset, Vector* initializers);

ASTNode* MemberPointerMaterializePrvalue(struct Syntax* syntax,
                                                SourceLocation location,
                                                ASTNode* member_ptr,
                                                TypeRecord* type);

ASTNode* MemberPointerApplyDataAccess(struct Syntax* syntax,
                                             SourceLocation location,
                                             ASTNode* receiver,
                                             bool receiver_is_pointer,
                                             ASTNode* member_ptr,
                                             TypeRecord* result_type);

ASTNode* MemberPointerBuildConversion(ASTNode* expr,
                                             TypeRecord* from_type,
                                             TypeRecord* to_type,
                                             bool is_cast);

bool MemberPointerLowerRuntimeFunctionCall(VectorASTNode* call,
                                           ASTNode* receiver,
                                           bool receiver_is_pointer,
                                           ASTNode* member_ptr,
                                           TypeRecord* member_ptr_type);

#endif  // member_pointer_h
