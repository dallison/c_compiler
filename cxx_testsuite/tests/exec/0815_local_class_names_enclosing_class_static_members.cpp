// A member function of a local class can name the static members of the class
// whose member function encloses it, unqualified, including private ones.

class Outer {
 public:
  static int Run();

 private:
  static int Twice(int v) { return v * 2; }
  static int Pick(int v) { return v + 1; }
  static int Pick(int v, int w) { return v + w; }
  static int base_;
};

int Outer::base_ = 10;

int Outer::Run() {
  struct Local {
    int Call(int t) { return Twice(t); }
    int One(int t) { return Pick(t); }
    int Two(int t) { return Pick(t, 3); }
    int Data() { return base_; }
  };
  Local local;
  if (local.Call(4) != 8) return 1;
  if (local.One(4) != 5) return 2;
  if (local.Two(4) != 7) return 3;
  if (local.Data() != 10) return 4;
  return 0;
}

int main() { return Outer::Run(); }
