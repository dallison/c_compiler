// As with libstdc++, <string> alone makes errno (and strerror) available.
#include <string>

struct Logger {
  std::string Describe(int code) {
    errno = code;
    return std::string(strerror(errno));
  }
};

int main() {
  Logger logger;
  if (logger.Describe(0).empty()) return 1;
  if (errno != 0) return 2;
  return 0;
}
