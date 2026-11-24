// Pac-Man for the BBC Master, linked with the sideways libc.
//
// Shadow Mode 2 (VDU 22,130): 160x256, eight steady colours — yellow Pac-Man,
// red Blinky, pink Pinky, cyan Inky, green Clyde, connected hollow blue walls,
// pink ghost-house gate, and white pellets. Actors glide in 4 sub-steps per
// tile at 50 Hz; VBLANK-first blitting in pacman_fx.s keeps sprites crisp.
//
// Arrows / WASD / ZX move. Space starts. Escape returns to BASIC.

#include <bbc.h>
#include <coroutine>
#include <stddef.h>

#include "pacman.h"

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
u8 lives;
u8 wave;
u8 phase;
u8 acccon_copy;
}

enum {
  PhaseTitle = 1,
  PhaseReady = 2,
  PhasePlay = 3,
  PhaseDying = 4,
  PhaseWave = 5,
  PhaseOver = 6
};

enum {
  ColBlack = 0,
  ColRed = 1,
  ColGreen = 2,
  ColYellow = 3,
  ColBlue = 4,
  ColMagenta = 5,
  ColCyan = 6,
  ColWhite = 7
};

class Pace {
 public:
  enum : unsigned {
    Title = 240,
    TitleStep = 10,
    Ready = 42,
    WantHold = 18,
    // Half-rate (25 Hz) ticks so u8 covers up to 10 seconds.
    FrightBase = 160,
    FrightFlash = 36,
    Death = 64,
    Wave = 70,
    Over = 90,
    EscapeGuard = 25,
    Scatter0 = 175,
    Chase0 = 250,
    Scatter1 = 150,
    Chase1 = 250,
    Scatter2 = 110,
    Chase2 = 250,
    Scatter3 = 100
  };
};

extern "C" {
void frame_wait();
void keys_read();
void hud_digits();
void cell_draw();
void cell_wipe();
void tile_draw();
void actor_draw();
void maze_reset();
void paint_rows();
void footprint_restore();
void patch_window();
void act_store();
void actors_draw();
void actors_refresh();
void actors_wipe_all();
void ghost_load();
void ghost_save();
void sound_chomp();
void sound_death();
void sound_eat();

extern const unsigned char k_pm_pac_closed[];
extern const unsigned char k_pm_pac_r0[];
extern const unsigned char k_pm_pac_r1[];
extern const unsigned char k_pm_pac_l0[];
extern const unsigned char k_pm_pac_l1[];
extern const unsigned char k_pm_pac_u0[];
extern const unsigned char k_pm_pac_u1[];
extern const unsigned char k_pm_pac_d0[];
extern const unsigned char k_pm_pac_d1[];
extern const unsigned char k_pm_die0[];
extern const unsigned char k_pm_die1[];
extern const unsigned char k_pm_die2[];
extern const unsigned char k_pm_blinky0[];
extern const unsigned char k_pm_blinky1[];
extern const unsigned char k_pm_pinky0[];
extern const unsigned char k_pm_pinky1[];
extern const unsigned char k_pm_inky0[];
extern const unsigned char k_pm_inky1[];
extern const unsigned char k_pm_clyde0[];
extern const unsigned char k_pm_clyde1[];
extern const unsigned char k_pm_ghost_f0[];
extern const unsigned char k_pm_ghost_f1[];
extern const unsigned char k_pm_eyes[];
extern const unsigned char k_pm_pellet[];
extern const unsigned char k_pm_power[];
extern const unsigned char k_pm_gate[];
extern const unsigned char k_pm_wall[];
extern const unsigned char k_pm_digits[];
}

