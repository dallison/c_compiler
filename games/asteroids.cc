// Asteroids for the BBC Master, linked with the sideways libc.
//
// Shadow mode 1 (VDU 22,129): 320x256, four colours, HIMEM at &8000.
// Title and play share that mode — do not call mode() twice.
//
// screen1.s and asteroids_fx.s paint the shadow screen and scan keys.
// This file keeps the rules and three dieted coroutines (ship / rocks /
// shots), each with one co_await Frame per vsync.
//
// Z/X or left/right rotate. SHIFT thrusts. Space or Return fires.
// Escape returns to BASIC.
//
// Build: .cursor/skills/build-invaders/scripts/build_and_pack.sh

#include <bbc.h>
#include <coroutine>
#include <stddef.h>

#include "asteroids.h"

typedef unsigned _BitInt(8) u8;
typedef _BitInt(8) i8;
typedef unsigned _BitInt(16) u16;
typedef _BitInt(16) i16;

extern "C" void bbc_return(void);
extern "C" void note_acccon(void);
extern "C" int __davecc_cxa_atexit(void*, void*, unsigned long, unsigned long,
                                   int) {
  return 0;
}

extern "C" {
u16 score;
u16 best;
u8 lives;
u8 wave;
u8 phase;
u8 acccon_copy;
}

enum {
  PhaseTitle = 1,
  PhasePlay = 2,
  PhaseDying = 3,
  PhaseWave = 4,
  PhaseOver = 5
};
enum { ColBlack = 0, ColGreen = 1, ColRed = 2, ColWhite = 3 };

class Pace {
 public:
  enum : unsigned {
    Title = 220,
    TitleSpin = 6,
    Death = 70,
    Wave = 90,
    Over = 80,
    EscapeGuard = 30,
    FireGap = 1,
    Invuln = 80,
    Anim = 40,
    ShotLife = 28,
    ShotSpeed = 10,
    ThrustMax = 4,
    TurnGap = 2,
    ThrustGap = 3,
    FrictionGap = 20
  };
};

// Playfield fits a 16-row soft blit without running past pixel 255
// (strip_addr only has 32 character rows; past that shreds memory).
// x in [0, FieldW), y in [FieldY0, FieldY1).
enum { FieldW = 304, FieldY0 = 16, FieldY1 = 240 };

class Screen {
 public:
  static void fill(int x, int y, int width, int height, int colour);
};

extern "C" {
extern u8 fx_col;
extern u8 fx_row;
extern u8 fx_len;
extern const unsigned char* fx_src;
void fx_cell();
void fx_wipe();

void frame_wait();
void keys_read();
void hud_digits();
void cell_draw();
void cell_wipe();
void cell_draw_large();
void cell_wipe_large();
void shot_draw();
void shot_wipe();
void soft_draw_large();
void soft_wipe_large();
void soft_draw_small();
void soft_wipe_small();
void soft_wipe_ship();
void ship_blit();
void ship_paint();
void rocks_step();
extern u8 as_hit;
void shots_step();
extern u8 as_shot_dead;
extern u8 as_shot_victim[];

extern const unsigned char k_as_ship0[];
extern const unsigned char k_as_ship1[];
extern const unsigned char k_as_ship2[];
extern const unsigned char k_as_ship3[];
extern const unsigned char k_as_ship4[];
extern const unsigned char k_as_ship5[];
extern const unsigned char k_as_ship6[];
extern const unsigned char k_as_ship7[];
extern const unsigned char k_as_ship_t0[];
extern const unsigned char k_as_ship_t1[];
extern const unsigned char k_as_ship_t2[];
extern const unsigned char k_as_ship_t3[];
extern const unsigned char k_as_ship_t4[];
extern const unsigned char k_as_ship_t5[];
extern const unsigned char k_as_ship_t6[];
extern const unsigned char k_as_ship_t7[];
extern const unsigned char k_as_rock_l0[];
extern const unsigned char k_as_rock_l1[];
extern const unsigned char k_as_rock_m0[];
extern const unsigned char k_as_rock_m1[];
extern const unsigned char k_as_rock_s0[];
extern const unsigned char k_as_rock_s1[];
extern const unsigned char k_as_shot0[];
extern const unsigned char k_as_shot1[];
extern const unsigned char k_as_shot2[];
extern const unsigned char k_as_shot3[];
extern const unsigned char k_as_boom0[];
extern const unsigned char k_as_boom1[];
extern const unsigned char k_digits[];
}

