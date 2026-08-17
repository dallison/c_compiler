// RUN: -std=c++23

static_assert(true, u8"prefixed message");
static_assert(true, "\x41");

auto prefixed_multichar = u'ab';
