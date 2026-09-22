// RUN: -std=c++17
// EXPECT_EXIT: 0

class FormatArgImpl {
 public:
  union Data {
    const void* ptr;
    char buf[8];
  };
  template <typename T>
  static bool Dispatch(Data, int, void*);
};

#define INSTANTIATE_(T, E) \
  E template bool FormatArgImpl::Dispatch<T>(Data, int, void*)

#define EXPAND_(...)                            \
  INSTANTIATE_(bool, __VA_ARGS__);              \
  INSTANTIATE_(unsigned short, /* NOLINT */     \
               __VA_ARGS__);                    \
  INSTANTIATE_(int, __VA_ARGS__)

EXPAND_(extern);

int main() { return 0; }
