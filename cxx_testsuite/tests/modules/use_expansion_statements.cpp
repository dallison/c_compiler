import expansion_statements;

static_assert(expansion_sum<1, 2, 3>() == 6);

int main() {
  return expansion_sum<1, 2, 3>() - 6;
}
