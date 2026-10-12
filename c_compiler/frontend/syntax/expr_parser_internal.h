//
//  expr_parser_internal.h
//  c_compiler
//
//  Declarations shared by the expression-parser feature files.
//

#ifndef expr_parser_internal_h
#define expr_parser_internal_h

#include "expr_parser.h"

// One entry in the builtin/intrinsic table. The table itself lives in
// expr_parser_intrinsic.c; this forward declaration lets other expression
// parsers ask whether a name is a builtin without seeing the table.
struct Intrinsic;

// See expr_parser_member.c. NULL outside a lambda body.
extern Symbol* lambda_enclosing_this;

// Binary operator precedences, loosest first.  Each of these was a function
// that called the next tighter one, so an expression entered eleven frames deep
// before it looked at its first operand -- and a parenthesized subexpression
// paid that again for every level of nesting.
typedef enum {
  kBinaryPrecedenceNone = 0,
  kBinaryPrecedenceLogicalOr,
  kBinaryPrecedenceLogicalAnd,
  kBinaryPrecedenceInclusiveOr,
  kBinaryPrecedenceExclusiveOr,
  kBinaryPrecedenceAnd,
  kBinaryPrecedenceEquality,
  kBinaryPrecedenceRelational,
  kBinaryPrecedenceCompare,
  kBinaryPrecedenceShift,
  kBinaryPrecedenceAdditive,
  kBinaryPrecedenceMultiplicative,
} BinaryPrecedence;

typedef struct {
  Syntax* syntax;
  bool found;
} CXXPackExpressionSearch;


ASTNode* ParseCXXCoYieldExpression(Syntax* syntax, TokenClass followers);
ASTNode* ParseCastExpression(Syntax* syntax, TokenClass followers);
ASTNode* ParsePostfixExpression(Syntax* syntax, TokenClass followers);
bool CXXExpressionContainsParameterPack(Syntax* syntax, ASTNode* node);
bool CXXTemplateArgumentReferencesParameterPack( CXXPackExpressionSearch* search, TemplateArgument* argument);
bool LookingAtCXXPackIndexSuffix(Syntax* syntax);
void MarkCXXPackExpansionIfPresent(Syntax* syntax, ASTNode* actual);
bool MemberPointerOperatorPrecedesFoldEllipsis(Syntax* syntax);
ASTNode* ParseCXXPackIndexExpression(Syntax* syntax, ASTNode* pack, TokenClass followers);
ASTNode* TryParseCXXFoldExpression(Syntax* syntax, TokenClass followers);
ASTNode* BitCastIntrinsic(Syntax* syntax, ASTNode* left, TokenClass followers);
const struct Intrinsic* GetIntrinsic(const char* name);
bool IntrinsicAvailableOnCurrentTarget( const struct Intrinsic* intrinsic);
bool IsBuiltinCallName(const char* name);
ASTNode* TypeTraitIntrinsic(Syntax* syntax, ASTNode* left, TokenClass followers);
ASTNode* VarargsIntrinsic(Syntax* syntax, ASTNode* left, TokenClass followers);
ASTNode* ParseCXXLambdaExpression(Syntax* syntax, TokenClass followers);
bool IsCXXSizeLiteralSuffix(String* suffix);
ASTNode* ParseCXXUserDefinedFloatingLiteral(Syntax* syntax);
ASTNode* ParseCXXUserDefinedIntegerLiteral(Syntax* syntax);
ASTNode* ParseCharacterConstant(Syntax* syntax, TokenClass followers);
ASTNode* ParseFloatingPointConstant(Syntax* syntax, TokenClass followers);
ASTNode* ParseIntegerConstant(Syntax* syntax, TokenClass followers);
ASTNode* ParseStringLiteral(Syntax* syntax, TokenClass followers);
ASTNode* ParseWideCharacterConstant(Syntax* syntax, TokenClass followers);
ASTNode* ParseWideStringLiteral(Syntax* syntax, TokenClass followers);
Symbol* CurrentClassSelfTagSymbol(Syntax* syntax, FullyQualifiedIdentifier* name);
Symbol* CurrentClassSpecializationPrimary(FullyQualifiedIdentifier* name, Struct* str);
Symbol* FindThisSymbol(Syntax* syntax);
ASTNode* NewMemberAccessFromEnclosingClass( Syntax* syntax, FullyQualifiedIdentifier* name);
ASTNode* NewMemberAccessFromThis(Syntax* syntax, FullyQualifiedIdentifier* name, bool allow_unresolved_member);
ASTNode* NewQualifiedBaseMemberAccessFromThis( Syntax* syntax, FullyQualifiedIdentifier* name);
ASTNode* NewStaticMemberReference(Syntax* syntax, FullyQualifiedIdentifier* name);
ASTNode* ParseStructMember(ASTNode* left, ASTOpcode op, Syntax* syntax, TokenClass followers);
ASTNode* ParseThisExpression(Syntax* syntax);
Struct* StructFromThisSymbol(Symbol* this_symbol);
Symbol* ThisSymbolForUnqualifiedMember( Syntax* syntax, FullyQualifiedIdentifier* name, bool allow_unresolved_member);
bool CXXNewConstructorSetHasInitializerList(StructMember* ctor);
StructMember* FindCXXConstructorForType(TypeRecord* type);
ASTNode* ParseCXXDeleteExpression(Syntax* syntax, TokenClass followers, bool global_scope);
ASTNode* ParseCXXNewExpression(Syntax* syntax, TokenClass followers, bool global_scope);
Vector* ParseCXXNewInitializerArguments(Syntax* syntax, Token open, TokenClass followers);
ASTNode* ParseAssignmentExpression(Syntax* syntax, TokenClass followers);
ASTNode* ParseBinaryExpressionOperands(Syntax* syntax, TokenClass followers, BinaryPrecedence min_precedence, bool constraint);
ASTNode* ParseConditionalExpression(Syntax* syntax, TokenClass followers);
ASTNode* ParsePointerToMember(Syntax* syntax, TokenClass followers);
ASTNode* ParsePossiblePreprocessorFunction(Syntax* syntax, TokenClass followers);
ASTNode* ParseReflectionExpression(Syntax* syntax, TokenClass followers, SourceLocation location);
ASTNode* ParseTokenSequenceLiteral(Syntax* syntax, TokenClass followers, SourceLocation location);
bool CXXExpressionNamesTemplateTypeParameter(ASTNode* node);
ASTNode* ParseCXXDependentValueInitialization(ASTNode* type_expr, Syntax* syntax, TokenClass followers);
ASTNode* ParseIdentifier(Syntax* syntax, TokenClass followers);

#endif /* expr_parser_internal_h */
