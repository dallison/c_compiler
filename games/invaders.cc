// Space Invaders for the BBC Master, linked with the sideways libc.
//
// Shadow mode 1 (VDU 22,129): 320x256, four colours, HIMEM at &8000.
// Title and play share that mode — do not call mode() twice (it re-parks the
// software stack and was returning to BASIC after the title keypress).
// Sprites are mode-1 bytes (logical 1 green, 2 red, 3 white after remap).
//
// screen1.s and invaders_fx.s, below &3000, paint the shadow screen and do
// the work of a frame of play: keys, cannon, fleet, laser, bombs, saucer,
// their collisions, firing, dropping bombs, and the score. This file keeps
// the rules (scoring, lives, waves, and timers), the state and tables the
// engine works on, and the rest: setting up a wave, the cannon's death,
// and launching the saucer. A frame is one vsync and everything moves
// every frame, so play holds 50 Hz.
//
// The per-frame code below takes no arguments and keeps its state at file
// scope: davecc spends about 1K cycles a call entering a function that
// loads `this` or arguments, against a 40K-cycle frame.
//
// Z X, A D, or the arrows move. Space or Return fires. Escape returns.

#include <bbc.h>
#include <coroutine>
#include <stddef.h>

#include "invaders.h"

// Bit-precise ints (C23 _BitInt as a C++ extension): no integer promotions.
typedef unsigned _BitInt(8) u8;
typedef _BitInt(8) i8;
typedef unsigned _BitInt(16) u16;
typedef _BitInt(16) i16;

extern "C" void bbc_return(void);
extern "C" void note_acccon(void);
// Coroutine frames never register atexit; keep the link free of heavy cxxabi.
extern "C" int __davecc_cxa_atexit(void*, void*, unsigned long, unsigned long,
                                   int) {
  return 0;
}

extern "C" {
u16 score;
u8 lives;
u8 wave;
u8 phase;
u8 acccon_copy;
}

enum { PhaseTitle = 1, PhasePlay = 2, PhaseDying = 3, PhaseWave = 4, PhaseOver = 5 };
enum { ColBlack = 0, ColGreen = 1, ColRed = 2, ColWhite = 3 };

class Metrics {
 public:
  // Plain unsigned — not _BitInt — so enum constants promote like normal ints.
  // Columns are bytes (4 px) and rows characters (8 px), as in screen1.s.
  // invaders.h has what invaders_fx.s shares.
  enum : unsigned {
    FleetCol = 13,
    FleetRow = 6,
    PlayerX = 152,
    BunkerCol = 8,
    BunkerGap = 18,
    BunkerRow = BUNKER_ROW,
    LineY = 24,
    GroundY = GROUND_Y
  };
};

// Frames at 50 Hz.
class Pace {
 public:
  enum : unsigned {
    Title = 200,
    TitleStep = 25,
    FirstBomb = 60,
    Bomb = 50,
    BombFew = 30,
    BombFast = 10,
    Ufo = 1000,
    Boom = 12,
    Death = 100,
    Wave = 100,
    Over = 100,
    EscapeGuard = 30
  };
};

// screen1.s
extern "C" {
extern u8 fx_col;
extern u8 fx_row;
extern u8 fx_y;
extern u8 fx_len;
extern const unsigned char* fx_src;
void fx_cell();
void fx_wipe();
void fx_blank();
}

class Screen {
 public:
  static void fill(int x, int y, int width, int height, int colour);
};

// invaders_fx.s, and sprites.s for the bitmaps.
extern "C" {
void frame_wait();
void keys_read();
void hud_digits();
void player_step();
void cannon_draw();
void cannon_image();
void cannon_erase();
void fleet_march();
void fleet_bounds();
void shot_step();
void bomb_drop();
void bombs_step();
void ufo_draw();
void ufo_step();
void sound_boom();
void sound_death();
void sound_ufo();
void sound_ufo_off();
void sound_ufo_boom();
extern const unsigned char k_squid0[];
extern const unsigned char k_squid1[];
extern const unsigned char k_crab0[];
extern const unsigned char k_crab1[];
extern const unsigned char k_oct0[];
extern const unsigned char k_oct1[];
extern const unsigned char k_ufo[];
extern const unsigned char k_cannon_red[];
extern const unsigned char k_blast[];
extern const unsigned char k_bomb0[];
extern const unsigned char k_bomb1[];
extern const unsigned char k_bomb2[];
extern const unsigned char k_boom[];
extern const unsigned char k_digits[];
extern const unsigned char k_bunker_top[];
extern const unsigned char k_bunker_base[];
}

