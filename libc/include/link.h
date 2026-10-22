#ifndef davecc_link_h
#define davecc_link_h

#include <elf.h>
#include <stddef.h>

struct dl_phdr_info {
  ElfW(Addr) dlpi_addr;
  const char* dlpi_name;
  const ElfW(Phdr) * dlpi_phdr;
  ElfW(Half) dlpi_phnum;
};

#ifdef __cplusplus
extern "C" {
#endif

int dl_iterate_phdr(int (*callback)(struct dl_phdr_info* info, size_t size,
                                    void* data),
                    void* data);

#ifdef __cplusplus
}
#endif

#endif
