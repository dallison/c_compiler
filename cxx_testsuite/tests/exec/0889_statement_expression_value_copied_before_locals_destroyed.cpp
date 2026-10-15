// A GNU statement expression yields its last expression statement's value,
// copied out before the block's locals and that statement's temporaries are
// destroyed (abseil's ABSL_CHECK macros wrap `({ ... })` around streams).
#include <string>

static int live = 0;

struct Tracked {
  int id;
  explicit Tracked(int i) : id(i) { live++; }
  Tracked(const Tracked& other) : id(other.id) { live++; }
  ~Tracked() {
    live--;
    id = -1;
  }
  int Get() const { return id; }
};

static std::string Concat(const std::string& a, const std::string& b) {
  return a + b;
}

int main() {
  int from_local = ({
    Tracked t(4);
    t.id + 1;
  });
  if (from_local != 5 || live != 0) return 1;

  int from_temporary = ({ Tracked(6).Get(); });
  if (from_temporary != 6 || live != 0) return 2;

  std::string s = ({
    std::string local = "a_string_longer_than_the_small_buffer";
    local + "!";
  });
  if (s != "a_string_longer_than_the_small_buffer!") return 3;

  std::string t = ({
    Tracked guard(1);
    Concat(std::string("left_part_long_enough_for_the_heap_"), "right");
  });
  if (t != "left_part_long_enough_for_the_heap_right" || live != 0) return 4;

  Tracked copied = ({
    Tracked inner(7);
    inner;
  });
  if (copied.id != 7 || live != 1) return 5;

  int plain = ({
    int x = 2;
    x * 3;
  });
  return plain == 6 ? 0 : 6;
}
