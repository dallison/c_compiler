// Space Invaders play for invaders.cc: each frame's keys, cannon, fleet
// march, laser, bombs, saucer, and their collisions, and what can happen
// on top of that in a frame of play: firing, dropping a bomb, the fleet's
// new bounds after a kill, and the score. davecc's C++ took 4K-12K cycles
// for each of those, enough to push a busy frame past the vsync.
// invaders.cc keeps the rules, the state and tables, and everything else.
//
// Lives in .text.blitter so it stays below &3000 next to screen1.s, with
// shot_pat, which fx_strip reads while the shadow screen is mapped. Units
// and the fleet's layout are in invaders.h.

#include "invaders.h"

.set OSWORD 0xfff1
.set OSBYTE 0xfff4
.set SYSVIA_LATCH 0xfe40
.set SYSVIA_DDRA 0xfe43
.set SYSVIA_PORTA 0xfe4f

.set E_NY 0x78
.set E_IDX 0x79
.set E_OC 0x7a
.set E_OR 0x7b
.set E_OLD 0x7c
.set E_C 0x7d
.set E_R 0x7e
.set E_AY 0x7f
.set E_T0 0x80
.set E_T1 0x81
.set E_LEFT 0x82
.set E_C5 0x83
.set E_RANK 0x84
.set E_X 0x85

.section ".text.blitter", "ax", @progbits

// The laser's strip for fx_strip: SHOT_LEN rows of its pixel, which
// shot_fire sets, then the dark rows that wipe its tail.
shot_pat:
.space SHOT_SPAN

// ---------------------------------------------------------------- misc

// OSBYTE 19 straight: through bbc::osbyte it cost 2K cycles a frame after
// the vsync.
.global frame_wait
frame_wait:
  LDA #19
  JMP OSBYTE

// Score (to 9999), lives and wave (to 9) in white under the labels
// Hud::labels writes, at the byte columns in invaders.h. A text column is
// two byte columns. Drawing them through OSWRCH cost more than a frame.
// hud_places is in invaders.cc.
.global hud_digits
hud_digits:
  LDA score
  STA E_T0
  LDA score+1
  STA E_T1
  CMP #0x27
  BCC hd_counted
  BNE hd_clamp
  LDA E_T0
  CMP #0x10
  BCC hd_counted
hd_clamp:
  LDA #0x0f
  STA E_T0
  LDA #0x27
  STA E_T1
hd_counted:
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
  STX E_IDX
  TYA
  JSR hud_glyph
  LDX E_IDX
  INX
  INX
  CPX #8
  BNE hd_place
  LDA lives
  LDX #HUD_LIVES_COL
  JSR hud_unit
  LDA wave
  LDX #HUD_WAVE_COL
hud_unit:
  STX E_C
  CMP #10
  BCC hud_glyph
  LDA #9
// A = digit drawn at byte column E_C, which moves on a text column.
hud_glyph:
  ASL A
  ASL A
  ASL A
  ASL A
  CLC
  ADC #%lo(k_digits)
  STA fx_src
  LDA #%hi(k_digits)
  ADC #0
  STA fx_src+1
  LDA E_C
  STA fx_col
  CLC
  ADC #2
  STA E_C
  LDA #HUD_ROW
  STA fx_row
  LDA #16
  STA fx_len
  JMP fx_cell

// ---------------------------------------------------------------- keys

// Scans the keys on the System VIA the way the MOS does for a single key:
// keyboard auto-scan off, key number out on port A, bit 7 back set when it
// is down. OSBYTE &79 per key cost 2.5K cycles a frame. Interrupts stay
// off throughout, since the MOS interrupt code drives the same port.
// key_codes and key_masks are in invaders.cc.
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
  LDA key_bits
  ORA key_masks, X
  STA key_bits
kr_up:
  DEX
  BPL kr_loop
  LDA #0x0b
  STA SYSVIA_LATCH
  CLI
  RTS

// ---------------------------------------------------------------- fleet

// Redraw up to FLEET_QUOTA live aliens of the current sweep, one step on.
// Starts the next sweep when the last one finished; fl_landed is set
// instead when that sweep would bring the fleet down onto the cannon.
.global fleet_march
fleet_march:
  LDA march_gap
  BEQ fm_marching
  DEC march_gap
