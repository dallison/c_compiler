// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <meta>

using namespace std::meta;

static_assert(__cpp_impl_reflection == 202603L);
static_assert(__cpp_lib_reflection == 202603L);

struct base {
  int inherited;
};

struct sample : base {
 public:
  int first;

 private:
  long hidden;
};

#define TEN_MEMBERS(prefix) \
  int prefix##0;            \
  int prefix##1;            \
  int prefix##2;            \
  int prefix##3;            \
  int prefix##4;            \
  int prefix##5;            \
  int prefix##6;            \
  int prefix##7;            \
  int prefix##8;            \
  int prefix##9

struct large_sample {
  TEN_MEMBERS(a);
  TEN_MEMBERS(b);
  TEN_MEMBERS(c);
  TEN_MEMBERS(d);
  TEN_MEMBERS(e);
  TEN_MEMBERS(f);
  TEN_MEMBERS(g);
  TEN_MEMBERS(h);
  TEN_MEMBERS(i);
  TEN_MEMBERS(j);
  TEN_MEMBERS(k);
  TEN_MEMBERS(l);
  TEN_MEMBERS(m);
  TEN_MEMBERS(n);
  TEN_MEMBERS(o);
  TEN_MEMBERS(p);
  TEN_MEMBERS(q);
  TEN_MEMBERS(r);
  TEN_MEMBERS(s);
  TEN_MEMBERS(t);
  TEN_MEMBERS(u);
  TEN_MEMBERS(v);
  TEN_MEMBERS(w);
  TEN_MEMBERS(x);
  TEN_MEMBERS(y);
  TEN_MEMBERS(z);
  TEN_MEMBERS(aa);
  TEN_MEMBERS(ab);
  TEN_MEMBERS(ac);
  TEN_MEMBERS(ad);
};

#undef TEN_MEMBERS

enum class color { red, green, blue };

using alias = int;

static_assert(is_type(^^sample), "is_type");
static_assert(is_type_alias(^^alias), "is_type_alias");
static_assert(dealias(^^alias) == ^^int, "dealias");
static_assert(type_of(^^sample::first) == ^^int, "type_of");
static_assert(parent_of(^^sample::first) == ^^sample, "parent_of");
static_assert(has_identifier(^^sample::first));
static_assert(has_identifier(^^sample::hidden));
static_assert(is_data_member(^^sample::first));
static_assert(is_public(^^sample::first));
static_assert(is_private(^^sample::hidden));
static_assert(is_complete_type(^^sample));
static_assert(is_enumerable_type(^^color));
static_assert(size_of(^^int) > 0);
static_assert(alignment_of(^^sample) >= alignment_of(^^int));
static_assert(offset_of(^^sample::first).bytes >= 0);
static_assert(is_integral_type(^^int));
static_assert(is_same_type(^^int, dealias(^^alias)));

constexpr auto public_members = nonstatic_data_members_of(^^sample);
constexpr auto all_members =
    nonstatic_data_members_of(^^sample, access_context::unchecked());
constexpr auto colors = enumerators_of(^^color);
constexpr auto large_members = nonstatic_data_members_of(^^large_sample);
constexpr auto bases = bases_of(^^sample);
constexpr auto subobjects = subobjects_of(^^sample, access_context::unchecked());

static_assert(public_members.size() == 1);
static_assert(all_members.size() == 2);
static_assert(is_private(all_members[1]));
static_assert(colors.size() == 3);
static_assert(large_members.size() == 300);
static_assert(is_data_member(public_members[0]));
static_assert(is_enumerator(colors[1]));
static_assert(bases.size() == 1);
static_assert(is_base(bases[0]));
static_assert(subobjects.size() >= 2);

consteval int count_public_members() {
  int count = 0;
  template for (constexpr auto member :
                nonstatic_data_members_of(^^sample)) {
    ++count;
  }
  return count;
}

static_assert(count_public_members() == 1);

consteval bool names_available() {
  return has_identifier(^^sample::first) &&
         has_identifier(^^sample::hidden);
}

static_assert(names_available());

consteval bool trait_checks() {
  return is_class_type(^^sample) && is_scalar_type(^^int) &&
         is_base_of_type(^^base, ^^sample) &&
         remove_cv(^^const int) == ^^int;
}

static_assert(trait_checks());

int main() {
  return 0;
}
