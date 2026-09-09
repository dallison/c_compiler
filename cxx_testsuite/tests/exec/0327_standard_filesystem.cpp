// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <filesystem>
#include <format>
#include <fstream>

#ifndef __cpp_lib_filesystem
#error "__cpp_lib_filesystem must be defined"
#endif

namespace fs = std::filesystem;

int main() {
  const char range_source[] = "gamma";
  fs::path ranged(range_source, range_source + 5);
  if (ranged != fs::path("gamma")) return 24;

  fs::path lexical("/alpha/./beta/../gamma.txt");
  if (lexical.lexically_normal() != fs::path("/alpha/gamma.txt")) return 1;
  if (lexical.filename() != fs::path("gamma.txt")) return 2;
  if (lexical.stem() != fs::path("gamma")) return 3;
  if (lexical.extension() != fs::path(".txt")) return 4;
  if (fs::path("alpha/beta").parent_path() != fs::path("alpha")) return 25;
  if (fs::path("/alpha/beta").parent_path() != fs::path("/alpha")) return 26;
  if (fs::path("alpha/").parent_path() != fs::path("alpha")) return 27;
  fs::path without_filename("alpha/beta");
  without_filename.remove_filename();
  if (without_filename != fs::path("alpha/")) return 28;
  if (fs::path("alpha/.").lexically_normal() != fs::path("alpha/")) return 34;
  if (fs::path("alpha/beta/..").lexically_normal() != fs::path("alpha/"))
    return 35;
  if (fs::path("../alpha/..").lexically_normal() != fs::path("..")) return 36;
  if (fs::path("alpha/..").lexically_normal() != fs::path(".")) return 37;

  std::error_code error;
  fs::path root = fs::temp_directory_path(error) / "davecc-filesystem-0327";
  if (error) return 5;
  fs::remove_all(root, error);
  error.clear();

  fs::path nested = root / "one" / "two";
  if (!fs::create_directories(nested, error) || error) return 6;
  if (!fs::is_directory(nested, error) || error) return 7;
  if (fs::create_directory(nested, error) || error) return 29;

  fs::path source = nested / "source.txt";
  {
    std::ofstream output(source.c_str());
    output << "filesystem";
  }
  if (fs::create_directory(source, error) || !error) return 30;
  error.clear();
  fs::path attributed = root / "attributed";
  if (!fs::create_directory(attributed, nested, error) || error) return 31;
  if (!fs::is_directory(attributed, error) || error) return 32;
  if (!fs::remove(attributed, error) || error) return 33;
  if (fs::file_size(source, error) != 10 || error) return 8;

  fs::path copied = nested / "copied.txt";
  if (!fs::copy_file(source, copied, error) || error) return 9;
  if (!fs::exists(copied, error) || error) return 10;

  fs::resize_file(copied, 4, error);
  if (error || fs::file_size(copied, error) != 4 || error) return 11;

  fs::path renamed = root / "renamed.txt";
  fs::rename(copied, renamed, error);
  if (error || !fs::exists(renamed, error) || error) return 12;

  fs::path hard = root / "hard.txt";
  fs::create_hard_link(renamed, hard, error);
  if (error || !fs::equivalent(renamed, hard, error) || error) return 13;

  fs::path symbolic = root / "symbolic.txt";
  fs::create_symlink(renamed, symbolic, error);
  if (error || !fs::is_symlink(symbolic, error) || error) return 14;
  if (fs::read_symlink(symbolic, error) != renamed || error) return 15;

  int direct_count = 0;
  for (const fs::directory_entry& entry : fs::directory_iterator(root, error)) {
    if (error || entry.path().empty()) return 16;
    ++direct_count;
  }
  if (error || direct_count != 4) return 17;

  int recursive_count = 0;
  for (const fs::directory_entry& entry :
       fs::recursive_directory_iterator(root, error)) {
    if (error || entry.path().empty()) return 18;
    ++recursive_count;
  }
  if (error || recursive_count != 6) return 19;

  fs::path canonical = fs::canonical(source, error);
  if (error || !canonical.is_absolute()) return 20;
  if (fs::relative(source, root, error) != fs::path("one/two/source.txt") ||
      error) {
    return 21;
  }

  fs::space_info info = fs::space(root, error);
  if (error || info.capacity == 0 || info.free > info.capacity) return 22;

  std::uintmax_t removed = fs::remove_all(root, error);
  if (error || removed != 7 || fs::exists(root, error) || error) return 23;

  if (std::format("{}", fs::path("alpha/beta")) != "alpha/beta") return 38;
  if (std::format("{:g}", fs::path("alpha/beta")) != "alpha/beta") return 39;

  return 0;
}
