//
//  expansion_semantics.h
//  c_compiler
//

#ifndef expansion_semantics_h
#define expansion_semantics_h

#include "ast.h"
#include "map.h"

// True while an expansion statement's initializer or item metadata still
// depends on template substitution or an unresolved pack expansion.
bool ExpansionStatementIsDependent(ExpansionStatementASTNode* node,
                                   Map* symbol_map, Map* pack_symbol_map);

// Lowers a `template for` into a compound of per-element blocks.  Item and
// pack symbols are remapped through symbol_map / pack_symbol_map when cloning
// a template body.  Returns the original node when still dependent; otherwise
// returns a new analyzed compound and deletes the expansion node.
ASTNode* SemanticMaterializeExpansionStatement(
    ExpansionStatementASTNode* node, Map* symbol_map, Map* pack_symbol_map);

// Clone `stmt`, rewriting identifiers that appear in `symbol_map`.
ASTNode* CloneExpansionIterationBody(ASTNode* stmt, Map* symbol_map);

// Expand `{ xs... }` pack elements using pack_symbol_map when present.
bool ExpandExpansionBracedInitializerElements(BracedInitializerASTNode* braced,
                                              Map* pack_symbol_map,
                                              Vector* out_elements,
                                              ASTNode* diagnostic);

#endif /* expansion_semantics_h */
