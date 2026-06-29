// RUN: -std=c++20
// Friendship is granted explicitly and is neither symmetric nor inherited:
// only the named friend may touch the private members.
// EXPECT: secret is a private member of Owner
// EXPECT: token is a private member of Friend

struct Owner {
 private:
  int secret;

 public:
  Owner() : secret(7) {}
  friend struct Friend;
};

struct Friend {
 private:
  int token;

 public:
  Friend() : token(9) {}
  // Friend may read Owner's private member: this is allowed.
  int read(const Owner& o) const { return o.secret; }
};

struct Stranger {
  // Not a friend of Owner: access must be rejected.
  int peek(const Owner& o) const { return o.secret; }
};

// Friendship is not symmetric: Owner is not a friend of Friend, so it cannot
// read Friend's private member.
int leak(const Friend& f) { return f.token; }

int main(void) {
  Owner o;
  Friend f;
  Stranger s;
  return s.peek(o) + leak(f);
}
