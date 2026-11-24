// Defender play for defender.cc (Shadow Mode 2, 160x256, 8 steady colours):
// keys, ship, expanding streaking laser beams, entity table (landers, mutants,
// bombers, humans, star-mines), parallax starfield, two-tone terrain ridge,
// framed colour radar scanner, HUD digits/icons, and sound.
//
// Lives in .text.blitter so every symbol stays below &3000 next to screen1.s:
// all state the paint loops read is defined here in low RAM because with
// ACCCON bit X set &3000-&7FFF maps shadow VRAM.

#include "defender.h"

.set OSWORD 0xfff1
.set OSBYTE 0xfff4
.set SYSVIA_LATCH 0xfe40
.set SYSVIA_DDRA 0xfe43
.set SYSVIA_PORTA 0xfe4f

// screen1.s keeps &70-&77 as scratch between calls, so blit_win borrows
// &70-&75 for its three character-row pointers (12-scanline sprites span 2 or
// 3 character rows). Other temps stay in &60-&6F and &78-&7C, clear of both
// the MOS (&90+) and davecc's zero page (ends &5C).
.set P1 0x70
.set P2 0x72
.set P3 0x74

.set Z_CNT 0x60
.set Z_SCAN 0x61
.set Z_NC 0x62
.set Z_YC 0x63
.set Z_N1 0x64
.set Z_CL 0x65
.set Z_CH 0x66
.set Z_VAL 0x67
.set Z_TOP 0x68
.set Z_LEN 0x69
.set Z_W 0x6a
.set Z_I 0x6b
.set E_T0 0x6c
.set E_T1 0x6d
.set E_C 0x6e
.set E_R 0x6f

.set Z_N2 0x78
.set Z_N3 0x79
.set Z_COL 0x7a
.set Z_MASK 0x7b
.set Z_STEP 0x7c

.section ".text.blitter", "ax", @progbits

// ------------------------------------------------------------- state
// Entity table (see defender.h for the slot map).
.global e_type
e_type: .space DF_ENTS
.global e_xl
e_xl: .space DF_ENTS
.global e_xh
e_xh: .space DF_ENTS
.global e_y
e_y: .space DF_ENTS
.global e_vx
e_vx: .space DF_ENTS
.global e_vy
e_vy: .space DF_ENTS
.global e_st
e_st: .space DF_ENTS
.global e_aux
e_aux: .space DF_ENTS
.global e_boom
e_boom: .space DF_ENTS
.global e_shown
e_shown: .space DF_ENTS
e_pcol: .space DF_ENTS
e_py: .space DF_ENTS
e_pnc: .space DF_ENTS
e_psl: .space DF_ENTS
e_psh: .space DF_ENTS

// Where each scanner 2-scanline blip was drawn (index DF_ENTS is the ship),
// plus the two White viewport bracket columns on y=234 / y=254.
sc_pl: .space DF_ENTS+1
sc_ph: .space DF_ENTS+1
sc_vl: .byte 0
sc_vr: .byte 0

// Parallax sky stars: saved screen pointer + byte written last time.
st_pl: .space DF_STARS
st_ph: .space DF_STARS
st_pv: .space DF_STARS

// Expanding streaking laser beams.
s_on: .space DF_SHOTS
s_xl: .space DF_SHOTS
s_xh: .space DF_SHOTS
s_y: .space DF_SHOTS
s_dir: .space DF_SHOTS
s_life: .space DF_SHOTS
s_age: .space DF_SHOTS
s_shown: .space DF_SHOTS
s_phead: .space DF_SHOTS
s_plen: .space DF_SHOTS
s_py: .space DF_SHOTS
s_ppat: .space DF_SHOTS
s_pdir: .space DF_SHOTS

// Ship.
.global ship_xl
ship_xl: .byte 0
.global ship_xh
ship_xh: .byte 0
.global ship_y
ship_y: .byte 0
.global ship_vx
ship_vx: .byte 0
.global ship_face
ship_face: .byte 0
.global ship_on
ship_on: .byte 0
.global ship_inv
ship_inv: .byte 0
.global ship_hit
ship_hit: .byte 0
.global ship_shown
ship_shown: .byte 0
.global ship_src
ship_src: .hword 0
ship_py: .byte 0
ship_pnc: .byte DF_SHIP_NC
ship_psl: .byte 0
ship_psh: .byte 0
fire_cool: .byte 0
.global rev_held
rev_held: .byte 0
.global bomb_req
bomb_req: .byte 0
bomb_held: .byte 0
.global hyper_req
hyper_req: .byte 0
hyp_held: .byte 0
.global cam_xl
cam_xl: .byte 0
.global cam_xh
cam_xh: .byte 0

// Counters the C++ side reads.
.global hostiles
hostiles: .byte 0
.global humans_left
humans_left: .byte 0
.global tick
tick: .byte 0
.global rnd
rnd: .byte 0x5b

// Terrain: ridge run per world column (top row, length), plus what each
// screen column currently shows so unchanged columns are skipped.
.global terr_top
terr_top: .space DF_TERR_COLS
.global terr_len
terr_len: .space DF_TERR_COLS
t_ptop: .space DF_VIEW_COLS
t_plen: .space DF_VIEW_COLS
.global t_force
t_force: .byte 1
t_cam: .byte 0xff
sc_par: .byte 0

// Blit arguments (blit_win).
b_col: .byte 0
b_y: .byte 0
b_nc: .byte 0
b_src: .hword 0
b_ocol: .byte 0
b_oy: .byte 0
b_onc: .byte 0
b_wipe: .byte 0
b_draw: .byte 0

// Scratch bytes.
e_i: .byte 0
sh_i: .byte 0
rm_i: .byte 0
k_i: .byte 0
sb_i: .byte 0
v_dl: .byte 0
v_dh: .byte 0
v_dir: .byte 0
v_g: .byte 0
v_r: .byte 0
v_sxl: .byte 0
v_sxh: .byte 0
v_pxl: .byte 0
v_pxh: .byte 0
v_py: .byte 0
v_bestd: .hword 0
v_best: .byte 0
v_hud: .byte 0

// Image per (type * 2 + frame).
img_tab:
  .hword k_df_boom0
  .hword k_df_boom1
  .hword k_df_lander0
  .hword k_df_lander1
  .hword k_df_human0
  .hword k_df_human1
  .hword k_df_mutant0
  .hword k_df_mutant1
  .hword k_df_bomber0
  .hword k_df_bomber1
  .hword k_df_bomb0
  .hword k_df_bomb1

pts_lo:
.byte 0, 150, 0, 150, 250, 0

// Mode-2 radar blip colours: none, Green lander, Magenta human, Cyan mutant,
// Yellow bomber, none (mine). Ship uses 0x3f White.
sc_val:
.byte 0x00, 0x0c, 0x33, 0x3c, 0x0f, 0x00

// Top-minimap vertical bands k=0..9 -> even scanlines 4,6,8,10,12,14,16,18,20,22
// at byte column 0 (rows 0..2: &3004,&3006, &3280..&3286, &3500..&3506).
sc_ylo:
.byte 0x04, 0x06, 0x80, 0x82, 0x84, 0x86, 0x00, 0x02, 0x04, 0x06
sc_yhi:
.byte 0x30, 0x30, 0x32, 0x32, 0x32, 0x32, 0x35, 0x35, 0x35, 0x35

// Planetoid / Defender streaking laser patterns (16 bytes from leading
// spearhead [0] to fracturing plasma wake [15]; 0x00 = transparent gap).
// Right-firing phases (0, 16) then left-firing phases (32, 48).
bolt_pat:
.byte 0x3f, 0x3f, 0x1f, 0x0f, 0x2d, 0x3c, 0x14, 0x39
.byte 0x33, 0x00, 0x11, 0x0b, 0x00, 0x03, 0x00, 0x01
.byte 0x3f, 0x1f, 0x0f, 0x3c, 0x3c, 0x00, 0x36, 0x33
.byte 0x22, 0x07, 0x00, 0x03, 0x10, 0x00, 0x02, 0x00
.byte 0x3f, 0x3f, 0x2f, 0x0f, 0x1e, 0x3c, 0x28, 0x36
.byte 0x33, 0x00, 0x22, 0x07, 0x00, 0x03, 0x00, 0x02
.byte 0x3f, 0x2f, 0x0f, 0x3c, 0x3c, 0x00, 0x39, 0x33
.byte 0x11, 0x0b, 0x00, 0x03, 0x20, 0x00, 0x01, 0x00

