// RUN: -std=c++26

template <typename T>
concept Predicate = true;

template <typename T>
struct ClassTemplate {};

template <typename T>
constexpr bool variable_template = true;

template <template <typename> concept C>
struct NeedsConcept {};

template <template <typename> auto V>
struct NeedsVariable {};

template <template <typename> typename T>
struct NeedsType {};

NeedsConcept<ClassTemplate> wrong_concept;
NeedsVariable<Predicate> wrong_variable;
NeedsType<variable_template> wrong_type;
