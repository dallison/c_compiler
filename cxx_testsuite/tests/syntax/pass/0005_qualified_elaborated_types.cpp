// RUN: -std=c++17
namespace records {
  struct Record {
    int value;
  };

  enum Kind {
    KindA,
  };
}

struct records::Record* record_ptr;
enum records::Kind kind_value;

int main(void) {
  return 0;
}
