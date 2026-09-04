// RUN: -std=c++20
// EXPECT_EXIT: 0

static int constructions;
static int copies;
static int assignments;
static int destructions;

struct element {
  int value;

  explicit element(int number) : value(number) {
    ++constructions;
  }

  element(const element& other) : value(other.value) {
    ++copies;
  }

  element& operator=(const element& other) {
    value = other.value;
    ++assignments;
    return *this;
  }

  ~element() {
    ++destructions;
  }
};

struct aggregate {
  element member;
};

struct owner {
  aggregate value;

  owner() : value{{42}} {}
};

int main() {
  {
    owner object;
    if (object.value.member.value != 42) {
      return 1;
    }
    if (constructions != 1) {
      return constructions == 0 ? 2 : 6;
    }
    if (copies != 0) {
      return 3;
    }
    if (assignments != 0) {
      return 4;
    }
  }
  if (destructions != 1) {
    return 5;
  }
  return 0;
}
