// RUN: -std=c++17
// EXPECT_EXIT: 0

namespace example {

class Outer {
 public:
  class Owner {
   public:
    class Secret {
      friend class ::example::Outer::Owner;

     private:
      static int Read() { return 31; }
    };

    static int ReadSecret() { return Secret::Read(); }
  };
};

}  // namespace example

int main() { return example::Outer::Owner::ReadSecret() == 31 ? 0 : 1; }
