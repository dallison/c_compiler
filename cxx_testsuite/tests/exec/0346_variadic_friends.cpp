// RUN: -std=c++26
// EXPECT_EXIT: 0

struct First;
struct Second;
struct Auditor;

template <class... Friends>
class Vault {
  friend Friends..., Auditor;

  int value;

 public:
  explicit Vault(int v) : value(v) {}
};

using TestVault = Vault<First, Second>;

struct First {
  static int read(const TestVault& vault) { return vault.value; }
};

struct Second {
  static void write(TestVault& vault, int value) { vault.value = value; }
};

struct Auditor {
  static int read(const TestVault& vault) { return vault.value; }
};

template <class Owner, class Tag>
struct Receiver {
  static int read(const Owner& owner);
};

template <class... Tags>
struct Dispatcher {
  friend Receiver<Dispatcher, Tags>...;

  int value;

 public:
  explicit Dispatcher(int v) : value(v) {}
};

template <class Owner, class Tag>
int Receiver<Owner, Tag>::read(const Owner& owner) {
  return owner.value;
}

int main() {
  TestVault vault(7);
  Vault<int> ignored_non_class(3);
  (void)ignored_non_class;
  if (First::read(vault) != 7) {
    return 1;
  }
  Second::write(vault, 11);
  if (Auditor::read(vault) != 11) {
    return 2;
  }
  Dispatcher<int, char, long> dispatcher(19);
  if (Receiver<Dispatcher<int, char, long>, char>::read(dispatcher) != 19) {
    return 3;
  }
  return 0;
}
