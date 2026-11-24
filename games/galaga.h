// Galaga constants for galaga.cc and galaga_fx.s. The assembler reads this
// file too, so it holds plain #defines and nothing else.
//
// Shadow Mode 2 (VDU 22,130): 160x256, 8 steady colours, 2 px per byte.
// Units are screen1.s's: byte columns (2 px in Mode 2), character rows
// (8 px), and pixel rows. Both modes have 80 byte columns, so the fleet
// byte-column layout matches Galaxians; only the horizontal pixel maths
// (>>1, &1, *2), the sprite colours, and the HUD differ.

// 5 ranks of 8. Alien i is rank i / 8, column i % 8, at byte column
// origin + 5 * column and character row origin + 2 * rank. Spacing is 5 so
// the 6-column sprite windows share a blank pad and do not tear.
#define FLEET_SIZE 40
#define FLEET_COLS 8
#define FLEET_RANKS 5
// Live aliens the convoy redraws a frame, a step on.
#define FLEET_QUOTA 2

// Concurrent diving attackers (more than Galaxians for a faster game).
#define DIVERS 4

// Concurrent player laser bolts.
#define SHOTS 2

// The laser: a 1 px line SHOT_LEN rows long, SHOT_STEP rows a frame.
#define SHOT_LEN 6
#define SHOT_STEP 4
#define SHOT_SPAN 10
#define SHOT_START 202
#define SHOT_TOP 24

#define BOMBS 4
#define BOMB_LEN 8
#define BOMB_STEP 3

#define PLAYER_ROW 26
#define PLAYER_Y 208
// Mode-2 pixel range of the cannon's left edge (8 px fighter in 160 px).
#define CANNON_MIN 4
#define CANNON_MAX 148
#define CANNON_STEP 1

// Diver velocity, Mode-2 pixels per frame.
#define DIVE_VX 1
#define DIVE_VY 2

#define GROUND_Y 232
#define LINE_Y 24

// key_bits: what keys_read found held this frame.
#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_FIRE 4
#define KEY_ESCAPE 8
#define KEY_COUNT 9

// HUD on character row 0 (80 byte columns = 20 text columns in Mode 2).
#define HUD_ROW 0
#define HUD_SCORE_COL 8
#define HUD_WAVE_COL 44
#define HUD_LIVES_COL 68

// Rank kinds (idx_rank maps to these).
#define RANK_BOSS 0
#define RANK_FLY0 1
#define RANK_FLY1 2
#define RANK_BEE0 3
#define RANK_BEE1 4
