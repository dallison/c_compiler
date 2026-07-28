//
//  module_syntax.c
//  c_compiler
//

#include "module_syntax.h"

#include "compiler.h"
#include "module_unit.h"

static bool ModuleImportHandlerAvailable(void) {
  return CompilerHasModuleImportHandler();
}

static ModuleUnitInfo* ModuleInfo(void) { return &compiler->module_unit; }

static bool ModuleSyntaxTracksTranslationUnit(Syntax* syntax) {
  if (syntax == NULL || syntax->lex == NULL || syntax->lex->source == NULL) {
    return true;
  }
  return !StringEqual(&syntax->lex->source->filename, "builtin");
}

static ASTNode* EmptyDeclarationList(SourceLocation location) {
  return NewDeclarationListASTNode(NewVector(), location);
}

static void AppendDeclarationsFromNode(Vector* declarations, ASTNode* node) {
  if (node == NULL) {
    return;
  }
  if (node->op != AST_OP(decl_list)) {
    VectorAppend(declarations, node);
    return;
  }

  DeclarationListASTNode* list = (DeclarationListASTNode*)node;
  VectorAppendVector(declarations, list->declarations);
  VectorClear(list->declarations);
}

bool ModuleSyntaxAtContextualKeyword(Syntax* syntax, const char* spelling) {
  return CompilerCXXAtLeast(kLanguageStandardCXX20) &&
         LexLookingAt(syntax->lex, TOK(identifier)) &&
         StringEqual(&syntax->lex->spelling, spelling);
}

bool ModuleSyntaxAllowsExporting(Syntax* syntax) {
  if (!ModuleSyntaxTracksTranslationUnit(syntax)) {
    return true;
  }
  return ModuleUnitAllowsExportDeclarations(ModuleInfo());
}

static bool LookingAtModuleSemicolon(Syntax* syntax) {
  if (!ModuleSyntaxAtContextualKeyword(syntax, "module")) {
    return false;
  }
  LexCheckpoint cp;
  LexCheckpointSave(syntax->lex, &cp);
  LexNextToken(syntax->lex);
  bool ok = LexLookingAt(syntax->lex, TOK(semicolon));
  LexCheckpointRestore(syntax->lex, &cp);
  LexCheckpointDestruct(&cp);
  return ok;
}

static bool LookingAtPrivateModuleFragment(Syntax* syntax) {
  if (!ModuleSyntaxAtContextualKeyword(syntax, "module")) {
    return false;
  }
  LexCheckpoint cp;
  LexCheckpointSave(syntax->lex, &cp);
  LexNextToken(syntax->lex);
  bool ok = LexMatch(syntax->lex, TOK(colon)) &&
            LexLookingAt(syntax->lex, TOK(private));
  LexCheckpointRestore(syntax->lex, &cp);
  LexCheckpointDestruct(&cp);
  return ok;
}

static bool LookingAtNamedModuleDeclaration(Syntax* syntax) {
  if (!ModuleSyntaxAtContextualKeyword(syntax, "module")) {
    return false;
  }
  LexCheckpoint cp;
  LexCheckpointSave(syntax->lex, &cp);
  LexNextToken(syntax->lex);
  bool ok = LexLookingAt(syntax->lex, TOK(identifier));
  LexCheckpointRestore(syntax->lex, &cp);
  LexCheckpointDestruct(&cp);
  return ok;
}

static bool LookingAtModuleImport(Syntax* syntax) {
  if (!ModuleSyntaxAtContextualKeyword(syntax, "import")) {
    return false;
  }
  LexCheckpoint cp;
  LexCheckpointSave(syntax->lex, &cp);
  LexNextToken(syntax->lex);
  bool ok = LexLookingAt(syntax->lex, TOK(identifier)) ||
            LexLookingAt(syntax->lex, TOK(colon)) ||
            LexLookingAt(syntax->lex, TOK(string)) ||
            LexLookingAt(syntax->lex, TOK(less));
  LexCheckpointRestore(syntax->lex, &cp);
  LexCheckpointDestruct(&cp);
  return ok;
}

