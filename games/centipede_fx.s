// Centipede play for centipede.cc: keys, player, shot, crawl, mushrooms,
// spider. Lives in .text.blitter below &3000 with screen1.s. Board units
// and key bits are in centipede.h.

#include "centipede.h"

.set OSWORD 0xfff1
.set OSBYTE 0xfff4
.set SYSVIA_LATCH 0xfe40
.set SYSVIA_DDRA 0xfe43
.set SYSVIA_PORTA 0xfe4f

.set E_T0 0x78
.set E_T1 0x79
.set E_C 0x7a
.set E_R 0x7b
.set E_IDX 0x7c
.set E_X 0x7d
.set E_NY 0x7f
.set E_OLD 0x80
.set E_SLOT 0x81

.section ".text.blitter", "ax", @progbits

shot_pat:
.space CE_SHOT_SPAN

old_px_lo:
.byte 0
old_px_hi:
.byte 0
old_prow:
.byte 0
row_cd:
.byte 0

.global frame_wait
frame_wait:
  LDA #19
  JMP OSBYTE

.global hud_digits
hud_digits:
  LDA score
  STA E_T0
  LDA score+1
  STA E_T1
  CMP #0x27
  BCC hd_ok
  BNE hd_clamp
  LDA E_T0
  CMP #0x10
  BCC hd_ok
hd_clamp:
  LDA #0x0f
  STA E_T0
  LDA #0x27
  STA E_T1
hd_ok:
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

// ---------------------------------------------------------------- cells

// ce_col / ce_row -> fx_col / fx_row for an 8x8 cell.
cell_addr:
  LDA ce_col
  ASL A
  CLC
  ADC #CE_ORIGIN_COL
  STA fx_col
  LDA ce_row
  CLC
  ADC #CE_ORIGIN_ROW
  STA fx_row
  RTS

.global cell_draw
cell_draw:
  JSR cell_addr
  LDA ce_src
  STA fx_src
  LDA ce_src+1
  STA fx_src+1
  LDA #16
  STA fx_len
  JMP fx_cell

.global cell_wipe
cell_wipe:
  JSR cell_addr
  LDA #16
  STA fx_len
  JMP fx_wipe

// E_T0/E_T1 -> &mush[ce_row * CE_COLS + ce_col] (16-bit; board is 672 cells).
index_xy:
  LDA #0
  STA E_T0
  STA E_T1
  LDX ce_row
  BEQ ix_col
ix_add:
  CLC
  LDA E_T0
  ADC #CE_COLS
  STA E_T0
  BCC ix_nc
  INC E_T1
ix_nc:
  DEX
  BNE ix_add
ix_col:
  CLC
  LDA E_T0
  ADC ce_col
  STA E_T0
  BCC ix_base
  INC E_T1
ix_base:
  CLC
  LDA E_T0
  ADC #%lo(mush)
  STA E_T0
  LDA E_T1
  ADC #%hi(mush)
  STA E_T1
  RTS

// Redraw the mushroom at (ce_col, ce_row) from mush[], or wipe if empty.
.global mush_draw
mush_draw:
  JSR index_xy
  LDY #0
  LDA (E_T0), Y
  BEQ md_wipe
  SEC
  SBC #1
  ASL A
  TAX
  LDA mush_spr, X
  STA ce_src
  LDA mush_spr+1, X
  STA ce_src+1
  JMP cell_draw
md_wipe:
  JMP cell_wipe

// ---------------------------------------------------------------- player

cannon_at:
  LDA player_x
  STA E_X
  LDA player_x+1
  STA E_X+1
  LDA player_row
  CLC
  ADC #CE_ORIGIN_ROW
  STA fx_row
  RTS

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

.global player_step
player_step:
  LDA key_bits
  AND #KEY_FIRE
  ORA fire_want
  STA fire_want
  LDA player_x
  STA old_px_lo
  LDA player_x+1
  STA old_px_hi
  LDA player_row
  STA old_prow
  LDA #0
  STA E_OLD

  // Reset vertical repeat timer on release so fresh taps respond immediately.
  LDA key_bits
  AND #(KEY_UP | KEY_DOWN)
  BNE ps_cd_tick
  STA row_cd
  JMP ps_horiz
ps_cd_tick:
  LDA row_cd
  BEQ ps_horiz
  DEC row_cd

ps_horiz:
  LDA key_bits
  AND #KEY_LEFT
  BNE ps_left
  LDA key_bits
  AND #KEY_RIGHT
  BNE ps_right
  JMP ps_vert
ps_left:
  LDA player_x+1
  BNE ps_go_l
  LDA player_x
  CMP #36
  BCC ps_vert