fm_marching:
  LDA alive
  BEQ fm_out
  LDA fl_mover
  CMP #FLEET_SIZE
  BCC fm_run
  JSR fleet_turn
  BCS fm_out
fm_run:
  LDA #FLEET_QUOTA
  STA E_LEFT
fm_loop:
  LDX fl_mover
  LDA fl_on, X
  BEQ fm_next
  JSR alien_redraw
  DEC E_LEFT
fm_next:
  INC fl_mover
  LDA fl_mover
  CMP #FLEET_SIZE
  BCS fm_end
  LDA E_LEFT
  BNE fm_loop
fm_out:
  RTS
fm_end:
  LDA #255
  STA fl_mover
  RTS

// Next step: a byte sideways, or at a wall a character row down and an
// about-face. Carry set when that lands the fleet. E_T0 is the step
// sideways.
fleet_turn:
  LDA #0
  STA E_T0
  STA fl_dy
  LDA fl_dir
  BMI ft_left
  LDX fl_hi
  LDA idx_col5, X
  CLC
  ADC fl_col
  ADC #5
  CMP #79
  BCS ft_down
  LDA #1
  STA E_T0
  JMP ft_check
ft_left:
  LDX fl_lo
  LDA idx_col5, X
  CLC
  ADC fl_col
  CMP #3
  BCC ft_down
  LDA #255
  STA E_T0
  JMP ft_check
ft_down:
  LDA #1
  STA fl_dy
  LDA fl_dir
  EOR #0xfe
  STA fl_dir
ft_check:
  LDA fl_bot
  ASL A
  CLC
  ADC fl_row
  ADC fl_dy
  ADC #1
  CMP #PLAYER_ROW
  BCC ft_go
  LDA #1
  STA fl_landed
  SEC
  RTS
ft_go:
  LDA fl_col
  STA fl_old_col
  CLC
  ADC E_T0
  STA fl_col
  LDA fl_row
  STA fl_old_row
  CLC
  ADC fl_dy
  STA fl_row
  LDA fl_frame
  EOR #1
  STA fl_frame
  LDA #0
  STA fl_mover
  JSR march_note
  CLC
  RTS

// The next of the march's four notes, unless one sounded in the last
// MARCH_GAP frames.
march_note:
  LDA march_gap
  BNE mn_out
  LDA #MARCH_GAP
  STA march_gap
  LDX march_step
  LDA march_pitch, X
  STA snd_march+4
  INX
  TXA
  AND #3
  STA march_step
  LDX #%lo(snd_march)
  LDY #%hi(snd_march)
  JMP sound_play
mn_out:
  RTS

// X = alien. Its window at the current origin; on a step down, also the
// cell it left one character row up.
alien_redraw:
  LDA idx_rank, X
  STA E_RANK
  ASL A
  CLC
  ADC fl_row
  STA fx_row
  LDA idx_col5, X
  STA E_C5
  CLC
  ADC fl_col
  SEC
  SBC #1
  STA fx_col
  LDA E_RANK
  ASL A
  ORA fl_frame
  ASL A
  TAY
  LDA alien_spr, Y
  STA fx_src
  LDA alien_spr+1, Y
  STA fx_src+1
  LDA #48
  STA fx_len
  JSR fx_cell
  LDA fl_dy
  BEQ ar_done
  LDA E_RANK
  ASL A
  CLC
  ADC fl_old_row
  STA fx_row
  LDA E_C5
  CLC
  ADC fl_old_col
  STA fx_col
  LDA #32
  STA fx_len
  JSR fx_wipe
ar_done:
  RTS

// Narrow fl_lo / fl_hi to the outermost live columns, fl_bot to the
// lowest live rank. Leaves them alone when nobody is left.
.global fleet_bounds
fleet_bounds:
  LDX #0
fb_lo:
  JSR column_live
  BNE fb_lo_found
  INX
  CPX #FLEET_COLS
  BNE fb_lo
  RTS
fb_lo_found:
  STX fl_lo
  LDX #FLEET_COLS-1
