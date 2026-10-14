// RUN: -std=c++17 -Wno-return-type
// EXPECT_EXIT: 0

// An empty body gets no prologue, but its return value still reserves a
// frame.  The unwind info must not refer to prologue labels that were never
// emitted.

#include <string>
#include <string_view>

struct Lookup {
  std::string_view Find(std::string_view key);
  std::string storage = "value";
};

std::string_view Lookup::Find(std::string_view key) {}

int main(int argc, char** argv) {
  Lookup lookup;
  if (argc > 1000) {
    return static_cast<int>(lookup.Find(argv[0]).size());
  }
  return lookup.storage.size() == 5 ? 0 : 1;
}
