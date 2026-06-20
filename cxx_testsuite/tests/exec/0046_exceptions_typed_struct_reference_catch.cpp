// RUN: -std=c++20

struct Payload {
  int value;
};

Payload payload = {11};

int main(void) {
  try {
    throw payload;
    return 1;
  } catch (Payload& value) {
    value.value = value.value + 1;
    return payload.value - 12;
  }
}
