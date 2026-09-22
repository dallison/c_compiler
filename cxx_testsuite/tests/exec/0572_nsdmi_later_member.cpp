// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Sink {
  char* pos_ = buf_;
  char buf_[8];
};

int main() {
  Sink s;
  return s.pos_ == s.buf_ ? 0 : 1;
}
