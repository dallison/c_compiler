// Galaga for the BBC Master, linked with the sideways libc.
//
// Shadow Mode 2 (VDU 22,130): 160x256, eight steady colours. A five-rank
// convoy sways in formation — yellow/green bosses, red/magenta butterflies,
// blue/cyan bees — and attackers peel off and dive at the fighter, faster and
// in greater numbers than Galaxians (up to four divers, dual lasers, quicker
// bombs). galaga_fx.s (below &3000) does the per-frame work: keys, cannon,
// sway, divers, lasers, bombs, and collisions. This file keeps the rules,
// timers, scoring, and coroutine play loop.
//
// Z X, A D, or the arrows move. Space or Return fires. Escape returns.

#include <bbc.h>
#include <coroutine>
#include <stddef.h>

#include "galaga.h"

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

enum { PhaseTitle = 1, PhasePlay = 2, PhaseDying = 3, PhaseWave = 4, PhaseOver = 5 };
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

class Metrics {
 public:
  enum : unsigned {
    FleetCol = 16,
    FleetRow = 5,
    PlayerX = 76,
    LineY = LINE_Y,
    GroundY = GROUND_Y
  };
};

class Pace {
 public:
  enum : unsigned {
    Title = 240,
    TitleStep = 25,
    FirstDive = 70,
    Dive = 50,
    DiveFew = 28,
    DiveFast = 10,
    FirstBomb = 40,
    Bomb = 30,
    BombFew = 18,
    Boom = 12,
    Death = 100,
    Wave = 100,
    Over = 100,
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
void fx_wipe();
void fx_blank();
}

class Screen {
 public:
  static void sprite(u8 col, u8 row, const unsigned char* image);
};

extern "C" {
void frame_wait();
void keys_read();
void hud_digits();
void hud_lines();
void player_step();
void cannon_draw();
void cannon_image();
void cannon_erase();
void fleet_sway();
void fleet_bounds();
void dive_launch();
void divers_step();
void divers_clear();
void diver_kill();
void fall_step();
void shot_step();
void bomb_drop();
void bombs_step();
void sound_boom();
void sound_death();
void sound_dive();
extern const unsigned char k_ga_boss0[];
extern const unsigned char k_ga_boss1[];
extern const unsigned char k_ga_fly0[];
extern const unsigned char k_ga_fly1[];
extern const unsigned char k_ga_bee0[];
extern const unsigned char k_ga_bee1[];
extern const unsigned char k_ga_ship[];
extern const unsigned char k_ga_ship_red[];
extern const unsigned char k_ga_blast[];
extern const unsigned char k_ga_boom[];
extern const unsigned char k_ga_bomb0[];
extern const unsigned char k_ga_bomb1[];
extern const unsigned char k_ga_digits[];
}

enum { KeyLeft = KEY_LEFT, KeyRight = KEY_RIGHT, KeyFire = KEY_FIRE, KeyEscape = KEY_ESCAPE };
enum { ShotFlying = 0, ShotFleet = 1, ShotDiver = 2, ShotSpent = 3 };

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
enum { EnvShot = 1, EnvBoom = 2, EnvDeath = 3, EnvDive = 4 };

static const Envelope envelopes[4] = {
    {EnvShot, NoRepeat | 1, {-6, 0, 0}, {15, 0, 0}, 126, -6, 0, -126, 110, 0},
    {EnvBoom, NoRepeat | 1, {0, 0, 0}, {0, 0, 0}, 126, -5, 0, -126, 120, 0},
    {EnvDeath, NoRepeat | 2, {0, 0, 0}, {0, 0, 0}, 126, -2, 0, -126, 126, 0},
    {EnvDive, NoRepeat | 1, {3, -2, 0}, {8, 8, 0}, 126, -3, 0, -126, 100, 40}};

extern "C" {
u8 key_bits;

u8 alive;
u8 fl_on[FLEET_SIZE];
u8 fl_col;
u8 fl_row;
u8 fl_old_col;
u8 fl_old_row;
u8 fl_mover;
u8 fl_dir;
u8 fl_frame;
u8 fl_lo;
u8 fl_hi;

u8 cannon_x;
u8 fire_want;

u8 shot_i;
u8 shot_on[SHOTS];
u8 shot_col[SHOTS];
u8 shot_y[SHOTS];
u8 shot_mask[SHOTS];
u8 shot_event;
u8 shot_hit_col;
u8 shot_hit_row;
u8 shot_hit_rank;

u8 bomb_on[BOMBS];
u8 bomb_col[BOMBS];
u8 bomb_y[BOMBS];
u8 bomb_anim[BOMBS];
u8 bomb_hit;
u8 drop_col;

u8 dv_on[DIVERS];
u8 dv_idx[DIVERS];
u8 dv_x[DIVERS];
u8 dv_y[DIVERS];
i8 dv_vx[DIVERS];
i8 dv_vy[DIVERS];
u8 dv_phase[DIVERS];
u8 dive_pick;
u8 dive_event;
u8 dive_hit;
u8 dive_kill_slot;
u8 dive_kill_rank;

// Tractor beam / capture / dual fighter.
u8 dual;              // 1 while the player has two joined ships
u8 capt_on;           // 1 while a boss is holding a captured ship
u8 capt_idx;          // formation slot of that boss
u8 dv_beam[DIVERS];   // per-diver: this dive tractor-beams
u8 beam_t;            // beam dwell countdown (0 => no beam active)
u8 capt_event;        // asm -> C++: the player was just captured
u8 dual_hit;          // asm -> C++: a dual ship was just demoted
u8 fall_on;           // 1 while a freed captive is falling
u8 fall_x;
u8 fall_y;
u8 rescue_event;      // asm -> C++: the falling captive was caught

extern const Note snd_shot = {Flush | 2, EnvShot, 180, 4};
extern const Note snd_boom = {Flush | 0, EnvBoom, 5, 5};
extern const Note snd_death = {Flush | 0, EnvDeath, 6, 30};
extern const Note snd_dive = {Flush | 1, EnvDive, 140, 8};

extern const u8 idx_rank[FLEET_SIZE] = {
    0, 0, 0, 0, 0, 0, 0, 0,  //
    1, 1, 1, 1, 1, 1, 1, 1,  //
    2, 2, 2, 2, 2, 2, 2, 2,  //
    3, 3, 3, 3, 3, 3, 3, 3,  //
    4, 4, 4, 4, 4, 4, 4, 4};
extern const u8 idx_col5[FLEET_SIZE] = {
    0, 5, 10, 15, 20, 25, 30, 35,  //
    0, 5, 10, 15, 20, 25, 30, 35,  //
    0, 5, 10, 15, 20, 25, 30, 35,  //
    0, 5, 10, 15, 20, 25, 30, 35,  //
    0, 5, 10, 15, 20, 25, 30, 35};
// rank * 2 + frame -> sprite (pointer words for the asm).
extern const unsigned char* const alien_spr[10] = {
    k_ga_boss0, k_ga_boss1, k_ga_fly0, k_ga_fly1, k_ga_fly0,
    k_ga_fly1,  k_ga_bee0,  k_ga_bee1, k_ga_bee0, k_ga_bee1};
extern const unsigned char* const bomb_pat[2] = {k_ga_bomb0, k_ga_bomb1};
extern const u8 key_codes[KEY_COUNT] = {0x61, 0x41, 0x19, 0x42, 0x32,
                                        0x79, 0x62, 0x49, 0x70};
extern const u8 key_masks[KEY_COUNT] = {KeyLeft,  KeyLeft,  KeyLeft,
                                        KeyRight, KeyRight, KeyRight,
                                        KeyFire,  KeyFire,  KeyEscape};
extern const u16 hud_places[4] = {1000, 100, 10, 1};
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
  enum : unsigned { TableCol = 12, TableRow = 12 };
  static void table(u8 look);
};

class Fleet {
 public:
  static void reset();
  static void draw();
};

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
static u8 hud_dirty;
static u8 frames_played;
static u8 dive_t;
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

void Screen::sprite(u8 col, u8 row, const unsigned char* image) {
  fx_col = col;
  fx_row = row;
  fx_src = image;
  fx_len = 48;
  fx_cell();
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
  bbc::colour(ColYellow);
  bbc::tab(0, 0);
  Text::write("SC");
  bbc::colour(ColCyan);
  bbc::tab(9, 0);
  Text::write("WV");
  bbc::colour(ColGreen);
  bbc::tab(15, 0);
  Text::write("LV");
}

void Hud::banner(const char* line, u8 colour, u8 row) {
  bbc::colour(128);
  Text::centre(row, colour, line);
}

void Fleet::reset() {
  u8 i;
  for (i = 0; i < FLEET_SIZE; i++) {
    fl_on[i] = 1;
  }
  // Classic-ish: only four bosses in the centre of rank 0.
  fl_on[0] = 0;
  fl_on[1] = 0;
  fl_on[6] = 0;
  fl_on[7] = 0;
  alive = FLEET_SIZE - 4;
  fl_col = Metrics::FleetCol;
  fl_old_col = Metrics::FleetCol;
  fl_old_row = fl_row;
  fl_dir = 1;
  fl_frame = 0;
  fl_lo = 2;
  fl_hi = 5;
  fl_mover = 255;
  for (i = 0; i < DIVERS; i++) {
    dv_on[i] = 0;
    dv_beam[i] = 0;
  }
}

void Fleet::draw() {
  fl_old_col = fl_col;
  fl_old_row = fl_row;
  fl_mover = 0;
  while (fl_mover < FLEET_SIZE) {
    fleet_sway();
  }
}

void Volley::reset() {
  u8 b;
  fire_want = 0;
  for (b = 0; b < SHOTS; b++) {
    shot_on[b] = 0;
  }
  for (b = 0; b < BOMBS; b++) {
    bomb_on[b] = 0;
  }
}

void Volley::clear() {
  u8 b;
  for (b = 0; b < SHOTS; b++) {
    if (shot_on[b] != 0) {
      shot_on[b] = 0;
      fx_col = shot_col[b];
      fx_y = shot_y[b];
      fx_len = SHOT_LEN;
      fx_blank();
    }
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
  divers_clear();
}

struct ScoreLine {
  const unsigned char* look[2];
  const char* points;
  u8 colour;
};

static const ScoreLine score_table[3] = {
    {{k_ga_boss0, k_ga_boss1}, "150 / 400", ColGreen},
    {{k_ga_fly0, k_ga_fly1}, "80 / 160", ColRed},
    {{k_ga_bee0, k_ga_bee1}, "50 / 100", ColCyan}};

void Title::table(u8 look) {
  u8 i;
  for (i = 0; i < 3; i++) {
    Screen::sprite(TableCol, static_cast<u8>(TableRow + i * 3),
                   score_table[i].look[look]);
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
  Text::centre(2, ColYellow, "DAVECC GALAGA");
  Text::centre(5, ColCyan, "RESCUE = DUAL SHIP");
  table(look);
  for (i = 0; i < 3; i++) {
    bbc::colour(score_table[i].colour);
    bbc::tab(9, static_cast<u8>(TableRow + i * 3));
    Text::write(score_table[i].points);
  }
  Text::centre(24, ColGreen, "Z X OR ARROWS MOVE");
  Text::centre(26, ColGreen, "RETURN/SPACE FIRE");
  Text::centre(27, ColGreen, "ESCAPE QUITS");
  Text::centre(30, ColMagenta, "C++20 DAVECC");
  Text::centre(31, ColMagenta, "BY DAVE ALLISON");
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  for (frames = 0; frames < Pace::Title; frames++) {
    frame_wait();
    keys_read();
    if ((key_bits & KeyFire) == 0) {
      armed = 1;
    } else if (armed) {
      break;
    }
    step_t--;
    if (step_t == 0) {
      step_t = Pace::TitleStep;
      look ^= 1;
      table(look);
    }
  }
  return 1;
}

namespace Game {

static void add_score(u16 points) {
  u16 next = static_cast<u16>(score + points);
  score = next < score ? 65535u : next;
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

static u16 formation_points(u8 rank) {
  static const u16 pts[5] = {150, 80, 80, 50, 50};
  return pts[rank];
}

static u16 diving_points(u8 rank) {
  static const u16 pts[5] = {400, 160, 160, 100, 100};
  return pts[rank];
}

static void deploy() {
  u8 drop = static_cast<u8>(wave - 1);
  if (drop > 4) {
    drop = 4;
  }
  phase = PhasePlay;
  Volley::reset();
  boom_t = 0;
  // A new wave drops any beam/capture/fall in flight but keeps the dual ship.
  capt_on = 0;
  capt_event = 0;
  rescue_event = 0;
  dual_hit = 0;
  beam_t = 0;
  fall_on = 0;
  fl_row = static_cast<u8>(Metrics::FleetRow + drop);
  Fleet::reset();
  dive_t = Pace::FirstDive;
  bomb_t = Pace::FirstBomb;
  bbc::cls();
  bbc::cursor(0);
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  hud_lines();
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
  show_cannon(k_ga_blast);
  sound_death();
}

static void dying() {
  if ((death_t & 7) == 0) {
    show_cannon((death_t & 8) != 0 ? k_ga_ship_red : k_ga_blast);
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
    Hud::banner("GAME OVER", ColRed, 14);
    Text::centre(17, ColWhite, "PRESS SPACE");
    return;
  }
  cannon_x = Metrics::PlayerX;
  Volley::reset();
  phase = PhasePlay;
  cannon_draw();
}

static void fleet_hit() {
  add_score(formation_points(shot_hit_rank));
  clear_boom();
  // The 4-col boom sits one column in from the 6-col window origin.
  boom_col = static_cast<u8>(shot_hit_col + 1);
  boom_row = shot_hit_row;
  boom_t = Pace::Boom;
  fx_col = boom_col;
  fx_row = boom_row;
  fx_src = k_ga_boom;
  fx_len = 32;
  fx_cell();
  sound_boom();
  fleet_bounds();
  if (alive == 0) {
    phase = PhaseWave;
    wave_t = Pace::Wave;
  }
}

static void diver_hit() {
  add_score(diving_points(dive_kill_rank));
  clear_boom();
  boom_col = shot_col[shot_i];
  boom_row = static_cast<u8>(shot_y[shot_i] >> 3);
  boom_t = Pace::Boom;
  fx_col = boom_col;
  fx_row = boom_row;
  fx_src = k_ga_boom;
  fx_len = 32;
  fx_cell();
  sound_boom();
}

static void note_alive_after_diver_kill() {
  if (alive > 0) {
    alive--;
  }
  fleet_bounds();
  if (alive == 0) {
    phase = PhaseWave;
    wave_t = Pace::Wave;
  }
}

static void launch_dive() {
  u8 tries;
  u8 i;
  dive_t = Pace::Dive;
  if (alive < 12) {
    dive_t = Pace::DiveFew;
  }
  if (wave > 2 && dive_t > Pace::DiveFast) {
    dive_t = static_cast<u8>(dive_t - Pace::DiveFast);
  }
  // When nobody is held, sometimes send a boss (rank 0, slots 2..5) so it can
  // open its tractor beam.
  if (capt_on == 0 && (rng_next() & 3) == 0) {
    for (i = 2; i <= 5; i++) {
      if (fl_on[i] != 0) {
        dive_pick = i;
        dive_launch();
        if (dive_event) {
          sound_dive();
          return;
        }
      }
    }
  }
  for (tries = 0; tries < 16; tries++) {
    i = static_cast<u8>((rng_next() >> 8) % FLEET_SIZE);
    // Never dive the captor: the captive is only shown while it sits in
    // formation under its boss.
    if (capt_on != 0 && i == capt_idx) {
      continue;
    }
    if (fl_on[i] != 0) {
      dive_pick = i;
      dive_launch();
      if (dive_event) {
        sound_dive();
        return;
      }
    }
  }
}

static void launch_bomb() {
  bomb_t = Pace::Bomb;
  if (alive < 12) {
    bomb_t = Pace::BombFew;
  }
  drop_col = static_cast<u8>((rng_next() >> 8) & 7);
  bomb_drop();
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
  for (shot_i = 0; shot_i < SHOTS; shot_i++) {
    u8 was_capt = capt_on;
    shot_step();
    if (shot_event == ShotFleet) {
      fleet_hit();
      // Shooting the captor frees the captive (asm clears capt_on).
      if (was_capt && capt_on == 0) {
        add_score(500);
      }
    } else if (shot_event == ShotDiver) {
      note_alive_after_diver_kill();
      diver_hit();
    }
  }
}

void beat_hazards() {
  if (phase != PhasePlay) {
    return;
  }
  // Advance a freed captive; if it reaches the cannon the asm sets dual=1.
  fall_step();
  if (rescue_event) {
    rescue_event = 0;
    add_score(1000);
    sound_dive();
  }
  divers_step();
  // A completed beam capture costs a life but leaves the captor holding us.
  if (capt_event) {
    capt_event = 0;
    kill_player();
    return;
  }
  if (dive_hit) {
    kill_player();
    return;
  }
  bombs_step();
  if (bomb_hit) {
    kill_player();
    return;
  }
  // A hit while dual only demotes back to a single ship.
  if (dual_hit) {
    dual_hit = 0;
    sound_boom();
  }
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
  fleet_sway();
  if (dive_t > 0) {
    dive_t--;
  } else {
    launch_dive();
  }
  if (bomb_t > 0) {
    bomb_t--;
  } else {
    launch_bomb();
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
      Hud::banner("GET READY", ColWhite, 16);
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
  frames_played = 0;
  dual = 0;
  capt_on = 0;
  fall_on = 0;
  beam_t = 0;
  capt_event = 0;
  rescue_event = 0;
  dual_hit = 0;
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

static void palette() {
  for (u8 c = 0; c < 8; c++) {
    bbc::palette(c, c);
  }
}

void run() {
  u8 i;
  palette();
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
  bbc::mode(130);
  bbc::osbyte(bbc::osbyte_command::escape_status, 1, 0);
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  note_acccon();
  for (u8 c = 0; c < 8; c++) {
    bbc::palette(c, c);
  }
  bbc::colour(128);
  Title::show();
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  Game::run();
  return 0;
}
