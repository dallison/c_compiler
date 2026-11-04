// Space Invaders for the BBC Master, linked with the sideways libc.
//
// Shadow mode 1 (VDU 22,129): 320x256, four colours, HIMEM at &8000.
// Title and play share that mode — do not call mode() twice (it re-parks the
// software stack and was returning to BASIC after the title keypress).
// Sprites are mode-1 bytes (logical 1 green, 2 red, 3 white after remap).
// screen1.s, below &3000, copies one batch onto the shadow screen.
//
// Z X, A D, or the arrows move. Space or Return fires. Escape returns.

#include <bbc.h>
#include <coroutine>
#include <stddef.h>

// Bit-precise ints (C23 _BitInt as a C++ extension): no integer promotions.
// Play is three dieted coroutines (player / hazards / fleet), one co_await
// Frame per vsync each. Keep a single await per coro — a second co_await in
// fleet overflowed play_pool and smashed ZP (text garbage, quit to BASIC).
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
u8 alive;
u8 acccon_copy;
}

enum { PhaseTitle = 1, PhasePlay = 2, PhaseDying = 3, PhaseWave = 4, PhaseOver = 5 };
enum { ColBlack = 0, ColGreen = 1, ColRed = 2, ColWhite = 3 };

class Metrics {
 public:
  // Plain unsigned — not _BitInt — so enum constants promote like normal ints.
  enum : unsigned {
    Columns = 11,
    Ranks = 5,
    AlienW = 16,
    AlienH = 8,
    OriginX = 52,
    OriginY = 48,
    PlayerX = 152,
    PlayerY = 208,
    BunkerY = 176,
    GroundY = 232,
    LineY = 24,
    UfoY = 32,
    Shot = 8
  };
};

// Bitmaps live in sprites.s (.rodata.sprites), linked below &3000.
extern "C" {
extern const unsigned char k_squid0[];
extern const unsigned char k_squid1[];
extern const unsigned char k_crab0[];
extern const unsigned char k_crab1[];
extern const unsigned char k_oct0[];
extern const unsigned char k_oct1[];
extern const unsigned char k_cannon[];
extern const unsigned char k_cannon_red[];
extern const unsigned char k_ufo[];
extern const unsigned char k_boom[];
extern const unsigned char k_shot[];
extern const unsigned char k_bomb0[];
extern const unsigned char k_bomb1[];
extern const unsigned char k_bomb2[];
extern const unsigned char k_bunker[];
extern const unsigned char k_bit[];
}

// close/fill/stamp live in screen1.s (mangled Screen::* names).
class Screen {
 public:
  static void close();
  static void fill(int x, int y, int width, int height, int colour);
  static void stamp(int x, int y, const unsigned char* bits, int width, int height);
};

#if INVADERS_USE_CORO
struct Frame {
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  void await_resume() const noexcept {}
};

// Fixed pool — operator delete is a no-op so destroy skips free/malloc.
// Must stay small: this sits in high BSS with Game; 128 overlapped MOS
// temps / score and quit to BASIC with garbled text.
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

class Keys {
 public:
  // Plain C types — same as snake. _BitInt args/returns made OSBYTE &79
  // scans flaky (move/fire only worked occasionally).
  static int axis();
  static int fire();
  static int escape();

 private:
  static int down(unsigned char code);
};

class Text {
 public:
  static void write(const char* text);
  static void quad(u16 value);
};

class Rng {
 public:
  void seed();
  u16 next();

 private:
  u16 state_;
};

class Shelters;

class Fleet {
 public:
  void reset(u8 origin_y);
  void erase();
  void draw();
  // 0 idle, 1 worked this call, 2 landed. Advances a few aliens per call.
  u8 step(Shelters* shelters);
  u8 busy() const { return mover_ >= 0; }
  u16 shoot(u16 x, u8 y);
  u8 drop_at(u8 col, u16* x, u8* y) const;
  u16 column_x(u8 col) const;
  u8 column_alive(u8 col) const;
  u8 pace(u8 wave_number) const;
  u16 last_x_;
  u8 last_y_;

 private:
  void render(u8 draw_it);
  void tighten();
  u8 rank_alive(u8 rank) const;
  u8 rank_y(u8 rank) const;
  u8 cell(u8 rank, u8 col) const;
  u8 points(u8 rank) const;
  const unsigned char* picture(u8 rank) const;
  u16 origin_x_;
  u8 origin_y_;
  u16 old_origin_x_;
  u8 old_origin_y_;
  i8 march_dx_;
  i8 march_dy_;
  i8 dir_;
  i8 lo_;
  i8 hi_;
  i8 bot_;
  i8 mover_;
  u8 frame_;
  u8 on_[55];
};

class Shelters {
 public:
  void reset();
  void draw();
  u8 strike(u16 x, u8 y);

 private:
  u16 left(u8 index) const;
  u8 solid_[4];
};

class Saucer {
 public:
  void reset();
  void erase();
  void draw();
  void launch(u8 from_left);
  u8 step();
  u8 contains(u16 x, u8 y) const;
  u16 x_;
  u8 on_;

