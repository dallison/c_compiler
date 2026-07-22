#include <iostream>
#include <__iostream_output>

namespace std {

static __fd_ostreambuf __clog_buffer(STDERR_FILENO);
ostream clog(&__clog_buffer);

}  // namespace std