enum { KeyLeft = KEY_LEFT, KeyRight = KEY_RIGHT, KeyFire = KEY_FIRE, KeyEscape = KEY_ESCAPE };
enum { ShotFlying = 0, ShotAlien = 1, ShotUfo = 2, ShotSpent = 3 };

// A SOUND block for OSWORD 7. channel + Flush cuts off what the channel is
// playing, so the sound starts at once and OSWORD never waits for room in
// the channel's queue. amplitude is -15 (loudest) to 0, or an envelope.
// pitch is in quarter semitones, 53 being middle C; on channel 0, 4 to 6
// are high, middle and low white noise. duration is in twentieths of a
// second, or -1 for until the channel's next sound.
struct Note {
  u16 channel;
  i16 amplitude;
  u16 pitch;
  i16 duration;
};

// An ENVELOPE block for OSWORD 8. step is a step's length in centiseconds,
// plus NoRepeat to play the pitch sections once rather than over and over.
// Then the pitch change a step in each of three sections and the steps in
// each; the amplitude change a step in attack, decay, sustain and release;
// and the levels (0 to 126) that attack and decay end at.
struct Envelope {
  u8 number;
  u8 step;
  i8 pitch[3];
  u8 steps[3];
  i8 attack;
  i8 decay;
  i8 sustain;
  i8 release;
  u8 attack_to;
  u8 decay_to;
};

// Channel 0 is noise, for the explosions. 1 has the march, 2 the laser and
// 3 the saucer.
enum { Flush = 0x10, NoRepeat = 0x80 };
enum { EnvShot = 1, EnvBoom = 2, EnvDeath = 3, EnvUfo = 4 };

static const Envelope envelopes[4] = {
    // The laser: a fast slide down that dies away.
    {EnvShot, NoRepeat | 1, {-6, 0, 0}, {15, 0, 0}, 126, -6, 0, -126, 110, 0},
    // An alien: a short burst of noise.
    {EnvBoom, NoRepeat | 1, {0, 0, 0}, {0, 0, 0}, 126, -5, 0, -126, 120, 0},
    // The cannon or the saucer: a longer one.
    {EnvDeath, NoRepeat | 2, {0, 0, 0}, {0, 0, 0}, 126, -2, 0, -126, 126, 0},
    // The saucer: up six semitones and back, eight times a second.
    {EnvUfo, 1, {4, -4, 0}, {6, 6, 0}, 126, -1, 0, -126, 90, 80}};