 private:
  i8 dir_;
  u8 tick_;
};

class Cannon {
 public:
  void reset();
  void erase();
  void draw();
  void move(int dir);
  u16 x_;
};

class Volley {
 public:
  void reset();
  void erase();
  u8 fire(u16 x);
  void drop(u16 x, u8 y);
  u16 hop_shot(Fleet* fleet, Shelters* shelters, Saucer* saucer, u8 hops);
  u8 move_bombs(Shelters* shelters, u16 px);
  u8 shot_live() const { return shot_on_; }

 private:
  const unsigned char* bomb_bits(u8 frame) const;
  u16 shot_x_;
  u8 shot_y_;
  u8 shot_on_;
  u16 bomb_x_[3];
  u8 bomb_y_[3];
  u8 bomb_on_[3];
  u8 bomb_frame_[3];
};

class Hud {
 public:
  static void draw();
  static void banner(const char* line, u8 colour, u8 row);
};

class Title {
 public:
  static u8 show();
};

class Game {
 public:
  void run();
#if INVADERS_USE_CORO
  void beat_player();
  void beat_hazards();
  void beat_fleet();
#endif

 private:
  u8 session();
  u8 tick();
#if !INVADERS_USE_CORO
  void play_slice();
#endif
  void deploy();
  void present();
  void add_score(u16 points);
  void kill_player();
  void score_hit(u16 scored);
  void march();
  void launch_bomb();
  void launch_ufo();

  Rng rng_;
  Fleet fleet_;
  Shelters shelters_;
  Saucer saucer_;
  Cannon cannon_;
  Volley volley_;
  u16 ufo_wait_;
  u16 boom_x_;
  u8 boom_y_;
  u8 landed_;
  u8 hud_dirty_;
  u8 clock_;
  u8 step_t_;
  u8 bomb_t_;
  u8 boom_t_;
  u8 death_t_;
  u8 wave_t_;
  u8 over_t_;
  u8 boom_on_;
  u8 fire_want_;
};

static Game game;

#if INVADERS_USE_CORO
static Play player_job;
static Play hazard_job;
static Play fleet_job;

Play player_coro(Game* g) {
  for (;;) {
    co_await Frame{};
    g->beat_player();
  }
}

Play hazard_coro(Game* g) {
  for (;;) {
    co_await Frame{};
    g->beat_hazards();
  }
}

Play fleet_coro(Game* g) {
  for (;;) {
    co_await Frame{};
    g->beat_fleet();
  }
}

static void resume_job(Play* job) {
  if (job->h) {
    job->h.resume();
  }
}
#endif

// OSBYTE &79 keyboard scan — same scheme as snake (internal code EOR &80).
// Call the asm osbyte(unsigned char,...) overload directly — the enum
// wrapper adds an extra frame around every scan.
int Keys::down(unsigned char code) {
  bbc::os_regs regs =
      bbc::osbyte((unsigned char)0x79, (unsigned char)(code ^ 0x80), 0);
  return (regs.x & 0x80) != 0;
}

int Keys::axis() {
  if (down(0x61) || down(0x19) || down(0x41)) {
    return -1;
  }
  if (down(0x42) || down(0x79) || down(0x32)) {
    return 1;
  }
  return 0;
}

int Keys::fire() { return down(0x62) || down(0x49); }

int Keys::escape() { return down(0x70); }




void Text::write(const char* text) {
  while (*text != 0) {
    bbc::oswrch((unsigned char)*text);
    text++;
  }
}

void Text::quad(u16 value) {
  static const u16 place_value[4] = {1000, 100, 10, 1};
  u8 i;
  if (value > 9999) {
    value = 9999;
  }
  for (i = 0; i < 4; i++) {
    u16 place = place_value[i];
    u8 digit = '0';
    while (value >= place) {
      value -= place;
      digit++;
    }
    bbc::oswrch(digit);
  }
}

void Rng::seed() { state_ = 1; }

u16 Rng::next() {
  state_ = (unsigned)(state_ + (state_ << 5) + 13849);
  return state_;
}

u8 Fleet::cell(u8 rank, u8 col) const { return (rank << 3) + (rank << 1) + rank + col; }

u16 Fleet::column_x(u8 col) const {
  // Widen before shift: u8<<4 saturates in _BitInt(8) and corrupts X.
  return origin_x_ + ((u16)col << 4) + ((u16)col << 2);
}

u8 Fleet::rank_y(u8 rank) const { return origin_y_ + (u8)((u16)rank << 4); }

const unsigned char* Fleet::picture(u8 rank) const {
  if (rank == 0) {
    if (frame_) {
      return k_squid1;
    }
    return k_squid0;
  }
  if (rank < 3) {
    if (frame_) {
      return k_crab1;
    }
    return k_crab0;
  }
  if (frame_) {
    return k_oct1;
  }
  return k_oct0;
}