enum {
  KeyLeft = KEY_LEFT,
  KeyRight = KEY_RIGHT,
  KeyUp = KEY_UP,
  KeyDown = KEY_DOWN,
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
enum { EnvChomp = 1, EnvDeath = 2, EnvEat = 3 };

static const Envelope envelopes[3] = {
    {EnvChomp, NoRepeat | 1, {4, -4, 0}, {4, 4, 0}, 126, -12, 0, -126, 96, 0},
    {EnvDeath, NoRepeat | 2, {-2, -3, 0}, {8, 8, 0}, 126, -3, 0, -126, 120, 0},
    {EnvEat, NoRepeat | 1, {8, 4, 0}, {5, 5, 0}, 126, -8, 0, -126, 120, 0}};

extern "C" {
u8 key_bits;

u8 pm_col;
u8 pm_row;
u8 pm_end_row;
u8 pm_sx;
u8 pm_sy;
u8 pm_g;
u8 pm_wx;
u8 pm_wy;
u8 pm_wc;
u8 pm_wr;
const unsigned char* pm_src;

u8 act_sx[5];
u8 act_sy[5];
u8 act_spr_lo[5];
u8 act_spr_hi[5];
u8 pac_dirty;

u8 maze[PM_COLS * PM_ROWS];

u8 ghost_x[PM_GHOSTS];
u8 ghost_y[PM_GHOSTS];
u8 ghost_sx[PM_GHOSTS];
u8 ghost_sy[PM_GHOSTS];
u8 ghost_dir[PM_GHOSTS];
u8 ghost_mode[PM_GHOSTS];
u8 ghost_house[PM_GHOSTS];
u8 ghost_wait[PM_GHOSTS];

// Active ghost working set loaded/saved by ghost_load / ghost_save.
u8 gx;
u8 gy;
u8 gsx;
u8 gsy;
u8 gdir;
u8 gmode;
u8 ghouse;
u8 gwait;

extern const Note snd_chomp = {Flush | 2, EnvChomp, 104, 2};
extern const Note snd_death = {Flush | 0, EnvDeath, 84, 24};
extern const Note snd_eat = {Flush | 1, EnvEat, 140, 5};

// Z/A/Left, X/D/Right, Up/W/:, Down/S/?, Space/Return, Escape.
extern const u8 key_codes[KEY_COUNT] = {0x61, 0x41, 0x19, 0x42, 0x32, 0x79,
                                        0x39, 0x21, 0x48, 0x29, 0x51, 0x68,
                                        0x62, 0x49, 0x70};
extern const u8 key_masks[KEY_COUNT] = {
    KeyLeft, KeyLeft,  KeyLeft,  KeyRight, KeyRight, KeyRight,
    KeyUp,   KeyUp,    KeyUp,    KeyDown,  KeyDown,  KeyDown,
    KeyFire, KeyFire,  KeyEscape};
extern const u16 hud_places[5] = {10000, 1000, 100, 10, 1};
}

enum {
  GhostScatter = 0,
  GhostChase = 1,
  GhostFright = 2,
  GhostEyes = 3
};

static const i8 scatter_x[PM_GHOSTS] = {17, 1, 17, 1};
static const i8 scatter_y[PM_GHOSTS] = {0, 0, 18, 18};
static const u8 dir_pref[4] = {PM_DIR_UP, PM_DIR_LEFT, PM_DIR_DOWN, PM_DIR_RIGHT};
static const i8 dx_tab[4] = {1, -1, 0, 0};
static const i8 dy_tab[4] = {0, 0, -1, 1};
static const u8 opposite[4] = {PM_DIR_LEFT, PM_DIR_RIGHT, PM_DIR_DOWN, PM_DIR_UP};

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
  enum : unsigned { Cols = 20 };
  static void write(const char* text);
  static void centre(u8 row, u8 colour, const char* line);
};

class Hud {
 public:
  static void labels();
  static void banner(const char* line, u8 colour, u8 row);
};

namespace Game {
void run();
u8 tick();
void beat_input();
void beat_pac();
void beat_ghosts();
}  // namespace Game

static u8 pac_x;
static u8 pac_y;
static u8 pac_sx;
static u8 pac_sy;
static u8 pac_dir;
static u8 pac_want;
static u8 pac_want_t;
static u8 pac_anim;

static u16 rng_state;
static u8 prev_keys;
static u8 hud_dirty;
static u8 frames_played;
static u8 step_clock;
static u8 ghost_anim;
static u8 ready_t;
static u8 fright_t;
static u8 death_t;
static u8 wave_t;
static u8 over_t;
static u8 pellets_left;
static u8 eat_streak;
static u8 mode_t;
static u8 mode_wave;
static u8 global_mode;

static Play pac_job;
static Play ghost_job;
static u8 jobs_ok;

Play pac_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_input();
    Game::beat_pac();
  }
}

Play ghost_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_ghosts();
  }
}

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

void Hud::labels() {
  bbc::colour(128);
  bbc::colour(ColYellow);
  bbc::tab(0, 0);
  Text::write("SC");
  bbc::colour(ColCyan);
  bbc::tab(9, 0);
  Text::write("WV");
  bbc::colour(ColYellow);
  bbc::tab(15, 0);
  Text::write("LV");
}

void Hud::banner(const char* line, u8 colour, u8 row) {
  bbc::colour(128);
  Text::centre(row, colour, line);
}