fb_hi:
  JSR column_live
  BNE fb_hi_found
  DEX
  BPL fb_hi
fb_hi_found:
  STX fl_hi
  LDX #FLEET_SIZE-1
fb_bot:
  LDA fl_on, X
  BNE fb_bot_found
  DEX
  BPL fb_bot
fb_bot_found:
  LDA idx_rank, X
  STA fl_bot
  RTS

// X = column. Z clear when any rank of it is alive.
column_live:
  LDA fl_on, X
  ORA fl_on+11, X
  ORA fl_on+22, X
  ORA fl_on+33, X
  ORA fl_on+44, X
  RTS

// ---------------------------------------------------------------- laser

// A 1 px line, SHOT_LEN rows, from the cannon's turret. Does nothing while
// one is already up.
shot_fire:
  LDA shot_on
  BNE sf_out
  CLC
  LDA cannon_x
  ADC #8
  STA shot_x
  LDA cannon_x+1
  ADC #0
  STA shot_x+1
  LSR A
  LDA shot_x
  ROR A
  LSR A
  STA shot_col
  LDA shot_x
  AND #3
  TAX
  LDA #0x88
  CPX #0
  BEQ sf_mask
sf_shift:
  LSR A
  DEX
  BNE sf_shift
sf_mask:
  LDX #SHOT_LEN-1
sf_lit:
  STA shot_pat, X
  DEX
  BPL sf_lit
  LDA #0
  LDX #SHOT_SPAN-1
sf_dark:
  STA shot_pat, X
  DEX
  CPX #SHOT_LEN-1
  BNE sf_dark
  LDA #SHOT_START
  STA shot_y
  LDA #1
  STA shot_on
  LDX #%lo(snd_shot)
  LDY #%hi(snd_shot)
  JMP sound_play
sf_out:
  RTS

// Climb SHOT_STEP rows. shot_event: 0 still flying (or none), 1 hit alien
// (shot_hit_*; it is already off the fleet), 2 hit the saucer (wiped),
// 3 spent on a bomb, a shelter, or the top. A spent shot is wiped.
//
// Collisions use the span the line covers this frame: SHOT_LEN lit rows
// plus the SHOT_STEP rows wiped behind them, so nothing slips between
// frames.
.global shot_step
shot_step:
  LDA #0
  STA shot_event
  LDA shot_on
  BNE ss_go
  RTS
ss_go:
  LDA shot_y
  SEC
  SBC #SHOT_STEP
  STA E_NY
  CMP #SHOT_TOP
  BCS ss_up
  LDA #3
  JMP shot_spent
ss_up:
  JSR hit_ufo
  BCS ss_out
  JSR hit_aliens
  BCS ss_out
  JSR hit_bombs
  BCS ss_out
  JSR hit_bunker
  BCS ss_out
  LDA E_NY
  STA shot_y
  STA fx_y
  LDA shot_col
  STA fx_col
  LDA #%lo(shot_pat)
  STA fx_src
  LDA #%hi(shot_pat)
  STA fx_src+1
  LDA #SHOT_SPAN
  STA fx_len
  JMP fx_strip
ss_out:
  RTS

// A = shot_event. Wipes the line where it was last drawn.
shot_spent:
  STA shot_event
  LDA #0
  STA shot_on
shot_wipe:
  LDA shot_y
  STA fx_y
  LDA shot_col
  STA fx_col
  LDA #SHOT_LEN
  STA fx_len
  JMP fx_blank

hit_ufo:
  LDA ufo_on
  BEQ hu_miss
  LDA E_NY
  CMP #UFO_Y+8
  BCS hu_miss
  ADC #SHOT_SPAN
  CMP #UFO_Y+1
  BCC hu_miss
  SEC
  LDA shot_x
  SBC ufo_x
  STA E_T0
  LDA shot_x+1
  SBC ufo_x+1
  BNE hu_miss
  LDA E_T0
  CMP #16
  BCS hu_miss
  LDA #0
  STA ufo_on
  JSR ufo_erase
  LDA #2
  JSR shot_spent
  SEC
  RTS
hu_miss:
  CLC
  RTS