u8 Fleet::points(u8 rank) const {
  if (rank == 0) {
    return 30;
  }
  if (rank < 3) {
    return 20;
  }
  return 10;
}

u8 Fleet::column_alive(u8 col) const {
  u8 rank;
  for (rank = 0; rank < Metrics::Ranks; rank++) {
    if (on_[cell(rank, col)]) {
      return 1;
    }
  }
  return 0;
}

u8 Fleet::rank_alive(u8 rank) const {
  u8 col;
  for (col = 0; col < Metrics::Columns; col++) {
    if (on_[cell(rank, col)]) {
      return 1;
    }
  }
  return 0;
}

void Fleet::tighten() {
  while (lo_ < hi_ && !column_alive(lo_)) {
    lo_++;
  }
  while (hi_ > lo_ && !column_alive(hi_)) {
    hi_--;
  }
  while (bot_ > 0 && !rank_alive(bot_)) {
    bot_--;
  }
}

void Fleet::reset(u8 origin_y) {
  u8 i;
  origin_x_ = Metrics::OriginX;
  origin_y_ = origin_y;
  old_origin_x_ = origin_x_;
  old_origin_y_ = origin_y_;
  march_dx_ = 0;
  march_dy_ = 0;
  dir_ = 1;
  frame_ = 0;
  lo_ = 0;
  hi_ = Metrics::Columns - 1;
  bot_ = Metrics::Ranks - 1;
  mover_ = -1;
  last_x_ = 0;
  last_y_ = 0;
  for (i = 0; i < 55; i++) {
    on_[i] = 1;
  }
  alive = 55;
}

void Fleet::render(u8 draw_it) {
  u8 rank;
  u8 col;
  for (rank = 0; rank < Metrics::Ranks; rank++) {
    for (col = 0; col < Metrics::Columns; col++) {
      u16 x;
      u8 y;
      if (!on_[cell(rank, col)]) {
        continue;
      }
      x = column_x(col);
      y = rank_y(rank);
      if (draw_it) {
        Screen::stamp(x, y, picture(rank), Metrics::AlienW, Metrics::AlienH);
      } else {
        Screen::fill(x, y, Metrics::AlienW, Metrics::AlienH, ColBlack);
      }
    }
  }
}

void Fleet::erase() { render(0); }

void Fleet::draw() { render(1); }

u8 Fleet::step(Shelters* shelters) {
  u8 moved;
  u16 edge;
  i8 col;
  u8 rank;
  if (alive == 0) {
    mover_ = -1;
    return 0;
  }
  if (mover_ < 0) {
    march_dx_ = 0;
    march_dy_ = 0;
    if (dir_ > 0) {
      edge = column_x((u8)hi_) + Metrics::AlienW;
      if (edge + 4 > 312) {
        march_dy_ = 8;
        dir_ = -1;
      } else {
        march_dx_ = 4;
      }
    } else {
      edge = column_x((u8)lo_);
      if (edge < 12) {
        march_dy_ = 8;
        dir_ = 1;
      } else {
        march_dx_ = -4;
      }
    }
    if (rank_y((u8)bot_) + march_dy_ + Metrics::AlienH >= Metrics::PlayerY) {
      return 2;
    }
    old_origin_x_ = origin_x_;
    old_origin_y_ = origin_y_;
    origin_x_ += march_dx_;
    origin_y_ += march_dy_;
    frame_ ^= 1;
    mover_ = 0;
  }
  moved = 0;
  // Cap aliens per vsync so play_slice still samples keys every frame.
  // A full 55-stamp hop starved input (cannon/fire felt intermittent).
  while (mover_ < 55 && moved < 8) {
    if (on_[mover_]) {
      u16 ox;
      u8 oy;
      u16 nx;
      u8 ny;
      u8 c;
      rank = 0;
      c = (u8)mover_;
      while (c >= Metrics::Columns) {
        c = (u8)(c - Metrics::Columns);
        rank++;
      }
      ox = old_origin_x_ + ((u16)c << 4) + ((u16)c << 2);
      oy = old_origin_y_ + (u8)((u16)rank << 4);
      nx = origin_x_ + ((u16)c << 4) + ((u16)c << 2);
      ny = origin_y_ + (u8)((u16)rank << 4);
      Screen::stamp(nx, ny, picture(rank), Metrics::AlienW, Metrics::AlienH);
      if (march_dx_ > 0) {
        Screen::fill(ox, oy, march_dx_, Metrics::AlienH, ColBlack);
      } else if (march_dx_ < 0) {
        Screen::fill(nx + Metrics::AlienW, ny, -march_dx_, Metrics::AlienH, ColBlack);
      } else if (march_dy_ > 0) {
        Screen::fill(ox, oy, Metrics::AlienW, march_dy_, ColBlack);
      }
      moved++;
    }
    mover_++;
  }
  if (mover_ >= 55) {
    mover_ = -1;
    for (col = lo_; col <= hi_; col++) {
      if (on_[cell((u8)bot_, (u8)col)]) {
        shelters->strike(column_x((u8)col) + 8, rank_y((u8)bot_) + 4);
      }
    }
  }
  return 1;
}

