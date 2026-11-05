// Snake constants for snake.cc and snake_fx.s. The assembler reads this
// file too, so it holds plain #defines and nothing else.
//
// Units are screen1.s's: byte columns (4 px) and character rows (8 px).
// Each board cell is one Mode-1 character cell: two byte columns by one
// character row (8x8 pixels).

#define SN_COLS 36
#define SN_ROWS 24
#define SN_MAX 128
#define SN_FRUITS 5
#define SN_ROCKS 28

// Board origin on the shadow screen: byte column and character row of
// cell (0, 0). Leaves a two-text-column margin and room for the HUD.
#define SN_ORIGIN_COL 4
#define SN_ORIGIN_ROW 4

// HUD digit row and first-digit byte columns (text columns are 2 bytes).
#define SN_HUD_ROW 1
#define SN_HUD_SCORE_COL 5
#define SN_HUD_BEST_COL 39
#define SN_HUD_LIVES_COL 70

// key_bits from keys_read.
#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_UP 4
#define KEY_DOWN 8
#define KEY_FIRE 16
#define KEY_ESCAPE 32
#define KEY_COUNT 15

// Fruit kinds in fruit_kind[].
#define FRUIT_NONE 0
#define FRUIT_APPLE 1
#define FRUIT_BONUS 2
