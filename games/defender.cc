// Defender (Planetoid style) for the BBC Master, linked with the sideways
// libc.
//
// Shadow Mode 2 (VDU 22,130): 160x256, eight steady colours. The world is
// 512 units round (128 byte columns, 1-pixel Mode-2 sub-column phase); the
// delta fighter stays near the centre of the viewport while parallax stars,
// shaded volcanic mountains, landers, mutants, bombers, humans, and twinkling
// star-mines scroll past at 50 Hz. Landers hunt humans and carry them to the
// top of the sky where they mutate; shoot the lander to drop the human and
// catch him before he hits the ground.
//
// defender_fx.s (below &3000) does all per-frame work: keys, ship, expanding
// streaking laser beams, entities, parallax stars, terrain, and the framed
// colour radar scanner. This file keeps the rules, wave setup, scoring,
// title showcase, and C++20 coroutine play loop.

#include <bbc.h>
#include <coroutine>
#include <stddef.h>

#include "defender.h"

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
u8 smart_bombs;
u8 hud_dirty;
}

enum {
  PhaseTitle = 1,
  PhasePlay = 2,
  PhaseDying = 3,
  PhaseWave = 4,
  PhaseOver = 5
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
    TitleStep = 12,
    Spawn = 100,
    FirstSpawn = 150,
    Bomb = 70,
    Death = 96,
    Wave = 100,
    Over = 100,
    Flash = 3,
    EscapeGuard = 30
  };
};

extern "C" {
extern u8 fx_col;
extern u8 fx_row;
extern u8 fx_y;
extern u8 fx_len;
extern const unsigned char* fx_src;
void fx_cell();
void fx_strip();
void fx_sprite();
void title_beam();
}

extern "C" {
void frame_wait();
void keys_read();
void hud_digits();
void playfield_frame();
void ship_step();
void ship_draw();
void ship_paint();
void ship_erase();
void cam_update();
void shots_reset();
void shots_clear();
void shots_step();
void ents_reset();
void ents_step();
void smart_bomb();
void bomb_drop();
void stars_paint();
void terrain_reset();
void terrain_paint();
void scanner_paint();
void sound_boom();
void sound_death();
void sound_shot();
void sound_bomb();

extern const unsigned char k_df_ship_r0[];
extern const unsigned char k_df_ship_r1[];
extern const unsigned char k_df_ship_r2[];
extern const unsigned char k_df_lander0[];
extern const unsigned char k_df_lander1[];
extern const unsigned char k_df_mutant0[];
extern const unsigned char k_df_mutant1[];
extern const unsigned char k_df_bomber0[];
extern const unsigned char k_df_bomber1[];
extern const unsigned char k_df_human0[];
extern const unsigned char k_df_human1[];
extern const unsigned char k_df_bomb0[];
extern const unsigned char k_df_bomb1[];
extern const unsigned char k_df_boom0[];
extern const unsigned char k_df_boom1[];
}

enum {
  KeyThrust = KEY_THRUST,
  KeyReverse = KEY_REVERSE,
  KeyUp = KEY_UP,
  KeyDown = KEY_DOWN,
  KeyFire = KEY_FIRE,
  KeyEscape = KEY_ESCAPE,
  KeyBomb = KEY_BOMB,
  KeyHyper = KEY_HYPER
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

enum { NoRepeat = 0x80 };
enum { EnvShot = 1, EnvBoom = 2, EnvDeath = 3, EnvBomb = 4 };

// Sweeping Planetoid-style laser zap + deep explosion envelopes.
static const Envelope envelopes[4] = {
    {EnvShot, NoRepeat | 1, {-5, -10, -16}, {3, 4, 6}, 126, -8, 0, -126, 124, 0},
    {EnvBoom, NoRepeat | 1, {0, -1, 0}, {4, 8, 0}, 126, -5, 0, -126, 122, 0},
    {EnvDeath, NoRepeat | 2, {0, 0, 0}, {0, 0, 0}, 126, -2, 0, -126, 126, 0},
    {EnvBomb, NoRepeat | 1, {4, -3, -6}, {6, 8, 8}, 126, -3, 0, -126, 112, 48}};

extern "C" {
u8 key_bits;

// State owned by defender_fx.s below &3000.
extern u8 e_type[DF_ENTS];
extern u8 e_xl[DF_ENTS];
extern u8 e_xh[DF_ENTS];
extern u8 e_y[DF_ENTS];
extern u8 e_vx[DF_ENTS];
extern u8 e_vy[DF_ENTS];
extern u8 e_st[DF_ENTS];
extern u8 e_aux[DF_ENTS];
extern u8 e_boom[DF_ENTS];
extern u8 e_shown[DF_ENTS];
extern u8 terr_top[DF_TERR_COLS];
extern u8 terr_len[DF_TERR_COLS];
extern u8 ship_xl;
extern u8 ship_xh;
extern u8 ship_y;
extern u8 ship_vx;
extern u8 ship_face;
extern u8 ship_on;
extern u8 ship_inv;
extern u8 ship_hit;
extern u8 ship_shown;
extern const unsigned char* ship_src;
extern u8 rev_held;
extern u8 bomb_req;
extern u8 hyper_req;
extern u8 t_force;
extern u8 hostiles;
extern u8 humans_left;
extern u8 rnd;
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

class Title {
 public:
  static u8 show();

 private:
  static void draw_cast(u8 anim);
};

namespace Game {
void run();
u8 tick();
void beat_player();
void beat_shots();
void beat_enemies();
void beat_world();
}  // namespace Game

static u16 rng_state;
static u8 frames_played;
static u8 spawn_t;
static u8 bomb_t;
static u8 death_t;
static u8 wave_t;
static u8 over_t;
static u8 flash_t;
static u8 quota;
static u8 terrain[DF_TERR_COLS];

static Play player_job;
static Play shots_job;
static Play enemy_job;
static Play world_job;
static u8 jobs_ok;

Play player_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_player();
  }
}

