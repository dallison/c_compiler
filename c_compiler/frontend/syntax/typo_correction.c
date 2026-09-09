//
//  typo_correction.c
//  c_compiler
//
//  Identifier typo suggestions.  When lookup fails, nearby names in the same
//  lookup set are ranked by Levenshtein distance and a unique closest match
//  within clang's (len+2)/3 bound is offered as "did you mean ...?".
//

#include "typo_correction.h"

#include <strings.h>
#include <string.h>

#include "compiler.h"
#include "type.h"

enum { kTypoMaxIdentifier = 256 };

typedef struct TypoSearch {
  const char* typo;
  size_t typo_len;
  unsigned max_dist;
  const char* best;
  unsigned best_dist;
  int ties;
} TypoSearch;

static unsigned MaxEditDistance(size_t typo_len) {
  return (unsigned)((typo_len + 2) / 3);
}

static unsigned EditDistance(const char* a, size_t alen, const char* b,
                             size_t blen, unsigned max_dist) {
  if (alen > blen) {
    const char* tmp = a;
    a = b;
    b = tmp;
    size_t n = alen;
    alen = blen;
    blen = n;
  }
  if (blen - alen > max_dist || blen > kTypoMaxIdentifier) {
    return max_dist + 1;
  }

  unsigned prev[kTypoMaxIdentifier + 1];
  unsigned curr[kTypoMaxIdentifier + 1];
  for (size_t j = 0; j <= blen; j++) {
    prev[j] = (unsigned)j;
  }
  for (size_t i = 1; i <= alen; i++) {
    curr[0] = (unsigned)i;
    unsigned row_min = curr[0];
    for (size_t j = 1; j <= blen; j++) {
      unsigned cost = ((unsigned char)a[i - 1] == (unsigned char)b[j - 1])
                          ? 0u
                          : 1u;
      unsigned del = prev[j] + 1;
      unsigned ins = curr[j - 1] + 1;
      unsigned sub = prev[j - 1] + cost;
      unsigned v = del < ins ? del : ins;
      if (sub < v) {
        v = sub;
      }
      curr[j] = v;
      if (v < row_min) {
        row_min = v;
      }
    }
    if (row_min > max_dist) {
      return max_dist + 1;
    }
    memcpy(prev, curr, (blen + 1) * sizeof(unsigned));
  }
  return prev[blen];
}

static bool NameIsSuggestable(const char* name) {
  if (name == NULL || name[0] == '\0') {
    return false;
  }
  if (strncmp(name, "__invented__", 12) == 0) {
    return false;
  }
  if (strncmp(name, "operator", 8) == 0) {
    return false;
  }
  return true;
}

static void InitTypoSearch(TypoSearch* search, const char* typo) {
  search->typo = typo != NULL ? typo : "";
  search->typo_len = strlen(search->typo);
  search->max_dist = MaxEditDistance(search->typo_len);
  search->best = NULL;
  search->best_dist = search->max_dist + 1;
  search->ties = 0;
}

static void ConsiderName(TypoSearch* search, const char* name) {
  if (search->max_dist == 0 || !NameIsSuggestable(name) ||
      strcmp(name, search->typo) == 0) {
    return;
  }
  // Keep reserved identifiers out of ordinary typo suggestions unless the
  // misspelling itself looks like one.
  if (name[0] == '_' && name[1] == '_' &&
      !(search->typo[0] == '_' && search->typo[1] == '_')) {
    return;
  }
  size_t name_len = strlen(name);
  unsigned dist;
  if (name_len == search->typo_len && strcasecmp(name, search->typo) == 0) {
    dist = 0;
  } else {
    if (name_len > search->typo_len + search->max_dist ||
        search->typo_len > name_len + search->max_dist) {
      return;
    }
    dist = EditDistance(search->typo, search->typo_len, name, name_len,
                        search->max_dist);
    if (dist > search->max_dist) {
      return;
    }
  }
  if (search->best == NULL || dist < search->best_dist) {
    search->best = name;
    search->best_dist = dist;
    search->ties = 1;
  } else if (dist == search->best_dist && strcmp(name, search->best) != 0) {
    search->ties++;
  }
}

static void ConsiderSymbol(TypoSearch* search, Symbol* symbol) {
  if (symbol == NULL || symbol->flags.invented || symbol->flags.is_temp) {
    return;
  }
  ConsiderName(search, symbol->name.value);
}

static void VisitSymbolNode(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  SymbolNode* sn = (SymbolNode*)node;
  if (sn != NULL) {
    ConsiderSymbol((TypoSearch*)data, sn->symbol);
  }
}

static void CollectFromTree(BinaryTree* tree, TypoSearch* search) {
  if (tree != NULL) {
    BinaryTreeTraverse(tree, VisitSymbolNode, search);
  }
}

static void VisitGlobalBucket(void* entry, void* data) {
  CollectFromTree((BinaryTree*)entry, (TypoSearch*)data);
}

static void CollectFromNamespace(Namespace* ns, TypoSearch* search);

static void CollectInlineNamespace(Namespace* child, void* ctx) {
  CollectFromNamespace(child, (TypoSearch*)ctx);
}

static void CollectFromNamespace(Namespace* ns, TypoSearch* search) {
  if (ns == NULL) {
    return;
  }
  CollectFromTree(&ns->symbol_table, search);
  if (ns->anonymous_child != NULL) {
    CollectFromNamespace(ns->anonymous_child, search);
  }
  NamespaceForEachInlineChild(ns, CollectInlineNamespace, search);
}

static void CollectFromStruct(Struct* str, TypoSearch* search, int depth) {
  if (str == NULL || depth > 64) {
    return;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->is_anon) {
      continue;
    }
    ConsiderSymbol(search, member->symbol);
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL ||
        base->type->info.struct_info == str) {
      continue;
    }
    CollectFromStruct(base->type->info.struct_info, search, depth + 1);
  }
}

