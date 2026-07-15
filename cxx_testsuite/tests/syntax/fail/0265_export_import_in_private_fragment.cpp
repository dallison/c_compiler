// RUN: -std=c++20
// EXPECT: Export declarations are not allowed in the private module fragment
export module syntax_export_import_private;

export int iface();

module :private;

export import syntax_export_import_private;
