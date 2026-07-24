// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <vector>

int test_vector_at_throws(void) {
  std::vector<int> values;
  values.push_back(7);
  values.push_back(8);

  int caught = 0;
  try {
    (void)values.at(2);
    return 1;
  } catch (const std::exception& ex) {
    const char* msg = ex.what();
    if (msg == 0 || msg[0] != 'v') {
      return 2;
    }
    caught = 1;
  } catch (...) {
    return 3;
  }

  if (!caught || values.size() != 2 || values[0] != 7 || values[1] != 8) {
    return 4;
  }

  const std::vector<int>& cref = values;
  try {
    (void)cref.at(4);
    return 5;
  } catch (const std::logic_error& ex) {
    const char* msg = ex.what();
    if (msg == 0 || msg[0] != 'v') {
      return 6;
    }
  } catch (...) {
    return 7;
  }

  return 0;
}

int test_vector_at_catch_all(void) {
  std::vector<int> values;
  int caught = 0;
  try {
    (void)values.at(0);
    return 20;
  } catch (...) {
    caught = 1;
  }
  if (!caught || !values.empty()) {
    return 21;
  }
  return 0;
}

int test_direct_stdexcept_throw(void) {
  try {
    throw std::out_of_range((const char*)"direct");
  } catch (const std::out_of_range& ex) {
    const char* msg = ex.what();
    if (msg == 0 || msg[0] != 'd') {
      return 10;
    }
  } catch (...) {
    return 11;
  }
  return 0;
}

int main(void) {
  int result = test_direct_stdexcept_throw();
  if (result != 0) {
    return result;
  }
  result = test_vector_at_throws();
  if (result != 0) {
    return result;
  }
  result = test_vector_at_catch_all();
  if (result != 0) {
    return result;
  }
  return 0;
}