// Aliens the sweep has not reached stand at the old origin.
hit_aliens:
  LDA fl_col
  STA E_OC
  LDA fl_row
  STA E_OR
  LDA #0
  STA E_OLD
  JSR probe_fleet
  BCS ha_hit
  LDA fl_mover
  CMP #FLEET_SIZE
  BCS ha_miss
  LDA fl_old_col
  STA E_OC
  LDA fl_old_row
  STA E_OR
  LDA #1
  STA E_OLD
  JSR probe_fleet
  BCS ha_hit
ha_miss:
  CLC
  RTS
ha_hit:
  LDA #0
  STA fl_on, X
  DEC alive
  LDA idx_rank, X
  STA shot_hit_rank
  ASL A
  CLC
  ADC E_OR
  STA shot_hit_row
  LDA idx_col5, X
  CLC
  ADC E_OC
  STA shot_hit_col
  LDA #1
  JSR shot_spent
  SEC
  RTS

// Carry set and X = alien when the span meets a live alien that stands at
// origin (E_OC, E_OR): aliens below fl_mover when E_OLD is 0, the rest
// when it is 1. The lowest rank wins.
//
// With the left columns gone the fleet marches on past the left edge and
// the origin goes negative, so the column is taken mod 256: a shot left of
// the origin comes out at 186 or more and fails the test against 55.
probe_fleet:
  LDA shot_col
  SEC
  SBC E_OC
  CMP #55
  BCS pf_miss
  LDX #0
pf_div:
  CMP #5
  BCC pf_divided
  SBC #5
  INX
  JMP pf_div
pf_divided:
  CMP #4
  BEQ pf_miss
  STX E_C
  LDA #4
  STA E_R
pf_rank:
  LDA E_R
  ASL A
  CLC
  ADC E_OR
  ASL A
  ASL A
  ASL A
  STA E_AY
  LDA E_NY
  CLC
  ADC #SHOT_SPAN
  CMP E_AY
  BCC pf_next
  BEQ pf_next
  LDA E_AY
  CLC
  ADC #8
  CMP E_NY
  BCC pf_next
  BEQ pf_next
  LDX E_R
  LDA rank11, X
  CLC
  ADC E_C
  TAX
  LDA fl_on, X
  BEQ pf_next
  CPX fl_mover
  LDA #0
  ROL A
  CMP E_OLD
  BNE pf_next
  SEC
  RTS
pf_next:
  DEC E_R
  BPL pf_rank
pf_miss:
  CLC
  RTS

// A bomb in the line's byte column that the span meets: both go.
hit_bombs:
  LDX #BOMBS-1
hb_loop:
  LDA bomb_on, X
  BEQ hb_next
  LDA bomb_col, X
  CMP shot_col
  BNE hb_next
  LDA E_NY
  CLC
  ADC #SHOT_SPAN
  CMP bomb_y, X
  BCC hb_next
  BEQ hb_next
  LDA bomb_y, X
  CLC
  ADC #BOMB_LEN
  CMP E_NY
  BCC hb_next
  BEQ hb_next
  STX E_IDX
  JSR bomb_spent
  LDA #3
  JSR shot_spent
  SEC
  RTS
hb_next:
  DEX
  BPL hb_loop
  CLC
  RTS

// Rows the line climbs into this frame, where they cross the shelters'
// band. Anything lit there is shelter: the shot bites into it.
hit_bunker:
  LDA E_NY
  CMP #BUNKER_END
  BCS hk_miss
  ADC #SHOT_STEP
  CMP #BUNKER_TOP+1
  BCC hk_miss
  LDA E_NY
  CMP #BUNKER_TOP
  BCS hk_start
  LDA #BUNKER_TOP
hk_start:
  STA fx_y
  LDA E_NY
  CLC
  ADC #SHOT_STEP
  CMP #BUNKER_END
  BCC hk_end
  LDA #BUNKER_END
hk_end:
  SEC
  SBC fx_y
  STA fx_len
  LDA shot_col
  STA fx_col
  JSR fx_peek
  BEQ hk_miss
  LDA E_NY
  SEC
  SBC #2
  CMP #BUNKER_TOP
  BCS hk_bite
  LDA #BUNKER_TOP
