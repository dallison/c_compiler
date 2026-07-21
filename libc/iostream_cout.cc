#include <iostream>
#include <__iostream_output>

namespace std {

static __stdio_ostreambuf __cout_buffer(stdout);
ostream cout(&__cout_buffer);

}  // namespace std