ps_go_l:
  SEC
  LDA player_x
  SBC #2
  STA player_x
  LDA player_x+1
  SBC #0
  STA player_x+1
  LDA #1
  STA E_OLD
  JMP ps_vert
ps_right:
  LDA player_x+1
  BNE ps_go_r
  LDA player_x
  CMP #240
  BCS ps_vert
ps_go_r:
  CLC
  LDA player_x
  ADC #2
  STA player_x
  LDA player_x+1
  ADC #0
  STA player_x+1
  LDA #1
  STA E_OLD

ps_vert:
  LDA row_cd
  BNE ps_apply
  LDA key_bits
  AND #KEY_UP
  BNE ps_up
  LDA key_bits
  AND #KEY_DOWN
  BNE ps_down
  JMP ps_apply
ps_up:
  LDA player_row
  CMP #CE_PLAYER_MIN_ROW
  BEQ ps_apply
  BCC ps_apply
  DEC player_row
  JMP ps_vset
ps_down:
  LDA player_row
  CMP #CE_ROWS-1
  BCS ps_apply
  INC player_row
ps_vset:
  LDA #3
  STA row_cd
  LDA E_OLD
  ORA #2
  STA E_OLD

ps_apply:
  LDA E_OLD
  BEQ ps_fire
  JSR player_draw
  LDA E_OLD
  AND #2
  BEQ ps_fire
  // Row changed: wipe old 6-col window and restore any mushrooms on old_prow.
  LDA old_px_lo
  STA E_X
  LDA old_px_hi
  STA E_X+1
  LDA old_prow
  CLC
  ADC #CE_ORIGIN_ROW
  STA fx_row
  JSR window_wipe
  LDA fx_col
  SEC
  SBC #CE_ORIGIN_COL
  BCS ps_mc_ok
  LDA #0
  JMP ps_mc_go
ps_mc_ok:
  LSR A
ps_mc_go:
  STA E_C
  LDA #4
  STA E_SLOT
ps_mush_loop:
  LDA E_C
  CMP #CE_COLS
  BCS ps_mush_done
  STA ce_col
  LDA old_prow
  STA ce_row
  JSR mush_draw
  INC E_C
  DEC E_SLOT
  BNE ps_mush_loop
ps_mush_done:

ps_fire:
  LDA fire_want
  BEQ ps_out
  LDA shot_on
  BNE ps_out
  STA fire_want
  JMP shot_fire
ps_out:
  RTS

.global player_draw
player_draw:
  LDA #%lo(k_ce_ship)
  STA fx_src
  LDA #%hi(k_ce_ship)
  STA fx_src+1
.global player_image
player_image:
  JSR cannon_at
  JMP window_draw

.global player_erase
player_erase:
  JSR cannon_at
  JMP window_wipe

// ---------------------------------------------------------------- shot

shot_fire:
  LDA player_x
  CLC
  ADC #8
  STA shot_x
  LDA player_x+1
  ADC #0
  LSR A
  LDA shot_x
  ROR A
  LSR A
  STA shot_col
  LDA player_row
  CLC
  ADC #CE_ORIGIN_ROW
  ASL A
  ASL A
  ASL A
  SEC
  SBC #CE_SHOT_SPAN
  STA shot_y
  LDA #1
  STA shot_on
  LDA shot_x
  AND #3
  TAX
  LDA shot_masks, X
  STA E_T0
  LDX #0
sf_lit:
  LDA E_T0
  STA shot_pat, X
  INX
  CPX #CE_SHOT_LEN
  BCC sf_lit
sf_dark:
  LDA #0
  STA shot_pat, X
  INX
  CPX #CE_SHOT_SPAN
  BCC sf_dark
  LDA #%lo(shot_pat)
  STA fx_src
  LDA #%hi(shot_pat)
  STA fx_src+1
  LDA shot_col
  STA fx_col
  LDA shot_y
  STA fx_y
  LDA #CE_SHOT_SPAN
  STA fx_len
  JSR fx_strip
  LDX #%lo(snd_shot)
  LDY #%hi(snd_shot)
  LDA #7
  JMP OSWORD

shot_masks:
.byte 0x88, 0x44, 0x22, 0x11

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
  SBC #CE_SHOT_STEP
  STA E_NY
  CMP #CE_ORIGIN_ROW*8
  BCS ss_up
  JMP shot_spent
ss_up:
  STA shot_y
  LDA #%lo(shot_pat)
  STA fx_src
  LDA #%hi(shot_pat)
  STA fx_src+1
  LDA shot_col
  STA fx_col
  LDA shot_y
  STA fx_y
  LDA #CE_SHOT_SPAN
  STA fx_len
  JSR fx_strip
  JSR hit_segs
  LDA shot_event
  BNE ss_out
  JSR hit_mush
  LDA shot_event
  BNE ss_out
  JSR hit_spider