static u16 rng_next() {
  rng_state = static_cast<unsigned>(rng_state + (rng_state << 5) + 13849);
  return rng_state;
}

static void present() {
  hud_digits();
  hud_dirty = 0;
}

static void add_score(u16 points) {
  u16 next = static_cast<u16>(score + points);
  score = next < score ? 65535u : next;
  hud_dirty = 1;
}

static u16 maze_index(u8 x, u8 y) {
  return static_cast<u16>(static_cast<u16>(y) * PM_COLS + x);
}

static u8 open_tile(u8 x, u8 y) {
  if (x >= PM_COLS || y >= PM_ROWS) {
    return 0;
  }
  u8 t = maze[maze_index(x, y)];
  return (t == PM_EMPTY || t == PM_PELLET || t == PM_POWER) ? 1 : 0;
}

static u8 wrap_tile_x(u8 x, u8 y, u8 d) {
  if (y == PM_TUNNEL_Y) {
    if (d == PM_DIR_RIGHT && x == PM_COLS - 1) {
      return 0;
    }
    if (d == PM_DIR_LEFT && x == 0) {
      return PM_COLS - 1;
    }
  }
  return static_cast<u8>(x + dx_tab[d]);
}

static void stamp_actor(u8 sx, u8 sy, const unsigned char* src) {
  pm_sx = sx;
  pm_sy = sy;
  pm_src = src;
  actor_draw();
}

static void restore_at(u8 sx, u8 sy) {
  pm_sx = sx;
  pm_sy = sy;
  footprint_restore();
}

static const unsigned char* const pac_art[8] = {
    k_pm_pac_r0, k_pm_pac_r1, k_pm_pac_l0, k_pm_pac_l1,
    k_pm_pac_u0, k_pm_pac_u1, k_pm_pac_d0, k_pm_pac_d1};

static const unsigned char* const ghost_art[8] = {
    k_pm_blinky0, k_pm_blinky1, k_pm_pinky0, k_pm_pinky1,
    k_pm_inky0,   k_pm_inky1,   k_pm_clyde0, k_pm_clyde1};

static const unsigned char* pac_sprite() {
  u8 phase4 = (pac_anim >> 1) & 3;
  u8 d = pac_dir < 4 ? pac_dir : 0;
  if (phase4 == 0) {
    return k_pm_pac_closed;
  }
  return pac_art[(d << 1) | (phase4 == 2 ? 1 : 0)];
}

static const unsigned char* ghost_sprite() {
  if (gmode == GhostEyes) {
    return k_pm_eyes;
  }
  if (gmode == GhostFright) {
    if (fright_t < Pace::FrightFlash && ((fright_t >> 2) & 1) != 0) {
      return k_pm_ghost_f1;
    }
    return k_pm_ghost_f0;
  }
  return ghost_art[(pm_g << 1) | ((ghost_anim >> 3) & 1)];
}

static void paint_band(u8 y0, u8 y1) {
  pm_row = y0;
  pm_end_row = y1;
  paint_rows();
}

static void place_actors() {
  static const u8 gx0[PM_GHOSTS] = {PM_EXIT_X, 9, 8, 10};
  static const u8 gy0[PM_GHOSTS] = {PM_EXIT_Y, PM_HOUSE_Y, PM_HOUSE_Y, PM_HOUSE_Y};
  static const u8 gd0[PM_GHOSTS] = {PM_DIR_LEFT, PM_DIR_UP, PM_DIR_UP, PM_DIR_UP};
  static const u8 gh0[PM_GHOSTS] = {0, 1, 1, 1};
  static const u8 gw0[PM_GHOSTS] = {0, 18, 70, 125};
  pac_x = PM_PAC_X0;
  pac_y = PM_PAC_Y0;
  pac_sx = PM_PAC_X0 * PM_SUB;
  pac_sy = PM_PAC_Y0 * PM_SUB;
  pac_dir = PM_DIR_LEFT;
  pac_want = PM_DIR_NONE;
  pac_want_t = 0;
  pac_anim = 2;
  pac_dirty = 0;

  for (pm_g = 0; pm_g < PM_GHOSTS; pm_g++) {
    gx = gx0[pm_g];
    gy = gy0[pm_g];
    gsx = static_cast<u8>(gx << 2);
    gsy = static_cast<u8>(gy << 2);
    gdir = gd0[pm_g];
    ghouse = gh0[pm_g];
    gwait = gw0[pm_g];
    gmode = GhostScatter;
    ghost_save();
  }
}

