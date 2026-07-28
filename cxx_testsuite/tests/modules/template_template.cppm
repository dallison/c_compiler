export module template_template;

export template <class T>
struct module_box {
  T value;
};

export template <template <class> class C, class T>
struct module_holder {
  C<T> value;
};
