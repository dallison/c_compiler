// Space Invaders for the BBC Master, linked with the sideways libc.
//
// Shadow mode 1 (VDU 22,129): 320x256, four colours, HIMEM at &8000.
// Title and play share that mode — do not call mode() twice (it re-parks the
// software stack and was returning to BASIC after the title keypress).
// Sprites are mode-1 bytes (logical 1 green, 2 red, 3 white after remap).
//
// screen1.s and invaders_fx.s, below &3000, paint the shadow screen and do
// the per-frame work: keys, cannon, fleet, laser, bombs, saucer, and their
// collisions. This file keeps the rules: scoring, lives, waves, and timers.
// A frame is one vsync and everything moves every frame, so play holds
// 50 Hz.
//
// The per-frame code below takes no arguments and keeps its state at file
// scope: davecc spends about 1K cycles a call entering a function that
// loads `this` or arguments, against a 40K-cycle frame.
//
// Z X, A D, or the arrows move. Space or Return fires. Escape returns.

#include <bbc.h>
#include <coroutine>
#include <stddef.h>

// Bit-precise ints (C23 _BitInt as a C++ extension): no integer promotions.
// Play is four dieted coroutines (player / laser / hazards / fleet), each
// one co_await Frame per vsync, resumed in that order every frame. Keep a
// single await per coro — a second co_await in fleet overflowed play_pool
// and smashed ZP (text garbage, quit to BASIC).
#ifndef INVADERS_USE_CORO
#define INVADERS_USE_CORO 1
#endif
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
  // invaders_fx.s has the cannon and saucer limits.
  enum : unsigned {
    FleetRow = 6,
    PlayerX = 152,
    BunkerCol = 8,
    BunkerGap = 18,
    BunkerRow = 22,
    LineY = 24,
    GroundY = 232
  };
};

