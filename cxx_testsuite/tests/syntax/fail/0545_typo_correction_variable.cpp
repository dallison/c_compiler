// RUN: -std=c++17
// EXPECT: No such symbol "widget_countt"; did you mean "widget_count"?
int main() {
  int widget_count = 0;
  return widget_countt;
}