// The engine's state: invaders_fx.s reads and writes it every frame, and
// the rules below set it up. The fleet's layout is in invaders.h.
extern "C" {
// KeyLeft | KeyRight | KeyFire | KeyEscape, held this frame (keys_read).
u8 key_bits;

// fl_on[i] while alien i lives; alive counts them.
u8 alive;
u8 fl_on[FLEET_SIZE];
// The origin, and where it stood before this sweep began.
u8 fl_col;
u8 fl_row;
u8 fl_old_col;
u8 fl_old_row;
// Next alien the sweep redraws. 255 between sweeps, which also makes
// "index < fl_mover" mean "already at the new origin" for every alien.
u8 fl_mover;
// 1 marching right, 255 left. fl_dy is set for a sweep that steps down.
u8 fl_dir;
u8 fl_dy;
// Which of each alien's two images the sweep draws.
u8 fl_frame;
// The outermost live columns and the lowest live rank.
u8 fl_lo;
u8 fl_hi;
u8 fl_bot;
// Set when a sweep would bring the fleet down onto the cannon.
u8 fl_landed;

// Pixel x of the cannon's left edge, and fire seen since the last shot
// went up.
u16 cannon_x;
u8 fire_want;

// The saucer's pixel x, and 1 for right or 255 for left.
u16 ufo_x;
u8 ufo_on;
u8 ufo_dir;

// The laser: pixel x, byte column, and top row of the line.
u8 shot_on;
u16 shot_x;
u8 shot_col;
u8 shot_y;
// What shot_step found (ShotFlying..ShotSpent), and for ShotAlien where
// the alien stood.
u8 shot_event;
u8 shot_hit_col;
u8 shot_hit_row;
u8 shot_hit_rank;

// Falling bombs: byte column, top row, and a count that picks the image.
u8 bomb_on[BOMBS];
u8 bomb_col[BOMBS];
u8 bomb_y[BOMBS];
u8 bomb_anim[BOMBS];
// Set by bombs_step when a bomb meets the cannon.
u8 bomb_hit;
// The fleet column bomb_drop drops from; 11 or more means over the cannon.
u8 drop_col;

// Frames before the march may sound again, and which of its notes is next.
u8 march_gap;
u8 march_step;
// The march's note; march_note in invaders_fx.s sets the pitch.
Note snd_march = {Flush | 1, -12, 0, 2};

// The sounds invaders_fx.s plays.
extern const Note snd_shot = {Flush | 2, EnvShot, 180, 4};
extern const Note snd_boom = {Flush | 0, EnvBoom, 5, 5};
extern const Note snd_death = {Flush | 0, EnvDeath, 6, 30};
extern const Note snd_ufo = {Flush | 3, EnvUfo, 120, -1};
extern const Note snd_ufo_off = {Flush | 3, 0, 0, 1};
extern const Note snd_ufo_boom = {Flush | 0, EnvDeath, 4, 20};
extern const u8 march_pitch[4] = {20, 12, 4, 0};

// Each alien's rank and 5 * column, and the first alien of each rank.
extern const u8 idx_rank[FLEET_SIZE] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  //
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  //
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
extern const u8 idx_col5[FLEET_SIZE] = {
    0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50,  //
    0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50,  //
    0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50,  //
    0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50,  //
    0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50};
extern const u8 rank11[5] = {0, 11, 22, 33, 44};
// rank * 2 + fl_frame -> the alien's 6-column window.
extern const unsigned char* const alien_spr[10] = {
    k_squid0, k_squid1, k_crab0, k_crab1, k_crab0,
    k_crab1,  k_oct0,   k_oct1,  k_oct0,  k_oct1};
extern const unsigned char* const bomb_pat[3] = {k_bomb0, k_bomb1, k_bomb2};
// Internal key numbers and what they set: Z, A and cursor left; X, D and
// cursor right; Space and Return; Escape.
extern const u8 key_codes[KEY_COUNT] = {0x61, 0x41, 0x19, 0x42, 0x32,
                                        0x79, 0x62, 0x49, 0x70};
extern const u8 key_masks[KEY_COUNT] = {KeyLeft,  KeyLeft,  KeyLeft,
                                        KeyRight, KeyRight, KeyRight,
                                        KeyFire,  KeyFire,  KeyEscape};
// The score's places, for hud_digits.
extern const u16 hud_places[4] = {1000, 100, 10, 1};
}

// Play is four dieted coroutines (player / laser / hazards / fleet), each
// one co_await Frame per vsync, resumed in that order every frame. Keep a
// single await per coro — a second co_await in fleet overflowed play_pool
// and smashed ZP (text garbage, quit to BASIC).
struct Frame {
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  void await_resume() const noexcept {}
};

// Fixed pool — operator delete is a no-op so destroy skips free/malloc.
// Must stay small: this sits in high BSS; 128 overlapped MOS temps / score
// and quit to BASIC with garbled text.
static unsigned char play_pool[96];
static unsigned play_used;

struct Play {
  struct promise_type {
    Play get_return_object() {
      return Play{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    // Required with nothrow class new — without it a null frame was written
    // through and smashed 6502 zero page (__sp).
    static Play get_return_object_on_allocation_failure() noexcept {
      return Play{};
    }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
    static void* operator new(size_t n) noexcept {
      if (play_used + static_cast<unsigned>(n) > sizeof(play_pool)) {
        return 0;
      }
      void* p = play_pool + play_used;
      play_used += static_cast<unsigned>(n);
      return p;
    }
    static void operator delete(void*) noexcept {}
    static void operator delete(void*, size_t) noexcept {}
  };

  std::coroutine_handle<promise_type> h{};
  Play() = default;
  explicit Play(std::coroutine_handle<promise_type> handle) : h(handle) {}
  Play(Play&& other) noexcept : h(other.h) { other.h = {}; }
  Play& operator=(Play&& other) noexcept {
    h = other.h;
    other.h = {};
    return *this;
  }
  Play(const Play&) = delete;
  Play& operator=(const Play&) = delete;
};

class Text {
 public:
  enum : unsigned { Cols = 40 };
  static void write(const char* text);
  static void centre(u8 row, u8 colour, const char* line);
};

// SCORE on the left, WAVE in the middle and LIVES on the right of the top
// row; hud_digits draws the numbers under them.
class Hud {
 public:
  static void labels();
  static void banner(const char* line, u8 colour, u8 row);
};

// The splash: the name, a big invader, the score table, the keys and the
// credit, each line centred. It stays up for Pace::Title frames, or until
// a fresh press of fire, and the invaders step as they do in the march.
class Title {
 public:
  static u8 show();