// 2nd-scanline spearhead glow under the front 5 columns (right 0..4, left 8..12).
bolt_glow:
.byte 0x0f, 0x2d, 0x3c, 0x03, 0x01, 0x00, 0x00, 0x00
.byte 0x0f, 0x1e, 0x3c, 0x03, 0x02, 0x00, 0x00, 0x00

// Parallax sky stars (16 stars in open sky y=40..204, single Mode-2 pixel inks).
star_wx:
.byte 5, 19, 28, 37, 46, 53, 62, 71
.byte 79, 88, 95, 103, 110, 117, 122, 126
star_y:
.byte 42, 82, 136, 56, 178, 102, 200, 70
.byte 156, 48, 120, 190, 90, 168, 62, 144
star_ink:
.byte 0x20, 0x14, 0x2a, 0x11, 0x10, 0x28, 0x0a, 0x15
.byte 0x22, 0x10, 0x2a, 0x14, 0x20, 0x05, 0x11, 0x28

// Row addresses of character rows 24..31 (pixel rows 192..255) at col 0.
yr_lo:
.byte 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80
yr_hi:
.byte 0x6c, 0x6e, 0x71, 0x73, 0x76, 0x78, 0x7b, 0x7d
m2_pair:
.byte 0x00, 0x15, 0x2a, 0x3f

hud_places:
.hword 10000, 1000, 100, 10, 1

// Planetoid-style BBC keyboard matrix codes:
//   up     : A (0x41), Up (0x39)
//   down   : Z (0x61), Down (0x29)
//   thrust : Shift (0x00)
//   reverse: Space (0x62)
//   fire   : Return (0x49), Keypad Return (0x3c)
//   bomb   : Tab (0x60), Ctrl (0x01), B (0x64)
//   hyper  : H (0x54)
//   escape : Escape (0x70)
key_codes:
.byte 0x41, 0x39
.byte 0x61, 0x29
.byte 0x00
.byte 0x62
.byte 0x49, 0x3c
.byte 0x60, 0x01, 0x64
.byte 0x54
.byte 0x70
key_masks:
.byte KEY_UP, KEY_UP
.byte KEY_DOWN, KEY_DOWN
.byte KEY_THRUST
.byte KEY_REVERSE
.byte KEY_FIRE, KEY_FIRE
.byte KEY_BOMB, KEY_BOMB, KEY_BOMB
.byte KEY_HYPER
.byte KEY_ESCAPE

snd_shot:
.hword 0x0012, 1, 196, 5
snd_boom:
.hword 0x0010, 2, 5, 10
snd_death:
.hword 0x0010, 3, 6, 30
snd_bomb:
.hword 0x0011, 4, 140, 14

// ---------------------------------------------------------------- misc

.global frame_wait
frame_wait:
  LDA #19
  JMP OSBYTE

// Draw a 12-scanline sprite directly at (fx_col, fx_y) with fx_len columns
// from fx_src (used by Title::show to display and animate the cast).
.global fx_sprite
fx_sprite:
  LDA #0
  STA b_wipe
  LDA #1
  STA b_draw
  LDA fx_col
  STA b_col
  LDA fx_y
  STA b_y
  LDA fx_len
  STA b_nc
  LDA fx_src
  STA b_src
  LDA fx_src+1
  STA b_src+1
  JMP blit_win

// Draw a 30-column shimmering laser lance across cols 23..52 at y=56 on the
// title screen (fx_col = 0 or 1 selects shimmer phase).
.global title_beam
title_beam:
  LDA fx_col
  AND #1
  ASL A
  ASL A
  ASL A
  ASL A
  STA Z_W
  LDA #56
  JSR bolt_row_base
  LDA #52
  STA Z_COL
  LDA #30
  STA Z_CNT
  LDA #0
  STA Z_I
  JSR shadow_on
tb_loop:
  JSR bolt_col_ptr
  LDA Z_I
  AND #15
  CLC
  ADC Z_W
  TAX
  LDY #0
  LDA bolt_pat, X
  STA (P1), Y
  INY
  LDA #0
  LDX Z_I
  CPX #5
  BCS tb_put2
  LDA bolt_glow, X
tb_put2:
  STA (P1), Y
  DEC Z_COL
  INC Z_I
  DEC Z_CNT
  BNE tb_loop
  JMP shadow_off

.global hud_digits
hud_digits:
  LDA score
  STA E_T0
  LDA score+1
  STA E_T1
  LDA #0x0f
  STA Z_MASK
  LDA #HUD_SCORE_COL
  STA E_C
  LDX #0
hd_place:
  LDY #0
hd_sub:
  SEC
  LDA E_T0
  SBC hud_places, X
  STA E_R
  LDA E_T1
  SBC hud_places+1, X
  BCC hd_digit
  STA E_T1
  LDA E_R
  STA E_T0
  INY
  BNE hd_sub
hd_digit:
  STX v_hud
  TYA
  JSR hud_glyph
  LDX v_hud
  INX
  INX
  CPX #10
  BNE hd_place
  // 2-digit Wave in White (0x3f).
  LDA #0x3f
  STA Z_MASK
  LDA #HUD_WAVE_COL
  STA E_C
  LDA wave
  LDY #0
hd_w10:
  CMP #10
  BCC hd_wdone
  SBC #10
  INY
  BNE hd_w10
hd_wdone:
  PHA
  TYA
  JSR hud_glyph
  PLA
  JSR hud_glyph
  // Graphical Smart-Bomb + Ship Life icons on HUD_ICON_ROW (4 slots each).
  LDA #%lo(k_df_sbomb)
  STA fx_src
  LDA #%hi(k_df_sbomb)
  STA fx_src+1
  LDA #HUD_BOMB_COL
  STA E_C
  LDA smart_bombs
  JSR hud_icons
  LDA #%lo(k_df_life)
  STA fx_src
  LDA #%hi(k_df_life)
  STA fx_src+1
  LDA #HUD_LIVES_COL
  STA E_C
  LDA lives
hud_icons:
  STA Z_VAL
  LDX #0
hi_loop:
  STX v_hud
  LDA E_C
  STA fx_col
  CLC
  ADC #4
  STA E_C
  LDA #HUD_ICON_ROW
  STA fx_row
  LDA #24
  STA fx_len
  LDA v_hud
  CMP Z_VAL
  BCS hi_wipe
  JSR fx_cell
  JMP hi_next
hi_wipe:
  JSR fx_wipe
hi_next:
  LDX v_hud
  INX
  CPX #4
  BNE hi_loop
  RTS

// A = digit 0..9 -> unpack 8 bytes of k_df_digits into 32 Mode-2 bytes
// at (E_C, HUD_NUM_ROW), tinted with Z_MASK.
hud_glyph:
  ASL A
  ASL A
  ASL A
  STA Z_W
  LDA E_C
  STA fx_col
  CLC
  ADC #4
  STA E_C
  LDA #HUD_NUM_ROW*8
  STA fx_y
  JSR strip_addr
  JSR shadow_on
  LDX Z_W
  LDY #0
  LDA #8
  STA Z_NC
hg_byte:
  LDA k_df_digits, X
  STA E_R
  STX Z_COL
  LDA #4
  STA Z_CNT
hg_pair:
  LDA #0
  ASL E_R
  ROL A
  ASL E_R
  ROL A
  TAX
  LDA m2_pair, X
  AND Z_MASK
  STA (P1), Y
  INY
  DEC Z_CNT
  BNE hg_pair
  LDX Z_COL
  INX
  DEC Z_NC
  BNE hg_byte
  JMP shadow_off

// Draw the top dashboard Blue horizon bar (y=31 across cols 0..79), the
// framed top-centre radar minimap (cols 23..56, y=0..31), and the miniature
// Red/Yellow planetoid ridge profile inside the minimap (cols 24..55, y=25..27).
.global playfield_frame
playfield_frame:
  JSR shadow_on
  // Full-width Blue bottom dashboard bar at y=31 across cols 0..79.
  LDA #79
  STA Z_I
pf_bar:
  LDA Z_I
  LDY #31
  LDA #0x30
  JSR pf_top_put
  DEC Z_I
  BPL pf_bar
  // Minimap frame + mini terrain across cols 23..56.
  LDA #23
  STA Z_I
pf_sc_col:
  // Top Blue bezel at y=0.
  LDY #0
  LDA #0x30
  JSR pf_top_put
  // Side Blue pillars at col 23 and col 56 (y=1..30).
  LDA Z_I
  CMP #23
  BEQ pf_pillar
  CMP #56
  BNE pf_mini
pf_pillar:
  LDA #30
  STA Z_CNT
pf_pil_lp:
  LDY Z_CNT
  LDA #0x30
  JSR pf_top_put
  DEC Z_CNT
  BNE pf_pil_lp
  BEQ pf_sc_next
