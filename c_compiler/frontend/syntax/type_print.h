//
//  type_print.h
//  c_compiler
//

#ifndef type_print_h
#define type_print_h

#include "type_defs.h"

void TypeRecordPrint(TypeRecord* record, FILE* fp);
void TypeRecordPrintDetails(TypeRecord* record, bool with_function_body, FILE* fp);
void TypeRecordToString(TypeRecord* type, String* result);
void TypeRecordFunctionPrettyName(TypeRecord* func, String* result);
void SymbolFunctionPrettyName(Symbol* symbol, String* result);
void SymbolFunctionDiagnosticSuffix(Symbol* symbol, String* result);
void SymbolFunctionDiagnosticName(Symbol* symbol, String* result);

#endif /* type_print_h */