static void sync_actors() {
  for (pm_g = 0; pm_g < PM_GHOSTS; pm_g++) {
    ghost_load();
    pm_sx = gsx;
    pm_sy = gsy;
    pm_src = ghost_sprite();
    act_store();
  }
  pm_g = 4;
  pm_sx = pac_sx;
  pm_sy = pac_sy;
  pm_src = pac_sprite();
  act_store();
}

static void draw_actors() {
  sync_actors();
  actors_draw();
}

static void refresh_actors() {
  sync_actors();
  actors_refresh();
}

static void set_game_palette() {
  for (u8 c = 0; c < 8; c++) {
    bbc::palette(c, c);
  }
}

static void set_global_mode(u8 mode) {
  u8 flip = (global_mode != mode) ? 1 : 0;
  global_mode = mode;
  for (pm_g = 0; pm_g < PM_GHOSTS; pm_g++) {
    ghost_load();
    if (gmode == GhostScatter || gmode == GhostChase) {
      if (flip && ghouse == 0) {
        gdir = opposite[gdir];
      }
      gmode = mode;
      ghost_save();
    }
  }
}

static void start_mode_clock() {
  mode_wave = 0;
  mode_t = Pace::Scatter0;
  global_mode = GhostScatter;
}

static void tick_mode_clock() {
  static const u8 wave_len[7] = {
      Pace::Scatter0, Pace::Chase0, Pace::Scatter1, Pace::Chase1,
      Pace::Scatter2, Pace::Chase2, Pace::Scatter3};
  if (fright_t != 0 || mode_wave >= 7) {
    return;
  }
  if (mode_t > 0) {
    mode_t--;
  }
  if (mode_t != 0) {
    return;
  }
  mode_wave++;
  if (mode_wave >= 7) {
    set_global_mode(GhostChase);
    return;
  }
  mode_t = wave_len[mode_wave];
  set_global_mode((mode_wave & 1) ? GhostChase : GhostScatter);
}

static const u8 dir_key_tab[4] = {KeyRight, KeyLeft, KeyUp, KeyDown};

static u8 pick_dir_from_bits(u8 bits, u8 skip_dir) {
  if ((bits & KeyUp) != 0 && skip_dir != PM_DIR_UP) {
    return PM_DIR_UP;
  }
  if ((bits & KeyDown) != 0 && skip_dir != PM_DIR_DOWN) {
    return PM_DIR_DOWN;
  }
  if ((bits & KeyLeft) != 0 && skip_dir != PM_DIR_LEFT) {
    return PM_DIR_LEFT;
  }
  if ((bits & KeyRight) != 0 && skip_dir != PM_DIR_RIGHT) {
    return PM_DIR_RIGHT;
  }
  return PM_DIR_NONE;
}

static u8 fright_duration() {
  u8 dec = static_cast<u8>((wave - 1) * 20);
  if (dec + 60 >= Pace::FrightBase) {
    return 60;
  }
  return static_cast<u8>(Pace::FrightBase - dec);
}

static void eat_tile() {
  u16 cell = maze_index(pac_x, pac_y);
  u8 t = maze[cell];
  if (t != PM_PELLET && t != PM_POWER) {
    return;
  }
  maze[cell] = PM_EMPTY;
  pac_dirty = 1;
  if (pellets_left > 0) {
    pellets_left--;
  }
  if (t == PM_PELLET) {
    for (pm_g = 0; pm_g < PM_GHOSTS; pm_g++) {
      u8 w = ghost_wait[pm_g];
      ghost_wait[pm_g] = w > 4 ? static_cast<u8>(w - 4) : 0;
    }
    add_score(10);
    sound_chomp();
    return;
  }
  add_score(50);
  fright_t = fright_duration();
  eat_streak = 0;
  for (pm_g = 0; pm_g < PM_GHOSTS; pm_g++) {
    ghost_load();
    if (gmode == GhostScatter || gmode == GhostChase) {
      gmode = GhostFright;
      if (ghouse == 0) {
        gdir = opposite[gdir];
      }
      ghost_save();
    }
  }
  sound_eat();
}

static void kill_player();

static void collide() {
  u8 save_g = pm_g;
  for (pm_g = 0; pm_g < PM_GHOSTS; pm_g++) {
    i8 dx = static_cast<i8>(pac_sx - ghost_sx[pm_g]);
    i8 dy = static_cast<i8>(pac_sy - ghost_sy[pm_g]);
    if (dx < -2 || dx > 2 || dy < -2 || dy > 2) {
      continue;
    }
    u8 m = ghost_mode[pm_g];
    if (m == GhostFright) {
      ghost_mode[pm_g] = GhostEyes;
      if (pm_g == save_g) {
        gmode = GhostEyes;
      }
      eat_streak++;
      add_score(static_cast<u16>(100u << (eat_streak > 4 ? 4 : eat_streak)));
      sound_eat();
    } else if (m == GhostScatter || m == GhostChase) {
      pm_g = save_g;
      kill_player();
      return;
    }
  }
  pm_g = save_g;
}