pf_mini:
  // Interior column s = Z_I - 24 (0..31): sample terr_top[s * 4] -> y=25..27.
  LDA Z_I
  SEC
  SBC #DF_SCAN_COL
  ASL A
  ASL A
  TAY
  LDA terr_top, Y
  LDY #25
  CMP #226
  BCC pf_mplot
  INY
  CMP #236
  BCC pf_mplot
  INY
pf_mplot:
  STY Z_CNT
  LDA #0x07
  JSR pf_top_put
  LDY Z_CNT
  CPY #27
  BEQ pf_sc_next
  INY
  LDA #0x03
  JSR pf_top_put
pf_sc_next:
  INC Z_I
  LDA Z_I
  CMP #57
  BNE pf_sc_col
  JMP shadow_off

// Write byte A at column Z_I, scanline Y (0..31).
pf_top_put:
  PHA
  STY fx_y
  LDA Z_I
  STA fx_col
  JSR strip_addr
  PLA
  STA (P1), Y
  RTS

// X = row offset 0..63 (from y=192) -> P1 = cell scanline at column Z_CH:Z_CL,
// Y = 0, X preserved.
y192_ptr:
  TXA
  LSR A
  LSR A
  LSR A
  TAY
  TXA
  AND #7
  ORA yr_lo, Y
  CLC
  ADC Z_CL
  STA P1
  LDA yr_hi, Y
  ADC Z_CH
  STA P1+1
  LDY #0
  RTS

.global keys_read
keys_read:
  LDA #0
  STA key_bits
  SEI
  LDA #0x7f
  STA SYSVIA_DDRA
  LDA #0x03
  STA SYSVIA_LATCH
  LDX #KEY_COUNT-1
kr_loop:
  LDA key_codes, X
  STA SYSVIA_PORTA
  LDA SYSVIA_PORTA
  BPL kr_up
  LDA key_masks, X
  ORA key_bits
  STA key_bits
kr_up:
  DEX
  BPL kr_loop
  LDA #0x0b
  STA SYSVIA_LATCH
  CLI
  RTS

rnd_next:
  LDA rnd
  ASL A
  BCC rn_ok
  EOR #0x1d
rn_ok:
  STA rnd
  RTS

// ---------------------------------------------------------------- blit

// Wipe the old 12-scanline window and draw the new one under one shadow pass.
// A 12-row column spans 2 character rows when (y & 7) <= 4 and 3 character
// rows when (y & 7) >= 5; P1, P2 (+0x278), and P3 (+0x4f0) let the same Y
// walk straight down all 12 scanlines without touching any pointer per column.
blit_win:
  JSR shadow_on
  LDA b_wipe
  BEQ bw_draw
  LDA b_ocol
  STA fx_col
  LDA b_oy
  STA fx_y
  JSR strip_addr
  STY Z_SCAN
  JSR prep_p23
  LDA b_onc
  STA Z_NC
  JSR wipe_win
bw_draw:
  LDA b_draw
  BEQ bw_out
  LDA b_col
  STA fx_col
  LDA b_y
  STA fx_y
  JSR strip_addr
  STY Z_SCAN
  JSR prep_p23
  LDA b_src
  STA dw_src1+1
  STA dw_src2+1
  STA dw_src3+1
  LDA b_src+1
  STA dw_src1+2
  STA dw_src2+2
  STA dw_src3+2
  LDA b_nc
  STA Z_NC
  JSR draw_win
bw_out:
  JMP shadow_off

prep_p23:
  CLC
  LDA P1
  ADC #0x78
  STA P2
  LDA P1+1
  ADC #2
  STA P2+1
  CLC
  LDA P2
  ADC #0x78
  STA P3
  LDA P2+1
  ADC #2
  STA P3+1
  LDA Z_SCAN
  STA Z_YC
  LDA #8
  SEC
  SBC Z_SCAN
  STA Z_N1
  LDA #DF_SPR_H
  SEC
  SBC Z_N1
  CMP #9
  BCC pp_two
  PHA
  LDA #8
  STA Z_N2
  PLA
  SEC
  SBC #8
  STA Z_N3
  RTS
pp_two:
  STA Z_N2
  LDA #0
  STA Z_N3
  RTS

wipe_win:
ww_col:
  LDY Z_YC
  LDA Z_N1
  STA Z_CNT
  LDA #0
ww_s1:
  STA (P1), Y
  INY
  DEC Z_CNT
  BNE ww_s1
  LDX Z_N2
ww_s2:
  STA (P2), Y
  INY
  DEX
  BNE ww_s2
  LDX Z_N3
  BEQ ww_next
ww_s3:
  STA (P3), Y
  INY
  DEX
  BNE ww_s3
ww_next:
  LDA Z_YC
  CLC
  ADC #8
  STA Z_YC
  DEC Z_NC
  BNE ww_col
  RTS

draw_win:
  LDX #0
dw_col:
  LDY Z_YC
  LDA Z_N1
  STA Z_CNT
dw_s1:
dw_src1:
  LDA 0xffff, X
  STA (P1), Y
  INX
  INY
  DEC Z_CNT
  BNE dw_s1
  LDA Z_N2
  STA Z_CNT
dw_s2:
dw_src2:
  LDA 0xffff, X
  STA (P2), Y
  INX
  INY
  DEC Z_CNT
  BNE dw_s2
  LDA Z_N3
  BEQ dw_next
  STA Z_CNT
dw_s3:
dw_src3:
  LDA 0xffff, X
  STA (P3), Y
  INX
  INY
  DEC Z_CNT
  BNE dw_s3
dw_next:
  LDA Z_YC
  CLC
  ADC #8
  STA Z_YC
  DEC Z_NC
  BNE dw_col
  RTS

// ---------------------------------------------------------------- ship

.global cam_update
cam_update:
  SEC
  LDA ship_xl
  SBC #DF_CAM_OFF
  AND #0xfc
  STA cam_xl
  LDA ship_xh
  SBC #0
  AND #1
  STA cam_xh
  RTS

.global ship_step
ship_step:
  LDA ship_inv
  BEQ ss_noinv
  DEC ship_inv
ss_noinv:
  LDA fire_cool
  BEQ ss_nocool
  DEC fire_cool
ss_nocool:
  // Space reverses facing direction once per press (preserving velocity).
  LDA key_bits
  AND #KEY_REVERSE
  BEQ ss_rev_up
  LDA rev_held
  BNE ss_chk_thrust
  LDA #1
  STA rev_held
  LDA ship_face
  EOR #1
  STA ship_face
  JMP ss_chk_thrust
ss_rev_up:
  STA rev_held
ss_chk_thrust:
  // Shift thrusts every frame; braking against opposite momentum steps by 2
  // so reverse-thrust turns feel crisp and responsive.
  LDA key_bits
  AND #KEY_THRUST
  BEQ ss_coast
  LDA ship_face
  BNE ss_thr_left
  LDA ship_vx
  BPL ss_tr_one
  INC ship_vx
ss_tr_one:
  LDA ship_vx
  CMP #DF_VMAX
  BEQ ss_move
  INC ship_vx
  JMP ss_move
ss_thr_left:
  LDA ship_vx
  BMI ss_tl_one
  BEQ ss_tl_one
  DEC ship_vx
ss_tl_one:
  LDA ship_vx
  CLC
  ADC #DF_VMAX
  BEQ ss_move
  DEC ship_vx
  JMP ss_move
ss_coast:
  LDA tick
  AND #7
  BNE ss_move
  LDA ship_vx
  BEQ ss_move
  BMI ss_coastneg
  DEC ship_vx
  JMP ss_move
ss_coastneg:
  INC ship_vx
ss_move:
  LDA ship_vx
  BEQ ss_vert
  BMI ss_negx
  CLC
  ADC ship_xl
  STA ship_xl
  LDA ship_xh
  ADC #0
  AND #1
  STA ship_xh
  JMP ss_vert
ss_negx:
  CLC
  ADC ship_xl
  STA ship_xl
  LDA ship_xh
  ADC #0xff
  AND #1
  STA ship_xh
ss_vert:
  LDA key_bits
  AND #KEY_UP
  BEQ ss_notup
  LDA ship_y
  SEC
  SBC #DF_STEP_Y
  CMP #DF_Y_MIN
  BCS ss_upok
  LDA #DF_Y_MIN
ss_upok:
  STA ship_y
ss_notup:
  LDA key_bits
  AND #KEY_DOWN
  BEQ ss_notdown
  LDA ship_y
  CLC
  ADC #DF_STEP_Y
  CMP #DF_Y_MAX+1
  BCC ss_downok
  LDA #DF_Y_MAX
