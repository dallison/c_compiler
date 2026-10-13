struct Reflection {
  template <class T>
  T* get(int) const {
    return T::missing();
  }
};

template <>
int* Reflection::get(int) const {
  static int value = 7;
  return &value;
}

int main() {
  Reflection reflection;
  return *reflection.get<int>(0) != 7;
}
