// RUN: -std=c++11
// A using-directive does not declare C in this scope.  Defining class C
// (including with a base-clause that names C) must not crash.
namespace NS {
class C {};
}
using namespace NS;
class C : C {};