ss_out:
  RTS

shot_spent:
  LDA #3
  STA shot_event
shot_wipe:
  LDA #0
  STA shot_on
  LDA shot_col
  STA fx_col
  LDA shot_y
  STA fx_y
  LDA #CE_SHOT_LEN
  STA fx_len
  JMP fx_blank

hit_segs:
  LDX #CE_SEGS-1
hs_loop:
  LDA seg_on, X
  BEQ hs_next
  // board col of segment vs shot byte col.
  LDA seg_x, X
  ASL A
  CLC
  ADC #CE_ORIGIN_COL
  STA E_C
  LDA shot_col
  SEC
  SBC E_C
  CMP #2
  BCS hs_next
  LDA seg_y, X
  CLC
  ADC #CE_ORIGIN_ROW
  ASL A
  ASL A
  ASL A
  STA E_R
  LDA shot_y
  SEC
  SBC E_R
  CLC
  ADC #4
  CMP #12
  BCS hs_next
  STX shot_hit_seg
  LDA #1
  STA shot_event
  JMP shot_wipe
hs_next:
  DEX
  BPL hs_loop
  RTS

hit_mush:
  // board cell under the shot tip.
  LDA shot_col
  SEC
  SBC #CE_ORIGIN_COL
  BCC hm_miss
  LSR A
  CMP #CE_COLS
  BCS hm_miss
  STA ce_col
  LDA shot_y
  LSR A
  LSR A
  LSR A
  SEC
  SBC #CE_ORIGIN_ROW
  BCC hm_miss
  CMP #CE_ROWS
  BCS hm_miss
  STA ce_row
  JSR index_xy
  LDY #0
  LDA (E_T0), Y
  BEQ hm_miss
  SEC
  SBC #1
  STA (E_T0), Y
  STA E_OLD
  LDA #2
  STA shot_event
  JSR shot_wipe
  LDA E_OLD
  BEQ hm_gone
  JMP mush_draw
hm_gone:
  JMP cell_wipe
hm_miss:
  RTS

hit_spider:
  LDA spider_on
  BEQ hsp_miss
  LDA spider_x
  ASL A
  CLC
  ADC #CE_ORIGIN_COL
  STA E_C
  LDA shot_col
  SEC
  SBC E_C
  CMP #2
  BCS hsp_miss
  LDA spider_y
  CLC
  ADC #CE_ORIGIN_ROW
  ASL A
  ASL A
  ASL A
  STA E_R
  LDA shot_y
  SEC
  SBC E_R
  CLC
  ADC #4
  CMP #12
  BCS hsp_miss
  LDA #4
  STA shot_event
  JMP shot_wipe
hsp_miss:
  RTS

// ---------------------------------------------------------------- crawl

// Advance the centipede one cell. Sets crawl_event: 0 ok, 1 player hit.
.global crawl_step
crawl_step:
  LDA #0
  STA crawl_event
  LDA seg_count
  BNE cs_go
  RTS
cs_go:
  // Wipe the tail cell (restore mushroom if any).
  LDX seg_count
  DEX
  LDA seg_x, X
  STA ce_col
  LDA seg_y, X
  STA ce_row
  STX E_SLOT
  JSR mush_draw
  // Shift body toward the head: seg[i] = seg[i-1] for i = n-1 .. 1.
  LDX E_SLOT
  BEQ cs_head
cs_shift:
  TXA
  TAY
  DEX
  LDA seg_x, X
  STA seg_x, Y
  LDA seg_y, X
  STA seg_y, Y
  LDA seg_dir, X
  STA seg_dir, Y
  TXA
  BNE cs_shift
cs_head:
  // Move head.
  LDX #0
  LDA seg_dir, X
  BMI cs_left
  INC seg_x, X
  LDA seg_x, X
  CMP #CE_COLS
  BCS cs_turn
  JSR head_blocked
  BCS cs_undo_right
  JMP cs_draw
cs_undo_right:
  DEC seg_x, X
  JMP cs_turn
cs_left:
  LDA seg_x, X
  BEQ cs_turn
  DEC seg_x, X
  JSR head_blocked
  BCS cs_undo_left
  JMP cs_draw
cs_undo_left:
  INC seg_x, X
cs_turn:
  // Drop a row and reverse.
  LDA seg_dir, X
  EOR #0xfe
  STA seg_dir, X
  INC seg_y, X
  LDA seg_y, X
  CMP #CE_ROWS
  BCC cs_turn_x
  LDA #CE_ROWS-1
  STA seg_y, X
cs_turn_x:
  LDA seg_dir, X
  BMI cs_after_l
  LDA seg_x, X
  CMP #CE_COLS
  BCC cs_draw
  LDA #CE_COLS-1
  STA seg_x, X
  JMP cs_draw
