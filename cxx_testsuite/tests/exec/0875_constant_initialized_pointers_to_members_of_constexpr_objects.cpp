// Static initializers holding the address of a member of a constexpr object,
// as protobuf's ClassData holds `&_table_.header`: copying the pointer through
// a by-value base must keep it pointing at the member, not at itself.
#include <stdint.h>

struct Header {
  uint16_t has_bits_offset;
  uint32_t entries_offset;
};
struct Table {
  Header header;
  int entries[3];
};
constexpr Table table = {{4, 8}, {1, 2, 3}};
int instance;

struct Plain {
  const void* prototype;
  const Header* header;
};
struct Data {
  const void* prototype;
  const Header* header;
  constexpr Data(const void* prototype, const Header* header)
      : prototype(prototype), header(header) {}
};
struct DataFull : Data {
  constexpr DataFull(Data base, const void* extra)
      : Data(base), reflection(), extra(extra) {}
  mutable const void* reflection;
  const void* extra;
};
struct OnlyHeader {
  const Header* header;
  constexpr OnlyHeader(const Header* header) : header(header) {}
};

const Header* header_pointer = &table.header;
constinit const Plain plain{&instance, &table.header};
constinit const Data data{&instance, &table.header};
constinit const DataFull full{Data{&instance, &table.header}, &instance};
constinit const OnlyHeader only{&table.header};
struct Element {
  const void* prototype;
  const int* entry;
};
constinit const Element element{&instance, &table.entries[1]};

// The table is defined after the object that points into it.
struct Message {
  static constexpr auto Generate();
  static const Table table_;
};
constexpr auto Message::Generate() {
  return DataFull{Data{&instance, &table_.header}, &table_};
}
constinit const DataFull message_data = Message::Generate();
constexpr Table Message::table_ = {{16, 24}, {4, 5, 6}};

template <typename T>
const T* Launder(const T* p) {
  const T* volatile copy = p;
  return copy;
}

int main() {
  if (header_pointer != &table.header) return 1;
  if (plain.header != &table.header) return 2;
  if (data.header != &table.header || data.prototype != &instance) return 3;
  if (full.header != &table.header || full.extra != &instance) return 4;
  if (only.header != &table.header) return 5;
  if (Launder(&full)->header->entries_offset != 8) return 6;
  if (element.entry != &table.entries[1] || *Launder(element.entry) != 2)
    return 7;
  if (message_data.header != &Message::table_.header) return 8;
  if (Launder(&message_data)->header->entries_offset != 24) return 9;
  if (Launder(&message_data)->header->has_bits_offset != 16) return 10;
  if (message_data.extra != &Message::table_) return 11;
  return 0;
}
