// RUN: -std=c++17
// EXPECT: widget_countt is not a member of struct/union Widget; did you mean "widget_count"?
struct Widget {
  int widget_count;
};
int main() {
  Widget widget;
  return widget.widget_countt;
}