ss_downok:
  STA ship_y
ss_notdown:
  JSR cam_update
  // Terrain: check the ship's 5 hull columns against the ridge top.
  LDA ship_inv
  BNE ss_noterr
  LDA ship_xh
  LSR A
  LDA ship_xl
  ROR A
  LSR A
  CLC
  ADC #1
  AND #127
  TAX
  LDA ship_y
  CLC
  ADC #11
  STA v_g
  LDY #5
ss_terr:
  LDA v_g
  CMP terr_top, X
  BCC ss_tclear
  BEQ ss_tclear
  LDA #1
  STA ship_hit
  BNE ss_noterr
ss_tclear:
  INX
  TXA
  AND #127
  TAX
  DEY
  BNE ss_terr
ss_noterr:
  LDA key_bits
  AND #KEY_FIRE
  BEQ ss_nofire
  LDA fire_cool
  BNE ss_nofire
  LDA #DF_FIRE_RATE
  STA fire_cool
  JSR shot_fire
ss_nofire:
  LDA key_bits
  AND #KEY_BOMB
  BEQ ss_bombup
  LDA bomb_held
  BNE ss_chk_hyp
  LDA #1
  STA bomb_held
  STA bomb_req
  BNE ss_chk_hyp
ss_bombup:
  STA bomb_held
ss_chk_hyp:
  LDA key_bits
  AND #KEY_HYPER
  BEQ ss_hypup
  LDA hyp_held
  BNE ss_draw
  LDA #1
  STA hyp_held
  STA hyper_req
  BNE ss_draw
ss_hypup:
  STA hyp_held
ss_draw:
  JMP ship_draw

ship_r_tab:
  .hword k_df_ship_r0
  .hword k_df_ship_r1
  .hword k_df_ship_r2
ship_l_tab:
  .hword k_df_ship_l0
  .hword k_df_ship_l1
  .hword k_df_ship_l2

.global ship_draw
ship_draw:
  LDY #0
  LDA key_bits
  AND #KEY_THRUST
  BEQ sd_pick
  LDA tick
  LSR A
  AND #1
  CLC
  ADC #1
  ASL A
  TAY
sd_pick:
  LDA ship_face
  BNE sd_left
  LDA ship_r_tab, Y
  STA ship_src
  LDA ship_r_tab+1, Y
  STA ship_src+1
  JMP ship_paint
sd_left:
  LDA ship_l_tab, Y
  STA ship_src
  LDA ship_l_tab+1, Y
  STA ship_src+1

// Paint ship_src at (DF_SHIP_COL, ship_y), wiping where it was.
.global ship_paint
ship_paint:
  LDA ship_shown
  BEQ sp_draw
  LDA ship_y
  CMP ship_py
  BNE sp_draw
  LDA ship_src
  CMP ship_psl
  BNE sp_draw
  LDA ship_src+1
  CMP ship_psh
  BNE sp_draw
  RTS
sp_draw:
  LDA ship_shown
  STA b_wipe
  LDA #DF_SHIP_COL
  STA b_ocol
  STA b_col
  LDA ship_py
  STA b_oy
  LDA ship_pnc
  STA b_onc
  LDA #DF_SHIP_NC
  LDY ship_on
  BNE sp_nc_ok
  LDA #5
sp_nc_ok:
  STA b_nc
  STA ship_pnc
  LDA #1
  STA b_draw
  LDA ship_y
  STA b_y
  LDA ship_src
  STA b_src
  LDA ship_src+1
  STA b_src+1
  JSR blit_win
  LDA #1
  STA ship_shown
  LDA ship_src
  STA ship_psl
  LDA ship_src+1
  STA ship_psh
  LDA ship_y
  STA ship_py
  CMP #204
  BCC sp_ret
  LDA #1
  STA t_force
sp_ret:
  RTS

.global ship_erase
ship_erase:
  LDA ship_shown
  BEQ se_ret
  LDA #1
  STA b_wipe
  LDA #0
  STA b_draw
  STA ship_shown
  LDA #DF_SHIP_COL
  STA b_ocol
  LDA ship_py
  STA b_oy
  CMP #204
  BCC se_nt
  LDA #1
  STA t_force
se_nt:
  LDA ship_pnc
  STA b_onc
  JMP blit_win
se_ret:
  RTS

// ------------------------------------------------------------ laser beams

.global shots_reset
shots_reset:
  LDX #DF_SHOTS-1
  LDA #0
sr_loop:
  STA s_on, X
  STA s_shown, X
  DEX
  BPL sr_loop
  RTS

.global shots_clear
shots_clear:
  LDA #DF_SHOTS-1
  STA sh_i
sc_loop:
  LDX sh_i
  LDA s_on, X
  BEQ sc_next
  JSR bolt_erase
  LDX sh_i
  LDA #0
  STA s_on, X
sc_next:
  DEC sh_i
  BPL sc_loop
  RTS

shot_fire:
  LDX #DF_SHOTS-1
sf_find:
  LDA s_on, X
  BEQ sf_got
  DEX
  BPL sf_find
  RTS
sf_got:
  LDA #1
  STA s_on, X
  LDA #0
  STA s_shown, X
  STA s_age, X
  LDA #DF_SHOT_LIFE
  STA s_life, X
  LDA ship_face
  STA s_dir, X
  BNE sf_left
  CLC
  LDA ship_xl
  ADC #24
  STA s_xl, X
  LDA ship_xh
  ADC #0
  JMP sf_x
sf_left:
  SEC
  LDA ship_xl
  SBC #4
  STA s_xl, X
  LDA ship_xh
  SBC #0
sf_x:
  AND #1
  STA s_xh, X
  // Force even scanline so (P1),0 and (P1),1 are always in the same cell.
  LDA ship_y
  CLC
  ADC #6
  AND #0xfe
  STA s_y, X
  JMP sound_shot

// Set Z_CL:Z_CH = character-row base of pixel row A + (A & 7).
bolt_row_base:
  STA fx_y
  LDA #0
  STA fx_col
  JSR strip_addr
  CLC
  TYA
  ADC P1
  STA Z_CL
  LDA P1+1
  ADC #0
  STA Z_CH
  RTS

// Z_COL (1..78) -> P1 = Z_CL:Z_CH + Z_COL * 8.
bolt_col_ptr:
  LDA #0
  STA P1+1
  LDA Z_COL
  ASL A
  ROL P1+1
  ASL A
  ROL P1+1
  ASL A
  ROL P1+1
  CLC
  ADC Z_CL
  STA P1
  LDA P1+1
  ADC Z_CH
  STA P1+1
  RTS

// Erase bolt sh_i column by column, clearing only bytes that still match
// the exact pattern written last frame so sprites painted over it stay intact.
bolt_erase:
  LDX sh_i
  LDA s_shown, X
  BNE be_go
  RTS
be_go:
  LDA #0
  STA s_shown, X
  STA Z_MASK
  LDA s_phead, X
  STA Z_COL
  LDA s_plen, X
  STA Z_CNT
  LDA s_ppat, X
  STA Z_W
  LDY s_pdir, X
  LDA s_py, X
  CMP #214
  BCC bolt_pass
  PHA
  LDA #1
  STA t_force
  PLA
  JMP bolt_pass

// Draw expanding streaking beam sh_i from head column (v_sxh:v_sxl >> 2)
// back toward its plasma wake.
bolt_draw:
  LDX sh_i
  LDA v_sxh
  LSR A
  LDA v_sxl
  ROR A
  LSR A
  AND #127
  STA s_phead, X
  STA Z_COL
  LDA s_life, X
  CMP #4
  BCS bd_grow
  ASL A
  ASL A
  BNE bd_have_len
bd_grow:
  LDA s_age, X
  CMP #1
  BNE bd_a2
  LDA #5
  BNE bd_have_len
bd_a2:
  CMP #2
  BNE bd_a3
  LDA #10
  BNE bd_have_len
bd_a3:
  CMP #3
  BNE bd_afull
  LDA #14
  BNE bd_have_len
bd_afull:
  LDA #16
bd_have_len:
  STA s_plen, X
  STA Z_CNT
  LDA tick
  AND #1
  ASL A
  ASL A
  ASL A
  ASL A
  LDY s_dir, X
  BEQ bd_pat_ok
  CLC
  ADC #32
bd_pat_ok:
  STA s_ppat, X
  STA Z_W
  TYA
  STA s_pdir, X
  LDA #1
  STA s_shown, X
  STA Z_MASK
  LDA s_y, X
  STA s_py, X