static void step_pac() {
  u8 nx;
  u8 ny;
  if (pac_want < 4 && pac_dir < 4 && pac_want == opposite[pac_dir]) {
    pac_dir = pac_want;
    pac_want = PM_DIR_NONE;
    pac_want_t = 0;
  }

  if ((pac_sx & 3) == 0 && (pac_sy & 3) == 0) {
    pac_x = static_cast<u8>(pac_sx >> 2);
    pac_y = static_cast<u8>(pac_sy >> 2);
    eat_tile();
    if (pac_want < 4) {
      nx = wrap_tile_x(pac_x, pac_y, pac_want);
      ny = static_cast<u8>(pac_y + dy_tab[pac_want]);
      if (open_tile(nx, ny)) {
        pac_dir = pac_want;
        pac_want = PM_DIR_NONE;
        pac_want_t = 0;
      }
    }
    if (pac_dir >= 4) {
      return;
    }
    nx = wrap_tile_x(pac_x, pac_y, pac_dir);
    ny = static_cast<u8>(pac_y + dy_tab[pac_dir]);
    if (open_tile(nx, ny) == 0) {
      return;
    }
  }

  if (pac_dir >= 4) {
    return;
  }
  if (pac_sy == PM_TUNNEL_Y * PM_SUB && pac_dir == PM_DIR_LEFT && pac_sx == 0) {
    pac_sx = (PM_COLS - 1) * PM_SUB;
  } else if (pac_sy == PM_TUNNEL_Y * PM_SUB && pac_dir == PM_DIR_RIGHT &&
             pac_sx == (PM_COLS - 1) * PM_SUB) {
    pac_sx = 0;
  } else {
    pac_sx = static_cast<u8>(pac_sx + dx_tab[pac_dir]);
    pac_sy = static_cast<u8>(pac_sy + dy_tab[pac_dir]);
  }
  pac_x = static_cast<u8>(pac_sx >> 2);
  pac_y = static_cast<u8>(pac_sy >> 2);
  pac_anim++;
  if ((pac_sx & 3) == 0 && (pac_sy & 3) == 0) {
    eat_tile();
  }
  collide();
}

static u16 dist2(i8 ax, i8 ay, i8 bx, i8 by) {
  static const u16 sq[32] = {
      0,   1,   4,   9,   16,  25,  36,  49,  64,  81,  100,
      121, 144, 169, 196, 225, 256, 289, 324, 361, 400, 441,
      484, 529, 576, 625, 676, 729, 784, 841, 900, 961};
  i8 dx = static_cast<i8>(ax - bx);
  i8 dy = static_cast<i8>(ay - by);
  u8 ux = static_cast<u8>(dx < 0 ? -dx : dx);
  u8 uy = static_cast<u8>(dy < 0 ? -dy : dy);
  if (ux > 31) ux = 31;
  if (uy > 31) uy = 31;
  return static_cast<u16>(sq[ux] + sq[uy]);
}

static i8 tgt_x;
static i8 tgt_y;

static void ghost_target() {
  u8 pd = pac_dir < 4 ? pac_dir : 0;
  i8 px = static_cast<i8>(pac_x);
  i8 py = static_cast<i8>(pac_y);
  if (gmode == GhostEyes) {
    tgt_x = PM_EXIT_X;
    tgt_y = PM_EXIT_Y;
    return;
  }
  if (gmode == GhostScatter) {
    tgt_x = scatter_x[pm_g];
    tgt_y = scatter_y[pm_g];
    return;
  }
  if (pm_g == PM_BLINKY) {
    tgt_x = px;
    tgt_y = py;
    return;
  }
  if (pm_g == PM_PINKY) {
    tgt_x = static_cast<i8>(px + dx_tab[pd] * 3 - (pd == PM_DIR_UP ? 3 : 0));
    tgt_y = static_cast<i8>(py + dy_tab[pd] * 3);
    return;
  }
  if (pm_g == PM_INKY) {
    i8 ax = static_cast<i8>(px + dx_tab[pd] * 2 - (pd == PM_DIR_UP ? 2 : 0));
    i8 ay = static_cast<i8>(py + dy_tab[pd] * 2);
    tgt_x = static_cast<i8>(ax + ax - static_cast<i8>(ghost_x[PM_BLINKY]));
    tgt_y = static_cast<i8>(ay + ay - static_cast<i8>(ghost_y[PM_BLINKY]));
    return;
  }
  if (dist2(static_cast<i8>(gx), static_cast<i8>(gy), px, py) > 36) {
    tgt_x = px;
    tgt_y = py;
  } else {
    tgt_x = scatter_x[PM_CLYDE];
    tgt_y = scatter_y[PM_CLYDE];
  }
}