// Frames at 50 Hz.
class Pace {
 public:
  enum : unsigned {
    Title = 60,
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
extern u8 fx_len;
extern const unsigned char* fx_src;
void fx_cell();
void fx_wipe();
}

class Screen {
 public:
  static void fill(int x, int y, int width, int height, int colour);
};

// invaders_fx.s, and sprites.s for the bitmaps drawn from here.
extern "C" {
extern u8 key_bits;
extern u8 alive;
extern u8 fl_row;
extern u8 fl_landed;
extern u16 cannon_x;
extern u8 ufo_on;
extern u8 ufo_dir;
extern u8 shot_event;
extern u8 shot_hit_col;
extern u8 shot_hit_row;
extern u8 shot_hit_rank;
extern u8 bomb_hit;
extern u8 drop_col;
void frame_wait();
void keys_read();
void hud_digits();
void player_step();
void fleet_reset();
void fleet_draw();
void fleet_march();
void fleet_bounds();
void shot_step();
void bomb_drop();
void bombs_step();
void volley_reset();
void volley_clear();
void cannon_draw();
void cannon_draw_red();
void cannon_draw_blast();
void cannon_erase();
void ufo_launch();
void ufo_step();
extern const unsigned char k_boom[];
extern const unsigned char k_bunker_top[];
extern const unsigned char k_bunker_base[];
}

enum { KeyLeft = 1, KeyRight = 2, KeyFire = 4, KeyEscape = 8 };
enum { ShotFlying = 0, ShotAlien = 1, ShotUfo = 2, ShotSpent = 3 };

#if INVADERS_USE_CORO
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
      if (play_used + (unsigned)n > sizeof(play_pool)) {
        return 0;
      }
      void* p = play_pool + play_used;
      play_used += (unsigned)n;
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
#endif

class Text {
 public:
  static void write(const char* text);
};

// The top row reads "SC 0000 L3 W1"; hud_digits fills in the numbers.
class Hud {
 public:
  static void labels();
  static void banner(const char* line, u8 colour, u8 row);
};

class Title {
 public:
  static u8 show();
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

#if INVADERS_USE_CORO
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
#endif

void Text::write(const char* text) {
  while (*text != 0) {
    bbc::oswrch((unsigned char)*text);
    text++;
  }
}

static u16 rng_next() {
  rng_state = (unsigned)(rng_state + (rng_state << 5) + 13849);
  return rng_state;
}

void Hud::labels() {
  bbc::colour(128);
  bbc::colour(ColGreen);
  bbc::tab(0, 0);
  Text::write("SC");
  bbc::tab(8, 0);
  Text::write("L");
  bbc::tab(11, 0);
  Text::write("W");
}

void Hud::banner(const char* line, u8 colour, u8 row) {
  bbc::colour(128);
  bbc::tab(12, row);
  bbc::colour(colour);
  Text::write(line);
}

u8 Title::show() {
  u8 frames;
  phase = PhaseTitle;
  bbc::cursor(0);
  bbc::cls();
  bbc::tab(1, 8);
  bbc::colour(3);
  Text::write("SPACE INVADERS");
  bbc::tab(2, 14);
  bbc::colour(1);
  Text::write("Z X OR ARROWS");
  bbc::tab(3, 16);
  bbc::colour(2);
  Text::write("SPACE FIRES");
  bbc::tab(2, 20);
  bbc::colour(3);
  Text::write("ESCAPE QUITS");
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  // Timed splash only — no key gate (waiting on Space was aborting to BASIC).
  for (frames = 0; frames < Pace::Title; frames++) {
    frame_wait();
  }
  return 1;
}

namespace Game {

static void add_score(u16 points) {
  score = (unsigned)(score + points);
  hud_dirty = 1;
}

static void present() {
  hud_digits();
  hud_dirty = 0;
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
  u8 drop = (u8)(wave - 1);
  if (drop > 5) {
    drop = 5;
  }
  phase = PhasePlay;
  ufo_on = 0;
  volley_reset();
  boom_t = 0;
  fl_row = (u8)(Metrics::FleetRow + drop);
  fleet_reset();
  bomb_t = Pace::FirstBomb;
  ufo_wait = Pace::Ufo;
  bbc::cls();
  bbc::cursor(0);
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  Screen::fill(0, Metrics::LineY, 320, 2, ColGreen);
  Screen::fill(0, Metrics::GroundY, 320, 2, ColGreen);
  draw_bunkers();
  fleet_draw();
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
  volley_clear();
  clear_boom();
  phase = PhaseDying;
  death_t = Pace::Death;
  if (lives > 0) {
    lives--;
  }
  hud_dirty = 1;
  cannon_draw_blast();
}

static void dying() {
  if ((death_t & 7) == 0) {
    if ((death_t & 8) != 0) {
      cannon_draw_red();
    } else {
      cannon_draw_blast();
    }
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
    bbc::tab(13, 15);
    bbc::colour(ColWhite);
    Text::write("PRESS SPACE");
    return;
  }
  cannon_x = Metrics::PlayerX;
  volley_reset();
  phase = PhasePlay;
  cannon_draw();
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
  fleet_bounds();
  if (alive == 0) {
    phase = PhaseWave;
    wave_t = Pace::Wave;
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
  drop_col = (u8)((rng_next() >> 8) & 15);
  bomb_drop();
}

static void launch_ufo() {
  ufo_wait = Pace::Ufo;
  ufo_dir = (rng_next() & 0x8000) != 0 ? 255 : 1;
  ufo_launch();
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
#if INVADERS_USE_CORO
  if (jobs_ok) {
    player_job.h.resume();
    laser_job.h.resume();
    hazard_job.h.resume();
    fleet_job.h.resume();
    return;
  }
#endif
  beat_player();
  beat_laser();
  beat_hazards();
  beat_fleet();
}

static void quit() {
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
#if INVADERS_USE_CORO
  play_used = 0;
  player_job = player_coro();
  laser_job = laser_coro();
  hazard_job = hazard_coro();
  fleet_job = fleet_coro();
  jobs_ok = player_job.h && laser_job.h && hazard_job.h && fleet_job.h;
#endif
  while (tick() == 0) {
  }
#if INVADERS_USE_CORO
  jobs_ok = 0;
  player_job = Play{};
  laser_job = Play{};
  hazard_job = Play{};
  fleet_job = Play{};
#endif
}

void run() {
  bbc::palette(0, ColBlack);
  bbc::palette(1, 2);
  bbc::palette(2, 1);
  bbc::palette(3, 7);
  bbc::colour(128);
  bbc::cursor(0);
  rng_state = 1;
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
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
