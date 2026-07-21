#include <iostream>
#include <__iostream_output>

namespace std {

static __stdio_ostreambuf __clog_buffer(stderr);
ostream clog(&__clog_buffer);

}  // namespace std
