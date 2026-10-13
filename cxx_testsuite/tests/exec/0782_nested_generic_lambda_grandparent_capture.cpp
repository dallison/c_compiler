struct Info {
  static constexpr bool is_map = true;

  template <class Function>
  void visit_elements(Function function) {
    function(1, 7);
  }
};

template <class Function>
void apply(Function function) {
  function(Info{});
}

template <class Function>
int visit(Function&& function) {
  int keys = 0;
  apply([&](auto info) {
    if constexpr (info.is_map) {
      info.visit_elements([&](auto key, auto value) {
        function(value);
        keys += key;
      });
    }
  });
  return keys;
}

int main() {
  int value = 0;
  int keys = visit([&](int input) { value = input; });
  return value != 7 || keys != 1;
}
