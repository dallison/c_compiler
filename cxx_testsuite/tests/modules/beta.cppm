export module beta;

int secret();
export int beta_value() { return secret(); }
int secret() { return 4; }