static u8 ghost_pick() {
  u8 pi;
  u8 d;
  u8 best;
  u8 n = 0;
  u8 opts[4];
  u8 rev = opposite[gdir];
  u16 best_score;
  u16 s;

  for (pi = 0; pi < 4; pi++) {
    d = dir_pref[pi];
    if (d != rev && open_tile(wrap_tile_x(gx, gy, d),
                              static_cast<u8>(gy + dy_tab[d]))) {
      opts[n++] = d;
    }
  }
  if (n == 0) {
    return rev;
  }
  if (gmode == GhostFright) {
    u8 r = static_cast<u8>(rng_next() >> 4) & 3;
    if (r >= n) {
      r = static_cast<u8>(r & 1);
      if (r >= n) r = 0;
    }
    return opts[r];
  }
  ghost_target();
  best = opts[0];
  best_score = 65535u;
  for (pi = 0; pi < n; pi++) {
    d = opts[pi];
    s = dist2(static_cast<i8>(wrap_tile_x(gx, gy, d)),
              static_cast<i8>(gy + dy_tab[d]), tgt_x, tgt_y);
    if (s < best_score) {
      best_score = s;
      best = d;
    }
  }
  return best;
}

static void step_ghost() {
  if (ghouse == 1) {
    if (gwait > 0) {
      return;
    }
    if (gsx < PM_EXIT_X * PM_SUB) {
      gsx++;
      gdir = PM_DIR_RIGHT;
    } else if (gsx > PM_EXIT_X * PM_SUB) {
      gsx--;
      gdir = PM_DIR_LEFT;
    } else if (gsy > PM_EXIT_Y * PM_SUB) {
      gsy--;
      gdir = PM_DIR_UP;
    }
    if (gsx == PM_EXIT_X * PM_SUB && gsy == PM_EXIT_Y * PM_SUB) {
      ghouse = 0;
      gdir = PM_DIR_LEFT;
    }
  } else if (ghouse == 2) {
    if (gsy < PM_HOUSE_Y * PM_SUB) {
      gsy++;
      gdir = PM_DIR_DOWN;
    } else {
      gmode = global_mode;
      ghouse = 1;
      gwait = 14;
    }
  } else {
    if ((gsx & 3) == 0 && (gsy & 3) == 0) {
      gx = static_cast<u8>(gsx >> 2);
      gy = static_cast<u8>(gsy >> 2);
      if (gmode == GhostEyes && gx == PM_EXIT_X && gy == PM_EXIT_Y) {
        ghouse = 2;
        gdir = PM_DIR_DOWN;
        ghost_save();
        return;
      }
      gdir = ghost_pick();
    }
    if (gsy == PM_TUNNEL_Y * PM_SUB && gdir == PM_DIR_LEFT && gsx == 0) {
      gsx = (PM_COLS - 1) * PM_SUB;
    } else if (gsy == PM_TUNNEL_Y * PM_SUB && gdir == PM_DIR_RIGHT &&
               gsx == (PM_COLS - 1) * PM_SUB) {
      gsx = 0;
    } else {
      gsx = static_cast<u8>(gsx + dx_tab[gdir]);
      gsy = static_cast<u8>(gsy + dy_tab[gdir]);
    }
  }

  gx = static_cast<u8>(gsx >> 2);
  gy = static_cast<u8>(gsy >> 2);
  ghost_save();
  if (ghouse != 2) {
    collide();
  }
}

static void show_ready() {
  phase = PhaseReady;
  ready_t = Pace::Ready;
  Hud::banner("READY!", ColYellow, 18);
}

static void start_life(u8 fresh_board) {
  fright_t = 0;
  eat_streak = 0;
  step_clock = 0;
  if (fresh_board) {
    bbc::cls();
    bbc::cursor(0);
    bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
    maze_reset();
    pellets_left = PM_PELLET_TOTAL;
  }
  place_actors();
  start_mode_clock();
  paint_band(0, PM_ROWS - 1);
  draw_actors();
  Hud::labels();
  present();
  show_ready();
}

