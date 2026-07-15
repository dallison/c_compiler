// RUN: -std=c++20
export module syntax_private;

export int private_exported() { return 2; }

module :private;

int private_helper() { return private_exported() + 1; }
