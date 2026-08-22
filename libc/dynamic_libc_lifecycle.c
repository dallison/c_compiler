#include <stddef.h>

typedef void (*DaveCCDynamicInitFiniFn)(void);

extern DaveCCDynamicInitFiniFn __davecc_dso_init_array_start[];
extern DaveCCDynamicInitFiniFn __davecc_dso_init_array_end[];
extern DaveCCDynamicInitFiniFn __davecc_dso_fini_array_start[];
extern DaveCCDynamicInitFiniFn __davecc_dso_fini_array_end[];

static unsigned char shared_init_done;
static unsigned char shared_fini_done;

void __davecc_shared_init(void) {
  if (shared_init_done != 0) return;
  shared_init_done = 1;
  for (DaveCCDynamicInitFiniFn* entry = __davecc_dso_init_array_start;
       entry < __davecc_dso_init_array_end; ++entry) {
    if (*entry != NULL) (*entry)();
  }
}

void __davecc_shared_fini(void) {
  if (shared_fini_done != 0) return;
  shared_fini_done = 1;
  for (DaveCCDynamicInitFiniFn* entry = __davecc_dso_fini_array_end;
       entry > __davecc_dso_fini_array_start;) {
    --entry;
    if (*entry != NULL) (*entry)();
  }
}
