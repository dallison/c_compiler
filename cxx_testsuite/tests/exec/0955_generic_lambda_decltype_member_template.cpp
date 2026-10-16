// RUN: -std=c++17
// A generic lambda inside a member function template names a type through
// `decltype` of its `auto` parameter and calls a member template of the
// enclosing class with it.  The decltype operand is rebound when the lambda
// body is instantiated, including from a lambda nested inside another generic
// lambda, so the call resolves instead of staying dependent.  An operand that
// names the lambda's own still-dependent parameter keeps that parameter when
// only the enclosing template is instantiated.
template <class T> struct Tag { using type = T; };
struct NodeBase { char storage[32]; };

struct Base {
  int storage = 4;
  int k = 3;
  double v = 0.5;

  template <typename T> T* Get() { return reinterpret_cast<T*>(&storage); }
  template <typename T> T* GetKey() { return reinterpret_cast<T*>(&k); }
  template <typename T> T* GetValue() { return reinterpret_cast<T*>(&v); }
  template <typename T> T* GetNodeValue(NodeBase* node) const {
    return reinterpret_cast<T*>(node->storage);
  }

  template <typename G> int ThisCapture(G) {
    auto l = [this](auto tag) {
      return *this->Get<typename decltype(tag)::type>();
    };
    return l(Tag<int>{});
  }

  template <typename G> int LocalOfDecltypeType(G) {
    auto l = [this](auto tag) {
      typename decltype(tag)::type value = storage;
      return value;
    };
    return l(Tag<int>{});
  }

  template <typename G> int ImplicitThis(G) {
    auto l = [&](auto tag) { return *Get<typename decltype(tag)::type>(); };
    return l(Tag<int>{});
  }

  template <typename F> auto VisitNodeType(F f) const { return f(Tag<int>{}); }
  template <typename F> auto VisitNode(NodeBase* node, F f) const {
    return VisitNodeType([&](auto value_type) {
      return f(GetNodeValue<typename decltype(value_type)::type>(node));
    });
  }

  template <typename F> void VisitKeyType(F f) { f(Tag<int>{}); }
  template <typename F> void VisitValueType(F f) { f(Tag<double>{}); }
  template <typename F> void VisitAll(F f) {
    VisitKeyType([&](auto key_type) {
      VisitValueType([&](auto value_type) {
        f(GetKey<typename decltype(key_type)::type>(),
          GetValue<typename decltype(value_type)::type>());
      });
    });
  }

  template <typename G> int NonGenericInner(G) {
    auto outer = [&](auto key_type) {
      auto inner = [&](int) {
        return *GetKey<typename decltype(key_type)::type>();
      };
      return inner(0);
    };
    return outer(Tag<int>{});
  }
};

template <class R, class Visitor> R VisitForwarded(Visitor&& vis, int& var) {
  auto invoke = [&](auto&& value) -> R {
    return vis(static_cast<decltype(value)&&>(value));
  };
  return invoke(var);
}

int main() {
  Base b;
  if (b.ThisCapture(0) != 4) return 1;
  if (b.LocalOfDecltypeType(0) != 4) return 2;
  if (b.ImplicitThis(0) != 4) return 3;

  NodeBase node;
  int r = 0;
  b.VisitNode(&node, [&](auto* value) {
    *value = 3;
    r = *value;
  });
  if (r != 3) return 4;

  r = 0;
  b.VisitAll([&](int* key, double* value) { r = *key + (int)(*value * 2); });
  if (r != 4) return 5;

  if (b.NonGenericInner(0) != 3) return 6;

  int var = 4;
  int seen = 0;
  VisitForwarded<void>([&](auto item) { seen = (int)item; }, var);
  if (seen != 4) return 7;
  if (VisitForwarded<int>([](auto item) { return item + 1; }, var) != 5)
    return 8;
  return 0;
}
