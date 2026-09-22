// RUN: -std=c++17
// EXPECT_EXIT: 0

struct string_view {
  const char* p;
};

template <class T>
void InvokeFlush(T*, string_view) {}

class FormatRawSinkImpl {
 public:
  template <typename T>
  FormatRawSinkImpl(T* raw)
      : sink_(raw), write_(&FormatRawSinkImpl::Flush<T>) {}

  void Write(string_view s) { write_(sink_, s); }

 private:
  template <typename T>
  static void Flush(void* r, string_view s) {
    InvokeFlush(static_cast<T*>(r), s);
  }

  void* sink_;
  void (*write_)(void*, string_view);
};

int main() {
  int x = 0;
  FormatRawSinkImpl s(&x);
  s.Write(string_view{});
  return 0;
}
