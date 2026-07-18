#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main() {
  std::streamoff offset = 0;
  std::streamsize count = 1;
  std::streampos position = 2;
  if (offset != 0 || count != 1 || position != 2) {
    return 1;
  }
  if (std::cin.tie() != &std::cout ||
      (std::cerr.flags() & std::ios_base::unitbuf) == 0 ||
      std::cin.rdbuf() == nullptr || std::cout.rdbuf() == nullptr ||
      std::cerr.rdbuf() == nullptr || std::clog.rdbuf() == nullptr) {
    return 2;
  }

  std::ostringstream output;
  output << std::hex << 255 << ' ' << std::dec << -7 << ' '
         << std::boolalpha << true << std::endl;
  if (output.str() != "ff -7 true\n") {
    return 3;
  }

  std::istringstream input("12 34 alpha");
  int first = 0;
  int second = 0;
  std::string word;
  input >> first >> second >> word;
  if (!input || first != 12 || second != 34 || word != "alpha") {
    return 4;
  }

  std::istringstream character_input("beta");
  char characters[8];
  character_input >> characters;
  if (!character_input.eof()) {
    return 5;
  }
  if (std::string(characters) != "beta") {
    return 11;
  }

  std::istringstream lines("first line\nsecond line");
  std::string line;
  if (!std::getline(lines, line) || line != "first line" ||
      !std::getline(lines, line) || line != "second line") {
    return 6;
  }

  std::stringstream both;
  both << 41;
  int round_trip = 0;
  both >> round_trip;
  if (round_trip != 41) {
    return 7;
  }

  const char* path = "/tmp/davecc_cxx_streams_0275.txt";
  {
    std::ofstream file(path);
    if (!file.is_open()) {
      return 8;
    }
    file << "file " << 123 << std::endl;
    file.close();
    if (file.fail()) {
      return 9;
    }
  }
  {
    std::ifstream file(path);
    std::string label;
    int value = 0;
    file >> label >> value;
    if (!file || label != "file" || value != 123) {
      return 10;
    }
  }
  std::cout << "streams-ok " << round_trip << std::endl;
  return 0;
}
