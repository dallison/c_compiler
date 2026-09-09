// RUN: -std=c++17
// EXPECT: No such symbol "demo::widget_countt"; did you mean "widget_count"?
namespace demo {
int widget_count() { return 1; }
}
int main() {
  return demo::widget_countt();
}
