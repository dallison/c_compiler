#ifndef unicode_name_h
#define unicode_name_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Performs the exact, case-sensitive Unicode name lookup required for a
// C++23 named universal character escape.
bool UnicodeCodePointFromName(const char* name, size_t length,
                              uint32_t* codepoint);

#endif
