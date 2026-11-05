// Asteroids constants for asteroids.cc and asteroids_fx.s. The assembler
// reads this file too, so it holds plain #defines and nothing else.
//
// Units are screen1.s's: byte columns (4 px) and character rows (8 px).
// Positions in C++ are pixels; paint helpers convert to those units.

#define AS_ROCKS 12
#define AS_SHOTS 4

// Ship / rock sprite window: 8x8 → two byte columns, one character row.
#define AS_CELL_LEN 16
// Large rock: 16x16 → four byte columns on each of two character rows.
#define AS_LARGE_LEN 32
// Missile: vertical strip of AS_SHOT_LEN pixel rows (fx_strip), with
// four horizontal phases so it steps one Mode-1 pixel at a time.
#define AS_SHOT_LEN 3
// Soft rocks: column-major strips at pixel Y. 2px X phases are paired
// back-to-back (shift0 then shift2). Large = 6 cols x 16 rows; small =
// 4 cols x 8 rows (leading/trailing blank columns for edge wipe).
#define AS_SOFT_LARGE_COLS 6
#define AS_SOFT_LARGE_ROWS 16
#define AS_SOFT_LARGE_PHASE 96
#define AS_SOFT_SMALL_COLS 4
#define AS_SOFT_SMALL_ROWS 8
#define AS_SOFT_SMALL_PHASE 32
// Ship soft blit: cell16 drawn as 4 columns at pixel Y.
#define AS_SOFT_SHIP_COLS 4
#define AS_SOFT_SHIP_ROWS 16

#define AS_HUD_ROW 1
#define AS_HUD_SCORE_COL 5
#define AS_HUD_BEST_COL 32
#define AS_HUD_LIVES_COL 70

// key_bits from keys_read.
#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_THRUST 4
#define KEY_FIRE 8
#define KEY_ESCAPE 16
#define KEY_COUNT 10

// Rock sizes in rock_size[].
#define ROCK_LARGE 0
#define ROCK_MED 1
#define ROCK_SMALL 2
