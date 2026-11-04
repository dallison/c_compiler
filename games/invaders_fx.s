// Per-frame Space Invaders work for invaders.cc: keys, the fleet's march,
// the laser, the bombs, and their collisions. invaders.cc keeps the rules
// (scoring, lives, waves, timers) and calls these once per vsync.
//
// Lives in .text.blitter with its state so it stays below &3000 next to
// screen1.s. Units are screen1's: byte columns (4 px), character rows
// (8 px), and pixel rows. The fleet is 5 ranks x 11 columns; alien i is
// rank i / 11, column i % 11, at byte column origin + 5 * column and
// character row origin + 2 * rank.

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

.set SHOT_LEN 6
.set SHOT_STEP 4
.set SHOT_SPAN 10
.set SHOT_START 202
.set SHOT_TOP 28
.set BOMB_STEP 2
.set BOMB_LEN 8
.set GROUND_Y 232
.set PLAYER_Y 208
.set PLAYER_ROW 26
.set UFO_Y 32
.set UFO_ROW 4
.set BUNKER_TOP 176
.set BUNKER_END 192
.set KEY_FIRE 4
.set CANNON_MIN 8
.set CANNON_MAX 296
.set CANNON_STEP 2
.set UFO_MIN 4
.set UFO_MAX 300
.set UFO_STEP 2
.set HUD_SCORE_COL 6
.set HUD_LIVES_COL 18
.set HUD_WAVE_COL 24

.section ".text.blitter", "ax", @progbits

// ---------------------------------------------------------------- state

.global key_bits
key_bits:
.byte 0

.global alive
alive:
.byte 0
.global fl_on
fl_on:
.space 55
.global fl_col
fl_col:
.byte 0
.global fl_row
fl_row:
.byte 0
fl_old_col:
.byte 0
fl_old_row:
.byte 0
// Next alien the sweep redraws. 255 between sweeps, which also makes
// "index < fl_mover" mean "already at the new origin" for every alien.
.global fl_mover
fl_mover:
.byte 255
fl_dir:
.byte 1
fl_dx:
.byte 0
fl_dy:
.byte 0
fl_frame:
.byte 0
.global fl_lo
fl_lo:
.byte 0
.global fl_hi
fl_hi:
.byte 10
.global fl_bot
fl_bot:
.byte 4
.global fl_quota
fl_quota:
.byte 1
.global fl_landed
fl_landed:
.byte 0

.global cannon_x
cannon_x:
.hword 152
// Fire seen since the last shot went up.
fire_want:
.byte 0
.global ufo_x
ufo_x:
.hword 0
.global ufo_on
ufo_on:
.byte 0
// 1 right, 255 left.
.global ufo_dir
ufo_dir:
.byte 1

.global shot_on
shot_on:
.byte 0
shot_x:
.hword 0
shot_col:
.byte 0
shot_y:
.byte 0
shot_pat:
.space SHOT_SPAN
.global shot_event
shot_event:
.byte 0
.global shot_hit_col
shot_hit_col:
.byte 0
.global shot_hit_row
shot_hit_row:
.byte 0
.global shot_hit_rank
shot_hit_rank:
.byte 0

bomb_on:
.space 3
bomb_col:
.space 3
bomb_y:
.space 3
bomb_anim:
.space 3
.global bomb_hit
bomb_hit:
.byte 0
.global drop_col
drop_col:
.byte 0

// ---------------------------------------------------------------- tables

idx_rank:
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
.byte 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
.byte 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
.byte 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
idx_col5:
.byte 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
.byte 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
.byte 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
.byte 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
.byte 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
rank11:
.byte 0, 11, 22, 33, 44

// (rank * 2 + frame) -> 6-column window
alien_spr:
.hword k_squid0
.hword k_squid1
.hword k_crab0
.hword k_crab1
.hword k_crab0
.hword k_crab1
.hword k_oct0
.hword k_oct1
.hword k_oct0
.hword k_oct1

bomb_pat:
.hword k_bomb0
.hword k_bomb1
.hword k_bomb2

// Internal key numbers, and the key_bits they set: 1 left (Z A cursor),
// 2 right (X D cursor), 4 fire (Space Return), 8 Escape.
key_codes:
.byte 0x61, 0x41, 0x19, 0x42, 0x32, 0x79, 0x62, 0x49, 0x70
key_masks:
.byte 1, 1, 1, 2, 2, 2, 4, 4, 8

// 1000, 100, 10, 1
place_lo:
.byte 0xe8, 0x64, 0x0a, 0x01
place_hi:
.byte 0x03, 0x00, 0x00, 0x00

// ---------------------------------------------------------------- misc

.global frame_wait
frame_wait:
  LDA #19
  JMP OSBYTE

// Score (to 9999), lives and wave (to 9) in white beside the labels
// Hud::labels writes on the top text row: score in text columns 3..6,
// lives in 9, wave in 12. A text column is two byte columns. Drawing them
// through OSWRCH cost more than a frame.
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
  SBC place_lo, X
  STA E_R
  LDA E_T1
  SBC place_hi, X
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
  CPX #4
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
  LDA #0
  STA fx_row
  LDA #16
  STA fx_len
  JMP fx_cell

// Scans the keys on the System VIA the way the MOS does for a single key:
// keyboard auto-scan off, key number out on port A, bit 7 back set when it
// is down. OSBYTE &79 per key cost 2.5K cycles a frame. Interrupts stay
// off throughout, since the MOS interrupt code drives the same port.
.global keys_read
keys_read:
  LDA #0
  STA key_bits
  SEI
  LDA #0x7f
  STA SYSVIA_DDRA
  LDA #0x03
  STA SYSVIA_LATCH
  LDX #8
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

