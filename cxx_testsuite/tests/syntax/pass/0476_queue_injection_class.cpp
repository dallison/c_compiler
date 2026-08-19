// RUN: -std=c++29

#include <meta>

struct Holder {
  consteval {
    std::meta::queue_injection(^{ int injected_member; });
  }
};

static_assert(sizeof(Holder) == sizeof(int));

int main() {
  Holder holder{};
  holder.injected_member = 3;
  return holder.injected_member != 3;
}