 private:
  // The big invader is a sprite window's 16 x 8 pixels, each Scale pixels
  // square, centred across the screen from pixel row BigY. The score
  // table's sprites sit at text column TableCol from text row TableRow.
  enum : unsigned {
    Scale = 8,
    BigX = (320 - 16 * Scale) / 2,
    BigY = 40,
    TableCol = 12,
    TableRow = 15
  };
  static void invader(const unsigned char* image, const unsigned char* last);
  static void table(u8 look);
};

// Setting the fleet up for a wave; invaders_fx.s marches it.
class Fleet {
 public:
  static void reset();
  static void draw();
};

// The laser and the bombs, at the start of a wave or a life.
class Volley {
 public:
  static void reset();
  static void clear();
};

namespace Game {
void run();
u8 tick();
void beat_player();
void beat_laser();
void beat_hazards();
void beat_fleet();
}  // namespace Game

static u16 rng_state;
static u16 ufo_wait;
static u8 landed;
static u8 hud_dirty;
static u8 frames_played;
static u8 bomb_t;
static u8 boom_t;
static u8 boom_col;
static u8 boom_row;
static u8 death_t;
static u8 wave_t;
static u8 over_t;

static Play player_job;
static Play laser_job;
static Play hazard_job;
static Play fleet_job;

Play player_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_player();
  }
}

Play laser_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_laser();
  }
}

Play hazard_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_hazards();
  }
}

Play fleet_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_fleet();
  }
}

// Set once per session when all four frames fit in play_pool, so the
// per-frame resumes skip coroutine_handle::operator bool (1K cycles each).
static u8 jobs_ok;

void Text::write(const char* text) {
  while (*text != 0) {
    bbc::oswrch(static_cast<unsigned char>(*text));
    text++;
  }
}

void Text::centre(u8 row, u8 colour, const char* line) {
  u8 len = 0;
  while (line[len] != 0) {
    len++;
  }
  bbc::colour(colour);
  bbc::tab(static_cast<u8>(static_cast<u8>(Cols - len) >> 1), row);
  write(line);
}

static u16 rng_next() {
  rng_state = static_cast<unsigned>(rng_state + (rng_state << 5) + 13849);
  return rng_state;
}

void Hud::labels() {
  bbc::colour(128);
  bbc::colour(ColGreen);
  bbc::tab(2, 0);
  Text::write("SCORE");
  bbc::tab(18, 0);
  Text::write("WAVE");
  bbc::tab(33, 0);
  Text::write("LIVES");
}

void Hud::banner(const char* line, u8 colour, u8 row) {
  bbc::colour(128);
  Text::centre(row, colour, line);
}

// fl_row is the starting character row; everything else starts over.
void Fleet::reset() {
  u8 i;
  for (i = 0; i < FLEET_SIZE; i++) {
    fl_on[i] = 1;
  }
  alive = FLEET_SIZE;
  fl_col = Metrics::FleetCol;
  fl_old_col = Metrics::FleetCol;
  fl_old_row = fl_row;
  fl_dir = 1;
  fl_frame = 0;
  fl_dy = 0;
  fl_landed = 0;
  fl_lo = 0;
  fl_hi = FLEET_COLS - 1;
  fl_bot = 4;
  fl_mover = 255;
  march_gap = 0;
  march_step = 0;
}

