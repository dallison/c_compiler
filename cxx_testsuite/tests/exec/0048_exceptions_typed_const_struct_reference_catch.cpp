// RUN: -std=c++20

struct Payload {
  int value;
};

Payload payload = {13};

int main(void) {
  try {
    throw payload;
    return 1;
  } catch (const Payload& value) {
    return value.value - 13;
  }
}
