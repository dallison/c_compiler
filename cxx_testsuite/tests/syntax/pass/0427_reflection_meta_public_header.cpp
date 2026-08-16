// RUN: -std=c++26

#include <meta>

using namespace std::meta;

struct base {
  int inherited;
};

struct sample : base {
 public:
  int visible;
  static int counter;

 private:
  long hidden;
};

enum class color { red, green, blue };

template <class T>
struct holder {
  T value;
};

template <class T>
void fn(T) {}

static_assert(is_type(^^sample));
static_assert(is_type_alias(^^info));
static_assert(is_namespace(^^std));
static_assert(is_class_type(^^sample));
static_assert(is_enum_type(^^color));
static_assert(is_data_member(^^sample::visible));
static_assert(is_static_member(^^sample::counter));
static_assert(is_function(^^fn<int>));
static_assert(is_class_template(^^holder));
static_assert(has_parent(^^sample::visible));
static_assert(parent_of(^^sample::visible) == ^^sample);
static_assert(type_of(^^sample::visible) == ^^int);
static_assert(is_public(^^sample::visible));
static_assert(is_private(^^sample::hidden));
static_assert(is_complete_type(^^sample));
static_assert(is_enumerable_type(^^color));
static_assert(has_identifier(^^sample::visible));
static_assert(has_identifier(^^color::green));
static_assert(is_accessible(^^sample::visible, access_context::current()));
static_assert(!is_accessible(^^sample::hidden, access_context::current()));
static_assert(is_accessible(^^sample::hidden, access_context::unchecked()));
static_assert(has_inaccessible_nonstatic_data_members(^^sample,
                                                      access_context::current()));
static_assert(!has_inaccessible_nonstatic_data_members(
    ^^sample, access_context::unchecked()));

constexpr auto bases = bases_of(^^sample);
constexpr auto static_members = static_data_members_of(^^sample);
constexpr auto nonstatic_members =
    nonstatic_data_members_of(^^sample, access_context::unchecked());
constexpr auto subobjects =
    subobjects_of(^^sample, access_context::unchecked());
constexpr auto colors = enumerators_of(^^color);

static_assert(bases.size() == 1);
static_assert(is_base(bases[0]));
static_assert(static_members.size() == 1);
static_assert(nonstatic_members.size() == 2);
static_assert(subobjects.size() >= 2);
static_assert(colors.size() == 3);
static_assert(template_of(^^holder<int>) == ^^holder);
static_assert(return_type_of(^^fn<int>) == ^^void);
static_assert(size_of(^^sample) > 0);
static_assert(alignment_of(^^sample) > 0);
static_assert(offset_of(^^sample::visible).bytes >= 0);
static_assert(bit_size_of(^^int) == size_of(^^int) * 8);

static_assert(is_integral_type(^^int));
static_assert(is_void_type(^^void));
static_assert(is_pointer_type(^^int*));
static_assert(is_lvalue_reference_type(^^int&));
static_assert(is_rvalue_reference_type(^^int&&));
static_assert(is_array_type(^^int[3]));
static_assert(is_same_type(^^int, remove_cv(^^const int)));
static_assert(is_base_of_type(^^base, ^^sample));
static_assert(is_convertible_type(^^int, ^^long));
static_assert(decay(^^int[2]) == ^^int*);
static_assert(add_pointer(^^int) == ^^int*);
static_assert(remove_pointer(^^int*) == ^^int);
static_assert(add_const(^^int) == ^^const int);
static_assert(remove_const(^^const int) == ^^int);
static_assert(underlying_type(^^color) == ^^int);

consteval bool query_ranges_compile() {
  (void)members_of(^^sample, access_context::unchecked());
  (void)parameters_of(^^fn<int>);
  (void)template_arguments_of(^^holder<int>);
  return true;
}

static_assert(query_ranges_compile());
static_assert(has_identifier(^^sample::visible));

int main() { return 0; }