enum {
  KeyLeft = KEY_LEFT,
  KeyRight = KEY_RIGHT,
  KeyThrust = KEY_THRUST,
  KeyFire = KEY_FIRE,
  KeyEscape = KEY_ESCAPE
};

struct Note {
  u16 channel;
  i16 amplitude;
  u16 pitch;
  i16 duration;
};

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

enum { Flush = 0x10, NoRepeat = 0x80 };
enum { EnvFire = 1, EnvBoom = 2, EnvThrust = 3, EnvDeath = 4 };

static const Envelope envelopes[4] = {
    {EnvFire, NoRepeat | 1, {-4, 0, 0}, {8, 0, 0}, 126, -10, 0, -126, 100, 0},
    {EnvBoom, NoRepeat | 1, {0, 0, 0}, {0, 0, 0}, 126, -5, 0, -126, 120, 0},
    {EnvThrust, NoRepeat | 1, {2, 0, 0}, {3, 0, 0}, 80, -20, 0, -126, 60, 0},
    {EnvDeath, NoRepeat | 2, {0, 0, 0}, {0, 0, 0}, 126, -2, 0, -126, 126, 0}};

extern "C" {
u8 key_bits;

u8 as_col;
u8 as_row;
u8 as_y;
const unsigned char* as_src;

u8 ship_on;
u8 ship_shown;
u16 ship_x;
u16 ship_y;
u8 ship_px;
u8 ship_py;
u8 ship_dir_shown;
u8 ship_thrust_shown;
i8 ship_vx;
i8 ship_vy;
u8 ship_dir;
u8 ship_fire_t;
u8 fire_held;
u8 ship_invuln;
u8 ship_turn_t;
u8 ship_thrust_t;
u8 ship_fric_t;
u8 ship_thrusting;

u8 rock_on[AS_ROCKS];
u16 rock_x[AS_ROCKS];
u16 rock_y[AS_ROCKS];
u8 rock_col[AS_ROCKS];
u8 rock_sy[AS_ROCKS];
u8 rock_phase[AS_ROCKS];
u8 rock_shown[AS_ROCKS];
i8 rock_vx[AS_ROCKS];
i8 rock_vy[AS_ROCKS];
u8 rock_size[AS_ROCKS];
u8 rock_look[AS_ROCKS];

u8 shot_on[AS_SHOTS];
u16 shot_x[AS_SHOTS];
u16 shot_y[AS_SHOTS];
u8 shot_col[AS_SHOTS];
u8 shot_sy[AS_SHOTS];
u8 shot_phase[AS_SHOTS];
u8 shot_shown[AS_SHOTS];
i8 shot_vx[AS_SHOTS];
i8 shot_vy[AS_SHOTS];
u8 shot_life[AS_SHOTS];

u8 rock_alive;
u8 anim_t;
u8 frames_played;
u8 death_t;
u8 wave_t;
u8 over_t;
u8 hud_dirty;
extern const u16 hud_places[4] = {1000, 100, 10, 1};

// Z/A/left, X/D/right, SHIFT (both share matrix &00), Space/Return, Escape.
extern const u8 key_codes[KEY_COUNT] = {0x61, 0x41, 0x19, 0x42, 0x32, 0x79,
                                        0x00, 0x62, 0x49, 0x70};
extern const u8 key_masks[KEY_COUNT] = {
    KeyLeft, KeyLeft, KeyLeft, KeyRight, KeyRight, KeyRight,
    KeyThrust, KeyFire, KeyFire, KeyEscape};
}

struct Frame {
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  void await_resume() const noexcept {}
};

static unsigned char play_pool[96];
static unsigned play_used;

