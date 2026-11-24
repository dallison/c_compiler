// Defender constants for defender.cc and defender_fx.s.
// The assembler reads this file too, so it holds plain #defines and
// nothing else.
//
// Shadow Mode 2 (160x256, 8 steady colours, 2 px per byte column).
// World x is a 9-bit sub-pixel coordinate (0..511, wrapping): 4 world units
// is one screen byte column (2 Mode-2 pixels) and bit 1 selects the 1-pixel
// Mode-2 horizontal shift phase.

#define DF_WORLD_W 512
#define DF_SCREEN_W 320
#define DF_SPR_H 12

// The camera keeps the ship at byte column 37..42 (12 Mode-2 px wide).
#define DF_CAM_OFF 148
#define DF_SHIP_COL 37
#define DF_SHIP_NC 6

// Ship y band (top pixel row of the 12-row sprite, below y=31 dashboard).
#define DF_Y_MIN 34
#define DF_Y_MAX 238
#define DF_STEP_Y 4
#define DF_VMAX 6
#define DF_FIRE_RATE 4
#define DF_INVULN 90

// Entity slots. 0..7 landers/mutants, 8..9 bombers, 10..17 humans,
// 18..20 mines. Slots below DF_HOSTILE_END are what a laser can hit.
#define DF_ENTS 21
#define DF_FIRST_ENEMY 0
#define DF_N_ENEMY 8
#define DF_FIRST_BOMBER 8
#define DF_N_BOMBER 2
#define DF_FIRST_HUMAN 10
#define DF_N_HUMAN 8
#define DF_FIRST_BOMB 18
#define DF_N_BOMB 3
#define DF_HOSTILE_END 10

#define ET_NONE 0
#define ET_LANDER 1
#define ET_HUMAN 2
#define ET_MUTANT 3
#define ET_BOMBER 4
#define ET_BOMB 5

// Human states (e_st) and lander states.
#define HS_WALK 0
#define HS_CARRIED 1
#define HS_FALL 2
#define LS_HUNT 0
#define LS_RISE 2

#define DF_BOOM_FRAMES 10
#define DF_BOMB_LIFE 250
// A lander that climbs this high with a human turns into a mutant.
#define DF_TOP_Y 38
// A human that falls further than this dies on landing.
#define DF_FALL_MAX 72

// Expanding streaking laser beams: 5 byte-cols/frame, up to 16 byte-cols long.
#define DF_SHOTS 4
#define DF_SHOT_STEP 20
#define DF_SHOT_LIFE 11

// Parallax starfield in open sky (38..206).
#define DF_STARS 16

// Terrain: one height sample per byte column of world, 128 of them,
// anchored along the bottom of the screen (216..253).
#define DF_TERR_COLS 128
#define DF_TERR_MIN 216
#define DF_TERR_MAX 244
#define DF_TERR_Y0 192
#define DF_VIEW_COLS 80

// Top-centre framed radar minimap: interior cols 24..55 (32 cols), y=0..31.
#define DF_SCAN_COL 24

#define HUD_ICON_ROW 1
#define HUD_NUM_ROW 2
#define HUD_LIVES_COL 0
#define HUD_SCORE_COL 0
#define HUD_BOMB_COL 60
#define HUD_WAVE_COL 72

// key_bits: Planetoid-style controls read by keys_read each frame.
#define KEY_THRUST 1
#define KEY_REVERSE 2
#define KEY_UP 4
#define KEY_DOWN 8
#define KEY_FIRE 16
#define KEY_ESCAPE 32
#define KEY_BOMB 64
#define KEY_HYPER 128
#define KEY_COUNT 13