static void title_row(u8 sy, u8 row, u8 col, const unsigned char* art,
                      const char* name) {
  stamp_actor(16, sy, art);
  bbc::colour(col);
  bbc::tab(7, row);
  Text::write(name);
}

static void Title_show() {
  u8 frames;
  u8 armed = 0;
  u8 look = 0;
  u8 step_t = Pace::TitleStep;
  phase = PhaseTitle;
  bbc::cursor(0);
  bbc::cls();
  Text::centre(2, ColYellow, "PAC-MAN");

  title_row(12, 7, ColRed, k_pm_blinky0, "BLINKY");
  title_row(20, 10, ColMagenta, k_pm_pinky0, "PINKY");
  title_row(28, 13, ColCyan, k_pm_inky0, "INKY");
  title_row(36, 16, ColGreen, k_pm_clyde0, "CLYDE");

  stamp_actor(24, 48, k_pm_pac_r0);
  stamp_actor(36, 48, k_pm_ghost_f0);
  stamp_actor(48, 48, k_pm_power);

  Text::centre(21, ColCyan, "ARROWS / WASD");
  Text::centre(23, ColYellow, "SPACE TO PLAY");
  Text::centre(25, ColWhite, "ESCAPE QUITS");
  Text::centre(28, ColMagenta, "C++20 DAVECC");
  Text::centre(29, ColMagenta, "BY DAVE ALLISON");
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);

  for (frames = 0; frames < Pace::Title; frames++) {
    frame_wait();
    keys_read();
    if ((key_bits & (KeyFire | KeyLeft | KeyRight | KeyUp | KeyDown)) == 0) {
      armed = 1;
    } else if (armed) {
      break;
    }
    step_t--;
    if (step_t == 0) {
      step_t = Pace::TitleStep;
      look ^= 1;
      pm_col = 6;
      pm_row = 12;
      cell_wipe();
      stamp_actor(24, 48, look ? k_pm_pac_r1 : k_pm_pac_r0);
    }
  }
  prev_keys = key_bits;
}

static void kill_player() {
  if (phase != PhasePlay) {
    return;
  }
  phase = PhaseDying;
  death_t = Pace::Death;
  if (lives > 0) {
    lives--;
  }
  hud_dirty = 1;
  sound_death();
  actors_wipe_all();
  restore_at(pac_sx, pac_sy);
  stamp_actor(pac_sx, pac_sy, k_pm_pac_closed);
}

static void dying() {
  if ((death_t & 15) == 0) {
    restore_at(pac_sx, pac_sy);
    if (death_t >= 48) {
      stamp_actor(pac_sx, pac_sy, k_pm_die0);
    } else if (death_t >= 32) {
      stamp_actor(pac_sx, pac_sy, k_pm_die1);
    } else if (death_t >= 16) {
      stamp_actor(pac_sx, pac_sy, k_pm_die2);
    }
  }
  if (death_t > 0) {
    death_t--;
  }
  if (death_t != 0) {
    return;
  }
  if (lives == 0) {
    phase = PhaseOver;
    over_t = Pace::Over;
    Hud::banner("GAME OVER", ColRed, 11);
    Text::centre(16, ColYellow, "PRESS SPACE");
    return;
  }
  start_life(0);
}

static void ready_step() {
  Game::beat_input();
  if (ready_t > 0) {
    ready_t--;
  }
  if (ready_t != 0) {
    return;
  }
  paint_band(10, 12);
  draw_actors();
  phase = PhasePlay;
}