bolt_pass:
  STY Z_VAL
  JSR bolt_row_base
  LDA #0xff
  LDY Z_VAL
  BEQ bp_step
  LDA #1
bp_step:
  STA Z_STEP
  LDA #0
  STA Z_I
  JSR shadow_on
bp_loop:
  LDA Z_COL
  CMP #1
  BCC bp_skip
  CMP #79
  BCS bp_skip
  JSR bolt_col_ptr
  LDA Z_W
  CLC
  ADC Z_I
  TAX
  LDA bolt_pat, X
  BEQ bp_glow
  LDY #0
  LDX Z_MASK
  BNE bp_draw0
  CMP (P1), Y
  BNE bp_glow
  LDA #0
bp_draw0:
  STA (P1), Y
bp_glow:
  LDA Z_I
  CMP #5
  BCS bp_skip
  LDA Z_VAL
  BEQ bp_gr
  LDA #8
bp_gr:
  CLC
  ADC Z_I
  TAX
  LDY #1
  LDA Z_MASK
  BNE bp_draw1
  LDA bolt_glow, X
  CMP (P1), Y
  BNE bp_skip
  LDA #0
  BEQ bp_put1
bp_draw1:
  LDA (P1), Y
  BNE bp_skip
  LDA bolt_glow, X
bp_put1:
  STA (P1), Y
bp_skip:
  LDA Z_COL
  CLC
  ADC Z_STEP
  AND #127
  STA Z_COL
  INC Z_I
  DEC Z_CNT
  BNE bp_loop
  JMP shadow_off

.global shots_step
shots_step:
  LDA #DF_SHOTS-1
  STA sh_i
sl_loop:
  LDX sh_i
  LDA s_on, X
  BNE sl_live
  JMP sl_next
sl_live:
  JSR bolt_erase
  LDX sh_i
  INC s_age, X
  // Step 20 units/frame while stretching/cruising, 8 units/frame while tail
  // dissipates at the end of range.
  LDA #DF_SHOT_STEP
  LDY s_life, X
  CPY #4
  BCS sl_got_step
  LDA #8
sl_got_step:
  STA v_g
  LDA s_dir, X
  BNE sl_goleft
  CLC
  LDA s_xl, X
  ADC v_g
  STA s_xl, X
  LDA s_xh, X
  ADC #0
  JMP sl_moved
sl_goleft:
  SEC
  LDA s_xl, X
  SBC v_g
  STA s_xl, X
  LDA s_xh, X
  SBC #0
sl_moved:
  AND #1
  STA s_xh, X
  DEC s_life, X
  BEQ sl_kill
  SEC
  LDA s_xl, X
  SBC cam_xl
  STA v_sxl
  LDA s_xh, X
  SBC cam_xh
  AND #1
  STA v_sxh
  // Keep alive while head is in 0..383 (cols 0..95) or 448..511 (cols -16..-1).
  BEQ sl_vis
  LDA v_sxl
  CMP #68
  BCC sl_vis
  CMP #192
  BCC sl_kill
sl_vis:
  JSR shot_hit
  BCS sl_kill
  JSR bolt_draw
  JMP sl_next
sl_kill:
  LDX sh_i
  LDA #0
  STA s_on, X
sl_next:
  DEC sh_i
  BMI sl_done
  JMP sl_loop
sl_done:
  RTS

// Does beam sh_i strike a live hostile (slots below DF_HOSTILE_END)?
// Tests the active 32-unit front section of the beam across 12 scanlines.
shot_hit:
  LDY sh_i
  LDX #DF_HOSTILE_END-1
sh_loop:
  LDA e_type, X
  BEQ sh_next
  LDA e_boom, X
  BNE sh_next
  // Beam spans s_y..s_y+1; enemy spans e_y..e_y+11 -> hit if (s_y+1 - e_y) < 13.
  LDA s_y, Y
  CLC
  ADC #1
  SEC
  SBC e_y, X
  CMP #13
  BCS sh_next
  // For right-going beam, head is at s_x and body trails 32 units left:
  // hit if (s_x - e_x) mod 512 < 32 OR (e_x - s_x) mod 512 < 8.
  // For left-going beam, head is at s_x and body trails 32 units right:
  // hit if (e_x - s_x) mod 512 < 32 OR (s_x - e_x) mod 512 < 16.
  LDA s_dir, Y
  BNE sh_left_beam
  LDA #32
  STA v_g
  LDA #8
  STA v_r
  JMP sh_xtest
sh_left_beam:
  LDA #16
  STA v_g
  LDA #32
  STA v_r
sh_xtest:
  SEC
  LDA s_xl, Y
  SBC e_xl, X
  STA v_dl
  LDA s_xh, Y
  SBC e_xh, X
  AND #1
  BNE sh_fwd
  LDA v_dl
  CMP v_g
  BCC sh_hit
sh_fwd:
  SEC
  LDA e_xl, X
  SBC s_xl, Y
  STA v_dl
  LDA e_xh, X
  SBC s_xh, Y
  AND #1
  BNE sh_next
  LDA v_dl
  CMP v_r
  BCC sh_hit
sh_next:
  DEX
  BPL sh_loop
  CLC
  RTS
sh_hit:
  JSR ent_kill
  SEC
  RTS

// ---------------------------------------------------------------- entities

// Score for the kill, release any human a lander carries, start the blast.
ent_kill:
  STX k_i
  LDA e_type, X
  TAY
  CLC
  LDA score
  ADC pts_lo, Y
  STA score
  LDA score+1
  ADC #0
  STA score+1
  LDA #1
  STA hud_dirty
  CPY #ET_LANDER
  BNE ek_boom
  LDA e_st, X
  CMP #LS_RISE
  BNE ek_boom
  LDY e_aux, X
  CPY #0xff
  BEQ ek_boom
  LDA e_type, Y
  CMP #ET_HUMAN
  BNE ek_boom
  LDA e_st, Y
  CMP #HS_CARRIED
  BNE ek_boom
  LDA #HS_FALL
  STA e_st, Y
  LDA #0
  STA e_aux, Y
ek_boom:
  LDX k_i
  LDA #DF_BOOM_FRAMES
  STA e_boom, X
  JMP sound_boom

// Wipe slot X's picture and free it.
ent_remove:
  STX rm_i
  LDA e_shown, X
  BEQ er2_clear
  LDA e_pcol, X
  STA b_ocol
  LDA e_py, X
  STA b_oy
  CMP #204
  BCC er2_nt
  LDA #1
  STA t_force
er2_nt:
  LDA e_pnc, X
  STA b_onc
  LDA #1
  STA b_wipe
  LDA #0
  STA b_draw
  JSR blit_win
  LDX rm_i
er2_clear:
  LDA #0
  STA e_type, X
  STA e_shown, X
  STA e_boom, X
  RTS

.global ents_reset
ents_reset:
  LDX #DF_ENTS-1
  LDA #0
ere_loop:
  STA e_type, X
  STA e_shown, X
  STA e_boom, X
  DEX
  BPL ere_loop
  LDX #DF_ENTS
ere_sc:
  STA sc_ph, X
  DEX
  BPL ere_sc
  STA sc_vl
  STA sc_vr
  LDX #DF_STARS-1
ere_st:
  STA st_ph, X
  DEX
  BPL ere_st
  LDA #1
  STA hostiles
  RTS

// X = slot; C set if its x is on the 320-unit screen.
ent_onscreen:
  SEC
  LDA e_xl, X
  SBC cam_xl
  STA v_sxl
  LDA e_xh, X
  SBC cam_xh
  AND #1
  BEQ eon_yes
  LDA v_sxl
  CMP #64
  BCC eon_yes
  CLC
  RTS
eon_yes:
  SEC
  RTS

// Smart bomb: every hostile and mine on screen blows up.
.global smart_bomb
smart_bomb:
  LDA #DF_ENTS-1
  STA sb_i
sb_loop:
  LDX sb_i
  LDA e_type, X
  BEQ sb_next
  CMP #ET_HUMAN
  BEQ sb_next
  LDA e_boom, X
  BNE sb_next
  JSR ent_onscreen
  BCC sb_next
  LDX sb_i
  JSR ent_kill
sb_next:
  DEC sb_i
  BPL sb_loop
  RTS

// Drop a star-mine from the first live bomber into the first free mine slot.
.global bomb_drop
bomb_drop:
  LDX #DF_FIRST_BOMB+DF_N_BOMB-1
bdr_find:
  LDA e_type, X
  BEQ bdr_free
  DEX
  CPX #DF_FIRST_BOMB-1
  BNE bdr_find
  RTS
bdr_free:
  LDY #DF_FIRST_BOMBER+DF_N_BOMBER-1
