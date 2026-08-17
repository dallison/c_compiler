// RUN: -std=c++26 -Werror=deprecated-declarations

template <class... Types>
void function_parameter_pack(Types...);

void abbreviated_parameter_pack(auto...);
