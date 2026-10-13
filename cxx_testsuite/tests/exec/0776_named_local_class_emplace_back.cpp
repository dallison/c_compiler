#include <vector>

int main() {
  struct Member {
    explicit Member(int& value) : pointer(&value) {}
    int* pointer;
  };

  int value = 3;
  std::vector<Member> members;
  members.emplace_back(value);
  return *members[0].pointer != 3;
}
