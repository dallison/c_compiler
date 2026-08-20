// RUN: -std=c++29
// EXPECT: Use of implicitly deleted function

struct noncopyable {
  noncopyable() = default;
  noncopyable(const noncopyable&) = delete;
};

template <typename T>
struct counter {
  T value;

  counter& operator++() {
    return *this;
  }

  counter operator++(int) = default;
};

void use_deleted_postfix_template() {
  counter<noncopyable> value;
  value++;
}
