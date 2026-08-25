// RUN: -std=c++17
// EXPECT: No such symbol "files_db_data"
// A range-for whose range names an undeclared identifier, with an empty body,
// must diagnose and not crash while building the loop AST.
template <typename a> void b(a &);
void test() {
  for (auto file_data : b(files_db_data));
}