hk_bite:
  STA fx_y
  LDA shot_y
  CLC
  ADC #SHOT_LEN
  SEC
  SBC fx_y
  STA fx_len
  LDA shot_col
  STA fx_col
  JSR fx_blank
  LDA #0
  STA shot_on
  LDA #3
  STA shot_event
  SEC
  RTS
hk_miss:
  CLC
  RTS

// ---------------------------------------------------------------- bombs

// Drop from the lowest live alien in fleet column drop_col (0..10), or
// above the cannon when drop_col is 11 or more. An empty column passes to
// the next. Nothing happens with every bomb already falling.
.global bomb_drop
bomb_drop:
  LDX #BOMBS-1
bd_slot:
  LDA bomb_on, X
  BEQ bd_free
  DEX
  BPL bd_slot
  RTS
bd_free:
  STX E_IDX
  LDA drop_col
  CMP #FLEET_COLS
  BCC bd_have_col
  JSR column_above_cannon
bd_have_col:
  STA E_C
  LDA #FLEET_COLS
  STA E_LEFT
bd_column:
  LDA #4
  STA E_R
bd_rank:
  LDY E_R
  LDA rank11, Y
  CLC
  ADC E_C
  TAY
  LDA fl_on, Y
  BNE bd_found
  DEC E_R
  BPL bd_rank
  INC E_C
  LDA E_C
  CMP #FLEET_COLS
  BCC bd_wrap
  LDA #0
  STA E_C
bd_wrap:
  DEC E_LEFT
  BNE bd_column
  RTS
bd_found:
  LDA fl_col
  STA E_OC
  LDA fl_row
  STA E_OR
  CPY fl_mover
  BCC bd_origin
  LDA fl_old_col
  STA E_OC
  LDA fl_old_row
  STA E_OR
bd_origin:
  LDX E_IDX
  LDA idx_col5, Y
  SEC
  ADC E_OC
  STA bomb_col, X
  LDA idx_rank, Y
  ASL A
  CLC
  ADC E_OR
  ADC #1
  ASL A
  ASL A
  ASL A
  STA bomb_y, X
  TXA
  ASL A
  ASL A
  STA bomb_anim, X
  LDA #1
  STA bomb_on, X
  RTS

// A = the fleet column over the cannon's turret, clamped to 0..10. The
// turret is at byte column 4..76 and fl_col is -48..74, so the difference
// is negative exactly when bit 7 is set.
column_above_cannon:
  CLC
  LDA cannon_x
  ADC #8
  STA E_T0
  LDA cannon_x+1
  ADC #0
  LSR A
  LDA E_T0
  ROR A
  LSR A
  SEC
  SBC fl_col
  BPL ca_right
  LDA #0
  RTS
ca_right:
  LDX #0
ca_div:
  CMP #5
  BCC ca_done
  SBC #5
  INX
  JMP ca_div
ca_done:
  TXA
  CMP #FLEET_COLS
  BCC ca_out
  LDA #FLEET_COLS-1
ca_out:
  RTS

// Each falling bomb drops BOMB_STEP rows. bomb_hit is set when one meets
// the cannon (that bomb is gone; the cannon is left for the caller).
.global bombs_step
bombs_step:
  LDA #0
  STA bomb_hit
  LDX #BOMBS-1
bs_loop:
  STX E_IDX
  LDA bomb_on, X
  BEQ bs_next
  JSR bomb_move
bs_next:
  LDX E_IDX
  DEX
  BPL bs_loop
  RTS

// X = E_IDX = bomb.
bomb_move:
  LDA bomb_y, X
  CLC
  ADC #BOMB_STEP
  STA E_NY
  ADC #BOMB_LEN
  CMP #GROUND_Y+1
  BCC bm_laser
  JMP bomb_spent
bm_laser:
  LDA shot_on
  BEQ bm_cannon
  LDA bomb_col, X
  CMP shot_col
  BNE bm_cannon
  LDA E_NY
  CLC
  ADC #BOMB_LEN
  CMP shot_y
  BCC bm_cannon
  BEQ bm_cannon
  LDA shot_y
  CLC
  ADC #SHOT_LEN
  CMP E_NY
  BCC bm_cannon
  BEQ bm_cannon
  LDA #0
  JSR shot_spent
  LDX E_IDX
  JMP bomb_spent
