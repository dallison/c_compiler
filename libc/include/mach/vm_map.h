#ifndef davecc_mach_vm_map_h
#define davecc_mach_vm_map_h

#include <mach/mach_init.h>

#ifdef __cplusplus
extern "C" {
#endif

kern_return_t vm_deallocate(vm_map_t target_task, vm_address_t address,
                            vm_size_t size);

#ifdef __cplusplus
}
#endif

#endif
