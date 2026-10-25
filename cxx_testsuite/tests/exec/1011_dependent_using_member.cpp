// RUN: -std=c++17
// EXPECT_EXIT: 0

// Unqualified calls of names introduced by `using Base::name` in a class
// template, including static functions and a member function template.

struct Message {
  int runtime;
};

template <int kOffset, int kId>
struct FieldSpec {
  static constexpr int Offset() { return kOffset; }
  static constexpr int Id() { return kId; }
  Message* Parent() const { return parent; }
  Message* parent;
};

template <typename Loc>
class FieldBase : public Loc {
 public:
  using Loc::Id;
  int Find() const { return Id(); }

 protected:
  using Loc::Offset;
  using Loc::Parent;
  int Off() const { return (int)Offset(); }
  Message* Par() const { return Parent(); }
  template <typename T>
  int Store(T v) {
    return Id() + (int)Offset() + (Parent() != 0 ? (int)v : 0);
  }
};

template <typename Loc>
class Field : public FieldBase<Loc> {
 public:
  using FieldBase<Loc>::Store;
  int Go(int v) { return this->Store(v); }
};

int main() {
  Message msg;
  msg.runtime = 1;
  Field<FieldSpec<4, 7> > f;
  f.parent = &msg;
  return f.Go(3) == 14 ? 0 : 1;
}
