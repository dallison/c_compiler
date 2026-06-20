// RUN: -std=c++20

struct Payload {
  int value;
};

int main(void) {
  Payload payload = {17};
  try {
    throw payload;
    return 1;
  } catch (Payload value) {
    return value.value - 17;
  }
}