static bool ParseDottedModuleName(Syntax* syntax, String* out) {
  StringInit(out, "");
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    SyntaxError(syntax, "Expected module name");
    return false;
  }
  StringAppend(out, syntax->lex->spelling.value);
  LexNextToken(syntax->lex);
  while (LexMatch(syntax->lex, TOK(dot))) {
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected identifier after '.' in module name");
      return false;
    }
    StringAppendChar(out, '.');
    StringAppend(out, syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
  }
  return true;
}

static void AppendHeaderNameToken(Lex* lex, String* out) {
  if (LexLookingAt(lex, TOK(identifier))) {
    StringAppendString(out, &lex->spelling);
  } else if (LexLookingAt(lex, TOK(number)) ||
             LexLookingAt(lex, TOK(fnumber))) {
    StringAppendString(out, &lex->literal_spelling);
  } else {
    StringAppend(out, TokenName(lex->current_token));
  }
}

static bool ParseModulePartition(Syntax* syntax, String* partition) {
  StringInit(partition, "");
  if (!LexMatch(syntax->lex, TOK(colon))) {
    return true;
  }
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    SyntaxError(syntax, "Expected module partition name after ':'");
    return false;
  }
  StringAppend(partition, syntax->lex->spelling.value);
  LexNextToken(syntax->lex);
  return true;
}

static bool ParseModuleNameAndPartition(Syntax* syntax, ModuleId* id) {
  ModuleIdInit(id);
  if (!ParseDottedModuleName(syntax, &id->name)) {
    ModuleIdDestruct(id);
    return false;
  }
  if (!ParseModulePartition(syntax, &id->partition)) {
    ModuleIdDestruct(id);
    return false;
  }
  return true;
}

static bool ImportPlacementAllowed(ModuleUnitInfo* info) {
  switch (info->phase) {
    case kModuleParserPhaseStart:
    case kModuleParserPhasePreModule:
    case kModuleParserPhaseImportPreamble:
      return true;
    default:
      return false;
  }
}

static void DiagnoseDisallowedExport(Syntax* syntax) {
  ModuleUnitInfo* info = ModuleInfo();
  if (info->fragment == kModuleFragmentPrivate) {
    SyntaxError(syntax, "Export declarations are not allowed in the "
                        "private module fragment");
  } else if (info->fragment == kModuleFragmentGlobal) {
    SyntaxError(syntax, "Export declarations are not allowed in the global "
                        "module fragment");
  } else if (info->is_module_unit && !ModuleUnitIsInterfaceUnit(info)) {
    SyntaxError(syntax,
                "Export declarations are not allowed in module "
                "implementation units");
  } else {
    SyntaxError(syntax, "Export declarations are not allowed here");
  }
}

static void CloseImportPreamble(ModuleUnitInfo* info) {
  if (info->phase == kModuleParserPhaseImportPreamble) {
    info->phase = kModuleParserPhasePurview;
    info->fragment = kModuleFragmentPurview;
  } else if (info->phase == kModuleParserPhaseStart ||
             info->phase == kModuleParserPhasePreModule) {
    info->phase = kModuleParserPhasePurview;
  }
}

void ModuleSyntaxNoteNonImportDeclaration(Syntax* syntax) {
  if (!ModuleSyntaxTracksTranslationUnit(syntax)) {
    return;
  }
  CloseImportPreamble(ModuleInfo());
}

static void ImportPrimaryInterfaceIfNeeded(Syntax* syntax,
                                           ModuleUnitInfo* info) {
  if (!ModuleSyntaxTracksTranslationUnit(syntax) ||
      info->kind != kModuleUnitKindImplementation ||
      info->id.name.length == 0) {
    return;
  }

  ModuleId primary;
  ModuleIdInit(&primary);
  StringSetString(&primary.name, &info->id.name);
  ModuleUnitAddImport(info, &primary, false, false, false,
                      info->module_declaration_location);

  if (!ModuleImportHandlerAvailable()) {
    ModuleIdDestruct(&primary);
    return;
  }

  String import_name;
  StringInit(&import_name, "");
  ModuleIdFormat(&primary, &import_name);
  if (!CompilerImportModule(import_name.value)) {
    const char* detail = CompilerImportLastError();
    if (detail != NULL && detail[0] != '\0') {
      SyntaxError(syntax, "Cannot import primary module interface '%s': %s",
                  import_name.value, detail);
    } else {
      SyntaxError(syntax, "Cannot import primary module interface '%s'",
                  import_name.value);
    }
  }
  StringDestruct(&import_name);
  ModuleIdDestruct(&primary);
}

