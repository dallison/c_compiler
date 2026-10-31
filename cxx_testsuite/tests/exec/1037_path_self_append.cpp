// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <filesystem>

int main() {
  std::filesystem::path joined("foo");
  joined /= joined;
  if (joined != std::filesystem::path("foo/foo")) return 1;

  std::filesystem::path named("alpha/beta");
  named.replace_filename(named);
  if (named != std::filesystem::path("alpha/alpha/beta")) return 2;

  std::filesystem::path extended("file.txt");
  extended.replace_extension(extended);
  if (extended != std::filesystem::path("file.file.txt")) return 3;
  return 0;
}
