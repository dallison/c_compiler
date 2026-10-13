class Owner {
 public:
  int read() {
    struct Local {
      static int get(const Owner& owner) { return owner.value; }
    };
    return Local::get(*this);
  }

 private:
  int value = 7;
};

int main() {
  Owner owner;
  return owner.read() != 7;
}
