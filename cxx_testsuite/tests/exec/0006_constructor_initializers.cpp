#include <utility>

int trace;

struct Base {
  int base_value;
  Base(int value);
};

Base::Base(int value) {
  if (trace != 0) {
    trace = 100;
    return;
  }
  trace = 1;
  base_value = value;
}

struct Part {
  int part_value;
  Part(int value);
};

Part::Part(int value) {
  if (trace != 1) {
    trace = 101;
    return;
  }
  trace = 2;
  part_value = value;
}

struct Derived : public Base {
  Part part;
  int derived_value;
  Derived();
};

Derived::Derived() : derived_value(31), part(17), Base(11) {
  if (trace != 2) {
    trace = 102;
    return;
  }
  if (base_value != 11 || part.part_value != 17 || derived_value != 31) {
    trace = 103;
    return;
  }
  trace = 3;
}

struct InlineBeforeMembers {
  InlineBeforeMembers() : y(13), x(7) {}
  int x;
  int y;
};

template <class T>
struct TemplatePart {
  int value;
  TemplatePart() : value(19) {}
};

template <class T, class Member = TemplatePart<T> >
struct DependentMemberInitializer {
  DependentMemberInitializer() : marker(23), member() {}
  int marker;
  Member member;
};

template <class T>
struct TemplatePrivateCopy {
 private:
  T value;

 public:
  TemplatePrivateCopy(T v) : value(v) {}
  TemplatePrivateCopy(const TemplatePrivateCopy& other) : value(other.value) {}
  T get() const { return value; }
};

template <class T>
struct TemplateMoveMemberInitializer {
  using value_type = std::pair<const int, T>;
  value_type value;

  TemplateMoveMemberInitializer(value_type&& v)
      : value(v.first, std::move(v.second)) {}
};

int main(void) {
  Derived* derived = new Derived;
  if (trace != 3) {
    return trace;
  }
  if (derived->base_value != 11) {
    return 4;
  }
  if (derived->part.part_value != 17) {
    return 5;
  }
  if (derived->derived_value != 31) {
    return 6;
  }
  InlineBeforeMembers inline_before_members;
  if (inline_before_members.x != 7 || inline_before_members.y != 13) {
    return 7;
  }
  DependentMemberInitializer<int> dependent_member_initializer;
  if (dependent_member_initializer.marker != 23 ||
      dependent_member_initializer.member.value != 19) {
    return 8;
  }
  TemplatePrivateCopy<int> private_copy_source(31);
  TemplatePrivateCopy<int> private_copy(private_copy_source);
  if (private_copy.get() != 31) {
    return 9;
  }
  std::pair<const int, int> move_pair(41, 59);
  TemplateMoveMemberInitializer<int> move_initializer(std::move(move_pair));
  if (move_initializer.value.first != 41 ||
      move_initializer.value.second != 59) {
    return 10;
  }
  delete derived;
  return 0;
}
