#ifndef davecc_posix_host_h
#define davecc_posix_host_h

#include <stddef.h>
#include <stdint.h>

// Translate a guest address the CPU can already touch.  `size` is the number
// of bytes the host is about to read or write.  Returns NULL for a null guest
// address or an address the interpreter cannot resolve.
typedef void* (*DaveHostGuestPtr)(void* context, uint64_t guest, size_t size);

// Host pointer for a mapping created by DAVE_SYS_MMAP, or NULL.
void* DaveHostMappingFind(uint64_t guest, size_t size);

// `pointer_size` is 4 or 8 and selects the guest msghdr/iovec layout.
// `address_bits` is 32 or 64.  A 64-bit guest receives the host mapping
// address; a 32-bit guest receives an address recorded in the mapping table.
// Returns a non-negative result, or a negative guest errno.
int64_t DaveHostPosixSyscall(int number, int pointer_size, int address_bits,
                             DaveHostGuestPtr guest_ptr, void* context,
                             uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
                             uint64_t a4, uint64_t a5);

#endif
