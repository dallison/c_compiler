// RUN: -std=c++17
// EXPECT: No such symbol "widget_countt"; did you mean "widget_count"?
int widget_count() { return 1; }
int main() {
  return widget_countt;
}
