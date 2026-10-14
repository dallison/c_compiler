// RUN: -std=c++17
// EXPECT_EXIT: 0

// A standalone `class X;` inside a class or block declares a new X in that
// scope even when an enclosing namespace already has an X ([dcl.type.elab]).
// `friend class X;` still names the existing enclosing class.

class Pool;

namespace outer {
struct Message {
  int outer_field = 7;
};

namespace inner {
class Holder {
  friend class ::Pool;

 public:
  class Message;
  class Field {
   public:
    const Message* parent_ = nullptr;
    int Read() const;
  };
  class Message {
   public:
    int x = 3;
    Field field;
  };

  static int Inner() {
    Message m;
    m.field.parent_ = &m;
    return m.field.Read();
  }

 private:
  int secret_ = 5;
};

int Holder::Field::Read() const { return parent_->x; }
}  // namespace inner
}  // namespace outer

class Pool {
 public:
  static int Peek(const outer::inner::Holder& h) { return h.secret_; }
};

int Block() {
  struct Message;
  struct Message {
    int y = 4;
  };
  Message m;
  return m.y;
}

int main() {
  outer::inner::Holder h;
  outer::Message om;
  if (Pool::Peek(h) != 5) return 1;
  if (outer::inner::Holder::Inner() != 3) return 2;
  if (Block() != 4) return 3;
  if (om.outer_field != 7) return 4;
  return 0;
}
