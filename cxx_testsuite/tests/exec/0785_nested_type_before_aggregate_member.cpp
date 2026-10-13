#include <cstdint>
#include <vector>

struct Field;
struct Descriptor;

struct Entry {
  enum Kind { kField, kOffset };
  Kind kind;

  struct Range {
    std::int32_t first;
    std::int32_t last;
  };

  union {
    const Field* field;
    const Descriptor* descriptor;
    std::uint32_t offset;
    Range range;
  };
};

int main() {
  const Field* field = nullptr;
  std::vector<Entry> entries;
  entries.push_back({Entry::kField, {field}});
  return entries.size() != 1 || entries[0].kind != Entry::kField ||
         entries[0].field != nullptr;
}
