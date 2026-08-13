// RUN: -std=c++26
// EXPECT: Use of deleted function choose (void choose(int)): integral values are not supported

void choose(int) = delete("integral values are not supported");

int main() {
  choose(3);
  return 0;
}
