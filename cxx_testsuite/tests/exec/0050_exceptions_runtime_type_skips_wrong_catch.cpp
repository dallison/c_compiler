// RUN: -std=c++20

int main(void) {
  try {
    throw 42;
    return 1;
  } catch (char value) {
    return 2;
  } catch (int value) {
    return value - 42;
  } catch (...) {
    return 3;
  }
}
