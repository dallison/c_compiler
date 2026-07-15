//
//  module_syntax.h
//  c_compiler
//
//  C++20 module declaration parsing and module-unit state machine.
//

#ifndef module_syntax_h
#define module_syntax_h

#include "ast.h"
#include "syntax.h"

bool ModuleSyntaxAtContextualKeyword(Syntax* syntax, const char* spelling);
bool ModuleSyntaxAllowsExporting(Syntax* syntax);
void ModuleSyntaxNoteNonImportDeclaration(Syntax* syntax);
ASTNode* ModuleSyntaxParseExportDeclaration(Syntax* syntax);
ASTNode* ModuleSyntaxParseExternalDeclaration(Syntax* syntax);

#endif /* module_syntax_h */