bdr_b:
  LDA e_type, Y
  CMP #ET_BOMBER
  BNE bdr_nb
  LDA e_boom, Y
  BEQ bdr_got
bdr_nb:
  DEY
  CPY #DF_FIRST_BOMBER-1
  BNE bdr_b
  RTS
bdr_got:
  LDA #ET_BOMB
  STA e_type, X
  CLC
  LDA e_xl, Y
  ADC #4
  STA e_xl, X
  LDA e_xh, Y
  ADC #0
  AND #1
  STA e_xh, X
  LDA e_y, Y
  CLC
  ADC #6
  STA e_y, X
  LDA #DF_BOMB_LIFE
  STA e_aux, X
  LDA #0
  STA e_boom, X
  STA e_shown, X
  RTS

// One frame of every slot: think, hit-test the ship, paint.
.global ents_step
ents_step:
  INC tick
  LDA #0
  STA hostiles
  STA humans_left
  STA e_i
es_loop:
  LDX e_i
  LDA e_type, X
  BEQ es_next
  JSR ent_one
es_next:
  INC e_i
  LDA e_i
  CMP #DF_ENTS
  BNE es_loop
  RTS

ent_one:
  LDA e_boom, X
  BEQ eo_alive
  DEC e_boom, X
  BNE eo_draw
  JMP ent_remove
eo_draw:
  JMP ent_render
eo_alive:
  LDA e_type, X
  CMP #ET_HUMAN
  BNE eo_nothuman
  INC humans_left
  JMP eo_ai
eo_nothuman:
  CMP #ET_BOMB
  BEQ eo_ai
  INC hostiles
eo_ai:
  LDA e_type, X
  CMP #ET_LANDER
  BNE eo_n1
  JSR ai_lander
  JMP eo_after
eo_n1:
  CMP #ET_HUMAN
  BNE eo_n2
  JSR ai_human
  JMP eo_after
eo_n2:
  CMP #ET_MUTANT
  BNE eo_n3
  JSR ai_mutant
  JMP eo_after
eo_n3:
  CMP #ET_BOMBER
  BNE eo_n4
  JSR ai_bomber
  JMP eo_after
eo_n4:
  JSR ai_bomb
eo_after:
  LDX e_i
  LDA e_type, X
  BEQ eo_done
  JSR ent_ship_test
  LDX e_i
  JMP ent_render
eo_done:
  RTS

mv_x:
  BMI mx_neg
  CLC
  ADC e_xl, X
  STA e_xl, X
  LDA e_xh, X
  ADC #0
  AND #1
  STA e_xh, X
  RTS
mx_neg:
  CLC
  ADC e_xl, X
  STA e_xl, X
  LDA e_xh, X
  ADC #0xff
  AND #1
  STA e_xh, X
  RTS

// Landers: hunt the nearest walking human, drop onto him, carry him up.
ai_lander:
  LDA e_st, X
  CMP #LS_RISE
  BNE al_hunt
  JMP al_rise
al_hunt:
  LDY e_aux, X
  CPY #0xff
  BEQ al_pick
  LDA e_type, Y
  CMP #ET_HUMAN
  BNE al_pick
  LDA e_st, Y
  BNE al_pick
  JMP al_have
al_pick:
  JSR pick_human
  CPY #0xff
  BNE al_have
  LDA #ET_MUTANT
  STA e_type, X
  LDA #0
  STA e_st, X
  RTS
al_have:
  SEC
  LDA e_xl, Y
  SBC e_xl, X
  STA v_dl
  LDA e_xh, Y
  SBC e_xh, X
  AND #1
  BNE al_dleft
  LDA v_dl
  CMP #3
  BCC al_align
  LDA #1
  JSR mv_x
  JMP al_slow
al_dleft:
  LDA #0
  SEC
  SBC v_dl
  BEQ al_leftmove
  CMP #3
  BCC al_align
al_leftmove:
  LDA #0xff
  JSR mv_x
al_slow:
  LDA tick
  AND #1
  BNE al_ret
  LDA e_y, Y
  SEC
  SBC e_y, X
  CMP #24
  BCC al_ret
  INC e_y, X
al_ret:
  RTS
al_align:
  LDA e_y, Y
  SEC
  SBC e_y, X
  CMP #12
  BCC al_grab
  CMP #0xc0
  BCS al_grab
  INC e_y, X
  RTS
al_grab:
  LDA #HS_CARRIED
  STA e_st, Y
  TXA
  STA e_aux, Y
  LDA #LS_RISE
  STA e_st, X
  RTS
al_rise:
  DEC e_y, X
  LDY e_aux, X
  CPY #0xff
  BEQ al_nocarry
  LDA e_type, Y
  CMP #ET_HUMAN
  BNE al_nocarry
  LDA e_st, Y
  CMP #HS_CARRIED
  BNE al_nocarry
  LDA e_xl, X
  STA e_xl, Y
  LDA e_xh, X
  STA e_xh, Y
  LDA e_y, X
  CLC
  ADC #10
  STA e_y, Y
  LDA e_y, X
  CMP #DF_TOP_Y
  BCS al_ret
  TYA
  TAX
  JSR ent_remove
  LDX e_i
  LDA #ET_MUTANT
  STA e_type, X
  LDA #0
  STA e_st, X
  JMP sound_bomb
al_nocarry:
  LDA #LS_HUNT
  STA e_st, X
  LDA #0xff
  STA e_aux, X
  RTS

pick_human:
  LDA #0xff
  STA v_best
  STA v_bestd
  STA v_bestd+1
  LDY #DF_FIRST_HUMAN+DF_N_HUMAN-1
ph_loop:
  LDA e_type, Y
  CMP #ET_HUMAN
  BNE ph_next
  LDA e_st, Y
  BNE ph_next
  SEC
  LDA e_xl, Y
  SBC e_xl, X
  STA v_dl
  LDA e_xh, Y
  SBC e_xh, X
  AND #1
  STA v_dh
  BEQ ph_cmp
  SEC
  LDA #0
  SBC v_dl
  STA v_dl
  LDA #0
  ROL A
  STA v_dh
ph_cmp:
  LDA v_dl
  CMP v_bestd
  LDA v_dh
  SBC v_bestd+1
  BCS ph_next
  LDA v_dl
  STA v_bestd
  LDA v_dh
  STA v_bestd+1
  STY v_best
ph_next:
  DEY
  CPY #DF_FIRST_HUMAN-1
  BNE ph_loop
  LDY v_best
  TYA
  STA e_aux, X
  RTS

ai_mutant:
  JSR rnd_next
  STA v_r
  SEC
  LDA ship_xl
  SBC e_xl, X
  LDA ship_xh
  SBC e_xh, X
  AND #1
  STA v_dh
  LDA #1
  LDY v_dh
  BEQ am_dir
  LDA #0xff
am_dir:
  STA v_dir
  LDA v_r
  AND #7
  BNE am_nrev
  LDA #0
  SEC
  SBC v_dir
  STA v_dir
am_nrev:
  LDA v_r
  AND #0x10
  LSR A
  LSR A
  LSR A
  LSR A
  CLC
  ADC #1
  STA v_g
  LDA v_dir
  BPL am_pos
  LDA #0
  SEC
  SBC v_g
  STA v_g
am_pos:
  LDA v_g
  JSR mv_x
  LDA tick
  AND #1
  BNE am_out
  LDA ship_y
  CMP e_y, X
  BEQ am_out
  LDA #1
  BCS am_vd
  LDA #0xff
am_vd:
  STA v_g
  LDA v_r
  AND #0x60
  BNE am_vok
  LDA #0
  SEC
  SBC v_g
  STA v_g
am_vok:
  LDA e_y, X
  CLC
  ADC v_g
  STA e_y, X
  CMP #DF_Y_MIN
  BCS am_c1
  LDA #DF_Y_MIN
  STA e_y, X
am_c1:
  LDA e_y, X
  CMP #202
  BCC am_out
  LDA #202
  STA e_y, X
am_out:
  RTS

ai_bomber:
  LDA e_vx, X
  JSR mv_x
  LDA tick
  AND #1
  BNE ab_out
  LDA e_y, X
  CLC
  ADC e_vy, X
  STA e_y, X
  CMP #46
  BCS ab_c1
  LDA #1
  STA e_vy, X
ab_c1:
  LDA e_y, X
  CMP #172
  BCC ab_out
  LDA #0xff
  STA e_vy, X
ab_out:
  RTS

ai_bomb:
  DEC e_aux, X
  BNE ab2_out
  JMP ent_remove
ab2_out:
  RTS