bm_cannon:
  LDA E_NY
  CMP #PLAYER_Y+8
  BCS bm_bunker
  ADC #BOMB_LEN
  CMP #PLAYER_Y+1
  BCC bm_bunker
  // Pixels [4 * col, 4 * col + 4) against [cannon_x, cannon_x + 16):
  // they meet when 4 * col + 3 - cannon_x is 0..18.
  LDA #0
  STA E_T1
  LDA bomb_col, X
  ASL A
  ROL E_T1
  ASL A
  ROL E_T1
  CLC
  ADC #3
  STA E_T0
  LDA E_T1
  ADC #0
  STA E_T1
  SEC
  LDA E_T0
  SBC cannon_x
  STA E_T0
  LDA E_T1
  SBC cannon_x+1
  BNE bm_bunker
  LDA E_T0
  CMP #19
  BCS bm_bunker
  LDA #1
  STA bomb_hit
  JMP bomb_spent
bm_bunker:
  LDA bomb_y, X
  CLC
  ADC #BOMB_LEN
  STA E_T0
  CMP #BUNKER_END
  BCS bm_draw
  ADC #BOMB_STEP
  CMP #BUNKER_TOP+1
  BCC bm_draw
  LDA E_T0
  CMP #BUNKER_TOP
  BCS bk_start
  LDA #BUNKER_TOP
bk_start:
  STA fx_y
  LDA E_T0
  CLC
  ADC #BOMB_STEP
  CMP #BUNKER_END
  BCC bk_end
  LDA #BUNKER_END
bk_end:
  SEC
  SBC fx_y
  STA fx_len
  LDA bomb_col, X
  STA fx_col
  JSR fx_peek
  LDX E_IDX
  CMP #0
  BEQ bm_draw
  LDA bomb_y, X
  STA fx_y
  CLC
  ADC #BOMB_LEN+4
  CMP #BUNKER_END
  BCC bk_bite
  LDA #BUNKER_END
bk_bite:
  SEC
  SBC fx_y
  STA fx_len
  LDA bomb_col, X
  STA fx_col
  JSR fx_blank
  LDX E_IDX
  LDA #0
  STA bomb_on, X
  RTS
bm_draw:
  INC bomb_anim, X
  LDA bomb_anim, X
  CMP #12
  BCC bm_frame
  LDA #0
  STA bomb_anim, X
bm_frame:
  LSR A
  LSR A
  ASL A
  TAY
  LDA bomb_pat, Y
  STA fx_src
  LDA bomb_pat+1, Y
  STA fx_src+1
  LDA bomb_y, X
  STA fx_y
  LDA bomb_col, X
  STA fx_col
  LDA #BOMB_STEP+BOMB_LEN
  STA fx_len
  LDA E_NY
  STA bomb_y, X
  JMP fx_strip

// X = E_IDX = bomb: wipe it and take it off.
bomb_spent:
  LDA #0
  STA bomb_on, X
  LDA bomb_y, X
  STA fx_y
  LDA bomb_col, X
  STA fx_col
  LDA #BOMB_LEN
  STA fx_len
  JMP fx_blank

// ---------------------------------------------------------------- sprites

// 6-column window for a 16 px sprite at even pixel x = E_X: the image at
// fx_src for x a multiple of 4, else the copy 2 px right that follows it.
window_draw:
  LDA E_X
  AND #2
  BEQ wd_even
  CLC
  LDA fx_src
  ADC #48
  STA fx_src
  BCC wd_even
  INC fx_src+1
wd_even:
  JSR window_col
  LDA #48
  STA fx_len
  JMP fx_cell

window_wipe:
  JSR window_col
  LDA #48
  STA fx_len
  JMP fx_wipe

window_col:
  LDA E_X+1
  LSR A
  LDA E_X
  ROR A
  LSR A
  SEC
  SBC #1
  STA fx_col
  RTS

cannon_at:
  LDA cannon_x
  STA E_X
  LDA cannon_x+1
  STA E_X+1
  LDA #PLAYER_ROW
  STA fx_row
  RTS

