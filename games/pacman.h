// Pac-Man constants for pacman.cc and pacman_fx.s. The assembler reads
// this file too, so it holds plain #defines and nothing else.
//
// Shadow Mode 2 (VDU 22,130): 160x256, 8 steady colours, 2 pixels per byte
// (80 byte columns by 256 scanlines).
// Each maze tile is 4 byte columns (8 Mode-2 px) by 12 scanlines, divided
// into 4 smooth sub-steps horizontally and vertically.

#define PM_COLS 19
#define PM_ROWS 19
#define PM_GHOSTS 4

#define PM_SUB 4
#define PM_TILE_COLS 4
#define PM_TILE_H 12
#define PM_SPRITE_BYTES 48

// Board origin: byte column 2 (76 cols centred in 80) and scanline 16.
#define PM_ORIGIN_COL 2
#define PM_ORIGIN_Y 16

// Tile kinds in maze[]. Connected walls occupy 16..31 (PM_WALL_BASE | mask).
#define PM_EMPTY 0
#define PM_VOID 1
#define PM_PELLET 2
#define PM_POWER 3
#define PM_GATE 4
#define PM_WALL_BASE 16

// Directions in pac_dir / ghost_dir: 0 right, 1 left, 2 up, 3 down, 255 none.
#define PM_DIR_RIGHT 0
#define PM_DIR_LEFT 1
#define PM_DIR_UP 2
#define PM_DIR_DOWN 3
#define PM_DIR_NONE 255

#define PM_BLINKY 0
#define PM_PINKY 1
#define PM_INKY 2
#define PM_CLYDE 3

// Ghost house / gate / warp tunnel / Pac start in 19x19 maze coordinates.
#define PM_EXIT_X 9
#define PM_EXIT_Y 7
#define PM_GATE_X 9
#define PM_GATE_Y 8
#define PM_HOUSE_Y 9
#define PM_TUNNEL_Y 9
#define PM_PAC_X0 9
#define PM_PAC_Y0 13
#define PM_PELLET_TOTAL 165

// Mode-2 HUD on character row 0 (80 byte columns = 20 text columns).
#define HUD_ROW 0
#define HUD_SCORE_COL 8
#define HUD_WAVE_COL 44
#define HUD_LIVES_COL 68

#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_UP 4
#define KEY_DOWN 8
#define KEY_FIRE 16
#define KEY_ESCAPE 32
#define KEY_COUNT 15
