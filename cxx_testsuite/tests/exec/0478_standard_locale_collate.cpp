// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <locale>
#include <string>

int main() {
  const char direct_left[] = "abc";
  const char direct_right[] = "abd";
  std::collate_byname<char>* direct =
      new std::collate_byname<char>("C", 1);
  if (direct->compare(direct_left, direct_left + 3,
                      direct_right, direct_right + 3) >= 0) {
    return 10;
  }

  std::locale classic = std::locale::classic();
  if (!std::has_facet<std::collate<char> >(classic) ||
      !std::has_facet<std::collate<wchar_t> >(classic)) {
    return 1;
  }
  const std::collate<char>& facet =
      std::use_facet<std::collate<char> >(classic);
  const char abc[] = "abc";
  const char abd[] = "abd";
  if (facet.compare(abc, abc + 3, abd, abd + 3) >= 0) {
    return 2;
  }
  if (facet.compare(abc, abc + 3, abc, abc + 3) != 0) {
    return 3;
  }
  if (facet.transform(abc, abc + 3) != std::string("abc")) {
    return 4;
  }
  if (facet.hash(abc, abc + 3) != facet.hash(abc, abc + 3)) {
    return 5;
  }

  std::collate_byname<char>* by_name =
      new std::collate_byname<char>("C", 1);
  if (by_name->compare(abd, abd + 3, abc, abc + 3) <= 0) {
    return 6;
  }
  return 0;
}