struct Play {
  struct promise_type {
    Play get_return_object() {
      return Play{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
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

class Text {
 public:
  enum : unsigned { Cols = 40 };
  static void write(const char* text);
  static void centre(u8 row, u8 colour, const char* line);
};

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
void beat_ship();
void beat_rocks();
void beat_shots();
}  // namespace Game

static u16 rng_state;
static Play ship_job;
static Play rock_job;
static Play shot_job;
static u8 jobs_ok;

Play ship_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_ship();
  }
}

Play rock_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_rocks();
  }
}

Play shot_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_shots();
  }
}

void Text::write(const char* text) {
  while (*text != 0) {
    bbc::oswrch((unsigned char)*text);
    text++;
  }
}

void Text::centre(u8 row, u8 colour, const char* line) {
  u8 len = 0;
  while (line[len] != 0) {
    len++;
  }
  bbc::colour(colour);
  bbc::tab((u8)((u8)(Cols - len) >> 1), row);
  write(line);
}

static u16 rng_next() {
  rng_state = (unsigned)(rng_state + (rng_state << 5) + 13849);
  return rng_state;
}

void Hud::labels() {
  bbc::colour(128);
  bbc::colour(ColGreen);
  bbc::tab(2, 0);
  Text::write("SCORE");
  bbc::tab(16, 0);
  Text::write("BEST");
  bbc::tab(33, 0);
  Text::write("LIVES");
}

void Hud::banner(const char* line, u8 colour, u8 row) {
  bbc::colour(128);
  Text::centre(row, colour, line);
}

static void sound(const Note* note) {
  bbc::osword(bbc::osword_command::sound, const_cast<Note*>(note));
}

static void sound_fire() {
  static const Note n = {Flush | 2, EnvFire, 200, 2};
  sound(&n);
}

static void sound_boom() {
  static const Note n = {Flush | 0, EnvBoom, 5, 6};
  sound(&n);
}

static void sound_thrust() {
  static const Note n = {Flush | 1, EnvThrust, 40, 1};
  sound(&n);
}

static void sound_death() {
  static const Note n = {Flush | 0, EnvDeath, 6, 25};
  sound(&n);
}

static u16 wrap_x(u16 x) {
  while (x >= FieldW) {
    x = (u16)(x - FieldW);
  }
  return x;
}

static u16 wrap_y(u16 y) {
  // Keep the HUD clear and the 16x16 blit on the last character rows.
  while (y < FieldY0) {
    y = (u16)(y + (FieldY1 - FieldY0));
  }
  while (y >= FieldY1) {
    y = (u16)(y - (FieldY1 - FieldY0));
  }
  return y;
}

// Signed step then wrap. (u16)(pos + i8) is fine on davecc now, but the
// old unsigned wrap treated a leftover x>=320 as "still on screen" and let
// large sprites paint off the right/bottom — that hung the machine.
static u16 advance_x(u16 x, i8 v) {
  i16 n = (i16)((i16)x + (i16)v);
  while (n < 0) {
    n = (i16)(n + FieldW);
  }
  while (n >= FieldW) {
    n = (i16)(n - FieldW);
  }
  return (u16)n;
}

static u16 advance_y(u16 y, i8 v) {
  i16 n = (i16)((i16)y + (i16)v);
  i16 h = (i16)(FieldY1 - FieldY0);
  while (n < FieldY0) {
    n = (i16)(n + h);
  }
  while (n >= FieldY1) {
    n = (i16)(n - h);
  }
  return (u16)n;
}

static u8 pix_col(u16 x) { return (u8)(x >> 2); }

static u8 pix_row(u16 y) { return (u8)(y >> 3); }

// Tables beat long if-ladders for size under HIMEM (thrust frames cost
// 512 bytes of art; every code byte counts).
static const unsigned char* const k_ship_img[16] = {
    k_as_ship0,   k_as_ship1,   k_as_ship2,   k_as_ship3,
    k_as_ship4,   k_as_ship5,   k_as_ship6,   k_as_ship7,
    k_as_ship_t0, k_as_ship_t1, k_as_ship_t2, k_as_ship_t3,
    k_as_ship_t4, k_as_ship_t5, k_as_ship_t6, k_as_ship_t7};