u16 Fleet::shoot(u16 x, u8 y) {
  i8 rank;
  i8 col;
  for (rank = bot_; rank >= 0; rank--) {
    for (col = lo_; col <= hi_; col++) {
      u16 ax;
      u8 ay;
      u16 ox;
      u8 oy;
      u8 index = cell((u8)rank, (u8)col);
      if (!on_[index]) {
        continue;
      }
      ox = origin_x_;
      oy = origin_y_;
      if (mover_ >= 0 && index >= (u8)mover_) {
        ox = old_origin_x_;
        oy = old_origin_y_;
      }
      ax = ox + ((u16)(u8)col << 4) + ((u16)(u8)col << 2);
      ay = oy + (u8)((u16)(u8)rank << 4);
      if (x >= ax + Metrics::AlienW || ax >= x + Metrics::Shot) {
        continue;
      }
      if (y >= ay + Metrics::AlienH || ay >= y + Metrics::Shot) {
        continue;
      }
      on_[index] = 0;
      if (alive > 0) {
        alive--;
      }
      last_x_ = ax;
      last_y_ = ay;
      Screen::fill(ax, ay, Metrics::AlienW, Metrics::AlienH, ColBlack);
      if (col == lo_ || col == hi_ || rank == bot_) {
        tighten();
      }
      return points((u8)rank);
    }
  }
  return 0;
}

u8 Fleet::drop_at(u8 col, u16* x, u8* y) const {
  i8 rank;
  if (col >= Metrics::Columns) {
    return 0;
  }
  for (rank = Metrics::Ranks - 1; rank >= 0; rank--) {
    if (!on_[cell((u8)rank, col)]) {
      continue;
    }
    *x = column_x(col) + 4;
    *y = rank_y((u8)rank) + Metrics::AlienH;
    return 1;
  }
  return 0;
}

u8 Fleet::pace(u8 wave_number) const {
  (void)wave_number;
  (void)alive;
  return 0;
}

u16 Shelters::left(u8 index) const { return 32 + (index << 6) + (index << 3); }

void Shelters::reset() {
  u8 i;
  for (i = 0; i < 4; i++) {
    // All six 8x8 cells intact. 0x2f left the centre-bottom cell clear, so
    // draw() punched an 8x8 hole through the bunker legs on every deploy.
    solid_[i] = 0x3f;
  }
}

void Shelters::draw() {
  u8 i;
  for (i = 0; i < 4; i++) {
    u16 bx = left(i);
    u8 row;
    u8 col;
    Screen::stamp(bx, Metrics::BunkerY, k_bunker, 24, 16);
    for (row = 0; row < 2; row++) {
      for (col = 0; col < 3; col++) {
        u8 bit = k_bit[(row << 1) + row + col];
        if ((solid_[i] & bit) == 0) {
          Screen::fill(bx + (col << 3), Metrics::BunkerY + (row << 3), 8, 8, ColBlack);
        }
      }
    }
  }
}

u8 Shelters::strike(u16 x, u8 y) {
  u8 i;
  if (y < Metrics::BunkerY || y >= Metrics::BunkerY + 16) {
    return 0;
  }
  for (i = 0; i < 4; i++) {
    u16 bx = left(i);
    u8 col;
    u8 row;
    u8 bit;
    if (x < bx || x >= bx + 24) {
      continue;
    }
    col = (u8)((x - bx) >> 3);
    row = (u8)((y - Metrics::BunkerY) >> 3);
    if (col > 2 || row > 1) {
      continue;
    }
    bit = k_bit[(row << 1) + row + col];
    if ((solid_[i] & bit) == 0) {
      continue;
    }
    solid_[i] = (u8)(solid_[i] & (u8)~bit);
    Screen::fill(bx + (col << 3), Metrics::BunkerY + (row << 3), 8, 8, ColBlack);
    return 1;
  }
  return 0;
}

void Saucer::reset() {
  on_ = 0;
  dir_ = 4;
  tick_ = 0;
  x_ = 0;
}

void Saucer::erase() {
  if (on_) {
    Screen::fill(x_, Metrics::UfoY, 16, 8, ColBlack);
  }
}

void Saucer::draw() {
  if (on_) {
    Screen::stamp(x_, Metrics::UfoY, k_ufo, 16, 8);
  }
}

void Saucer::launch(u8 from_left) {
  on_ = 1;
  tick_ = 0;
  if (from_left) {
    x_ = 4;
    dir_ = 4;
  } else {
    x_ = 300;
    dir_ = -4;
  }
}

u8 Saucer::step() {
  if (!on_) {
    return 0;
  }
  tick_++;
  if (tick_ & 1) {
    return 1;
  }
  x_ += dir_;
  if (x_ > 304) {
    on_ = 0;
    return 0;
  }
  return 1;
}

