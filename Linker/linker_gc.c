//
//  linker_gc.c
//  linker
//
//  Reachability-based unused section garbage collection.  Input sections
//  stay separate until grouping, so -ffunction-sections can drop functions
//  that nothing reachable references.
//

#include "linker_gc.h"

#include <stdio.h>
#include <string.h>

#include "elf.h"
#include "linker.h"
#include "linker_file.h"
#include "linker_reloc.h"
#include "linker_symbols.h"

static bool SectionNameEquals(const String* name, const char* exact) {
  return StringEqual((String*)name, exact);
}

static bool SectionNameHasPrefix(const String* name, const char* prefix) {
  return StringStartsWith((String*)name, prefix);
}

static bool SectionIsGCRoot(const String* name, int64_t flags) {
  if ((flags & SHF(gnu_retain)) != 0) {
    return true;
  }
  return SectionNameEquals(name, ".init") || SectionNameEquals(name, ".fini") ||
         SectionNameEquals(name, ".ctors") || SectionNameEquals(name, ".dtors") ||
         SectionNameEquals(name, ".jcr") || SectionNameEquals(name, ".boot") ||
         SectionNameEquals(name, ".hwvectors") ||
         SectionNameEquals(name, ".preinit_array") ||
         SectionNameEquals(name, ".init_array") ||
         SectionNameEquals(name, ".fini_array") ||
         SectionNameHasPrefix(name, ".init_array.") ||
         SectionNameHasPrefix(name, ".preinit_array.") ||
         SectionNameHasPrefix(name, ".fini_array.") ||
         SectionNameHasPrefix(name, ".ctors.") ||
         SectionNameHasPrefix(name, ".dtors.");
}

static ELFReaderSection* RelocationTargetSection(ObjectFile* file,
                                                 Relocation* reloc) {
  if (reloc->symbol_section != NULL) {
    return reloc->symbol_section;
  }
  if (reloc->symbol != NULL && reloc->symbol->section != NULL) {
    return reloc->symbol->section;
  }
  if (reloc->symbol_name.length == 0) {
    return NULL;
  }
  LinkerSymbol* sym = ObjectFileFindSymbol(file, reloc->symbol_name.value);
  if (sym != NULL) {
    return sym->section;
  }
  return NULL;
}

static void MarkSectionLive(ELFReaderSection* section, Vector* worklist) {
  if (section == NULL || !section->discarded) {
    return;
  }
  if ((section->header->flags & SHF(alloc)) == 0) {
    section->discarded = false;
    return;
  }
  section->discarded = false;
  VectorAppend(worklist, section);
}

static void MarkDefinedSymbolSection(void* entry, void* data) {
  Vector* bucket = entry;
  Vector* worklist = data;
  for (size_t i = 0; i < bucket->length; i++) {
    LinkerSymbol* sym = bucket->value.p[i];
    if (sym->defined && sym->section != NULL) {
      MarkSectionLive(sym->section, worklist);
    }
  }
}

// One adjacency list per section: each relocation is an edge from its
// source section to the section that defines the referenced symbol.
// Built in O(R) and walked once per live section, so GC is O(S + R).
static void BuildSectionRelocGraph(Linker* linker) {
  for (size_t i = 0; i < linker->files.length; i++) {
    ObjectFile* file = linker->files.value.p[i];
    for (size_t r = 0; r < file->relocations.length; r++) {
      Relocation* reloc = file->relocations.value.p[r];
      if (reloc->section == NULL) {
        continue;
      }
      ELFReaderSection* target = RelocationTargetSection(file, reloc);
      if (target == NULL) {
        continue;
      }
      VectorAppend(&reloc->section->gc_refs, target);
    }
  }
}

void LinkerGarbageCollectSections(Linker* linker) {
  if (!linker->gc_sections) {
    return;
  }

  BuildSectionRelocGraph(linker);

  Vector worklist = {0};
  for (size_t i = 0; i < linker->files.length; i++) {
    ObjectFile* file = linker->files.value.p[i];
    for (size_t j = 0; j < file->elf_file->sections.length; j++) {
      ELFReaderSection* section = file->elf_file->sections.value.p[j];
      if ((section->header->flags & SHF(alloc)) == 0) {
        continue;
      }
      section->discarded = true;
      if (SectionIsGCRoot(&section->name, section->header->flags)) {
        MarkSectionLive(section, &worklist);
      }
    }
  }

  LinkerSymbol* entry =
      LinkerFindSymbol(&linker->global_symbol_table, linker->entry_symbol.value);
  if (entry != NULL && entry->section != NULL) {
    MarkSectionLive(entry->section, &worklist);
  }

  if (linker->building_dso) {
    HashTableTraverse(&linker->global_symbol_table, MarkDefinedSymbolSection,
                      &worklist);
  }

  while (worklist.length > 0) {
    ELFReaderSection* section = worklist.value.p[--worklist.length];
    for (size_t r = 0; r < section->gc_refs.length; r++) {
      MarkSectionLive(section->gc_refs.value.p[r], &worklist);
    }
  }
  VectorDestruct(&worklist);

  if (!linker->print_gc_sections) {
    return;
  }
  for (size_t i = 0; i < linker->files.length; i++) {
    ObjectFile* file = linker->files.value.p[i];
    for (size_t j = 0; j < file->elf_file->sections.length; j++) {
      ELFReaderSection* section = file->elf_file->sections.value.p[j];
      if (!section->discarded || section->header->size == 0) {
        continue;
      }
      fprintf(stderr, "removing unused section '%s' in file '%s'\n",
              section->name.value, file->filename.value);
    }
  }
}
