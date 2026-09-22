// RUN: -std=c++17
// EXPECT_EXIT: 0

class Sink {
 public:
  explicit Sink() : pos_(buf_) {}

  int append(int n, char c) {
    auto raw_append = [&](int count) {
      for (int i = 0; i < count; ++i) {
        *pos_ = c;
        ++pos_;
      }
    };
    raw_append(n);
    return (int)(pos_ - buf_);
  }

 private:
  char* pos_;
  char buf_[8];
};

int main() {
  Sink s;
  return s.append(3, 'x') == 3 ? 0 : 1;
}