// Every live alien where it stands: a sweep that goes nowhere, since
// fleet_march only moves the origin between sweeps.
void Fleet::draw() {
  fl_dy = 0;
  fl_mover = 0;
  while (alive != 0 && fl_mover < FLEET_SIZE) {
    fleet_march();
  }
}

// Laser and bombs off, and no shot queued; clear also wipes them.
void Volley::reset() {
  u8 b;
  fire_want = 0;
  shot_on = 0;
  for (b = 0; b < BOMBS; b++) {
    bomb_on[b] = 0;
  }
}

void Volley::clear() {
  u8 b;
  if (shot_on != 0) {
    shot_on = 0;
    fx_col = shot_col;
    fx_y = shot_y;
    fx_len = SHOT_LEN;
    fx_blank();
  }
  for (b = 0; b < BOMBS; b++) {
    if (bomb_on[b] != 0) {
      bomb_on[b] = 0;
      fx_col = bomb_col[b];
      fx_y = bomb_y[b];
      fx_len = BOMB_LEN;
      fx_blank();
    }
  }
}

// A row of the score table: its sprite's two images, and what it scores.
struct ScoreLine {
  const unsigned char* look[2];
  const char* points;
};

static const ScoreLine score_table[4] = {{{k_ufo, k_ufo}, "= 100 POINTS"},
                                         {{k_squid0, k_squid1}, "= 30 POINTS"},
                                         {{k_crab0, k_crab1}, "= 20 POINTS"},
                                         {{k_oct0, k_oct1}, "= 10 POINTS"}};

// image's sprite, Scale times the size, painting only the pixels that
// differ from last's; with no last, the lit ones on a clear screen. Window
// byte 8 + i is scanline i & 7 of byte column 1 + (i >> 3). A byte is four
// pixels, left first: bits 7 and 3 are the leftmost one's colour, and a
// shift left brings up the next.
void Title::invader(const unsigned char* image, const unsigned char* last) {
  u8 i;
  u8 p;
  for (i = 0; i < 32; i++) {
    u8 now = image[8 + i];
    u8 was = last != 0 ? last[8 + i] : 0;
    if (now == was) {
      continue;
    }
    for (p = 0; p < 4; p++) {
      u8 ink = static_cast<u8>(((now & 0x80) != 0 ? 2 : 0) | ((now & 0x08) != 0 ? 1 : 0));
      u8 old = static_cast<u8>(((was & 0x80) != 0 ? 2 : 0) | ((was & 0x08) != 0 ? 1 : 0));
      if (ink != old) {
        Screen::fill(BigX + ((i >> 3) * 4 + p) * Scale, BigY + (i & 7) * Scale, Scale, Scale,
                     ink);
      }
      now = static_cast<u8>(now << 1);
      was = static_cast<u8>(was << 1);
    }
  }
}

void Title::table(u8 look) {
  u8 i;
  for (i = 0; i < 4; i++) {
    fx_col = TableCol * 2;
    fx_row = static_cast<u8>(TableRow + i * 2);
    fx_src = score_table[i].look[look];
    fx_len = 48;
    fx_cell();
  }
}

u8 Title::show() {
  u8 frames;
  u8 i;
  u8 look = 0;
  u8 step_t = Pace::TitleStep;
  u8 armed = 0;
  phase = PhaseTitle;
  bbc::cursor(0);
  bbc::cls();
  Text::centre(2, ColWhite, "DAVECC INVADERS");
  invader(k_crab0, 0);
  table(look);
  bbc::colour(ColWhite);
  for (i = 0; i < 4; i++) {
    bbc::tab(TableCol + 4, static_cast<u8>(TableRow + i * 2));
    Text::write(score_table[i].points);
  }
  Text::centre(24, ColGreen, "Z X OR ARROWS MOVE");
  Text::centre(25, ColGreen, "RETURN/SPACE FIRES, ESCAPE QUITS");
  Text::centre(28, ColRed, "WRITTEN IN C++20 WITH DAVECC");
  Text::centre(29, ColRed, "BY DAVE ALLISON");
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  for (frames = 0; frames < Pace::Title; frames++) {
    frame_wait();
    keys_read();
    // The Return that ran the game may still be down, so only a press
    // after fire has been up skips the rest.
    if ((key_bits & KeyFire) == 0) {
      armed = 1;
    } else if (armed) {
      break;
    }
    step_t--;
    if (step_t == 0) {
      step_t = Pace::TitleStep;
      look ^= 1;
      invader(look ? k_crab1 : k_crab0, look ? k_crab0 : k_crab1);
      table(look);
    }
  }
  return 1;
}

