// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <filesystem>
#include <string>

int main() {
  std::filesystem::path hello("hello");
  hello.assign(hello.native().begin(), hello.native().end());
  if (hello != std::filesystem::path("hello")) return 1;

  std::filesystem::path longer(std::string(20, 'q'));
  longer.assign(longer.native().begin(), longer.native().end());
  if (longer.native() != std::string(20, 'q')) return 2;

  std::filesystem::path cleared("abc");
  cleared.assign(hello.native().begin(), hello.native().begin());
  if (!cleared.empty()) return 3;

  if (std::filesystem::path("a/b") != std::filesystem::path("a//b")) return 4;
  if (std::filesystem::path("/a") != std::filesystem::path("//a")) return 5;
  if (!(std::filesystem::path("a/b") < std::filesystem::path("a/b/"))) return 6;
  if (std::filesystem::hash_value(std::filesystem::path("a/b")) !=
      std::filesystem::hash_value(std::filesystem::path("a//b"))) {
    return 7;
  }

  if (std::filesystem::path("file.").stem() != std::filesystem::path("file"))
    return 8;
  if (std::filesystem::path("file.").extension() != std::filesystem::path("."))
    return 9;
  if (std::filesystem::path(".hidden").stem() !=
      std::filesystem::path(".hidden")) {
    return 10;
  }
  if (!std::filesystem::path(".hidden").extension().empty()) return 11;
  if (std::filesystem::path("..").stem() != std::filesystem::path(".."))
    return 12;
  if (!std::filesystem::path("..").extension().empty()) return 13;
  if (std::filesystem::path("...").stem() != std::filesystem::path(".."))
    return 14;
  if (std::filesystem::path("...").extension() != std::filesystem::path("."))
    return 15;
  if (std::filesystem::path("a.b.c").stem() != std::filesystem::path("a.b"))
    return 16;
  std::filesystem::path replaced("file.");
  replaced.replace_extension(".txt");
  if (replaced != std::filesystem::path("file.txt")) return 17;
  return 0;
}
