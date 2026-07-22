#include <iostream>
#include <__iostream_output>

namespace std {

static __fd_ostreambuf __cout_buffer(STDOUT_FILENO);
ostream cout(&__cout_buffer);

}  // namespace std