static void RecordNamedModuleDeclaration(Syntax* syntax, bool exported,
                                         const ModuleId* id,
                                         SourceLocation location) {
  if (!ModuleSyntaxTracksTranslationUnit(syntax)) {
    return;
  }
  ModuleUnitInfo* info = ModuleInfo();
  if (info->has_module_declaration) {
    SyntaxError(syntax,
                "Multiple module declarations in one translation unit");
    return;
  }
  switch (info->phase) {
    case kModuleParserPhaseStart:
    case kModuleParserPhaseGlobalFragment:
      break;
    case kModuleParserPhasePreModule:
      SyntaxError(syntax,
                  "Module declaration cannot appear after import "
                  "declarations");
      return;
    default:
      if (info->imports.length > 0 && !info->has_module_declaration) {
        SyntaxError(syntax,
                    "Module declaration cannot appear after import "
                    "declarations");
      } else {
        SyntaxError(syntax,
                    "Module declaration must appear before other declarations");
      }
      return;
  }

  info->has_module_declaration = true;
  info->is_module_unit = true;
  info->module_declaration_location = location;
  ModuleIdCopy(&info->id, id);

  if (id->partition.length > 0) {
    info->kind = exported ? kModuleUnitKindInterfacePartition
                          : kModuleUnitKindInternalPartition;
  } else if (exported) {
    info->kind = kModuleUnitKindPrimaryInterface;
  } else {
    info->kind = kModuleUnitKindImplementation;
  }

  info->phase = kModuleParserPhaseImportPreamble;
  info->fragment = kModuleFragmentPurview;
  ImportPrimaryInterfaceIfNeeded(syntax, info);
}

static void RecordGlobalModuleFragment(Syntax* syntax, SourceLocation location) {
  if (!ModuleSyntaxTracksTranslationUnit(syntax)) {
    return;
  }
  ModuleUnitInfo* info = ModuleInfo();
  if (info->saw_global_module_fragment) {
    SyntaxError(syntax,
                "Global module fragment declaration appears more than once");
    return;
  }
  if (info->has_module_declaration) {
    SyntaxError(syntax,
                "Global module fragment must precede the module declaration");
    return;
  }
  if (info->phase != kModuleParserPhaseStart) {
    SyntaxError(syntax,
                "Global module fragment must be the first declaration");
    return;
  }
  info->saw_global_module_fragment = true;
  info->phase = kModuleParserPhaseGlobalFragment;
  info->fragment = kModuleFragmentGlobal;
  (void)location;
}

static void RecordPrivateModuleFragment(Syntax* syntax, SourceLocation location) {
  if (!ModuleSyntaxTracksTranslationUnit(syntax)) {
    return;
  }
  ModuleUnitInfo* info = ModuleInfo();
  if (!info->has_module_declaration) {
    SyntaxError(syntax,
                "Private module fragment requires a preceding module "
                "declaration");
    return;
  }
  if (info->phase == kModuleParserPhasePrivateFragment) {
    SyntaxError(syntax,
                "Private module fragment declaration appears more than once");
    return;
  }
  if (info->phase != kModuleParserPhasePurview &&
      info->phase != kModuleParserPhaseImportPreamble) {
    SyntaxError(syntax,
                "Private module fragment must follow the module purview");
    return;
  }
  if (!ModuleUnitIsPrimaryInterfaceUnit(info)) {
    SyntaxError(syntax,
                "Private module fragment is only permitted in a primary "
                "module interface unit");
    return;
  }
  info->phase = kModuleParserPhasePrivateFragment;
  info->fragment = kModuleFragmentPrivate;
  (void)location;
}

static void ResolvePartitionImportName(const ModuleId* relative,
                                       ModuleId* resolved) {
  ModuleIdInit(resolved);
  if (ModuleInfo()->id.name.length == 0) {
    StringSet(&resolved->name, relative->partition.value);
    return;
  }
  StringSet(&resolved->name, ModuleInfo()->id.name.value);
  StringSet(&resolved->partition, relative->partition.value);
}

