// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <sstream>
#include <string>
#include <syncstream>
#include <utility>

#ifdef __DAVECC_HAS_GUEST_THREADS__
#include <thread>
#endif

class controlled_buffer : public std::streambuf {
 public:
  explicit controlled_buffer(std::streamsize first_limit)
      : first_limit_(first_limit), calls_(0), syncs_(0) {
  }

  std::string contents;
  int syncs() const {
    return syncs_;
  }

 protected:
  std::streamsize xsputn(const char* text, std::streamsize count) override {
    std::streamsize accepted = count;
    if (calls_ == 0 && first_limit_ >= 0 && accepted > first_limit_) {
      accepted = first_limit_;
    }
    ++calls_;
    contents.append(text, static_cast<std::size_t>(accepted));
    return accepted;
  }

  int sync() override {
    ++syncs_;
    return 0;
  }

 private:
  std::streamsize first_limit_;
  int calls_;
  int syncs_;
};

int main() {
  controlled_buffer partial(3);
  std::syncbuf buffered(&partial);
  buffered.sputn("abcdef", 6);
  if (buffered.emit() || partial.contents != "abc") {
    return 1;
  }
  if (!buffered.emit() || partial.contents != "abcdef") {
    return 2;
  }

  controlled_buffer flushing(-1);
  std::syncbuf immediate(&flushing);
  immediate.set_emit_on_sync(true);
  immediate.sputn("xy", 2);
  if (immediate.pubsync() != 0 || flushing.contents != "xy" ||
      flushing.syncs() != 1) {
    return 3;
  }

  std::syncbuf empty;
  if (empty.emit()) {
    return 4;
  }

  std::stringbuf wrapped;
  {
    std::osyncstream output(&wrapped);
    output << "value=" << 42;
  }
  if (wrapped.str() != "value=42") {
    return 5;
  }

  std::stringbuf moved_target;
  {
    std::osyncstream original(&moved_target);
    original << std::hex << "left:" ;
    std::osyncstream moved(std::move(original));
    moved << 16;
  }
  if (moved_target.str() != "left:10") {
    return 6;
  }

  std::stringbuf old_target;
  std::stringbuf new_target;
  {
    std::osyncstream old_stream(&old_target);
    old_stream << "old";
    std::osyncstream new_stream(&new_target);
    new_stream << "new";
    old_stream = std::move(new_stream);
  }
  if (old_target.str() != "old" || new_target.str() != "new") {
    return 7;
  }

#ifdef __DAVECC_HAS_GUEST_THREADS__
  std::stringbuf concurrent_target;
  std::thread workers[4];
  for (int i = 0; i < 4; ++i) {
    workers[i] = std::thread([&, i] {
      std::osyncstream output(&concurrent_target);
      for (int j = 0; j < 10; ++j) {
        output.put(static_cast<char>('A' + i));
      }
    });
  }
  for (int i = 0; i < 4; ++i) {
    workers[i].join();
  }
  std::string result = concurrent_target.str();
  if (result.size() != 40) {
    return 8;
  }
  unsigned int seen = 0;
  for (std::size_t offset = 0; offset < result.size(); offset += 10) {
    char value = result[offset];
    if (value < 'A' || value > 'D') {
      return 9;
    }
    unsigned int bit = 1u << static_cast<unsigned int>(value - 'A');
    if ((seen & bit) != 0) {
      return 10;
    }
    seen |= bit;
    for (std::size_t i = offset; i < offset + 10; ++i) {
      if (result[i] != value) {
        return 11;
      }
    }
  }
  if (seen != 0x0fu) {
    return 12;
  }
#endif

  return 0;
}
