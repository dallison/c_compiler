// RUN: -std=c++20
export module linkage_pass;

int module_local() { return 1; }
export int exported_fn() { return module_local(); }
