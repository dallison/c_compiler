struct Map {
  struct Iterator {
    int value;

    friend bool operator==(const Iterator& left, const Iterator& right) {
      return left.value == right.value;
    }
  };

  Iterator find() const { return {1}; }
  Iterator end() const { return {1}; }
};

auto make_callback() {
  Map map;
  return [map = static_cast<Map&&>(map)] {
    auto it = map.find();
    return it == map.end();
  };
}

int main() {
  auto callback = make_callback();
  return callback() ? 0 : 1;
}
