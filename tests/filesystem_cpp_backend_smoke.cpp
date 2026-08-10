#include <filesystem>

namespace fs = std::filesystem;

int main() {
  fs::path original("/alpha/./beta/../gamma.txt");
  if (!original.is_absolute()) return 2;
  fs::path normalized = original.lexically_normal();
  const char* expected = "/alpha/gamma.txt";
  for (int i = 0; i < static_cast<int>(normalized.native().size()) && i < 16;
       ++i) {
    if (normalized.native()[i] != expected[i]) return 20 + i;
  }
  if (normalized.native().size() != 16) {
    return 100 + static_cast<int>(normalized.native().size());
  }

  std::error_code error;
  fs::path current = fs::current_path(error);
  if (error || current.empty() || !fs::is_directory(current, error) || error)
    return 1;
  return 0;
}