ai_human:
  LDA e_st, X
  BEQ ah_walk
  CMP #HS_FALL
  BEQ ah_fall
  RTS
ah_walk:
  LDA tick
  CLC
  ADC e_i
  AND #7
  BNE ah_out
  JSR rnd_next
  AND #7
  BNE ah_nt
  LDA #0
  SEC
  SBC e_vx, X
  STA e_vx, X
ah_nt:
  LDA e_vx, X
  JSR mv_x
  JSR human_ground
  STA e_y, X
ah_out:
  RTS
ah_fall:
  LDA e_y, X
  CLC
  ADC #2
  STA e_y, X
  LDA e_aux, X
  CMP #200
  BCS ah_f1
  CLC
  ADC #2
  STA e_aux, X
ah_f1:
  JSR human_ground
  CMP e_y, X
  BEQ ah_land
  BCS ah_out
ah_land:
  STA e_y, X
  LDA e_aux, X
  CMP #DF_FALL_MAX
  BCS ah_die
  LDA #HS_WALK
  STA e_st, X
  LDA #0
  STA e_aux, X
  RTS
ah_die:
  JMP ent_remove

human_ground:
  LDA e_xh, X
  LSR A
  LDA e_xl, X
  ROR A
  LSR A
  AND #127
  TAY
  LDA terr_top, Y
  STA v_g
  INY
  TYA
  AND #127
  TAY
  LDA terr_top, Y
  CMP v_g
  BCS hg_1
  STA v_g
hg_1:
  INY
  TYA
  AND #127
  TAY
  LDA terr_top, Y
  CMP v_g
  BCS hg_2
  STA v_g
hg_2:
  LDA v_g
  SEC
  SBC #DF_SPR_H
  RTS

// Ship vs slot X. Hostiles and mines kill; a falling human is rescued.
ent_ship_test:
  LDA ship_on
  BEQ est_out
  LDA e_type, X
  CMP #ET_HUMAN
  BEQ est_human
  LDA ship_inv
  BNE est_out
  JSR ent_overlap
  BCC est_out
  LDA #1
  STA ship_hit
est_out:
  RTS
est_human:
  LDA e_st, X
  CMP #HS_FALL
  BNE est_out
  JSR ent_overlap
  BCC est_out
  CLC
  LDA score
  ADC #0xf4
  STA score
  LDA score+1
  ADC #1
  STA score+1
  LDA #1
  STA hud_dirty
  JSR human_ground
  STA e_y, X
  LDA #HS_WALK
  STA e_st, X
  LDA #0
  STA e_aux, X
  RTS

// C set if slot X's 12-row box touches the ship's 24x12 box.
ent_overlap:
  LDA e_y, X
  SEC
  SBC ship_y
  CLC
  ADC #11
  CMP #23
  BCS eov_no
  LDA #16
  LDY e_type, X
  CPY #ET_HUMAN
  BEQ eov_w12
  CPY #ET_BOMB
  BNE eov_w
eov_w12:
  LDA #12
eov_w:
  STA v_g
  SEC
  LDA e_xl, X
  SBC ship_xl
  STA v_dl
  LDA e_xh, X
  SBC ship_xh
  AND #1
  BNE eov_2
  LDA v_dl
  CMP #22
  BCC eov_yes
eov_2:
  SEC
  LDA ship_xl
  SBC e_xl, X
  STA v_dl
  LDA ship_xh
  SBC e_xh, X
  AND #1
  BNE eov_no
  LDA v_dl
  CMP v_g
  BCC eov_yes
eov_no:
  CLC
  RTS
eov_yes:
  SEC
  RTS

// Paint slot X. Off screen means wiped; unchanged means nothing to do.
ent_render:
  LDA e_boom, X
  BNE er_boom
  LDA e_type, X
  ASL A
  STA v_g
  LDA tick
  LSR A
  LSR A
  AND #1
  ORA v_g
  ASL A
  TAY
  LDA img_tab, Y
  STA b_src
  LDA img_tab+1, Y
  STA b_src+1
  LDA #5
  LDY e_type, X
  CPY #ET_HUMAN
  BEQ er_nc4
  CPY #ET_BOMB
  BNE er_pos
er_nc4:
  LDA #4
  BNE er_pos
er_boom:
  LDA #5
  STA b_nc
  LDA e_boom, X
  CMP #6
  BCC er_boom2
  LDA #%lo(k_df_boom0)
  STA b_src
  LDA #%hi(k_df_boom0)
  STA b_src+1
  JMP er_pos_set
er_boom2:
  LDA #%lo(k_df_boom1)
  STA b_src
  LDA #%hi(k_df_boom1)
  STA b_src+1
  JMP er_pos_set
er_pos:
  STA b_nc
er_pos_set:
  SEC
  LDA e_xl, X
  SBC cam_xl
  STA v_sxl
  LDA e_xh, X
  SBC cam_xh
  AND #1
  STA v_sxh
  BEQ er_vis
  LDA v_sxl
  CMP #64
  BCC er_vis
  JMP er_off
er_vis:
  LDA v_sxh
  LSR A
  LDA v_sxl
  ROR A
  LSR A
  CMP #1
  BCS er_c1
  JMP er_off
er_c1:
  LDY b_nc
  CPY #4
  BEQ er_lim4
  CMP #75
  BCC er_colok
  JMP er_off
er_lim4:
  CMP #76
  BCC er_colok
  JMP er_off
er_colok:
  STA b_col
  // Mode-2 1-pixel sub-column shift (bit 1 of screen x): +48 for 4-col, +60 for 5-col.
  LDA v_sxl
  AND #2
  BEQ er_ph0
  LDA #60
  LDY b_nc
  CPY #4
  BNE er_add_ph
  LDA #48
er_add_ph:
  CLC
  ADC b_src
  STA b_src
  LDA b_src+1
  ADC #0
  STA b_src+1
er_ph0:
  LDA e_y, X
  STA b_y
  LDA e_shown, X
  BEQ er_draw
  LDA b_col
  CMP e_pcol, X
  BNE er_draw
  LDA b_y
  CMP e_py, X
  BNE er_draw
  LDA b_src
  CMP e_psl, X
  BNE er_draw
  LDA b_src+1
  CMP e_psh, X
  BNE er_draw
  RTS
er_draw:
  LDA e_shown, X
  STA b_wipe
  LDA e_pcol, X
  STA b_ocol
  LDA e_py, X
  STA b_oy
  LDA e_pnc, X
  STA b_onc
  LDA #1
  STA b_draw
  JSR blit_win
  LDX e_i
  LDA b_col
  STA e_pcol, X
  LDA b_nc
  STA e_pnc, X
  LDA b_src
  STA e_psl, X
  LDA b_src+1
  STA e_psh, X
  LDA #1
  STA e_shown, X
  LDA b_y
  STA e_py, X
  CMP #204
  BCS er_hot
  LDA b_oy
  CMP #204
  BCC er_ret
er_hot:
  LDA #1
  STA t_force
er_ret:
  RTS
er_off:
  LDA e_shown, X
  BEQ er_ret
  LDA #1
  STA b_wipe
  LDA #0
  STA b_draw
  LDA e_pcol, X
  STA b_ocol
  LDA e_py, X
  STA b_oy
  LDA e_pnc, X
  STA b_onc
  JSR blit_win
  LDX e_i
  LDA #0
  STA e_shown, X
  LDA e_py, X
  CMP #204
  BCC er_ret
  LDA #1
  STA t_force
  RTS

// --------------------------------------------------------- parallax stars

// Half-speed parallax twinkling stars in open sky: never overwrite non-zero
// screen bytes and only erase bytes that still match the star's own ink.
.global stars_paint
stars_paint:
  // Half camera column: (cam_x >> 3) & 127.
  LDA cam_xh
  LSR A
  LDA cam_xl
  ROR A
  LSR A
  LSR A
  STA Z_W
  LDA #DF_STARS-1
  STA Z_I
  JSR shadow_on
st_loop:
  LDX Z_I
  LDA st_ph, X
  BEQ st_no_er
  STA P1+1
  LDA st_pl, X
  STA P1
  LDY #0
  LDA (P1), Y
  CMP st_pv, X
  BNE st_cl_ptr
  LDA #0
  STA (P1), Y
st_cl_ptr:
  LDA #0
  STA st_ph, X