namespace Game {

void beat_input() {
  u8 now = key_bits;
  u8 edges = static_cast<u8>(now & static_cast<u8>(~prev_keys));
  u8 cand;
  prev_keys = now;

  cand = pick_dir_from_bits(edges, PM_DIR_NONE);
  if (cand != PM_DIR_NONE) {
    pac_want = cand;
    pac_want_t = Pace::WantHold;
    return;
  }

  cand = pick_dir_from_bits(now, pac_dir);
  if (cand != PM_DIR_NONE) {
    pac_want = cand;
    pac_want_t = Pace::WantHold;
    return;
  }

  if (pac_dir == PM_DIR_NONE) {
    cand = pick_dir_from_bits(now, PM_DIR_NONE);
    if (cand != PM_DIR_NONE) {
      pac_want = cand;
      pac_want_t = Pace::WantHold;
      return;
    }
  }

  if (pac_want < 4) {
    if ((now & dir_key_tab[pac_want]) != 0) {
      pac_want_t = Pace::WantHold;
    } else if (pac_want_t > 0) {
      pac_want_t--;
      if (pac_want_t == 0) {
        pac_want = PM_DIR_NONE;
      }
    }
  }
}

void beat_pac() {
  if (phase != PhasePlay) {
    return;
  }
  step_pac();
  if (pellets_left == 0 && phase == PhasePlay) {
    refresh_actors();
    phase = PhaseWave;
    wave_t = Pace::Wave;
  }
}

static u8 ghost_due() {
  if (gmode == GhostEyes) {
    return 1;
  }
  if (gmode == GhostFright) {
    return (step_clock & 1) == 0 ? 1 : 0;
  }
  if (ghouse == 0 && gy == PM_TUNNEL_Y && (gx < 3 || gx >= PM_COLS - 3)) {
    return (step_clock & 1) == 0 ? 1 : 0;
  }
  if (wave == 1) {
    return (step_clock & 7) != 0 ? 1 : 0;
  }
  if (wave == 2) {
    return (step_clock & 15) != 0 ? 1 : 0;
  }
  return 1;
}

void beat_ghosts() {
  u8 half = step_clock & 1;
  if (phase != PhasePlay) {
    return;
  }
  ghost_anim++;
  if (half) {
    tick_mode_clock();
    if (fright_t > 0) {
      fright_t--;
      if (fright_t == 0) {
        for (pm_g = 0; pm_g < PM_GHOSTS; pm_g++) {
          if (ghost_mode[pm_g] == GhostFright) {
            ghost_mode[pm_g] = global_mode;
          }
        }
      }
    }
  }
  for (pm_g = 0; pm_g < PM_GHOSTS; pm_g++) {
    ghost_load();
    if (half && gwait > 0) {
      gwait--;
      ghost_wait[pm_g] = gwait;
    }
    if (ghost_due()) {
      step_ghost();
      if (phase != PhasePlay) {
        return;
      }
      if (gmode == GhostEyes) {
        step_ghost();
        if (phase != PhasePlay) {
          return;
        }
      }
    }
  }
}

static void play_frame() {
  step_clock++;
  if (jobs_ok) {
    pac_job.h.resume();
    ghost_job.h.resume();
  } else {
    beat_input();
    beat_pac();
    beat_ghosts();
  }
}

static void quit() {
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  bbc::cls();
  bbc::cursor(1);
  bbc_return();
}

u8 tick() {
  frame_wait();
  if (phase == PhasePlay) {
    refresh_actors();
  }
  if (hud_dirty) {
    present();
  }

  if (frames_played < 255) {
    frames_played++;
  }
  keys_read();
  if (frames_played > Pace::EscapeGuard && (key_bits & KeyEscape) != 0) {
    quit();
  }

  if (phase == PhaseReady) {
    ready_step();
  } else if (phase == PhasePlay) {
    play_frame();
  } else if (phase == PhaseDying) {
    dying();
  } else if (phase == PhaseWave) {
    if (wave_t == Pace::Wave) {
      Hud::banner("WAVE CLEAR!", ColYellow, 11);
    }
    if (wave_t > 0) {
      wave_t--;
    }
    if (wave_t == 0) {
      wave++;
      hud_dirty = 1;
      start_life(1);
    }
  } else if (phase == PhaseOver) {
    if (over_t > 0) {
      over_t--;
    } else if ((key_bits & KeyFire) != 0) {
      return 1;
    }
  }
  return 0;
}

static void session() {
  score = 0;
  lives = 3;
  wave = 1;
  frames_played = 0;
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  start_life(1);
  play_used = 0;
  pac_job = pac_coro();
  ghost_job = ghost_coro();
  jobs_ok = pac_job.h && ghost_job.h;
  while (tick() == 0) {
  }
  jobs_ok = 0;
  pac_job = Play{};
  ghost_job = Play{};
}

void run() {
  u8 i;
  set_game_palette();
  bbc::colour(128);
  bbc::cursor(0);
  rng_state = 1;
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  for (i = 0; i < 3; i++) {
    bbc::osword(bbc::osword_command::envelope,
                const_cast<Envelope*>(&envelopes[i]));
  }
  for (;;) {
    session();
  }
}

}  // namespace Game

int main() {
  bbc::mode(130);
  bbc::osbyte(bbc::osbyte_command::escape_status, 1, 0);
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  note_acccon();
  set_game_palette();
  bbc::colour(128);
  Title_show();
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  Game::run();
  return 0;
}
