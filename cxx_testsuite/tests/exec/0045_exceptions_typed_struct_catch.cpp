// RUN: -std=c++20

struct Payload {
  int value;
};

Payload payload = {9};

int main(void) {
  try {
    throw payload;
    return 1;
  } catch (Payload value) {
    value.value = 10;
    if (payload.value != 9) {
      return 2;
    }
    return value.value - 10;
  }
}