namespace Game {

static void add_score(u16 points) {
  score = static_cast<unsigned>(score + points);
  hud_dirty = 1;
}

static void present() {
  hud_digits();
  hud_dirty = 0;
}

static void show_cannon(const unsigned char* image) {
  fx_src = image;
  cannon_image();
}

static void draw_bunkers() {
  u8 i;
  u8 col = Metrics::BunkerCol;
  for (i = 0; i < 4; i++) {
    fx_col = col;
    fx_row = Metrics::BunkerRow;
    fx_src = k_bunker_top;
    fx_len = 48;
    fx_cell();
    fx_col = col;
    fx_row = Metrics::BunkerRow + 1;
    fx_src = k_bunker_base;
    fx_len = 48;
    fx_cell();
    col += Metrics::BunkerGap;
  }
}

static void deploy() {
  u8 drop = static_cast<u8>(wave - 1);
  if (drop > 5) {
    drop = 5;
  }
  phase = PhasePlay;
  ufo_on = 0;
  Volley::reset();
  boom_t = 0;
  fl_row = static_cast<u8>(Metrics::FleetRow + drop);
  Fleet::reset();
  bomb_t = Pace::FirstBomb;
  ufo_wait = Pace::Ufo;
  bbc::cls();
  bbc::cursor(0);
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  Screen::fill(0, Metrics::LineY, 320, 2, ColGreen);
  Screen::fill(0, Metrics::GroundY, 320, 2, ColGreen);
  draw_bunkers();
  Fleet::draw();
  cannon_draw();
  Hud::labels();
  present();
}

static void clear_boom() {
  if (boom_t == 0) {
    return;
  }
  boom_t = 0;
  fx_col = boom_col;
  fx_row = boom_row;
  fx_len = 32;
  fx_wipe();
}

static void kill_player() {
  if (phase != PhasePlay) {
    return;
  }
  Volley::clear();
  clear_boom();
  phase = PhaseDying;
  death_t = Pace::Death;
  if (lives > 0) {
    lives--;
  }
  hud_dirty = 1;
  show_cannon(k_blast);
  sound_ufo_off();
  sound_death();
}

static void dying() {
  if ((death_t & 7) == 0) {
    show_cannon((death_t & 8) != 0 ? k_cannon_red : k_blast);
  }
  if (death_t > 0) {
    death_t--;
  }
  if (death_t != 0) {
    return;
  }
  cannon_erase();
  if (lives == 0) {
    phase = PhaseOver;
    over_t = Pace::Over;
    Hud::banner(landed ? " INVADED " : "GAME OVER", ColRed, 12);
    Text::centre(15, ColWhite, "PRESS SPACE");
    return;
  }
  cannon_x = Metrics::PlayerX;
  Volley::reset();
  phase = PhasePlay;
  cannon_draw();
  if (ufo_on) {
    sound_ufo();
  }
}

static void alien_hit() {
  static const u8 points[5] = {30, 20, 20, 10, 10};
  add_score(points[shot_hit_rank]);
  clear_boom();
  boom_col = shot_hit_col;
  boom_row = shot_hit_row;
  boom_t = Pace::Boom;
  fx_col = boom_col;
  fx_row = boom_row;
  fx_src = k_boom;
  fx_len = 32;
  fx_cell();
  sound_boom();
  fleet_bounds();
  if (alive == 0) {
    phase = PhaseWave;
    wave_t = Pace::Wave;
    sound_ufo_off();
  }
}

static void launch_bomb() {
  bomb_t = Pace::Bomb;
  if (alive < 16) {
    bomb_t = Pace::BombFew;
  }
  if (wave > 2) {
    bomb_t -= Pace::BombFast;
  }
  // 11..15 drop over the cannon: about one bomb in three is aimed.
  drop_col = static_cast<u8>((rng_next() >> 8) & 15);
  bomb_drop();
}

// Brings the saucer in at the side it leaves from.
static void launch_ufo() {
  ufo_wait = Pace::Ufo;
  if ((rng_next() & 0x8000) != 0) {
    ufo_dir = 255;
    ufo_x = UFO_MAX;
  } else {
    ufo_dir = 1;
    ufo_x = UFO_MIN;
  }
  ufo_on = 1;
  ufo_draw();
  sound_ufo();
}

void beat_player() {
  if (phase == PhasePlay) {
    player_step();
  }
}

void beat_laser() {
  if (phase != PhasePlay) {
    return;
  }
  shot_step();
  if (shot_event == ShotAlien) {
    alien_hit();
  } else if (shot_event == ShotUfo) {
    add_score(100);
    sound_ufo_boom();
  }
}

void beat_hazards() {
  if (phase != PhasePlay) {
    return;
  }
  bombs_step();
  if (bomb_hit) {
    kill_player();
    return;
  }
  ufo_step();
  if (boom_t == 1) {
    clear_boom();
  } else if (boom_t > 1) {
    boom_t--;
  }
}

void beat_fleet() {
  if (phase != PhasePlay) {
    return;
  }
  fleet_march();
  if (fl_landed) {
    landed = 1;
    lives = 0;
    kill_player();
    return;
  }
  if (bomb_t > 0) {
    bomb_t--;
  } else {
    launch_bomb();
  }
  if (!ufo_on) {
    if (ufo_wait > 0) {
      ufo_wait--;
    } else {
      launch_ufo();
    }
  }
}

static void play_frame() {
  if (jobs_ok) {
    player_job.h.resume();
    laser_job.h.resume();
    hazard_job.h.resume();
    fleet_job.h.resume();
    return;
  }
  beat_player();
  beat_laser();
  beat_hazards();
  beat_fleet();
}

static void quit() {
  sound_ufo_off();
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  bbc::cls();
  bbc::cursor(1);
  bbc_return();
}

u8 tick() {
  if (frames_played < 255) {
    frames_played++;
  }
  frame_wait();
  keys_read();
  if (frames_played > Pace::EscapeGuard && (key_bits & KeyEscape) != 0) {
    quit();
  }
  if (phase == PhasePlay) {
    play_frame();
  } else if (phase == PhaseDying) {
    dying();
  } else if (phase == PhaseWave) {
    if (wave_t == Pace::Wave) {
      Hud::banner("GET READY", ColWhite, 14);
    }
    if (wave_t > 0) {
      wave_t--;
    }
    if (wave_t == 0) {
      wave++;
      deploy();
    }
  } else if (phase == PhaseOver) {
    if (over_t > 0) {
      over_t--;
    } else if ((key_bits & KeyFire) != 0) {
      return 1;
    }
  }
  if (hud_dirty) {
    present();
  }
  return 0;
}

static void session() {
  score = 0;
  lives = 3;
  wave = 1;
  landed = 0;
  frames_played = 0;
  cannon_x = Metrics::PlayerX;
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  deploy();
  play_used = 0;
  player_job = player_coro();
  laser_job = laser_coro();
  hazard_job = hazard_coro();
  fleet_job = fleet_coro();
  jobs_ok = player_job.h && laser_job.h && hazard_job.h && fleet_job.h;
  while (tick() == 0) {
  }
  jobs_ok = 0;
  player_job = Play{};
  laser_job = Play{};
  hazard_job = Play{};
  fleet_job = Play{};
}

void run() {
  u8 i;
  bbc::palette(0, ColBlack);
  bbc::palette(1, 2);
  bbc::palette(2, 1);
  bbc::palette(3, 7);
  bbc::colour(128);
  bbc::cursor(0);
  rng_state = 1;
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  for (i = 0; i < 4; i++) {
    bbc::osword(bbc::osword_command::envelope, const_cast<Envelope*>(&envelopes[i]));
  }
  for (;;) {
    session();
  }
}

}  // namespace Game

int main() {
  // Mode 1 first: leaves Mode 7 screen RAM so the soft stack at &7C00 is safe.
  bbc::mode(129);
  bbc::osbyte(bbc::osbyte_command::escape_status, 1, 0);
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  note_acccon();
  // Palette before title — Mode 1 only has logical 0..3 (colour 6 was junk).
  bbc::palette(0, ColBlack);
  bbc::palette(1, 2);
  bbc::palette(2, 1);
  bbc::palette(3, 7);
  bbc::colour(128);
  Title::show();
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  Game::run();
  bbc_return();
  return 0;
}