static const unsigned char* ship_image(u8 dir, u8 thrusting) {
  return k_ship_img[(u8)(dir + (thrusting ? 8 : 0))];
}

static const i8 k_thrust_dx[8] = {0, 1, 1, 1, 0, (i8)-1, (i8)-1, (i8)-1};
static const i8 k_thrust_dy[8] = {(i8)-1, (i8)-1, 0, 1, 1, 1, 0, (i8)-1};

static i8 thrust_dx(u8 dir) { return k_thrust_dx[dir]; }
static i8 thrust_dy(u8 dir) { return k_thrust_dy[dir]; }

static void paint_cell(u8 col, u8 row, const unsigned char* image) {
  as_col = col;
  as_row = row;
  as_src = image;
  cell_draw();
}

static void wipe_cell(u8 col, u8 row) {
  as_col = col;
  as_row = row;
  cell_wipe();
}

// Title-screen ship display at cell (18, 10): the blit draw writes
// every byte of the window, so spinning to the next heading needs no
// separate wipe.
static void paint_title_ship(const unsigned char* image) {
  as_col = 36;
  as_y = 80;
  as_src = image;
  ship_blit();
}

static void ship_erase() {
  if (ship_shown) {
    as_col = ship_px;
    as_y = ship_py;
    soft_wipe_ship();
    ship_shown = 0;
  }
}

static void ship_present() {
  u8 ncol;
  u8 sy;
  if (!ship_on) {
    ship_erase();
    return;
  }
  ncol = pix_col(ship_x);
  sy = (u8)ship_y;
  // Repaint every tick: a rock (or shot) wipe can clobber part of a
  // stationary ship's window — during the post-respawn invulnerable
  // seconds rocks pass straight through it — and with an unchanged-
  // ship skip the damage would never be repaired. The blit makes the
  // repaint cheap, and wipe+draw land in the same displayed frame.
  // ship_blit reads ship_px/ship_py/ship_shown, so update them after.
  as_col = ncol;
  as_y = sy;
  as_src = ship_image(ship_dir, ship_thrusting);
  ship_blit();
  ship_px = ncol;
  ship_py = sy;
  ship_dir_shown = ship_dir;
  ship_thrust_shown = ship_thrusting;
  ship_shown = 1;
}

static void rock_erase(u8 i) {
  if (!rock_shown[i]) {
    return;
  }
  as_col = rock_col[i];
  as_y = rock_sy[i];
  if (rock_size[i] == ROCK_LARGE) {
    soft_wipe_large();
  } else {
    soft_wipe_small();
  }
  rock_shown[i] = 0;
}

// Rocks are painted by beat_rocks (folded into its loop — a chain of
// per-rock C++ calls here cost more than the paint itself). A spawn just
// marks the rock unshown and the next beat paints it.
static void rock_spawn_draw(u8 i) { rock_shown[i] = 0; }

static const unsigned char* shot_image(u8 phase) {
  if (phase == 0) {
    return k_as_shot0;
  }
  if (phase == 1) {
    return k_as_shot1;
  }
  if (phase == 2) {
    return k_as_shot2;
  }
  return k_as_shot3;
}

static void paint_shot(u8 col, u8 y, u8 phase) {
  as_col = col;
  as_y = y;
  as_src = shot_image(phase);
  shot_draw();
}

static void wipe_shot(u8 col, u8 y) {
  as_col = col;
  as_y = y;
  shot_wipe();
}

static void shot_erase(u8 i) {
  if (shot_shown[i]) {
    wipe_shot(shot_col[i], shot_sy[i]);
    shot_shown[i] = 0;
  }
}

static const unsigned char* const k_shot_img[4] = {k_as_shot0, k_as_shot1,
                                                   k_as_shot2, k_as_shot3};

