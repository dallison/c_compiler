//
//  linker_gc.h
//  linker
//
//  Unused allocatable section garbage collection (--gc-sections).
//

#ifndef linker_gc_h
#define linker_gc_h

struct Linker;

// Discard allocatable input sections that are not reachable from the entry
// symbol, retained special sections, or (for shared objects) exported globals.
void LinkerGarbageCollectSections(struct Linker* linker);

#endif /* linker_gc_h */