st_no_er:
  // Subtle twinkle: one star blinks out briefly based on (tick >> 2).
  LDA tick
  LSR A
  LSR A
  AND #31
  CMP Z_I
  BEQ st_next
  LDX Z_I
  LDA star_wx, X
  SEC
  SBC Z_W
  AND #127
  CMP #1
  BCC st_next
  CMP #79
  BCS st_next
  STA fx_col
  LDA star_y, X
  STA fx_y
  JSR strip_addr
  LDA (P1), Y
  BNE st_next
  LDX Z_I
  TYA
  CLC
  ADC P1
  STA P1
  STA st_pl, X
  LDA P1+1
  ADC #0
  STA P1+1
  STA st_ph, X
  LDA star_ink, X
  STA st_pv, X
  LDY #0
  STA (P1), Y
st_next:
  DEC Z_I
  BPL st_loop
  JMP shadow_off

// ---------------------------------------------------------------- terrain

.global terrain_reset
terrain_reset:
  LDX #DF_VIEW_COLS-1
  LDA #0
tr_loop:
  STA t_ptop, X
  STA t_plen, X
  DEX
  BPL tr_loop
  LDA #1
  STA t_force
  LDA #0xff
  STA t_cam
  RTS

// Two-tone shaded ridge: bright Yellow sunlit crest, dithered Yellow/Red
// crag transition on row 2, and volcanic Red rock cliff below.
.global terrain_paint
terrain_paint:
  LDA cam_xh
  LSR A
  LDA cam_xl
  ROR A
  LSR A
  AND #127
  LDX t_force
  BNE tp_go
  CMP t_cam
  BNE tp_go
  RTS
tp_go:
  STA t_cam
  STA Z_W
  LDA #0
  STA t_force
  STA Z_I
  STA Z_CL
  STA Z_CH
  JSR shadow_on
tp_col:
  LDY Z_W
  LDA terr_top, Y
  STA Z_TOP
  LDA terr_len, Y
  STA Z_LEN
  LDX Z_I
  CMP t_plen, X
  BNE tp_chg
  LDA Z_TOP
  CMP t_ptop, X
  BNE tp_chg
  JMP tp_next
tp_chg:
  LDA t_plen, X
  BEQ tp_nodel
  STA Z_CNT
  LDA t_ptop, X
  SEC
  SBC #DF_TERR_Y0
  TAX
  JSR tp_wipe_run
tp_nodel:
  LDA Z_LEN
  STA Z_CNT
  LDA Z_TOP
  SEC
  SBC #DF_TERR_Y0
  TAX
  JSR tp_draw_run
  LDX Z_I
  LDA Z_TOP
  STA t_ptop, X
  LDA Z_LEN
  STA t_plen, X
tp_next:
  LDA Z_CL
  CLC
  ADC #8
  STA Z_CL
  BCC tp_nc
  INC Z_CH
tp_nc:
  INC Z_W
  LDA Z_W
  AND #127
  STA Z_W
  INC Z_I
  LDA Z_I
  CMP #DF_VIEW_COLS
  BEQ tp_done
  AND #15
  BNE tp_more
  JSR shadow_off
  NOP
  NOP
  NOP
  JSR shadow_on
tp_more:
  JMP tp_col
tp_done:
  JMP shadow_off

tp_wipe_run:
twr_lp:
  JSR y192_ptr
  TYA
  STA (P1), Y
  INX
  DEC Z_CNT
  BNE twr_lp
  RTS

tp_draw_run:
  LDY #0
  STY Z_COL
tdr_lp:
  JSR y192_ptr
  LDA Z_COL
  BNE tdr_n0
  LDA #0x0f
  BNE tdr_put
tdr_n0:
  CMP #1
  BNE tdr_rock
  LDA Z_W
  AND #1
  BEQ tdr_d0
  LDA #0x0b
  BNE tdr_put
tdr_d0:
  LDA #0x07
  BNE tdr_put
tdr_rock:
  LDA #0x03
tdr_put:
  STA (P1), Y
  INC Z_COL
  INX
  DEC Z_CNT
  BNE tdr_lp
  RTS

// ---------------------------------------------------------------- scanner

// v_pxl/v_pxh/v_py -> P1 = top-minimap 2-scanline cell (cols 24..55, y=4..23).
sc_ptr:
  LDA v_py
  SEC
  SBC #32
  BCS sp_ypos
  LDA #0
sp_ypos:
  LSR A
  LSR A
  LSR A
  LSR A
  CMP #10
  BCC sp_yok
  LDA #9
sp_yok:
  TAY
  LDA sc_ylo, Y
  STA Z_CL
  LDA sc_yhi, Y
  STA Z_CH
  LDA v_pxh
  LSR A
  LDA v_pxl
  ROR A
  LSR A
  LSR A
  LSR A
  CLC
  ADC #DF_SCAN_COL
  STA P1
  LDA #0
  STA P1+1
  ASL P1
  ROL P1+1
  ASL P1
  ROL P1+1
  ASL P1
  ROL P1+1
  LDA P1
  CLC
  ADC Z_CL
  STA P1
  LDA P1+1
  ADC Z_CH
  STA P1+1
  RTS

// Plot 2-scanline blip: X = scanner slot, Z_VAL = Mode-2 colour byte.
sp_plot:
  JSR sc_ptr
  LDY #0
  LDA Z_VAL
  STA (P1), Y
  INY
  STA (P1), Y
  LDA P1
  STA sc_pl, X
  LDA P1+1
  STA sc_ph, X
  RTS

// Plot Z_VAL on top-minimap bracket rows y=2 and y=29 at column A (24..55).
sc_bracket:
  CMP #DF_SCAN_COL
  BCC sbk_ret
  CMP #DF_SCAN_COL+32
  BCS sbk_ret
  STA Z_I
  LDY #2
  LDA Z_VAL
  JSR pf_top_put
  LDY #29
  LDA Z_VAL
  JMP pf_top_put
sbk_ret:
  RTS

// Every other frame: update camera-window White brackets and 2-scanline
// colour-coded entity/ship blips on the framed top-centre minimap.
.global scanner_paint
scanner_paint:
  LDA sc_par
  EOR #1
  STA sc_par
  BEQ sn_go
  RTS
sn_go:
  JSR shadow_on
  // Erase previous viewport brackets.
  LDA #0
  STA Z_VAL
  LDA sc_vl
  JSR sc_bracket
  LDA sc_vr
  JSR sc_bracket
  // Draw new White (0x3f) viewport brackets for cam_x .. cam_x + 304 (20 cols).
  LDA #0x3f
  STA Z_VAL
  LDA cam_xh
  LSR A
  LDA cam_xl
  ROR A
  LSR A
  LSR A
  LSR A
  PHA
  CLC
  ADC #DF_SCAN_COL
  STA sc_vl
  JSR sc_bracket
  PLA
  CLC
  ADC #19
  AND #31
  CLC
  ADC #DF_SCAN_COL
  STA sc_vr
  JSR sc_bracket
  // Erase last blips.
  LDX #DF_ENTS
sn_er:
  LDA sc_ph, X
  BEQ sn_ern
  STA P1+1
  LDA sc_pl, X
  STA P1
  LDY #0
  TYA
  STA (P1), Y
  INY
  STA (P1), Y
  LDA #0
  STA sc_ph, X
sn_ern:
  DEX
  BPL sn_er
  // Draw live entities in their Mode-2 colours.
  LDX #0
sn_dr:
  LDA e_type, X
  BEQ sn_drn
  LDA e_boom, X
  BNE sn_drn
  LDY e_type, X
  LDA sc_val, Y
  BEQ sn_drn
  STA Z_VAL
  LDA e_xl, X
  STA v_pxl
  LDA e_xh, X
  STA v_pxh
  LDA e_y, X
  STA v_py
  STX v_hud
  JSR sp_plot
  LDX v_hud
sn_drn:
  INX
  CPX #DF_ENTS
  BNE sn_dr
  LDA ship_on
  BEQ sn_out
  LDA ship_xl
  STA v_pxl
  LDA ship_xh
  STA v_pxh
  LDA ship_y
  STA v_py
  LDA #0x3f
  STA Z_VAL
  LDX #DF_ENTS
  JSR sp_plot
sn_out:
  JMP shadow_off

// ---------------------------------------------------------------- sound

sound_play:
  LDA #7
  JMP OSWORD

.global sound_boom
sound_boom:
  LDX #%lo(snd_boom)
  LDY #%hi(snd_boom)
  JMP sound_play

.global sound_death
sound_death:
  LDX #%lo(snd_death)
  LDY #%hi(snd_death)
  JMP sound_play

.global sound_shot
sound_shot:
  LDX #%lo(snd_shot)
  LDY #%hi(snd_shot)
  JMP sound_play

.global sound_bomb
sound_bomb:
  LDX #%lo(snd_bomb)
  LDY #%hi(snd_bomb)
  JMP sound_play