static Struct* CurrentClassForTypo(Syntax* syntax) {
  if (syntax->cxx_class_head != NULL) {
    return syntax->cxx_class_head;
  }
  if (syntax->local_symbol_stack != NULL && compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    return compiler->current_function->info.function.cxx_member_owner;
  }
  return NULL;
}

static Namespace* LookupNamespaceChild(Namespace* parent, String* name) {
  if (parent == NULL || name == NULL) {
    return NULL;
  }
  Namespace* alias = NamespaceFindDirectAlias(parent, name);
  if (alias != NULL) {
    return alias;
  }
  return NamespaceFindChildForQualifiedLookup(parent, name);
}

static Namespace* FindUnqualifiedNamespace(Syntax* syntax, String* name) {
  for (LocalSymbolTable* scope = syntax->local_symbol_stack; scope != NULL;
       scope = scope->prev) {
    Namespace* alias = FindDirectLocalNamespaceAlias(scope, name);
    if (alias != NULL) {
      return alias;
    }
  }
  Namespace* cursor = syntax->current_namespace != NULL
                          ? syntax->current_namespace
                          : compiler->global_namespace;
  while (cursor != NULL) {
    Namespace* child = LookupNamespaceChild(cursor, name);
    if (child != NULL) {
      return child;
    }
    cursor = cursor->parent;
  }
  return LookupNamespaceChild(compiler->global_namespace, name);
}

static Namespace* FindPrefixNamespace(Syntax* syntax,
                                      FullyQualifiedIdentifier* name) {
  if (name == NULL || name->components.length == 0) {
    return NULL;
  }
  size_t prefix_len =
      name->components.length > 0 ? name->components.length - 1 : 0;
  if (name->absolute && prefix_len == 0) {
    return compiler->global_namespace;
  }
  if (prefix_len == 0) {
    return NULL;
  }
  Namespace* ns = name->absolute
                      ? LookupNamespaceChild(compiler->global_namespace,
                                             name->components.value.p[0])
                      : FindUnqualifiedNamespace(syntax,
                                                 name->components.value.p[0]);
  for (size_t i = 1; ns != NULL && i < prefix_len; i++) {
    ns = LookupNamespaceChild(ns, name->components.value.p[i]);
  }
  return ns;
}

const char* TypoCorrectionFindVisibleName(Syntax* syntax, const char* typo) {
  if (syntax == NULL || typo == NULL) {
    return NULL;
  }
  TypoSearch search;
  InitTypoSearch(&search, typo);
  if (search.max_dist == 0) {
    return NULL;
  }

  for (LocalSymbolTable* scope = syntax->local_symbol_stack; scope != NULL;
       scope = scope->prev) {
    CollectFromTree(&scope->table, &search);
  }

  Struct* owner = CurrentClassForTypo(syntax);
  if (owner != NULL) {
    CollectFromStruct(owner, &search, 0);
  }

  Namespace* ns = syntax->current_namespace != NULL
                      ? syntax->current_namespace
                      : compiler->global_namespace;
  while (ns != NULL) {
    CollectFromNamespace(ns, &search);
    ns = ns->parent;
  }
  HashTableTraverse(&compiler->global_symbol_table, VisitGlobalBucket, &search);

  return search.ties == 1 ? search.best : NULL;
}

const char* TypoCorrectionFindMemberName(Struct* str, const char* typo) {
  if (str == NULL || typo == NULL) {
    return NULL;
  }
  TypoSearch search;
  InitTypoSearch(&search, typo);
  if (search.max_dist == 0) {
    return NULL;
  }
  CollectFromStruct(str, &search, 0);
  return search.ties == 1 ? search.best : NULL;
}

const char* TypoCorrectionFindForIdentifier(Syntax* syntax,
                                            FullyQualifiedIdentifier* name) {
  if (syntax == NULL || name == NULL) {
    return NULL;
  }
  const char* last = FullyQualifiedIdentifierLast(name);
  if (last == NULL) {
    return NULL;
  }
  if (!name->is_qualified && !name->absolute) {
    return TypoCorrectionFindVisibleName(syntax, last);
  }

  TypoSearch search;
  InitTypoSearch(&search, last);
  if (search.max_dist == 0) {
    return NULL;
  }

  if (name->components.length >= 2) {
    Symbol* owner = SyntaxFindQualifiedPrefixSymbol(
        syntax, name, name->components.length - 1);
    if (owner != NULL && owner->type != NULL &&
        TypeIsStructOrUnion(owner->type) &&
        owner->type->info.struct_info != NULL) {
      CollectFromStruct(owner->type->info.struct_info, &search, 0);
      return search.ties == 1 ? search.best : NULL;
    }
  }

  Namespace* ns = FindPrefixNamespace(syntax, name);
  if (ns == compiler->global_namespace) {
    CollectFromNamespace(ns, &search);
    HashTableTraverse(&compiler->global_symbol_table, VisitGlobalBucket,
                       &search);
  } else if (ns != NULL) {
    CollectFromNamespace(ns, &search);
  } else {
    return NULL;
  }
  return search.ties == 1 ? search.best : NULL;
}

void TypoCorrectionErrorUnknownSymbol(Syntax* syntax,
                                      FullyQualifiedIdentifier* name) {
  const char* suggestion = TypoCorrectionFindForIdentifier(syntax, name);
  if (suggestion != NULL) {
    SyntaxError(syntax, "No such symbol \"%s\"; did you mean \"%s\"?",
                name->spelling.value, suggestion);
  } else {
    SyntaxError(syntax, "No such symbol \"%s\"", name->spelling.value);
  }
}
