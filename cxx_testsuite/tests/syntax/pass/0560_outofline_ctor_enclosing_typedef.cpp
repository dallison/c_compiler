// RUN: -std=c++17
//
// An out-of-line nested constructor's parameter list is outside the class
// body.  A typedef of the enclosing class (`CordRep`) must still be found.
// A namespace function between the class and the definition must not hide it.

struct CordRep {
  int value;
};

class Cord {
  using CordRep = ::CordRep;

  class InlineRep {
   public:
    InlineRep() {}
    explicit InlineRep(CordRep* rep);
    int data_;
  };
};

void BetweenCordAndItsConstructor() {}

Cord::InlineRep::InlineRep(CordRep* rep) : data_(rep->value) {}