static void shot_present(u8 i) {
  u8 ncol;
  u8 ny;
  u8 phase;
  if (!shot_on[i]) {
    shot_erase(i);
    return;
  }
  // Inline the paint chain: this runs per shot per tick, and the
  // pix_col/wipe_shot/paint_shot/shot_image calls cost more than the
  // strip paint itself.
  ncol = (u8)(shot_x[i] >> 2);
  ny = (u8)shot_y[i];
  phase = (u8)(shot_x[i] & 3);
  if (shot_shown[i] &&
      (ncol != shot_col[i] || ny != shot_sy[i] || phase != shot_phase[i])) {
    as_col = shot_col[i];
    as_y = shot_sy[i];
    shot_wipe();
    shot_shown[i] = 0;
  }
  shot_col[i] = ncol;
  shot_sy[i] = ny;
  shot_phase[i] = phase;
  as_col = ncol;
  as_y = ny;
  as_src = k_shot_img[phase];
  shot_draw();
  shot_shown[i] = 1;
}

static void shot_spawn_draw(u8 i) {
  shot_shown[i] = 0;
  shot_present(i);
}

// davecc's signed _BitInt compares against small literals were compiling
// `v < 0` / `v > max` wrong inside clamp — every negative speed became
// +ThrustMax, so thrust felt random. Stay in unsigned byte space.
static i8 clamp_vel(i8 v) {
  u8 u = (u8)v;
  if (u <= (u8)Pace::ThrustMax) {
    return v;
  }
  if (u >= (u8)(256 - Pace::ThrustMax)) {
    return v;
  }
  if (u < 128) {
    return (i8)Pace::ThrustMax;
  }
  return (i8)(-Pace::ThrustMax);
}

static void friction(i8* v) {
  u8 u = (u8)(*v);
  if (u == 0) {
    return;
  }
  if (u < 128) {
    (*v) = (i8)(u - 1);
  } else {
    (*v) = (i8)(u + 1);
  }
}

static u8 free_rock() {
  u8 i;
  for (i = 0; i < AS_ROCKS; i++) {
    if (!rock_on[i]) {
      return i;
    }
  }
  return 255;
}

static void spawn_rock(u8 size, u16 x, u16 y) {
  u8 i = free_rock();
  i8 speed;
  if (i == 255) {
    return;
  }
  rock_on[i] = 1;
  rock_shown[i] = 0;
  rock_size[i] = size;
  rock_x[i] = wrap_x(x);
  rock_y[i] = wrap_y(y);
  rock_look[i] = (u8)(rng_next() & 1);
  speed = (i8)(1 + (rng_next() & 1));
  if (size == ROCK_SMALL) {
    speed = (i8)(speed + 1);
  }
  if ((rng_next() & 1) != 0) {
    speed = (i8)(-speed);
  }
  rock_vx[i] = speed;
  speed = (i8)(1 + (rng_next() & 1));
  if ((rng_next() & 1) != 0) {
    speed = (i8)(-speed);
  }
  rock_vy[i] = speed;
  // Keep rocks from racing — large ones drift, small ones a little faster.
  if (size == ROCK_LARGE) {
    if (rock_vx[i] > 1) {
      rock_vx[i] = 1;
    }
    if (rock_vx[i] < -1) {
      rock_vx[i] = -1;
    }
    if (rock_vy[i] > 1) {
      rock_vy[i] = 1;
    }
    if (rock_vy[i] < -1) {
      rock_vy[i] = -1;
    }
  }
  rock_alive++;
  rock_spawn_draw(i);
}

static void clear_shots() {
  u8 i;
  for (i = 0; i < AS_SHOTS; i++) {
    if (shot_on[i]) {
      shot_erase(i);
      shot_on[i] = 0;
    }
  }
}

static void clear_rocks() {
  u8 i;
  for (i = 0; i < AS_ROCKS; i++) {
    if (rock_on[i]) {
      rock_erase(i);
      rock_on[i] = 0;
    }
  }
  rock_alive = 0;
}

