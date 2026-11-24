// Centipede constants for centipede.cc and centipede_fx.s. The assembler
// reads this file too, so it holds plain #defines and nothing else.
//
// Units are screen1.s's: byte columns (4 px) and character rows (8 px).
// Each board cell is one Mode-1 character cell: two byte columns by one
// character row (8x8 pixels).

#define CE_COLS 28
#define CE_ROWS 24
#define CE_SEGS 12
#define CE_MUSH_MAX 4

// Board origin: byte column and character row of cell (0, 0).
#define CE_ORIGIN_COL 8
#define CE_ORIGIN_ROW 3

// Player is confined to the bottom CE_PLAYER_ROWS of the board.
#define CE_PLAYER_ROWS 4
#define CE_PLAYER_MIN_ROW (CE_ROWS - CE_PLAYER_ROWS)

#define CE_SHOT_LEN 6
#define CE_SHOT_STEP 4
#define CE_SHOT_SPAN 10

#define HUD_ROW 1
#define HUD_SCORE_COL 5
#define HUD_WAVE_COL 39
#define HUD_LIVES_COL 70

// key_bits from keys_read.
#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_UP 4
#define KEY_DOWN 8
#define KEY_FIRE 16
#define KEY_ESCAPE 32
#define KEY_COUNT 15

// Segment direction in seg_dir[]: +1 right, 255 left.
#define DIR_RIGHT 1
#define DIR_LEFT 255
