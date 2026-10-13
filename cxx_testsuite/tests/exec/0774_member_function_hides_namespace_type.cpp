namespace N {

struct Owner {
  struct Nested {};
};

struct User {
  static int Owner();

  template <class T>
  static int use(T) {
    Owner::Nested value{};
    Owner::Nested{};
    return sizeof(value);
  }
};

}  // namespace N

int main() { return N::User::use(0) != 1; }
