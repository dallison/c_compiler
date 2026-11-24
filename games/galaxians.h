// Galaxian constants for galaxians.cc and galaxians_fx.s. The assembler
// reads this file too, so it holds plain #defines and nothing else.
//
// Units are screen1.s's: byte columns (4 px), character rows (8 px), and
// pixel rows.

// 5 ranks of 8. Alien i is rank i / 8, column i % 8, at byte column
// origin + 5 * column and character row origin + 2 * rank. Spacing is
// 5 so the 6-column sprite windows share a blank pad and do not tear.
#define FLEET_SIZE 40
#define FLEET_COLS 8
#define FLEET_RANKS 5
// Live aliens the convoy redraws a frame, a step on.
#define FLEET_QUOTA 2

// Concurrent diving attackers.
#define DIVERS 2

// The laser: a 1 px line SHOT_LEN rows long, SHOT_STEP rows a frame.
#define SHOT_LEN 6
#define SHOT_STEP 4
#define SHOT_SPAN 10
#define SHOT_START 202
#define SHOT_TOP 24

#define BOMBS 3
#define BOMB_LEN 8
#define BOMB_STEP 2

#define PLAYER_ROW 26
#define PLAYER_Y 208
#define CANNON_MIN 8
#define CANNON_MAX 296
#define CANNON_STEP 2

#define GROUND_Y 232

// key_bits: what keys_read found held this frame.
#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_FIRE 4
#define KEY_ESCAPE 8
#define KEY_COUNT 9

#define HUD_ROW 1
#define HUD_SCORE_COL 5
#define HUD_WAVE_COL 39
#define HUD_LIVES_COL 70

// Rank kinds (idx_rank maps to these).
#define RANK_FLAG 0
#define RANK_RED 1
#define RANK_PURPLE 2
#define RANK_BLUE0 3
#define RANK_BLUE1 4
