#ifndef long_double_codegen_h
#define long_double_codegen_h

#include "codegen.h"
#include "ir.h"
#include "type.h"

int LongDoubleRuntimeFormat(void);
IRNode* LongDoubleObjectAddress(Generator* gen, IRNode* value,
                                TypeRecord* type);
IRNode* LoadLongDoubleFromAddress(Generator* gen, IRNode* addr,
                                  TypeRecord* type);
IRNode* StoreLongDoubleToAddress(Generator* gen, IRNode* dest_addr,
                                 IRNode* value, TypeRecord* type);
IRNode* GenerateLongDoubleConstant(Generator* gen, TypeRecord* type,
                                   double value);
IRNode* LongDoubleBinaryValues(Generator* gen, const char* helper,
                               IRNode* left, IRNode* right, TypeRecord* type);
IRNode* LongDoubleCompareValues(Generator* gen, IRNode* left, IRNode* right,
                                TypeRecord* type);
IRNode* LongDoubleAbsoluteValue(Generator* gen, IRNode* value,
                                TypeRecord* type);
IRNode* GenerateLongDoubleBinary(Generator* gen, BinaryASTNode* node);
IRNode* GenerateLongDoubleNegate(Generator* gen, IRNode* operand,
                                 TypeRecord* type);
IRNode* ConvertValueToLongDouble(Generator* gen, IRNode* value,
                                 TypeRecord* from, TypeRecord* to);
IRNode* ConvertValueFromLongDouble(Generator* gen, IRNode* value,
                                   TypeRecord* from, TypeRecord* to);

#endif
