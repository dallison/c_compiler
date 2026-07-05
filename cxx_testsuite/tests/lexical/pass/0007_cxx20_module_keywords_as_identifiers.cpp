// RUN: -std=c++20
// In C++20, `module` and `import` are identifiers with special meaning, not
// reserved keywords (see [lex.name]).  They only act as keywords when they
// begin a module-directive; elsewhere they remain usable as ordinary
// identifiers, so this must compile cleanly.
int module;
int import;

int main(void) {
  module = 1;
  import = 2;
  return module + import - 3;
}
