// Names the assembly printf/scanf image can call. The real printers start
// with __, which the shim generator does not export.

extern char* __PrintFloatFormat(double f, int precision, char* buf,
                                unsigned size);
extern char* __PrintScientificFormat(double f, int precision, char* buf,
                                     unsigned size);
extern char* __PrintGeneralFormat(double f, int precision, char* buf,
                                  unsigned size);
extern char* __PrintHexFormat(double f, int precision, int upper,
                              int alternate, char* buf, unsigned size);
extern double strtod(const char* str, char** endptr);

char* pf_fixed(double f, int precision, char* buf, unsigned size) {
  return __PrintFloatFormat(f, precision, buf, size);
}

char* pf_scientific(double f, int precision, char* buf, unsigned size) {
  return __PrintScientificFormat(f, precision, buf, size);
}

char* pf_general(double f, int precision, char* buf, unsigned size) {
  return __PrintGeneralFormat(f, precision, buf, size);
}

char* pf_hex(double f, int precision, int upper, int alternate, char* buf,
             unsigned size) {
  return __PrintHexFormat(f, precision, upper, alternate, buf, size);
}

double pf_strtod(const char* str, char** endptr) {
  return strtod(str, endptr);
}
