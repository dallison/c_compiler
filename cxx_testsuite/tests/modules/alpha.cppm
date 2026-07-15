export module alpha;

int secret();
export int alpha_value() { return secret(); }
int secret() { return 3; }