u8 Saucer::contains(u16 x, u8 y) const {
  if (!on_) {
    return 0;
  }
  return x < x_ + 16 && x_ < x + Metrics::Shot && y < Metrics::UfoY + 8 &&
         Metrics::UfoY < y + Metrics::Shot;
}

void Cannon::reset() { x_ = Metrics::PlayerX; }

void Cannon::erase() { Screen::fill(x_, Metrics::PlayerY, 16, 8, ColBlack); }

void Cannon::draw() { Screen::stamp(x_, Metrics::PlayerY, k_cannon, 16, 8); }

void Cannon::move(int dir) {
  if (dir < 0 && x_ >= 16) {
    x_ -= 8;
  } else if (dir > 0 && x_ <= 288) {
    x_ += 8;
  }
}

void Volley::reset() {
  u8 i;
  shot_on_ = 0;
  for (i = 0; i < 3; i++) {
    bomb_on_[i] = 0;
  }
}

void Volley::erase() {
  u8 i;
  if (shot_on_) {
    Screen::fill(shot_x_, shot_y_, Metrics::Shot, Metrics::Shot, ColBlack);
  }
  for (i = 0; i < 3; i++) {
    if (bomb_on_[i]) {
      Screen::fill(bomb_x_[i], bomb_y_[i], Metrics::Shot, Metrics::Shot, ColBlack);
    }
  }
}

const unsigned char* Volley::bomb_bits(u8 frame) const {
  if (frame == 1) {
    return k_bomb1;
  }
  if (frame == 2) {
    return k_bomb2;
  }
  return k_bomb0;
}

u8 Volley::fire(u16 x) {
  if (shot_on_) {
    return 0;
  }
  shot_on_ = 1;
  shot_x_ = x + 4;
  // One row above the cannon — never PlayerY, or hop_shot's trail fill
  // punches a hole through the gun sprite.
  shot_y_ = Metrics::PlayerY - Metrics::Shot;
  Screen::stamp(shot_x_, shot_y_, k_shot, Metrics::Shot, Metrics::Shot);
  return 1;
}

void Volley::drop(u16 x, u8 y) {
  u8 i;
  for (i = 0; i < 3; i++) {
    if (bomb_on_[i]) {
      continue;
    }
    bomb_on_[i] = 1;
    bomb_x_[i] = x;
    bomb_y_[i] = y;
    bomb_frame_[i] = i;
    Screen::stamp(x, y, bomb_bits(bomb_frame_[i]), Metrics::Shot, Metrics::Shot);
    return;
  }
}

u16 Volley::hop_shot(Fleet* fleet, Shelters* shelters, Saucer* saucer, u8 hops) {
  u16 scored;
  u8 step;
  if (!shot_on_) {
    return 0;
  }
  // Flush each hop so the bolt climbs on-screen during the frame. Callers
  // must run bombs before this so hazards are not stuck behind the trail.
  for (step = 0; step < hops; step++) {
    u8 old_y = shot_y_;
    shot_y_ -= 8;
    if (shot_y_ < 28) {
      Screen::fill(shot_x_, old_y, Metrics::Shot, Metrics::Shot, ColBlack);
      Screen::close();
      shot_on_ = 0;
      return 0;
    }
    scored = fleet->shoot(shot_x_, shot_y_);
    if (scored) {
      Screen::fill(shot_x_, old_y, Metrics::Shot, Metrics::Shot, ColBlack);
      Screen::close();
      shot_on_ = 0;
      return scored;
    }
    if (shot_y_ <= Metrics::UfoY + 8 && saucer->contains(shot_x_, shot_y_)) {
      Screen::fill(shot_x_, old_y, Metrics::Shot, Metrics::Shot, ColBlack);
      shot_on_ = 0;
      Screen::fill(saucer->x_, Metrics::UfoY, 16, 8, ColBlack);
      saucer->on_ = 0;
      Screen::close();
      return 1000;
    }
    if (shot_y_ + Metrics::Shot > Metrics::BunkerY - 4 &&
        shot_y_ < Metrics::BunkerY + 24 &&
        shelters->strike(shot_x_ + 4, shot_y_ + 4)) {
      Screen::fill(shot_x_, old_y, Metrics::Shot, Metrics::Shot, ColBlack);
      Screen::close();
      shot_on_ = 0;
      return 0;
    }
    Screen::stamp(shot_x_, shot_y_, k_shot, Metrics::Shot, Metrics::Shot);
    Screen::fill(shot_x_, old_y, Metrics::Shot, Metrics::Shot, ColBlack);
    Screen::close();
  }
  return 0;
}

