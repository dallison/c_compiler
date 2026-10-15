// A local class with member functions inside a member function of a class
// template (absl's ConvertToContainer<vector<string_view>>::raw_view) is
// rebuilt for each instantiation, like one inside a function template.
struct View {
  const char* p;
  int size;
};

template <typename D>
struct S {
  int Get() const {
    struct Raw {
      int n;
      int get() const { return n + (int)sizeof(D); }
    };
    Raw r{3};
    return r.get();
  }

  template <typename C, typename V>
  struct Convert {
    View operator()(const S&) const { return {nullptr, 0}; }
  };
  template <typename A>
  struct Convert<A*, View> {
    View operator()(const S&) const {
      struct raw_view {
        const char* data;
        int size;
        operator View() const { return {data, size}; }
      };
      raw_view r{"abc", 3};
      return r;
    }
  };
};

int main() {
  if (S<char>().Get() != 4 || S<int>().Get() != 7) {
    return 1;
  }
  S<int> s;
  View v = S<int>::Convert<long*, View>()(s);
  if (v.size != 3 || v.p[1] != 'b') {
    return 2;
  }
  return S<int>::Convert<long, View>()(s).size == 0 ? 0 : 3;
}
