#ifndef davecc_crypt_h
#define davecc_crypt_h

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DaveOS toy crypt(3). Setting "$d$" yields salt; otherwise a new salt is
 * chosen. Output format: "$d$<8hexsalt>$<8hexhash>" (FNV-1a of salt||key).
 * Empty password hashes as empty string (login accepts empty shadow field).
 */
char* crypt(const char* key, const char* setting);

#ifdef __cplusplus
}
#endif

#endif