u8 Volley::move_bombs(Shelters* shelters, u16 px) {
  u8 i;
  u8 killed = 0;
  for (i = 0; i < 3; i++) {
    u16 bx;
    u8 old_y;
    u8 by;
    if (!bomb_on_[i]) {
      continue;
    }
    bx = bomb_x_[i];
    old_y = bomb_y_[i];
    by = old_y + 8;
    bomb_y_[i] = by;
    bomb_frame_[i]++;
    if (bomb_frame_[i] > 2) {
      bomb_frame_[i] = 0;
    }
    if (by >= 224) {
      Screen::fill(bx, old_y, Metrics::Shot, Metrics::Shot, ColBlack);
      bomb_on_[i] = 0;
      continue;
    }
    if (shot_on_) {
      if (shot_x_ < bx + Metrics::Shot) {
        if (bx < shot_x_ + Metrics::Shot) {
          if (shot_y_ < by + Metrics::Shot) {
            if (by < shot_y_ + Metrics::Shot) {
              Screen::fill(bx, old_y, Metrics::Shot, Metrics::Shot, ColBlack);
              Screen::fill(shot_x_, shot_y_, Metrics::Shot, Metrics::Shot, ColBlack);
              bomb_on_[i] = 0;
              shot_on_ = 0;
              continue;
            }
          }
        }
      }
    }
    if (shelters->strike(bx + 4, by + 4)) {
      Screen::fill(bx, old_y, Metrics::Shot, Metrics::Shot, ColBlack);
      bomb_on_[i] = 0;
      continue;
    }
    if (bx < px + 14) {
      if (px + 2 < bx + Metrics::Shot) {
        if (by < Metrics::PlayerY + 8) {
          if (Metrics::PlayerY + 1 < by + Metrics::Shot) {
            Screen::fill(bx, old_y, Metrics::Shot, Metrics::Shot, ColBlack);
            bomb_on_[i] = 0;
            killed = 1;
            continue;
          }
        }
      }
    }
    Screen::stamp(bx, by, bomb_bits(bomb_frame_[i]), Metrics::Shot, Metrics::Shot);
    Screen::fill(bx, old_y, Metrics::Shot, Metrics::Shot, ColBlack);
  }
  return killed;
}

void Hud::draw() {
  u8 L = lives;
  u8 W = wave;
  if (L > 9) {
    L = 9;
  }
  if (W > 9) {
    W = 9;
  }
  bbc::colour(128);
  bbc::tab(0, 0);
  bbc::colour(ColGreen);
  Text::write("SC ");
  bbc::colour(ColWhite);
  Text::quad(score);
  bbc::colour(ColGreen);
  Text::write(" L");
  bbc::colour(ColWhite);
  bbc::oswrch((unsigned char)('0' + L));
  bbc::colour(ColGreen);
  Text::write(" W");
  bbc::colour(ColWhite);
  bbc::oswrch((unsigned char)('0' + W));
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
  for (frames = 0; frames < 60; frames++) {
    bbc::osbyte(bbc::osbyte_command::wait_vsync, 0, 0);
  }
  return 1;
}

void Game::add_score(u16 points) {
  score = (unsigned)(score + points);
  hud_dirty_ = 1;
}

void Game::present() {
  Screen::close();
  if (hud_dirty_) {
    Hud::draw();
    hud_dirty_ = 0;
  }
}

void Game::deploy() {
  u8 drop = (u8)((wave - 1) << 3);
  if (drop > 40) {
    drop = 40;
  }
  phase = PhasePlay;
  saucer_.reset();
  volley_.reset();
  boom_on_ = 0;
  fleet_.reset(Metrics::OriginY + drop);
  step_t_ = fleet_.pace(wave);
  bomb_t_ = 30;
  ufo_wait_ = 180;
  Screen::close();
  bbc::cls();
  bbc::cursor(0);
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  Screen::fill(0, Metrics::LineY, 320, 2, ColGreen);
  Screen::fill(0, Metrics::GroundY, 320, 2, ColGreen);
  shelters_.draw();
  fleet_.draw();
  cannon_.draw();
  hud_dirty_ = 1;
  present();
}

void Game::kill_player() {
  if (phase != PhasePlay) {
    return;
  }
  volley_.erase();
  phase = PhaseDying;
  death_t_ = 36;
  if (lives > 0) {
    lives--;
  }
  hud_dirty_ = 1;
  boom_on_ = 1;
  boom_x_ = cannon_.x_;
  boom_y_ = Metrics::PlayerY;
  boom_t_ = 36;
  volley_.reset();
}

void Game::score_hit(u16 scored) {
  if (scored == 1000) {
    add_score(100);
    return;
  }
  add_score(scored);
  shelters_.strike(fleet_.last_x_ + 8, fleet_.last_y_ + 4);
  boom_on_ = 1;
  boom_x_ = fleet_.last_x_;
  boom_y_ = fleet_.last_y_;
  boom_t_ = 6;
}

void Game::march() {
  u8 stepped;
  u8 was_busy;
  if (phase != PhasePlay) {
    return;
  }
  was_busy = fleet_.busy();
  if (!was_busy) {
    if (step_t_ > 0) {
      step_t_--;
      return;
    }
  }
  stepped = fleet_.step(&shelters_);
  if (stepped == 2) {
    landed_ = 1;
    lives = 0;
    kill_player();
    return;
  }
  if (stepped == 1 && !fleet_.busy()) {
    step_t_ = fleet_.pace(wave);
  }
}

