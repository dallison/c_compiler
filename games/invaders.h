// Space Invaders constants for invaders.cc and invaders_fx.s. The assembler
// reads this file too, so it holds plain #defines and nothing else.
//
// Units are screen1.s's: byte columns (4 px), character rows (8 px), and
// pixel rows.

// 5 ranks of 11. Alien i is rank i / 11, column i % 11, at byte column
// origin + 5 * column and character row origin + 2 * rank.
#define FLEET_SIZE 55
#define FLEET_COLS 11
// Live aliens the fleet redraws a frame, a step on.
#define FLEET_QUOTA 1

// The laser: a 1 px line SHOT_LEN rows long, SHOT_STEP rows a frame. Its
// strip is SHOT_SPAN rows, the line and then the rows it leaves behind.
#define SHOT_LEN 6
#define SHOT_STEP 4
#define SHOT_SPAN 10
#define SHOT_START 202
#define SHOT_TOP 28

#define BOMBS 3
#define BOMB_LEN 8
#define BOMB_STEP 2

#define PLAYER_ROW 26
#define PLAYER_Y 208
#define CANNON_MIN 8
#define CANNON_MAX 296
#define CANNON_STEP 2

#define UFO_ROW 4
#define UFO_Y 32
#define UFO_MIN 4
#define UFO_MAX 300
#define UFO_STEP 2

// Shelters fill character rows BUNKER_ROW and BUNKER_ROW + 1.
#define BUNKER_ROW 22
#define BUNKER_TOP 176
#define BUNKER_END 192
#define GROUND_Y 232

// key_bits: what keys_read found held this frame.
#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_FIRE 4
#define KEY_ESCAPE 8
// Keys keys_read scans: key_codes and key_masks in invaders.cc.
#define KEY_COUNT 9

// Byte columns of the score, lives and wave digits on the top row.
#define HUD_SCORE_COL 6
#define HUD_LIVES_COL 18
#define HUD_WAVE_COL 24
