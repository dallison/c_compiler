// RUN: -std=c++20
export module syntax_primary;

export int primary_answer() { return 7; }

int primary_hidden() { return 0; }
