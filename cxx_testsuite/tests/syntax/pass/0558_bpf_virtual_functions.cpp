// RUN: -target bpf -std=c++20
struct Base {
  virtual int id();
};

int go(Base* p) { return p->id(); }