// fl_row is the starting character row; everything else starts over.
.global fleet_reset
fleet_reset:
  LDX #54
  LDA #1
fr_loop:
  STA fl_on, X
  DEX
  BPL fr_loop
  LDA #55
  STA alive
  LDA #13
  STA fl_col
  STA fl_old_col
  LDA fl_row
  STA fl_old_row
  LDA #1
  STA fl_dir
  LDA #0
  STA fl_frame
  STA fl_dy
  STA fl_landed
  STA fl_lo
  LDA #10
  STA fl_hi
  LDA #4
  STA fl_bot
  LDA #255
  STA fl_mover
  RTS

.global fleet_draw
fleet_draw:
  LDA #0
  STA fl_dy
  LDX #0
fd_loop:
  LDA fl_on, X
  BEQ fd_next
  STX E_IDX
  JSR alien_redraw
  LDX E_IDX
fd_next:
  INX
  CPX #55
  BNE fd_loop
  RTS

// Redraw up to fl_quota live aliens of the current sweep, one step on.
// Starts the next sweep when the last one finished; fl_landed is set
// instead when that sweep would bring the fleet down onto the cannon.
.global fleet_march
fleet_march:
  LDA alive
  BEQ fm_out
  LDA fl_mover
  CMP #55
  BCC fm_run
  JSR fleet_turn
  BCS fm_out
fm_run:
  LDA fl_quota
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
  CMP #55
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
// about-face. Carry set when that lands the fleet.
fleet_turn:
  LDA #0
  STA fl_dx
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
  STA fl_dx
  JMP ft_check
ft_left:
  LDX fl_lo
  LDA idx_col5, X
  CLC
  ADC fl_col
  CMP #3
  BCC ft_down
  LDA #255
  STA fl_dx
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
  ADC fl_dx
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
  CLC
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
  CPX #11
  BNE fb_lo
  RTS
fb_lo_found:
  STX fl_lo
  LDX #10
fb_hi:
  JSR column_live
  BNE fb_hi_found
  DEX
  BPL fb_hi
fb_hi_found:
  STX fl_hi
  LDX #54
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
.global shot_fire
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
  CMP #55
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
probe_fleet:
  LDA shot_col
  SEC
  SBC E_OC
  BCC pf_miss
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
  LDX #2
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
// the next. Nothing happens with three bombs already falling.
.global bomb_drop
bomb_drop:
  LDX #2
bd_slot:
  LDA bomb_on, X
  BEQ bd_free
  DEX
  BPL bd_slot
  RTS
bd_free:
  STX E_IDX
  LDA drop_col
  CMP #11
  BCC bd_have_col
  JSR column_above_cannon
bd_have_col:
  STA E_C
  LDA #11
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
  CMP #11
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
  CLC
  ADC E_OC
  ADC #1
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

// A = the fleet column over the cannon's turret, clamped to 0..10.
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
  BCS ca_right
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
  CMP #11
  BCC ca_out
  LDA #10
ca_out:
  RTS

// Each falling bomb drops BOMB_STEP rows. bomb_hit is set when one meets
// the cannon (that bomb is gone; the cannon is left for the caller).
.global bombs_step
bombs_step:
  LDA #0
  STA bomb_hit
  LDX #2
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

// Laser and bombs off, and no shot queued; volley_clear also wipes them.
.global volley_reset
volley_reset:
  LDA #0
  STA fire_want
  STA shot_on
  STA bomb_on
  STA bomb_on+1
  STA bomb_on+2
  RTS

.global volley_clear
volley_clear:
  LDA shot_on
  BEQ vc_bombs
  LDA #0
  JSR shot_spent
vc_bombs:
  LDX #2
vc_loop:
  STX E_IDX
  LDA bomb_on, X
  BEQ vc_next
  JSR bomb_spent
vc_next:
  LDX E_IDX
  DEX
  BPL vc_loop
  RTS

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

// The cannon steps CANNON_STEP px while left or right is held (left wins).
// Fire is latched, so a press while the laser is up queues the next shot.
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
  CMP #%lo(CANNON_MAX)
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
  JSR cannon_at
  LDA #%lo(k_cannon)
  STA fx_src
  LDA #%hi(k_cannon)
  STA fx_src+1
  JMP window_draw

.global cannon_draw_red
cannon_draw_red:
  JSR cannon_at
  LDA #%lo(k_cannon_red)
  STA fx_src
  LDA #%hi(k_cannon_red)
  STA fx_src+1
  JMP window_draw

.global cannon_draw_blast
cannon_draw_blast:
  JSR cannon_at
  LDA #%lo(k_blast)
  STA fx_src
  LDA #%hi(k_blast)
  STA fx_src+1
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

.global ufo_erase
ufo_erase:
  JSR ufo_at
  JMP window_wipe

// Brings the saucer in at the side it leaves ufo_dir from.
.global ufo_launch
ufo_launch:
  LDA #0
  STA ufo_x+1
  LDA #UFO_MIN
  LDX ufo_dir
  BPL ul_set
  LDA #1
  STA ufo_x+1
  LDA #%lo(UFO_MAX)
ul_set:
  STA ufo_x
  LDA #1
  STA ufo_on
  JMP ufo_draw

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
  CMP #%lo(UFO_MAX)
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