static void PerformModuleImport(Syntax* syntax, const ModuleId* import_id,
                                bool is_partition_import, bool is_export_import,
                                bool is_header_unit,
                                SourceLocation location) {
  if (!ModuleSyntaxTracksTranslationUnit(syntax)) {
    return;
  }
  ModuleUnitInfo* info = ModuleInfo();
  if (info->phase == kModuleParserPhaseGlobalFragment) {
    SyntaxError(syntax,
                "Import declaration is not allowed in the global module "
                "fragment");
    return;
  }
  if (!ImportPlacementAllowed(info)) {
    SyntaxError(syntax,
                "Import declaration must appear before other declarations");
    return;
  }
  if (!is_header_unit && is_partition_import && !info->is_module_unit) {
    SyntaxError(syntax,
                "Relative partition import requires a named module unit");
    return;
  }
  if (!is_header_unit && !is_partition_import &&
      import_id->partition.length > 0 &&
      (!info->is_module_unit ||
       !StringEqual(&info->id.name, import_id->name.value))) {
    SyntaxError(syntax,
                "A module partition can only be imported by a unit of the "
                "same named module");
    return;
  }
  if (info->phase == kModuleParserPhaseStart) {
    info->phase = kModuleParserPhasePreModule;
  } else if (info->phase == kModuleParserPhasePreModule) {
    // Additional imports in the non-module import preamble.
  }

  ModuleId resolved;
  ModuleIdInit(&resolved);
  if (is_partition_import) {
    ResolvePartitionImportName(import_id, &resolved);
  } else {
    ModuleIdCopy(&resolved, import_id);
  }
  if (info->is_module_unit &&
      StringEqualString(&resolved.name, &info->id.name) &&
      StringEqualString(&resolved.partition, &info->id.partition)) {
    SyntaxError(syntax, "A module unit cannot import itself");
    ModuleIdDestruct(&resolved);
    return;
  }
  if (!is_header_unit && resolved.partition.length == 0 &&
      StringEqual(&resolved.name, "std") &&
      !CompilerCXXAtLeast(kLanguageStandardCXX23)) {
    SyntaxError(syntax, "The standard library module 'std' requires C++23");
    ModuleIdDestruct(&resolved);
    return;
  }
  ModuleUnitAddImport(info, &resolved, is_partition_import, is_export_import,
                      is_header_unit, location);

  String import_name;
  StringInit(&import_name, "");
  ModuleIdFormat(&resolved, &import_name);
  if (!CompilerImportModule(import_name.value)) {
    const char* detail = CompilerImportLastError();
    if (detail != NULL && detail[0] != '\0') {
      SyntaxError(syntax, "Cannot import module '%s': %s", import_name.value,
                  detail);
    } else {
      SyntaxError(syntax, "Cannot import module '%s'", import_name.value);
    }
  }
  StringDestruct(&import_name);
  ModuleIdDestruct(&resolved);
}

