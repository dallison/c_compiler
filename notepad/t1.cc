#include <vector>
#include <map>
#include <string>
//#include <cstdio>
#include <iostream>
#include <cstdlib>

int main(int argc, char** argv) {
  std::cout << "argc: " << argc << std::endl;
  if (argc < 2) {
    std::cerr << "need an arg\n";
    exit(1);
  }
  int n = atoi(argv[1]);
  std::vector<std::string> r;
  std::map<std::string, int> m;
  for (int i = 0; i < n; i++) {
    r.push_back("hello world " + std::to_string(i));
    m["x" + std::to_string(i)] = i*2;
  }
  for (size_t i = 0; i < r.size(); i++) {
    //printf("%s\n", r[i]);
    std::cout << r[i] << std::endl;
  }
  for (auto it = m.begin(); it != m.end(); it++) {
    std::cout << it->first << ": " << it->second << std::endl;
  }
}
