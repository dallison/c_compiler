// Pieces of iostream that must live in main RAM on the BBC.
//
// std::cout's constructor is linked with the user program, so the object
// is not stored in a ROM bank. getloc() and the locale destructor are the
// calls an integer inserter emits. On this target every locale is the
// classic "C" locale, and the destructor has nothing to release.
//
// __davecc_cxa_atexit records destructors in the C++ ROM's .bss. That bank
// ignores stores, so the registration would not survive. Returning success
// without recording keeps the link working; a newline still flushes cout
// through __fd_ostreambuf::xsputn.

#include <cstdint>
#include <iostream>
#include <locale>

extern "C" int __davecc_cxa_atexit(void* destructor, void* object,
                                   unsigned long count, unsigned long stride,
                                   int complete_object_argument) {
  (void)destructor;
  (void)object;
  (void)count;
  (void)stride;
  (void)complete_object_argument;
  return 0;
}

// The C++ image's vtable sits in the sideways window. A virtual call from
// the user program loads that slot with whichever image is latched, so the
// address is not the method. These symbols are the main-RAM shims: they
// page the C++ image in, call its vector, and page the previous image back.
// Slot order matches basic_streambuf<char>, and the stored pointer addresses
// the first function (the compiler's vptr skips the two-word RTTI header).
extern "C" {
void _ZNSt15__fd_ostreambufD1Ev();
void _ZNSt15__fd_ostreambuf4syncEv();
void _ZNSt15__fd_ostreambuf6xsputnEPKci();
void _ZNSt15__fd_ostreambuf8overflowEi();
}

namespace {

void streambuf_slot_unused() {}

const void* fd_ostreambuf_vtbl[] = {
    (const void*)_ZNSt15__fd_ostreambufD1Ev,          // +0x00
    (const void*)streambuf_slot_unused,               // +0x02 setbuf
    (const void*)streambuf_slot_unused,               // +0x04 seekoff
    (const void*)streambuf_slot_unused,               // +0x06 seekpos
    (const void*)_ZNSt15__fd_ostreambuf4syncEv,       // +0x08
    (const void*)streambuf_slot_unused,               // +0x0a showmanyc
    (const void*)streambuf_slot_unused,               // +0x0c xsgetn
    (const void*)streambuf_slot_unused,               // +0x0e underflow
    (const void*)streambuf_slot_unused,               // +0x10 uflow
    (const void*)streambuf_slot_unused,               // +0x12 pbackfail
    (const void*)_ZNSt15__fd_ostreambuf6xsputnEPKci,  // +0x14
    (const void*)_ZNSt15__fd_ostreambuf8overflowEi,   // +0x16
    (const void*)streambuf_slot_unused,               // +0x18 imbue
};

struct FixFdOstreamVptr {
  FixFdOstreamVptr() {
    std::streambuf* buf = std::cout.rdbuf();
    if (buf == nullptr) {
      return;
    }
    const void** vptr = reinterpret_cast<const void**>(buf);
    uintptr_t current = reinterpret_cast<uintptr_t>(*vptr);
    if (current < 0x8000 || current >= 0xc000) {
      return;
    }
    *vptr = fd_ostreambuf_vtbl;
  }
};

FixFdOstreamVptr fix_fd_ostream_vptr;

}  // namespace

namespace std {

locale ios_base::getloc() const {
  return locale();
}

locale::~locale() {}

}  // namespace std
