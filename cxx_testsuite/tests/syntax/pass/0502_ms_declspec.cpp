// RUN: -std=c++11
// Microsoft __declspec is parsed as a declaration attribute, not a call.
__declspec(dllexport) int exported_global;
int __declspec(dllexport) exported_after_type = 1;
__declspec(dllimport) extern int imported_global;
__declspec(align(8)) int aligned_global;
__declspec(dllexport) void exported_fn(void);
struct __declspec(dllexport) ExportedClass {
  int x;
};
void local_scope(void) {
  __declspec(dllexport) int local_var;
}
enum __declspec(dllexport) ExportedEnum { kA };
__declspec(dllexport) ExportedClass exported_object;
