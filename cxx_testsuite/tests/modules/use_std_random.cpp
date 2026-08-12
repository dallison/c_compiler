import std;

int main() {
  std::minstd_rand engine(17);
  if (engine() != 820607) return 1;

  int values[] = {1, 2, 3, 4};
  std::shuffle(values, values + 4, engine);
  std::ranges::sort(values);
  if (values[0] != 1 || values[1] != 2 ||
      values[2] != 3 || values[3] != 4) {
    return 2;
  }

  int population[] = {10, 20, 30, 40};
  int selected[2] = {};
  if (std::sample(population, population + 4, selected, 2, engine) !=
      selected + 2) {
    return 3;
  }
  if (selected[0] == selected[1]) return 4;
  return 0;
}
