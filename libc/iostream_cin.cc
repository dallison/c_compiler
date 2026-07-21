#include <iostream>
#include <__iostream_input>

namespace std {

static __stdio_istreambuf __cin_buffer(stdin);
istream cin(&__cin_buffer);

class __cin_initializer {
 public:
  __cin_initializer() { cin.tie(&cout); }
};

static __cin_initializer __cin_init;

}  // namespace std
