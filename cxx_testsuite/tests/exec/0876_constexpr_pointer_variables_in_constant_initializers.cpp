// Naming a constexpr pointer variable is a constant initializer, whether it
// holds a null pointer or the address of a non-const static object or array
// element.  Objects initialized that way are constant-initialized, so a
// dynamic initializer that runs first (here, one defined earlier in the
// file; in protobuf, another translation unit's) already sees their values.
struct EnumDescriptor;

struct Table {
  mutable bool is_initialized;
  int size;
  const char* filename;
  const EnumDescriptor** enums;
  int* counter;
  const int* element;
};

extern const Table table;
extern int* const counter_ptr;
extern const int* element_ptr;
extern const Table copies[2];

static int Probe() {
  if (table.filename == nullptr || table.filename[0] != 'a' ||
      table.size != 7) {
    return 1;
  }
  if (table.counter == nullptr || table.element == nullptr ||
      *table.element != 30) {
    return 2;
  }
  if (counter_ptr == nullptr || element_ptr == nullptr) {
    return 3;
  }
  if (copies[1].size != 9 || copies[1].counter == nullptr) {
    return 4;
  }
  return 0;
}

int early_result = Probe();

static int counter = 5;
static int values[] = {10, 20, 30};
static constexpr const EnumDescriptor** kNoEnums = nullptr;
static constexpr auto kNull = nullptr;
static constexpr int* kCounter = &counter;
static constexpr const int* kThird = &values[2];

const Table table = {false, 7, "any.proto", kNoEnums, kCounter, kThird};
int* const counter_ptr = kCounter;
const int* element_ptr = kThird;
const Table copies[2] = {
    {false, 8, "a", kNull, nullptr, nullptr},
    {false, 9, "b", kNoEnums, kCounter, kThird},
};

constexpr struct {
  int* counter;
  const int* element;
} kConstant = {kCounter, kThird};

int main() {
  if (early_result != 0) return early_result;
  if (table.enums != nullptr || copies[0].enums != nullptr) return 5;
  if (table.counter != &counter || counter_ptr != &counter) return 6;
  if (element_ptr != &values[2] || copies[1].element != &values[2]) return 7;
  if (kConstant.counter != &counter || kConstant.element != &values[2]) {
    return 8;
  }
  *table.counter += 1;
  if (counter != 6) return 9;
  return 0;
}