// The cannon steps CANNON_STEP px while left or right is held (left wins;
// KEY_LEFT and KEY_RIGHT are bits 0 and 1). Fire is latched, so a press
// while the laser is up queues the next shot.
.global player_step
player_step:
  LDA key_bits
  AND #KEY_FIRE
  ORA fire_want
  STA fire_want
  LDA key_bits
  LSR A
  BCS ps_left
  LSR A
  BCS ps_right
  BCC ps_fire
ps_left:
  LDA cannon_x+1
  BNE ps_go_left
  LDA cannon_x
  CMP #CANNON_MIN+1
  BCC ps_fire
ps_go_left:
  SEC
  LDA cannon_x
  SBC #CANNON_STEP
  STA cannon_x
  LDA cannon_x+1
  SBC #0
  STA cannon_x+1
  JMP ps_moved
ps_right:
  LDA cannon_x+1
  BEQ ps_go_right
  LDA cannon_x
  CMP #(CANNON_MAX & 0xff)
  BCS ps_fire
ps_go_right:
  CLC
  LDA cannon_x
  ADC #CANNON_STEP
  STA cannon_x
  LDA cannon_x+1
  ADC #0
  STA cannon_x+1
ps_moved:
  JSR cannon_draw
ps_fire:
  LDA fire_want
  BEQ ps_out
  LDA shot_on
  BNE ps_out
  STA fire_want
  JMP shot_fire
ps_out:
  RTS

.global cannon_draw
cannon_draw:
  LDA #%lo(k_cannon)
  STA fx_src
  LDA #%hi(k_cannon)
  STA fx_src+1
// The window at fx_src, over the cannon.
.global cannon_image
cannon_image:
  JSR cannon_at
  JMP window_draw

.global cannon_erase
cannon_erase:
  JSR cannon_at
  JMP window_wipe

ufo_at:
  LDA ufo_x
  STA E_X
  LDA ufo_x+1
  STA E_X+1
  LDA #UFO_ROW
  STA fx_row
  RTS

.global ufo_draw
ufo_draw:
  JSR ufo_at
  LDA #%lo(k_ufo)
  STA fx_src
  LDA #%hi(k_ufo)
  STA fx_src+1
  JMP window_draw

// The saucer off the screen, and its warble with it.
.global ufo_erase
ufo_erase:
  JSR ufo_at
  JSR window_wipe
  JMP sound_ufo_off

// UFO_STEP px along ufo_dir; off the far side it is wiped and gone.
.global ufo_step
ufo_step:
  LDA ufo_on
  BEQ us_out
  LDA ufo_dir
  BMI us_left
  LDA ufo_x+1
  BEQ us_go_right
  LDA ufo_x
  CMP #(UFO_MAX & 0xff)
  BCS us_gone
us_go_right:
  CLC
  LDA ufo_x
  ADC #UFO_STEP
  STA ufo_x
  LDA ufo_x+1
  ADC #0
  STA ufo_x+1
  JMP ufo_draw
us_left:
  LDA ufo_x+1
  BNE us_go_left
  LDA ufo_x
  CMP #UFO_MIN+1
  BCC us_gone
us_go_left:
  SEC
  LDA ufo_x
  SBC #UFO_STEP
  STA ufo_x
  LDA ufo_x+1
  SBC #0
  STA ufo_x+1
  JMP ufo_draw
us_gone:
  LDA #0
  STA ufo_on
  JMP ufo_erase
us_out:
  RTS

// ---------------------------------------------------------------- sound

// SOUND (OSWORD 7) with one of the blocks in invaders.cc, at Y:X. Each
// flushes its channel, so the MOS starts it at once rather than waiting
// for room in the channel's queue.
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

.global sound_ufo
sound_ufo:
  LDX #%lo(snd_ufo)
  LDY #%hi(snd_ufo)
  JMP sound_play

.global sound_ufo_off
sound_ufo_off:
  LDX #%lo(snd_ufo_off)
  LDY #%hi(snd_ufo_off)
  JMP sound_play

.global sound_ufo_boom
sound_ufo_boom:
  LDX #%lo(snd_ufo_boom)
  LDY #%hi(snd_ufo_boom)
  JMP sound_play
