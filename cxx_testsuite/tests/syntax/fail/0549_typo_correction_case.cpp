// RUN: -std=c++17
// EXPECT: No such symbol "WidgetCount"; did you mean "widget_count"?
int widget_count = 0;
int main() {
  return WidgetCount;
}
