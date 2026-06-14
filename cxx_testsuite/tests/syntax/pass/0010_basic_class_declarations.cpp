// RUN: -std=c++17
class Widget {
  int value;
  public:
    int other;
  private:
    int hidden;
  protected:
    int protected_value;
};

Widget widget;
class Widget* widget_pointer;

int main(void) {
  return 0;
}
