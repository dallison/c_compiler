//
//  lto_archive.h
//  c_compiler
//
//  DCCLTO03 IR-module objects.  The on-disk file is the 8-byte magic
//  "DCCLTO03" followed by a System V AR archive of serialized pools.
//

#ifndef lto_archive_h
#define lto_archive_h

#include <stdbool.h>
#include <stddef.h>

#include "dstring.h"
#include "lto_module.h"
#include "serialize.h"
#include "vector.h"

#define DCC_LTO_MAGIC "DCCLTO03"
#define DCC_LTO_MAGIC_LEN 8

typedef struct LoadedLTOModule {
  DeserializeContext* ctx;
  LTOModule module;
} LoadedLTOModule;

void LoadedLTOModuleInit(LoadedLTOModule* loaded);
void LoadedLTOModuleDestruct(LoadedLTOModule* loaded);

bool LTOArchiveIsLTOObject(const char* path);
bool LTOArchiveBufferIsLTO(const void* data, size_t len);
// Copy every DCCLTO03 member out of a System V `.a`.  Returns the number of
// members extracted, or -1 if `path` cannot be opened as an archive.
// `has_native` is set if any regular member is not LTO bitcode.
int LTOArchiveExtractLTOMembers(const char* path, Vector* blobs,
                                Vector* blob_lens, bool* has_native);
bool LTOArchiveWrite(const char* path, struct Compiler* compiler);
bool LTOArchiveRead(const char* path, LoadedLTOModule* loaded);
bool LTOArchiveReadFromMemory(const void* data, size_t len,
                              LoadedLTOModule* loaded);

String* CompileLTOIRObject(const char* filename, Vector* options,
                           Vector* target_opts, const char* output_path);
String* CompileLTOIRModules(Vector* inputs, Vector* options,
                            Vector* target_opts, bool whole_program,
                            Vector* preserve_asm_names);

#endif /* lto_archive_h */