void Game::launch_bomb() {
  u8 guard = 0;
  u8 col;
  if (phase != PhasePlay || alive == 0) {
    return;
  }
  if (bomb_t_ > 0) {
    bomb_t_--;
    return;
  }
  bomb_t_ = 12;
  if (alive < 16) {
    bomb_t_ = 6;
  }
  if (wave > 2 && bomb_t_ > 4) {
    bomb_t_ = (u8)(bomb_t_ - 2);
  }
  col = (u8)(rng_.next() & 15);
  while (guard < Metrics::Columns) {
    u16 bx;
    u8 by;
    if (col >= Metrics::Columns) {
      col = 0;
    }
    if (fleet_.drop_at(col, &bx, &by)) {
      volley_.drop(bx, by);
      return;
    }
    col++;
    guard++;
  }
}

void Game::launch_ufo() {
  if (phase != PhasePlay || saucer_.on_) {
    return;
  }
  if (ufo_wait_ > 0) {
    ufo_wait_--;
    return;
  }
  saucer_.launch(rng_.next() & 1);
  ufo_wait_ = 280;
}

#if INVADERS_USE_CORO
void Game::beat_player() {
  u16 old_x;
  if (phase != PhasePlay) {
    return;
  }
  if (boom_on_) {
    Screen::fill(boom_x_, boom_y_, 16, 8, ColBlack);
    if (boom_t_ == 0) {
      boom_on_ = 0;
    }
  }
  old_x = cannon_.x_;
  cannon_.move(Keys::axis());
  // Latch fire so a tap during an in-flight bolt still queues the next shot
  // instead of needing a second press after shot_on_ clears.
  if (Keys::fire()) {
    fire_want_ = 1;
  }
  if (fire_want_ && volley_.fire(cannon_.x_)) {
    fire_want_ = 0;
    cannon_.draw();
  }
  if (cannon_.x_ != old_x) {
    cannon_.draw();
    if (cannon_.x_ > old_x) {
      Screen::fill(old_x, Metrics::PlayerY, cannon_.x_ - old_x, 8, ColBlack);
    } else {
      Screen::fill(cannon_.x_ + 16, Metrics::PlayerY, old_x - cannon_.x_, 8, ColBlack);
    }
  }
}

void Game::beat_hazards() {
  u16 old_ufo_x;
  u8 ufo_was;
  if (phase != PhasePlay) {
    return;
  }
  if (volley_.move_bombs(&shelters_, cannon_.x_)) {
    kill_player();
  }
  if (phase != PhasePlay) {
    return;
  }
  ufo_was = saucer_.on_;
  old_ufo_x = saucer_.x_;
  if (saucer_.on_) {
    saucer_.step();
  }
  if (ufo_was && !saucer_.on_) {
    Screen::fill(old_ufo_x, Metrics::UfoY, 16, 8, ColBlack);
  } else if (saucer_.on_ && saucer_.x_ != old_ufo_x) {
    saucer_.draw();
    if (saucer_.x_ > old_ufo_x) {
      Screen::fill(old_ufo_x, Metrics::UfoY, saucer_.x_ - old_ufo_x, 8, ColBlack);
    } else {
      Screen::fill(saucer_.x_ + 16, Metrics::UfoY, old_ufo_x - saucer_.x_, 8, ColBlack);
    }
  } else if (saucer_.on_ && !ufo_was) {
    saucer_.draw();
  }
}

void Game::beat_fleet() {
  u16 scored;
  if (phase != PhasePlay) {
    return;
  }
  // 6 flushed hops (smooth mid-frame climb). Bombs already moved in
  // beat_hazards this tick — they sit during the laser trail by design.
  scored = volley_.hop_shot(&fleet_, &shelters_, &saucer_, 6);
  if (scored > 0) {
    score_hit(scored);
  }
  march();
  if (phase != PhasePlay) {
    return;
  }
  if (boom_on_) {
    Screen::stamp(boom_x_, boom_y_, k_boom, 16, 8);
    if (boom_t_ > 0) {
      boom_t_--;
    }
  }
  launch_bomb();
  launch_ufo();
  if (phase == PhasePlay && alive == 0) {
    phase = PhaseWave;
    wave_t_ = 24;
  }
}
#endif

