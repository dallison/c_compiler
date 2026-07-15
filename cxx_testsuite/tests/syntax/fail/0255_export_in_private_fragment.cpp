// RUN: -std=c++20
// EXPECT: Export declarations are not allowed in the private module fragment
export module syntax_private_export;

export int iface();

module :private;

export int bad_export();
