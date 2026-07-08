// RUN: -std=c++11
// Regression: a translation unit whose final line has no trailing newline must
// still lex its last token.  feof() reports EOF as soon as the last line is
// read, so space/comment skipping must be driven by the buffered line position
// rather than the source EOF flag.  IMPORTANT: this file must NOT end with a
// newline; the declaration below is intentionally the final unterminated line.
int no_trailing_newline = 42;