#if !INVADERS_USE_CORO
void Game::play_slice() {
  u16 old_x;
  u16 old_ufo_x;
  u8 ufo_was;
  u16 scored;
  if (phase != PhasePlay) {
    return;
  }
  if (boom_on_) {
    Screen::fill(boom_x_, boom_y_, 16, 8, ColBlack);
    if (boom_t_ == 0) {
      boom_on_ = 0;
    }
  }
  old_x = cannon_.x_;
  ufo_was = saucer_.on_;
  old_ufo_x = saucer_.x_;
  cannon_.move(Keys::axis());
  if (Keys::fire()) {
    fire_want_ = 1;
  }
  if (fire_want_ && volley_.fire(cannon_.x_)) {
    fire_want_ = 0;
    cannon_.draw();
  }
  if (volley_.move_bombs(&shelters_, cannon_.x_)) {
    kill_player();
  }
  if (phase != PhasePlay) {
    return;
  }
  if (saucer_.on_) {
    saucer_.step();
  }
  scored = volley_.hop_shot(&fleet_, &shelters_, &saucer_, 6);
  if (scored > 0) {
    score_hit(scored);
  }
  march();
  if (phase != PhasePlay) {
    return;
  }
  if (cannon_.x_ != old_x) {
    cannon_.draw();
    if (cannon_.x_ > old_x) {
      Screen::fill(old_x, Metrics::PlayerY, cannon_.x_ - old_x, 8, ColBlack);
    } else {
      Screen::fill(cannon_.x_ + 16, Metrics::PlayerY, old_x - cannon_.x_, 8, ColBlack);
    }
  }
  if (ufo_was && !saucer_.on_) {
    Screen::fill(old_ufo_x, Metrics::UfoY, 16, 8, ColBlack);
  } else if (saucer_.on_ && saucer_.x_ != old_ufo_x) {
    saucer_.draw();
    if (saucer_.x_ > old_ufo_x) {
      Screen::fill(old_ufo_x, Metrics::UfoY, saucer_.x_ - old_ufo_x, 8, ColBlack);
    } else {
      Screen::fill(saucer_.x_ + 16, Metrics::UfoY, old_ufo_x - saucer_.x_, 8, ColBlack);
    }
  } else if (saucer_.on_ && !ufo_was) {
    saucer_.draw();
  }
  if (boom_on_) {
    Screen::stamp(boom_x_, boom_y_, k_boom, 16, 8);
    if (boom_t_ > 0) {
      boom_t_--;
    }
  }
  launch_bomb();
  launch_ufo();
  if (phase == PhasePlay && alive == 0) {
    phase = PhaseWave;
    wave_t_ = 24;
  }
  present();
}
#endif

u8 Game::tick() {
  clock_++;
  bbc::osbyte(bbc::osbyte_command::wait_vsync, 0, 0);
  if (clock_ > 30 && Keys::escape()) {
    bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
    bbc_return();
  }
  if (phase == PhaseOver) {
    if (Keys::fire()) {
      return 1;
    }
    present();
    return 0;
  }
  if (phase == PhaseWave) {
    if (wave_t_ == 24) {
      Screen::close();
      Hud::banner("GET READY", ColWhite, 14);
    }
    if (wave_t_ > 0) {
      wave_t_--;
    }
    if (wave_t_ == 0) {
      wave++;
      hud_dirty_ = 1;
      deploy();
    }
    return 0;
  }
  if (phase == PhaseDying) {
    cannon_.erase();
    if ((death_t_ & 4) == 0) {
      Screen::stamp(boom_x_, boom_y_, k_boom, 16, 8);
    } else {
      Screen::stamp(cannon_.x_, Metrics::PlayerY, k_cannon_red, 16, 8);
    }
    present();
    if (death_t_ > 0) {
      death_t_--;
    }
    if (death_t_ == 0) {
      Screen::fill(boom_x_, boom_y_, 16, 8, ColBlack);
      boom_on_ = 0;
      if (lives == 0) {
        phase = PhaseOver;
        over_t_ = 80;
        Screen::close();
        Hud::banner(landed_ ? " INVADED " : "GAME OVER", ColRed, 12);
        bbc::tab(13, 15);
        bbc::colour(ColWhite);
        Text::write("PRESS SPACE");
        return 0;
      }
      cannon_.reset();
      volley_.reset();
      phase = PhasePlay;
      cannon_.draw();
      present();
    }
    return 0;
  }
#if INVADERS_USE_CORO
  resume_job(&player_job);
  resume_job(&hazard_job);
  resume_job(&fleet_job);
  present();
#else
  play_slice();
#endif
  return 0;
}

u8 Game::session() {
  score = 0;
  lives = 3;
  wave = 1;
  landed_ = 0;
  clock_ = 0;
  hud_dirty_ = 1;
  fire_want_ = 0;
  cannon_.reset();
  shelters_.reset();
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  deploy();
#if INVADERS_USE_CORO
  play_used = 0;
  player_job = player_coro(this);
  hazard_job = hazard_coro(this);
  fleet_job = fleet_coro(this);
#endif
  for (;;) {
    u8 result = tick();
    if (result != 0) {
#if INVADERS_USE_CORO
      player_job = Play{};
      hazard_job = Play{};
      fleet_job = Play{};
#endif
      return result;
    }
  }
}

void Game::run() {
  bbc::palette(0, ColBlack);
  bbc::palette(1, 2);
  bbc::palette(2, 1);
  bbc::palette(3, 7);
  bbc::colour(128);
  bbc::cursor(0);
  rng_.seed();
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  for (;;) {
    session();
  }
}

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
  game.run();
  bbc_return();
  return 0;
}