static void split_rock(u8 i) {
  u8 size = rock_size[i];
  u16 x = rock_x[i];
  u16 y = rock_y[i];
  rock_erase(i);
  rock_on[i] = 0;
  if (rock_alive > 0) {
    rock_alive--;
  }
  sound_boom();
  if (size == ROCK_LARGE) {
    score = (unsigned)(score + 20);
    spawn_rock(ROCK_MED, x + 4, y);
    spawn_rock(ROCK_MED, x, y + 4);
  } else if (size == ROCK_MED) {
    score = (unsigned)(score + 50);
    spawn_rock(ROCK_SMALL, x + 2, y);
    spawn_rock(ROCK_SMALL, x, y + 2);
  } else {
    score = (unsigned)(score + 100);
  }
  if (score > best) {
    best = score;
  }
  hud_dirty = 1;
}

static void fire_shot() {
  u8 i;
  for (i = 0; i < AS_SHOTS; i++) {
    if (!shot_on[i]) {
      shot_on[i] = 1;
      // Spawn from the nose of the 16x16 hull.
      shot_x[i] = advance_x(ship_x, (i8)(8 + thrust_dx(ship_dir) * 6));
      shot_y[i] = advance_y(ship_y, (i8)(8 + thrust_dy(ship_dir) * 6));
      shot_vx[i] =
          (i8)(ship_vx + thrust_dx(ship_dir) * (i8)Pace::ShotSpeed);
      shot_vy[i] =
          (i8)(ship_vy + thrust_dy(ship_dir) * (i8)Pace::ShotSpeed);
      if (shot_vx[i] == 0 && shot_vy[i] == 0) {
        shot_vy[i] = (i8)(-Pace::ShotSpeed);
      }
      shot_life[i] = Pace::ShotLife;
      shot_spawn_draw(i);
      sound_fire();
      ship_fire_t = Pace::FireGap;
      return;
    }
  }
}

static void place_wave() {
  u8 n = (u8)(3 + wave);
  u8 i;
  if (n > 6) {
    n = 6;
  }
  clear_rocks();
  for (i = 0; i < n; i++) {
    u16 x = (u16)(20 + (rng_next() & 255));
    u16 y = (u16)(24 + (rng_next() & 127));
    if (x > 280) {
      x = 280;
    }
    // Keep clear of the ship spawn.
    if (x > 140 && x < 180 && y > 100 && y < 140) {
      x = 40;
      y = 40;
    }
    spawn_rock(ROCK_LARGE, x, y);
  }
}

static void ship_reset() {
  ship_on = 1;
  ship_shown = 0;
  ship_x = 156;
  ship_y = 120;
  ship_vx = 0;
  ship_vy = 0;
  ship_dir = 0;
  ship_fire_t = 0;
  fire_held = 0;
  ship_turn_t = 0;
  ship_thrust_t = 0;
  ship_fric_t = 0;
  ship_thrusting = 0;
  ship_invuln = Pace::Invuln;
  ship_present();
}

static void present() {
  hud_digits();
  hud_dirty = 0;
}

static void deploy() {
  phase = PhasePlay;
  anim_t = Pace::Anim;
  clear_shots();
  bbc::cls();
  bbc::cursor(0);
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  Hud::labels();
  place_wave();
  ship_reset();
  present();
}

u8 Title::show() {
  u8 frames;
  u8 armed = 0;
  u8 spin = Pace::TitleSpin;
  u8 dir = 0;
  phase = PhaseTitle;
  bbc::cursor(0);
  bbc::cls();
  Text::centre(3, ColWhite, "DAVECC ASTEROIDS");
  as_col = 35;
  as_y = 64;
  as_src = k_as_rock_l0;
  soft_draw_large();
  paint_title_ship(k_as_ship0);
  paint_shot(28, 88, 0);
  as_col = 49;
  as_y = 100;
  as_src = k_as_rock_m0;
  soft_draw_small();
  as_col = 55;
  as_y = 116;
  as_src = k_as_rock_s0;
  soft_draw_small();
  Text::centre(18, ColGreen, "Z X ROTATE, SHIFT THRUSTS, SPACE FIRES");
  Text::centre(20, ColGreen, "BREAK THE ROCKS, CLEAR THE WAVE");
  Text::centre(24, ColWhite, "SPACE STARTS, ESCAPE QUITS");
  Text::centre(28, ColRed, "WRITTEN IN C++20 WITH DAVECC");
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  for (frames = 0; frames < Pace::Title; frames++) {
    frame_wait();
    keys_read();
    if ((key_bits & KeyFire) == 0) {
      armed = 1;
    } else if (armed) {
      break;
    }
    if ((key_bits & KeyEscape) != 0) {
      return 0;
    }
    spin--;
    if (spin == 0) {
      spin = Pace::TitleSpin;
      dir = (u8)((dir + 1) & 7);
      paint_title_ship(ship_image(dir, 0));
    }
  }
  return 1;
}

