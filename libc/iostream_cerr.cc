#include <iostream>
#include <__iostream_output>

namespace std {

static __stdio_ostreambuf __cerr_buffer(stderr);
ostream cerr(&__cerr_buffer);

class __cerr_initializer {
 public:
  __cerr_initializer() {
    cerr.tie(&cout);
    cerr.setf(ios_base::unitbuf);
  }
};

static __cerr_initializer __cerr_init;

}  // namespace std
