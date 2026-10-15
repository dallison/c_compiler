// A class at global scope and a class of the same name in a namespace are
// different types (gtest's `testing::Message` and Subspace's `::Message`).
#include <type_traits>

namespace testing {
class Message {
 public:
  int z;
};
}  // namespace testing

struct Message {
  int length;
};

static int Overloaded(const testing::Message&) { return 1; }
static int Overloaded(...) { return 0; }

template <typename T>
static int Template(const T&, const testing::Message&) {
  return 1;
}
template <typename T>
static int Template(const T&, ...) {
  return 0;
}

int main() {
  Message m{};
  if (Overloaded(m) != 0) return 1;
  if (Template(1, m) != 0) return 2;
  if (std::is_same<Message, testing::Message>::value) return 3;
  if (std::is_convertible<const Message&, const testing::Message&>::value)
    return 4;
  testing::Message t{};
  if (Overloaded(t) != 1 || Template(1, t) != 1) return 5;
  return 0;
}
