// An aggregate element is copy-initialized from its initializer, so a glvalue
// of a class with a nontrivial copy runs the copy or move constructor.
// Copying its bytes left a string pointing at the source's inline buffer
// (co::CoroutineOptions built as `{.name = std::move(opts.name), ...}`).
#include <string>
#include <utility>
#include <vector>

static int copies = 0;
static int moves = 0;

struct Counted {
  Counted() {}
  Counted(const Counted&) { copies++; }
  Counted(Counted&&) { moves++; }
};

struct Holder {
  Counted counted;
  int x;
};

struct Options {
  std::string name;
  int fd = -1;
  bool autostart = true;
};

struct Nested {
  std::vector<std::vector<int>> rows;
  int tag;
};

static int Use(const Options& options) {
  return (int)options.name.size() + options.fd;
}

int main() {
  Counted c;
  Holder h1{c};
  Holder h2{std::move(c), 1};
  Holder h3 = {c, 2};
  Holder h4{Counted()};
  if (copies != 2 || moves != 1) return 1;

  std::string s("short");
  Options a{s};
  Options b{std::move(s), 3};
  if (a.name != "short" || b.name != "short" || b.fd != 3) return 2;

  std::string longer(40, 'x');
  Options designated{.name = longer, .fd = 4};
  if (designated.name != longer || longer.size() != 40) return 3;
  if (Use({.name = std::move(longer), .fd = 5}) != 45) return 4;

  std::vector<std::vector<int>> rows{{1, 2}, {3}};
  Nested n{rows, 6};
  if (n.rows.size() != 2 || n.rows[0].size() != 2 || rows.size() != 2) return 5;

  Options copy_of_member{a.name, 7};
  a.name.assign(30, 'y');
  if (copy_of_member.name != "short") return 6;
  return 0;
}