static ASTNode* ParseGlobalModuleFragment(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // contextual `module`
  RecordGlobalModuleFragment(syntax, location);
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

static ASTNode* ParsePrivateModuleFragment(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // contextual `module`
  if (!LexMatch(syntax->lex, TOK(colon))) {
    SyntaxError(syntax, "Expected ':' after 'module' in private fragment");
  } else if (!LexMatch(syntax->lex, TOK(private))) {
    SyntaxError(syntax, "Expected 'private' after 'module :'");
  }
  RecordPrivateModuleFragment(syntax, location);
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

static ASTNode* ParseNamedModuleDeclaration(Syntax* syntax, bool exported) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // contextual `module`
  ModuleId id;
  if (ParseModuleNameAndPartition(syntax, &id)) {
    RecordNamedModuleDeclaration(syntax, exported, &id, location);
  }
  ModuleIdDestruct(&id);
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

static ASTNode* ParseImportDeclaration(Syntax* syntax, bool is_export_import) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // contextual `import`
  ModuleId id;
  bool is_partition_import = false;
  bool is_header_unit = false;
  bool valid_import = true;
  if (LexLookingAt(syntax->lex, TOK(string))) {
    is_header_unit = true;
    ModuleIdInit(&id);
    StringAppendChar(&id.name, '"');
    StringAppendString(&id.name, &syntax->lex->spelling);
    StringAppendChar(&id.name, '"');
    LexNextToken(syntax->lex);
  } else if (LexMatch(syntax->lex, TOK(less))) {
    is_header_unit = true;
    ModuleIdInit(&id);
    StringAppendChar(&id.name, '<');
    while (!LexEof(syntax->lex) &&
           !LexLookingAt(syntax->lex, TOK(greater))) {
      AppendHeaderNameToken(syntax->lex, &id.name);
      LexNextToken(syntax->lex);
    }
    if (!LexMatch(syntax->lex, TOK(greater))) {
      SyntaxError(syntax, "Expected '>' after header-unit name");
      valid_import = false;
    }
    StringAppendChar(&id.name, '>');
  } else if (LexMatch(syntax->lex, TOK(colon))) {
    is_partition_import = true;
    ModuleIdInit(&id);
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected module partition name after 'import :'");
      valid_import = false;
    } else {
      StringSetString(&id.partition, &syntax->lex->spelling);
      LexNextToken(syntax->lex);
    }
  } else if (ParseModuleNameAndPartition(syntax, &id)) {
    is_partition_import = false;
  } else {
    ModuleIdInit(&id);
    valid_import = false;
  }
  if (valid_import) {
    PerformModuleImport(syntax, &id, is_partition_import, is_export_import,
                        is_header_unit, location);
  }
  ModuleIdDestruct(&id);
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

ASTNode* ModuleSyntaxParseExportDeclaration(Syntax* syntax) {
  if (!ModuleSyntaxTracksTranslationUnit(syntax)) {
    LexNextToken(syntax->lex);
    syntax->export_depth++;
    ASTNode* result = SyntaxParseExternalDeclaration(syntax);
    syntax->export_depth--;
    return result;
  }

  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // `export`

  if (ModuleSyntaxAtContextualKeyword(syntax, "module")) {
    if (LookingAtModuleSemicolon(syntax)) {
      SyntaxError(syntax, "Cannot export the global module fragment");
      return ParseGlobalModuleFragment(syntax);
    }
    if (LookingAtPrivateModuleFragment(syntax)) {
      SyntaxError(syntax, "Cannot export the private module fragment");
      return ParsePrivateModuleFragment(syntax);
    }
    return ParseNamedModuleDeclaration(syntax, /*exported=*/true);
  }
  if (ModuleSyntaxAtContextualKeyword(syntax, "import")) {
    if (!ModuleSyntaxAllowsExporting(syntax)) {
      DiagnoseDisallowedExport(syntax);
    }
    return ParseImportDeclaration(syntax, /*is_export_import=*/true);
  }

  if (!ModuleSyntaxAllowsExporting(syntax)) {
    DiagnoseDisallowedExport(syntax);
  }

  syntax->export_depth++;
  ASTNode* result;
  if (LexMatch(syntax->lex, TOK(lbrace))) {
    Vector* declarations = NewVector();
    while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rbrace))) {
      ASTNode* node = SyntaxParseExternalDeclaration(syntax);
      AppendDeclarationsFromNode(declarations, node);
    }
    SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebrace) | TC(decl));
    result = NewDeclarationListASTNode(declarations, location);
  } else {
    result = SyntaxParseExternalDeclaration(syntax);
  }
  syntax->export_depth--;
  return result;
}

ASTNode* ModuleSyntaxParseExternalDeclaration(Syntax* syntax) {
  if (!ModuleSyntaxTracksTranslationUnit(syntax)) {
    return NULL;
  }
  if (LookingAtModuleSemicolon(syntax)) {
    return ParseGlobalModuleFragment(syntax);
  }
  if (LookingAtPrivateModuleFragment(syntax)) {
    return ParsePrivateModuleFragment(syntax);
  }
  if (LookingAtNamedModuleDeclaration(syntax)) {
    return ParseNamedModuleDeclaration(syntax, /*exported=*/false);
  }
  if (LookingAtModuleImport(syntax)) {
    return ParseImportDeclaration(syntax, /*is_export_import=*/false);
  }
  return NULL;
}