namespace Game {

static void kill_player() {
  u8 i;
  if (phase != PhasePlay || ship_invuln != 0) {
    return;
  }
  phase = PhaseDying;
  death_t = Pace::Death;
  if (lives > 0) {
    lives--;
  }
  hud_dirty = 1;
  ship_erase();
  ship_on = 0;
  clear_shots();
  paint_cell(pix_col(ship_x), pix_row(ship_y), k_as_boom0);
  sound_death();
  (void)i;
}

static void dying() {
  if ((death_t & 8) == 0) {
    paint_cell(pix_col(ship_x), pix_row(ship_y), k_as_boom1);
  } else {
    paint_cell(pix_col(ship_x), pix_row(ship_y), k_as_boom0);
  }
  if (death_t > 0) {
    death_t--;
  }
  if (death_t != 0) {
    return;
  }
  wipe_cell(pix_col(ship_x), pix_row(ship_y));
  if (lives == 0) {
    phase = PhaseOver;
    over_t = Pace::Over;
    Hud::banner("GAME OVER", ColRed, 12);
    Text::centre(15, ColWhite, "PRESS SPACE");
    return;
  }
  ship_reset();
  phase = PhasePlay;
}

void beat_ship() {
  if (phase != PhasePlay || !ship_on) {
    return;
  }
  if (ship_turn_t > 0) {
    ship_turn_t--;
  }
  if (ship_turn_t == 0) {
    if (key_bits & KeyLeft) {
      ship_dir = (u8)((ship_dir + 7) & 7);
      ship_turn_t = Pace::TurnGap;
    } else if (key_bits & KeyRight) {
      ship_dir = (u8)((ship_dir + 1) & 7);
      ship_turn_t = Pace::TurnGap;
    }
  }
  ship_thrusting = 0;
  if (key_bits & KeyThrust) {
    ship_thrusting = 1;
    if (ship_thrust_t > 0) {
      ship_thrust_t--;
    } else {
      ship_vx = clamp_vel((i8)(ship_vx + thrust_dx(ship_dir)));
      ship_vy = clamp_vel((i8)(ship_vy + thrust_dy(ship_dir)));
      sound_thrust();
      ship_thrust_t = Pace::ThrustGap;
    }
    ship_fric_t = Pace::FrictionGap;
  } else if (ship_fric_t > 0) {
    ship_fric_t--;
  } else {
    friction(&ship_vx);
    friction(&ship_vy);
    ship_fric_t = Pace::FrictionGap;
  }
  // Tap = one shot (even mid-gap). Hold = autofire when ship_fire_t hits 0.
  {
    u8 down = (u8)(key_bits & KeyFire);
    if (ship_fire_t > 0) {
      ship_fire_t--;
    }
    if (down) {
      if (!fire_held || ship_fire_t == 0) {
        fire_shot();
      }
      fire_held = 1;
    } else {
      fire_held = 0;
    }
  }
  // Move in byte arithmetic; advance_x/advance_y are 16-bit helper
  // calls that cost more than the move when called every tick.
  if ((u8)ship_vx != 0) {
    u8 uv = (u8)ship_vx;
    if (uv < 128) {
      ship_x = (u16)(ship_x + uv);
      if (ship_x >= FieldW) {
        ship_x = (u16)(ship_x - FieldW);
      }
    } else if (ship_x >= (u8)(0 - uv)) {
      ship_x = (u16)(ship_x - (u8)(0 - uv));
    } else {
      ship_x = (u16)(ship_x + (FieldW - (u8)(0 - uv)));
    }
  }
  if ((u8)ship_vy != 0) {
    u8 uv = (u8)ship_vy;
    if (uv < 128) {
      ship_y = (u16)(ship_y + uv);
      if (ship_y >= FieldY1) {
        ship_y = (u16)(ship_y - (FieldY1 - FieldY0));
      }
    } else if (ship_y >= (u8)(FieldY0 + (u8)(0 - uv))) {
      ship_y = (u16)(ship_y - (u8)(0 - uv));
    } else {
      ship_y = (u16)(ship_y + ((FieldY1 - FieldY0) - (u8)(0 - uv)));
    }
  }
  if (ship_invuln > 0) {
    ship_invuln--;
  }
  // The paint decision (including the every-other-tick repair
  // repaint of a stationary ship) is ship_paint in asteroids_fx.s;
  // the C++ version of this check cost ~8K cycles a call.
  ship_paint();
}

void beat_rocks() {
  if (phase != PhasePlay) {
    return;
  }
  if (anim_t > 0) {
    anim_t--;
  } else {
    anim_t = Pace::Anim;
  }
  // The step itself is rocks_step in asteroids_fx.s: doing it in C++
  // cost several vsyncs a tick in call and register-save overhead.
  rocks_step();
  if (as_hit != 0) {
    kill_player();
    return;
  }
  if (rock_alive == 0) {
    phase = PhaseWave;
    wave_t = Pace::Wave;
    Hud::banner("WAVE CLEAR", ColWhite, 14);
  }
}

void beat_shots() {
  u8 i;
  u16 claimed = 0;
  if (phase != PhasePlay) {
    return;
  }
  // Movement, wrapping, the shot/rock box test and the repaint of
  // surviving shots are shots_step in asteroids_fx.s; this loop only
  // does the bookkeeping for shots that died (erase, and split the
  // victim rock).
  shots_step();
  for (i = 0; i < AS_SHOTS; i++) {
    if (as_shot_dead & (u8)(1 << i)) {
      u8 v = as_shot_victim[i];
      shot_erase(i);
      // Two shots can claim the same rock in one step; split it once.
      // (split_rock may also reuse the slot for a child rock, which a
      // second claimant must not split again.)
      if (v != 0xFF && !(claimed & (u16)((u16)1 << v)) && rock_on[v]) {
        claimed = (u16)(claimed | (u16)((u16)1 << v));
        split_rock(v);
      }
    }
  }
}

static void play_frame() {
  // Rocks and shots first; ship last so it isn't blanked by their paints.
  if (jobs_ok) {
    rock_job.h.resume();
    shot_job.h.resume();
    ship_job.h.resume();
    return;
  }
  beat_rocks();
  beat_shots();
  beat_ship();
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
  frames_played = 0;
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  deploy();
  play_used = 0;
  ship_job = ship_coro();
  rock_job = rock_coro();
  shot_job = shot_coro();
  jobs_ok = ship_job.h && rock_job.h && shot_job.h;
  while (tick() == 0) {
  }
  jobs_ok = 0;
  ship_job = Play{};
  rock_job = Play{};
  shot_job = Play{};
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
    bbc::osword(bbc::osword_command::envelope,
                const_cast<Envelope*>(&envelopes[i]));
  }
  for (;;) {
    session();
  }
}

}  // namespace Game

int main() {
  bbc::mode(129);
  bbc::osbyte(bbc::osbyte_command::escape_status, 1, 0);
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  note_acccon();
  bbc::palette(0, ColBlack);
  bbc::palette(1, 2);
  bbc::palette(2, 1);
  bbc::palette(3, 7);
  bbc::colour(128);
  if (!Title::show()) {
    bbc_return();
    return 0;
  }
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  Game::run();
  bbc_return();
  return 0;
}