cs_after_l:
  LDA seg_x, X
  CMP #CE_COLS
  BCC cs_draw
  LDA #0
  STA seg_x, X
cs_draw:
  // Player collision: any segment in the player cell band.
  JSR crawl_vs_player
  // Draw head then bodies.
  LDX #0
cs_paint:
  LDA seg_on, X
  BEQ cs_pn
  LDA seg_x, X
  STA ce_col
  LDA seg_y, X
  STA ce_row
  CPX #0
  BNE cs_body
  LDA seg_dir, X
  BMI cs_hl
  LDA #%lo(k_ce_head_r)
  STA ce_src
  LDA #%hi(k_ce_head_r)
  STA ce_src+1
  JMP cs_pd
cs_hl:
  LDA #%lo(k_ce_head_l)
  STA ce_src
  LDA #%hi(k_ce_head_l)
  STA ce_src+1
  JMP cs_pd
cs_body:
  LDA #%lo(k_ce_body)
  STA ce_src
  LDA #%hi(k_ce_body)
  STA ce_src+1
cs_pd:
  STX E_SLOT
  JSR cell_draw
  LDX E_SLOT
cs_pn:
  INX
  CPX seg_count
  BCC cs_paint
  RTS

// Carry set if mush[head] != 0.
head_blocked:
  LDA seg_x
  STA ce_col
  LDA seg_y
  STA ce_row
  JSR index_xy
  LDY #0
  LDA (E_T0), Y
  CMP #1
  RTS

crawl_vs_player:
  // player board col from pixel x.
  LDA player_x
  SEC
  SBC #32
  STA E_T0
  LDA player_x+1
  SBC #0
  LSR A
  LDA E_T0
  ROR A
  LSR A
  LSR A
  STA E_C
  LDX #0
cv_loop:
  LDA seg_on, X
  BEQ cv_next
  LDA seg_y, X
  CMP player_row
  BNE cv_next
  LDA seg_x, X
  SEC
  SBC E_C
  CLC
  ADC #1
  CMP #3
  BCS cv_next
  LDA #1
  STA crawl_event
  RTS
cv_next:
  INX
  CPX seg_count
  BCC cv_loop
  RTS

// ---------------------------------------------------------------- spider

.global spider_step
spider_step:
  LDA spider_on
  BNE sp_go
  RTS
sp_go:
  LDA spider_x
  STA ce_col
  LDA spider_y
  STA ce_row
  JSR mush_draw
  LDA spider_x
  CLC
  ADC spider_vx
  STA spider_x
  LDA spider_y
  CLC
  ADC spider_vy
  STA spider_y
  LDA spider_x
  CMP #CE_COLS
  BCC sp_xok
  LDA spider_vx
  BMI sp_xlo
  LDA #CE_COLS-1
  STA spider_x
  JMP sp_xrev
sp_xlo:
  LDA #0
  STA spider_x
sp_xrev:
  LDA spider_vx
  EOR #0xff
  CLC
  ADC #1
  STA spider_vx
sp_xok:
  LDA spider_y
  CMP #CE_PLAYER_MIN_ROW
  BCC sp_yrev
  CMP #CE_ROWS
  BCC sp_draw
  LDA #CE_ROWS-1
  STA spider_y
sp_yrev:
  LDA spider_vy
  EOR #0xff
  CLC
  ADC #1
  STA spider_vy
  LDA spider_y
  CMP #CE_PLAYER_MIN_ROW
  BCS sp_draw
  LDA #CE_PLAYER_MIN_ROW
  STA spider_y
sp_draw:
  LDA spider_x
  STA ce_col
  LDA spider_y
  STA ce_row
  LDA spider_anim
  EOR #1
  STA spider_anim
  ASL A
  TAX
  LDA spider_spr, X
  STA ce_src
  LDA spider_spr+1, X
  STA ce_src+1
  JSR cell_draw
  // Hit player?
  LDA spider_y
  CMP player_row
  BNE sp_out
  LDA player_x
  SEC
  SBC #32
  STA E_T0
  LDA player_x+1
  SBC #0
  LSR A
  LDA E_T0
  ROR A
  LSR A
  LSR A
  STA E_C
  LDA spider_x
  SEC
  SBC E_C
  CLC
  ADC #1
  CMP #3
  BCS sp_out
  LDA #1
  STA spider_hit
sp_out:
  RTS

.global sound_boom
sound_boom:
  LDX #%lo(snd_boom)
  LDY #%hi(snd_boom)
  LDA #7
  JMP OSWORD

.global sound_death
sound_death:
  LDX #%lo(snd_death)
  LDY #%hi(snd_death)
  LDA #7
  JMP OSWORD