Play laser_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_shots();
  }
}

Play enemy_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_enemies();
  }
}

Play world_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_world();
  }
}

static void init_palette() {
  u8 c;
  for (c = 0; c < 8; c++) {
    bbc::palette(c, c);
  }
  bbc::colour(128);
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

static u16 rng_next() {
  rng_state = static_cast<unsigned>(rng_state + (rng_state << 5) + 13849);
  return rng_state;
}

void Hud::labels() {
  bbc::colour(128);
  bbc::colour(ColCyan);
  bbc::tab(0, 0);
  Text::write("SHIPS");
  bbc::tab(15, 0);
  Text::write("BOMBS");
  bbc::tab(15, 2);
  Text::write("WV");
}

void Hud::banner(const char* line, u8 colour, u8 row) {
  bbc::colour(128);
  Text::centre(row, colour, line);
}

void Title::draw_cast(u8 anim) {
  static const unsigned char* const f0[4] = {k_df_lander0, k_df_mutant0,
                                             k_df_bomber0, k_df_human0};
  static const unsigned char* const f1[4] = {k_df_lander1, k_df_mutant1,
                                             k_df_bomber1, k_df_human1};
  static const u8 cols[4] = {5, 5, 5, 4};
  u8 i;
  // Hero fighter + target blast diorama at y=50.
  fx_col = 14;
  fx_y = 50;
  fx_len = 6;
  fx_src = anim ? k_df_ship_r2 : k_df_ship_r1;
  fx_sprite();

  fx_col = 56;
  fx_y = 50;
  fx_len = 5;
  fx_src = anim ? k_df_boom1 : k_df_boom0;
  fx_sprite();

  fx_col = anim;
  title_beam();

  // Four animated enemy/colonist rows.
  for (i = 0; i < 4; i++) {
    fx_col = 14;
    fx_y = static_cast<u8>(86 + i * 24);
    fx_len = cols[i];
    fx_src = anim ? f1[i] : f0[i];
    fx_sprite();
  }
}

u8 Title::show() {
  static const char* const names[4] = {"LANDER   150", "MUTANT   150",
                                       "BOMBER   250", "HUMANOID 500"};
  static const u8 inks[4] = {ColGreen, ColCyan, ColYellow, ColMagenta};
  u8 frames;
  u8 i;
  u8 anim = 0;
  u8 step_t = Pace::TitleStep;
  u8 armed = 0;

  phase = PhaseTitle;
  bbc::cursor(0);
  bbc::cls();
  Text::centre(3, ColYellow, "DAVECC  DEFENDER");

  draw_cast(0);
  for (i = 0; i < 4; i++) {
    bbc::colour(inks[i]);
    bbc::tab(6, static_cast<u8>(11 + i * 3));
    Text::write(names[i]);
  }

  Text::centre(23, ColWhite, "A/Z UP DN  SHF THRST");
  Text::centre(24, ColCyan, "SPC REV     RET FIRE");
  Text::centre(25, ColGreen, "TAB BOMB    H HYPER");
  Text::centre(28, ColYellow, "C++20 DAVECC  MASTER");
  Text::centre(29, ColRed, "BY DAVE ALLISON");
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);

  for (frames = 0; frames < Pace::Title; frames++) {
    frame_wait();
    keys_read();
    if ((key_bits & (KeyFire | KeyReverse)) == 0) {
      armed = 1;
    } else if (armed) {
      break;
    }
    step_t--;
    if (step_t == 0) {
      step_t = Pace::TitleStep;
      anim ^= 1;
      draw_cast(anim);
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

static u8 ground_y(u16 x) {
  u8 c = static_cast<u8>((x >> 2) & 127);
  u8 a = terr_top[c];
  u8 b = terr_top[static_cast<u8>((c + 1) & 127)];
  u8 d = terr_top[static_cast<u8>((c + 2) & 127)];
  if (b < a) {
    a = b;
  }
  if (d < a) {
    a = d;
  }
  return static_cast<u8>(a - DF_SPR_H);
}

// Sculpted craggy mountain ridge: peaks every 8 columns, shaded 5..8 rows deep.
static void make_terrain() {
  u8 ctl[17];
  u8 s;
  u8 k;
  u8 i;
  for (s = 0; s < 16; s++) {
    ctl[s] = static_cast<u8>(DF_TERR_MIN + ((rng_next() >> 8) % 24));
  }
  ctl[16] = ctl[0];
  for (s = 0; s < 16; s++) {
    int a = ctl[s];
    int b = ctl[s + 1];
    for (k = 0; k < 8; k++) {
      terrain[s * 8 + k] = static_cast<u8>(a + ((b - a) * k) / 8);
    }
  }
  for (i = 0; i < DF_TERR_COLS; i++) {
    u8 a = terrain[i];
    u8 b = terrain[static_cast<u8>((i + 1) & 127)];
    u8 top = a < b ? a : b;
    u8 span = static_cast<u8>((a < b ? (b - a) : (a - b)) + 5);
    if (static_cast<unsigned>(top + span) > 253) {
      span = static_cast<u8>(253 - top);
    }
    terr_top[i] = top;
    terr_len[i] = span;
  }
}

static void place(u8 slot, u8 type, u16 x, u8 y) {
  e_type[slot] = type;
  e_xl[slot] = static_cast<u8>(x & 255);
  e_xh[slot] = static_cast<u8>((x >> 8) & 1);
  e_y[slot] = y;
  e_vx[slot] = 1;
  e_vy[slot] = 1;
  e_st[slot] = 0;
  e_aux[slot] = 0;
  e_boom[slot] = 0;
  e_shown[slot] = 0;
}

static void spawn_lander() {
  u8 i;
  for (i = DF_FIRST_ENEMY; i < DF_FIRST_ENEMY + DF_N_ENEMY; i++) {
    if (e_type[i] == 0) {
      u16 x = static_cast<unsigned>(ship_xl + (ship_xh << 8) + 192 + (rnd & 127)) & 511;
      place(i, ET_LANDER, x, 42);
      e_aux[i] = 0xff;
      quota--;
      return;
    }
  }
}

static void deploy() {
  u8 i;
  u8 n;
  phase = PhasePlay;
  bbc::cls();
  bbc::cursor(0);
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  shots_reset();
  ents_reset();
  make_terrain();
  terrain_reset();
  for (i = 0; i < DF_N_HUMAN; i++) {
    u16 x = static_cast<unsigned>(i * 64 + 8 + ((rng_next() >> 8) & 31));
    place(static_cast<u8>(DF_FIRST_HUMAN + i), ET_HUMAN, x, ground_y(x));
    e_vx[DF_FIRST_HUMAN + i] = (rng_next() & 0x100) != 0 ? 1 : 0xff;
  }
  n = static_cast<u8>(wave + 1);
  if (n > 4) {
    n = 4;
  }
  for (i = 0; i < n; i++) {
    u16 x = static_cast<unsigned>((380 + i * 97 + ((rng_next() >> 8) & 63)) & 511);
    place(i, ET_LANDER, x, static_cast<u8>(42 + i * 8));
    e_aux[i] = 0xff;
  }
  if (wave > 1) {
    place(DF_FIRST_BOMBER, ET_BOMBER, 100, 76);
  }
  if (wave > 3) {
    place(DF_FIRST_BOMBER + 1, ET_BOMBER, 330, 104);
    e_vx[DF_FIRST_BOMBER + 1] = 0xff;
  }
  quota = static_cast<u8>(wave * 2 + 2);
  if (quota > 14) {
    quota = 14;
  }
  spawn_t = Pace::FirstSpawn;
  bomb_t = Pace::Bomb;
  ship_xl = 0;
  ship_xh = 1;
  ship_y = 120;
  ship_vx = 0;
  ship_face = 0;
  ship_on = 1;
  ship_inv = 45;
  ship_hit = 0;
  ship_shown = 0;
  rev_held = 1;
  bomb_req = 0;
  hyper_req = 0;
  cam_update();
  Hud::labels();
  present();
  playfield_frame();
  terrain_paint();
  stars_paint();
  ship_draw();
}

static void kill_player() {
  if (phase != PhasePlay) {
    return;
  }
  shots_clear();
  phase = PhaseDying;
  death_t = Pace::Death;
  ship_on = 0;
  ship_hit = 0;
  if (lives > 0) {
    lives--;
  }
  hud_dirty = 1;
  ship_src = k_df_boom0;
  ship_paint();
  sound_death();
}

static void dying() {
  if ((death_t & 7) == 0) {
    ship_src = (death_t & 8) != 0 ? k_df_boom1 : k_df_boom0;
    ship_paint();
  }
  if (death_t > 0) {
    death_t--;
  }
  if (death_t != 0) {
    return;
  }
  ship_erase();
  if (lives == 0) {
    phase = PhaseOver;
    over_t = Pace::Over;
    Hud::banner("GAME OVER", ColRed, 12);
    Text::centre(15, ColYellow, "PRESS RETURN");
    return;
  }
  ship_y = 84;
  ship_vx = 0;
  ship_on = 1;
  ship_inv = DF_INVULN;
  ship_hit = 0;
  phase = PhasePlay;
  ship_draw();
}

void beat_player() {
  if (phase != PhasePlay) {
    return;
  }
  ship_step();
  if (ship_hit) {
    kill_player();
    return;
  }
  if (bomb_req) {
    bomb_req = 0;
    if (smart_bombs > 0) {
      smart_bombs--;
      hud_dirty = 1;
      smart_bomb();
      flash_t = Pace::Flash;
      bbc::palette(0, ColWhite);
      sound_bomb();
    }
  }
  if (hyper_req) {
    u16 r = rng_next();
    hyper_req = 0;
    shots_clear();
    ship_erase();
    ship_xl = static_cast<u8>(r & 255);
    ship_xh = static_cast<u8>((r >> 8) & 1);
    ship_y = static_cast<u8>(44 + ((r >> 9) & 127));
    ship_vx = 0;
    cam_update();
    t_force = 1;
    flash_t = 2;
    bbc::palette(0, ColBlue);
    sound_bomb();
    ship_draw();
  }
}

void beat_shots() {
  if (phase != PhasePlay) {
    return;
  }
  shots_step();
}

void beat_enemies() {
  if (phase != PhasePlay) {
    return;
  }
  ents_step();
  if (ship_hit) {
    kill_player();
    return;
  }
  if (hostiles == 0 && quota == 0) {
    add_score(static_cast<u16>(humans_left * 100));
    phase = PhaseWave;
    wave_t = Pace::Wave;
  }
}

void beat_world() {
  if (phase != PhasePlay) {
    return;
  }
  terrain_paint();
  stars_paint();
  scanner_paint();
  if (spawn_t > 0) {
    spawn_t--;
  } else {
    spawn_t = Pace::Spawn;
    u8 cap = static_cast<u8>(wave + 2);
    if (cap > 6) {
      cap = 6;
    }
    if (quota > 0 && hostiles < cap) {
      spawn_lander();
    }
  }
  if (bomb_t > 0) {
    bomb_t--;
  } else {
    bomb_t = Pace::Bomb;
    bomb_drop();
  }
}

static void play_frame() {
  if (jobs_ok) {
    player_job.h.resume();
    world_job.h.resume();
    shots_job.h.resume();
    enemy_job.h.resume();
    return;
  }
  beat_player();
  beat_world();
  beat_shots();
  beat_enemies();
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
  if (flash_t > 0) {
    flash_t--;
    if (flash_t == 0) {
      bbc::palette(0, ColBlack);
    }
  }
  if (phase == PhasePlay) {
    play_frame();
  } else if (phase == PhaseDying) {
    dying();
  } else if (phase == PhaseWave) {
    if (wave_t == Pace::Wave) {
      Hud::banner("GET READY", ColYellow, 13);
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
    } else if ((key_bits & (KeyFire | KeyReverse)) != 0) {
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
  smart_bombs = 3;
  frames_played = 0;
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  deploy();
  play_used = 0;
  player_job = player_coro();
  shots_job = laser_coro();
  enemy_job = enemy_coro();
  world_job = world_coro();
  jobs_ok = player_job.h && shots_job.h && enemy_job.h && world_job.h;
  while (tick() == 0) {
  }
  jobs_ok = 0;
  player_job = Play{};
  shots_job = Play{};
  enemy_job = Play{};
  world_job = Play{};
}

void run() {
  u8 i;
  init_palette();
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
  bbc::mode(130);
  bbc::osbyte(bbc::osbyte_command::escape_status, 1, 0);
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  note_acccon();
  init_palette();
  Title::show();
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  Game::run();
  return 0;
}
