// EXPECT_EXIT: 0

template <class C>
class basic_box;

typedef basic_box<char> box;
typedef basic_box<int> int_box;

template <class C>
class basic_box {
 public:
  C value;
  int size() const { return sizeof(C); }
};

struct Shape {
  virtual ~Shape() {}
  virtual int sides() const = 0;
};

typedef Shape shape_type;

struct Square : shape_type {
  int sides() const { return 4; }
};

int count_sides(const shape_type& shape) { return shape.sides(); }

int main() {
  box b;
  b.value = 'x';
  int_box i;
  i.value = 7;
  if (b.value != 'x' || b.size() != 1) {
    return 1;
  }
  if (i.value != 7 || i.size() != (int)sizeof(int)) {
    return 2;
  }
  Square square;
  return count_sides(square) == 4 ? 0 : 3;
}
