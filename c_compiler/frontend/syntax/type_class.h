//
//  type_class.h
//  c_compiler
//

#ifndef type_class_h
#define type_class_h

#include "type_parse.h"

Symbol* TypeParserParseStruct(TypeParser* parser, bool is_union, bool is_class);
Symbol* TypeParserParseCXXSpecialMemberDeclarator(TypeParser* parser);
void TypeEnsureCXXDeductionGuides(Symbol* class_template);

#endif /* type_class_h */
