// Snake for the BBC Master, linked with the sideways libc.
//
// Shadow mode 1 (VDU 22,129): 320x256, four colours, HIMEM at &8000.
// Title and play share that mode — do not call mode() twice (it re-parks
// the software stack and returns to BASIC).
//
// screen1.s and snake_fx.s, below &3000, paint the shadow screen and scan
// keys. This file keeps the rules, the board state, and the rest: waves,
// fruits, rocks, scoring, and the coroutines that drive a frame of play.
// A frame is one vsync; the snake steps every few frames so steering stays
// responsive at 50 Hz.
//
// Z X / A D / arrows / WASD steer. Space starts or fires a new life.
// Escape returns to BASIC.
//
// Build: .cursor/skills/build-invaders/scripts/build_and_pack.sh

#include <bbc.h>
#include <coroutine>
#include <stddef.h>

#include "snake.h"

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
u8 level;
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
enum { DirUp = 0, DirRight = 1, DirDown = 2, DirLeft = 3 };
enum { StepMoved = 0, StepAte = 1, StepBonus = 2, StepDied = 3 };

class Pace {
 public:
  enum : unsigned {
    Title = 200,
    TitleBlink = 20,
    Death = 60,
    Wave = 80,
    Over = 80,
    EscapeGuard = 30,
    FruitAnim = 8,
    BonusLife = 200,
    BonusGap = 350
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
  static void fill(int x, int y, int width, int height, int colour);
};

extern "C" {
void frame_wait();
void keys_read();
void hud_digits();
void cell_draw();
void cell_wipe();

extern const unsigned char k_sn_head_u[];
extern const unsigned char k_sn_head_d[];
extern const unsigned char k_sn_head_l[];
extern const unsigned char k_sn_head_r[];
extern const unsigned char k_sn_body[];
extern const unsigned char k_sn_tail[];
extern const unsigned char k_sn_apple0[];
extern const unsigned char k_sn_apple1[];
extern const unsigned char k_sn_bonus0[];
extern const unsigned char k_sn_bonus1[];
extern const unsigned char k_sn_rock[];
extern const unsigned char k_sn_wall[];
extern const unsigned char k_digits[];
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
enum { EnvEat = 1, EnvBonus = 2, EnvDeath = 3 };

static const Envelope envelopes[3] = {
    {EnvEat, NoRepeat | 1, {8, 0, 0}, {4, 0, 0}, 126, -8, 0, -126, 110, 0},
    {EnvBonus, NoRepeat | 1, {12, -4, 0}, {6, 4, 0}, 126, -4, 0, -126, 120, 0},
    {EnvDeath, NoRepeat | 2, {0, 0, 0}, {0, 0, 0}, 126, -2, 0, -126, 126, 0}};

extern "C" {
u8 key_bits;

u8 sn_col;
u8 sn_row;
const unsigned char* sn_src;

u8 snake_col[SN_MAX];
u8 snake_row[SN_MAX];
u8 snake_head;
u8 snake_len;
u8 snake_dir;
u8 snake_next;

u8 fruit_kind[SN_FRUITS];
u8 fruit_col[SN_FRUITS];
u8 fruit_row[SN_FRUITS];
u8 fruit_life[SN_FRUITS];

u8 rock_col[SN_ROCKS];
u8 rock_row[SN_ROCKS];
u8 rock_n;

u8 fruit_look;
u8 move_gap;
u8 move_wait;
u8 anim_wait;
u8 bonus_wait;
u8 ate_count;
u8 frames_played;
u8 death_t;
u8 wave_t;
u8 over_t;
u8 hud_dirty;

extern const u16 hud_places[4] = {1000, 100, 10, 1};

// Z/A/left, X/D/right, cursor-up/*/W, cursor-down/?/S, Space/Return, Escape.
extern const u8 key_codes[KEY_COUNT] = {0x61, 0x41, 0x19, 0x42, 0x32, 0x79,
                                        0x39, 0x21, 0x48, 0x29, 0x51, 0x68,
                                        0x62, 0x49, 0x70};
extern const u8 key_masks[KEY_COUNT] = {
    KeyLeft, KeyLeft,  KeyLeft,  KeyRight, KeyRight, KeyRight,
    KeyUp,   KeyUp,    KeyUp,    KeyDown,  KeyDown,  KeyDown,
    KeyFire, KeyFire,  KeyEscape};
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
void beat_input();
void beat_snake();
void beat_fruit();
}  // namespace Game

static u16 rng_state;
static Play input_job;
static Play snake_job;
static Play fruit_job;
static u8 jobs_ok;

Play input_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_input();
  }
}

Play snake_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_snake();
  }
}

Play fruit_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_fruit();
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

static u16 rng_next() {
  rng_state = static_cast<unsigned>(rng_state + (rng_state << 5) + 13849);
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

static void paint(u8 col, u8 row, const unsigned char* image) {
  sn_col = col;
  sn_row = row;
  sn_src = image;
  cell_draw();
}

static void wipe(u8 col, u8 row) {
  sn_col = col;
  sn_row = row;
  cell_wipe();
}

static const unsigned char* head_image(u8 dir) {
  if (dir == DirUp) {
    return k_sn_head_u;
  }
  if (dir == DirDown) {
    return k_sn_head_d;
  }
  if (dir == DirLeft) {
    return k_sn_head_l;
  }
  return k_sn_head_r;
}

static const unsigned char* fruit_image(u8 kind, u8 look) {
  // Pointer ternaries crash davecc's 6502 codegen (AssignRegisterOrOffset).
  if (kind == FRUIT_BONUS) {
    if (look != 0) {
      return k_sn_bonus1;
    }
    return k_sn_bonus0;
  }
  if (look != 0) {
    return k_sn_apple1;
  }
  return k_sn_apple0;
}

static void sound(const Note* note) {
  bbc::osword(bbc::osword_command::sound, const_cast<Note*>(note));
}

static void sound_eat() {
  static const Note n = {Flush | 2, EnvEat, 160, 3};
  sound(&n);
}

static void sound_bonus() {
  static const Note n = {Flush | 2, EnvBonus, 200, 6};
  sound(&n);
}

static void sound_death() {
  static const Note n = {Flush | 0, EnvDeath, 6, 25};
  sound(&n);
}

static int snake_covers(u8 col, u8 row, u8 ignore_tail) {
  u8 count = snake_len;
  u8 i;
  if (ignore_tail && count > 0) {
    count--;
  }
  for (i = 0; i < count; i++) {
    u8 index = static_cast<u8>((snake_head - i) & (SN_MAX - 1));
    if (snake_col[index] == col && snake_row[index] == row) {
      return 1;
    }
  }
  return 0;
}

static int is_rock(u8 col, u8 row) {
  u8 i;
  for (i = 0; i < rock_n; i++) {
    if (rock_col[i] == col && rock_row[i] == row) {
      return 1;
    }
  }
  return 0;
}

static int occupied(u8 col, u8 row, u8 ignore_tail) {
  return snake_covers(col, row, ignore_tail) || is_rock(col, row);
}

static int fruit_blocks(u8 col, u8 row) {
  u8 f;
  for (f = 0; f < SN_FRUITS; f++) {
    if (fruit_kind[f] != FRUIT_NONE && fruit_col[f] == col &&
        fruit_row[f] == row) {
      return 1;
    }
  }
  return 0;
}

static void place_free(u8* col, u8* row) {
  u8 guard = 0;
  u8 c;
  u8 r;
  do {
    c = static_cast<u8>(1 + (rng_next() & 31));
    r = static_cast<u8>(1 + (rng_next() & 15));
    if (c >= SN_COLS - 1) {
      c = static_cast<u8>(SN_COLS - 2);
    }
    if (r >= SN_ROWS - 1) {
      r = static_cast<u8>(SN_ROWS - 2);
    }
    guard++;
  } while ((occupied(c, r, 0) || fruit_blocks(c, r)) && guard < 48);
  if (!occupied(c, r, 0) && !fruit_blocks(c, r) && c < SN_COLS &&
      r < SN_ROWS) {
    *col = c;
    *row = r;
    return;
  }
  for (r = 1; r < SN_ROWS - 1; r++) {
    for (c = 1; c < SN_COLS - 1; c++) {
      if (!occupied(c, r, 0) && !fruit_blocks(c, r)) {
        *col = c;
        *row = r;
        return;
      }
    }
  }
  *col = SN_COLS / 2;
  *row = SN_ROWS / 2;
}

static void snake_reset() {
  u8 i;
  snake_len = 5;
  snake_head = 4;
  snake_dir = DirRight;
  snake_next = DirRight;
  for (i = 0; i < snake_len; i++) {
    snake_col[i] = static_cast<u8>(6 + i);
    snake_row[i] = SN_ROWS / 2;
  }
}

static void snake_draw() {
  u8 i;
  for (i = snake_len - 1; i > 0; i--) {
    u8 index = static_cast<u8>((snake_head - i) & (SN_MAX - 1));
    if (i + 1 == snake_len) {
      paint(snake_col[index], snake_row[index], k_sn_tail);
    } else {
      paint(snake_col[index], snake_row[index], k_sn_body);
    }
  }
  paint(snake_col[snake_head], snake_row[snake_head], head_image(snake_dir));
}

static void queue_dir(u8 want) {
  if (((snake_dir + 2) & 3) == want) {
    return;
  }
  if (((snake_next + 2) & 3) == want) {
    return;
  }
  snake_next = want;
}

static void sample_keys() {
  if (key_bits & KeyUp) {
    queue_dir(DirUp);
  } else if (key_bits & KeyDown) {
    queue_dir(DirDown);
  } else if (key_bits & KeyLeft) {
    queue_dir(DirLeft);
  } else if (key_bits & KeyRight) {
    queue_dir(DirRight);
  }
}

static u8 fruit_at(u8 col, u8 row) {
  u8 f;
  for (f = 0; f < SN_FRUITS; f++) {
    if (fruit_kind[f] != FRUIT_NONE && fruit_col[f] == col &&
        fruit_row[f] == row) {
      return f;
    }
  }
  return 255;
}

static void spawn_fruit(u8 kind) {
  u8 f;
  for (f = 0; f < SN_FRUITS; f++) {
    if (fruit_kind[f] == FRUIT_NONE) {
      place_free(&fruit_col[f], &fruit_row[f]);
      fruit_kind[f] = kind;
      fruit_life[f] = kind == FRUIT_BONUS ? static_cast<u8>(Pace::BonusLife) : 0;
      paint(fruit_col[f], fruit_row[f], fruit_image(kind, fruit_look));
      return;
    }
  }
}

static void clear_fruits() {
  u8 f;
  for (f = 0; f < SN_FRUITS; f++) {
    fruit_kind[f] = FRUIT_NONE;
    fruit_life[f] = 0;
  }
}

static void draw_fruits() {
  u8 f;
  for (f = 0; f < SN_FRUITS; f++) {
    if (fruit_kind[f] != FRUIT_NONE) {
      paint(fruit_col[f], fruit_row[f],
            fruit_image(fruit_kind[f], fruit_look));
    }
  }
}

static void draw_rocks() {
  u8 i;
  for (i = 0; i < rock_n; i++) {
    paint(rock_col[i], rock_row[i], k_sn_rock);
  }
}

static void draw_border() {
  u8 c;
  u8 r;
  for (c = 0; c < SN_COLS; c++) {
    paint(c, 0, k_sn_wall);
    paint(c, SN_ROWS - 1, k_sn_wall);
  }
  for (r = 1; r < SN_ROWS - 1; r++) {
    paint(0, r, k_sn_wall);
    paint(SN_COLS - 1, r, k_sn_wall);
  }
}

// Border is an edge test; rock_* holds interior obstacles that grow with
// the level. Keep the spawn corridor clear so a new life is fair.
static void build_rocks() {
  u8 extras = static_cast<u8>(2 + level * 2);
  u8 i;
  rock_n = 0;
  if (extras > 16) {
    extras = 16;
  }
  for (i = 0; i < extras && rock_n < SN_ROCKS; i++) {
    u8 c;
    u8 r;
    u8 guard = 0;
    do {
      c = static_cast<u8>(1 + (rng_next() & 31));
      r = static_cast<u8>(1 + (rng_next() & 15));
      if (c >= SN_COLS - 1) {
        c = static_cast<u8>(SN_COLS - 2);
      }
      if (r >= SN_ROWS - 1) {
        r = static_cast<u8>(SN_ROWS - 2);
      }
      guard++;
    } while ((snake_covers(c, r, 0) ||
              (c >= 4 && c <= 12 && r >= SN_ROWS / 2 - 1 &&
               r <= SN_ROWS / 2 + 1)) &&
             guard < 32);
    rock_col[rock_n] = c;
    rock_row[rock_n] = r;
    rock_n++;
  }
}

static int on_edge(u8 col, u8 row) {
  return col == 0 || row == 0 || col == SN_COLS - 1 || row == SN_ROWS - 1;
}

static u8 snake_step() {
  u8 x = snake_col[snake_head];
  u8 y = snake_row[snake_head];
  u8 prev = snake_head;
  u8 slot;
  snake_dir = snake_next;
  if (snake_dir == DirUp) {
    y--;
  } else if (snake_dir == DirRight) {
    x++;
  } else if (snake_dir == DirDown) {
    y++;
  } else {
    x--;
  }
  if (on_edge(x, y) || is_rock(x, y) || snake_covers(x, y, 1)) {
    return StepDied;
  }
  slot = fruit_at(x, y);
  snake_head = static_cast<u8>((snake_head + 1) & (SN_MAX - 1));
  snake_col[snake_head] = x;
  snake_row[snake_head] = y;
  paint(snake_col[prev], snake_row[prev], k_sn_body);
  paint(x, y, head_image(snake_dir));
  if (slot != 255) {
    u8 kind = fruit_kind[slot];
    fruit_kind[slot] = FRUIT_NONE;
    fruit_life[slot] = 0;
    if (snake_len < SN_MAX - 1) {
      snake_len++;
    }
    return kind == FRUIT_BONUS ? StepBonus : StepAte;
  }
  {
    u8 tail = static_cast<u8>((snake_head - snake_len) & (SN_MAX - 1));
    u8 new_tail = static_cast<u8>((snake_head - (snake_len - 1)) & (SN_MAX - 1));
    wipe(snake_col[tail], snake_row[tail]);
    if (snake_len > 1) {
      paint(snake_col[new_tail], snake_row[new_tail], k_sn_tail);
    }
  }
  return StepMoved;
}

static u8 pace_for_score() {
  if (score >= 400) {
    return 2;
  }
  if (score >= 250) {
    return 3;
  }
  if (score >= 120) {
    return 4;
  }
  if (score >= 50) {
    return 5;
  }
  return 6;
}

static void present() {
  hud_digits();
  hud_dirty = 0;
}

static void deploy() {
  u8 n;
  phase = PhasePlay;
  move_gap = pace_for_score();
  move_wait = move_gap;
  anim_wait = Pace::FruitAnim;
  bonus_wait = Pace::BonusGap;
  fruit_look = 0;
  ate_count = 0;
  clear_fruits();
  snake_reset();
  build_rocks();
  bbc::cls();
  bbc::cursor(0);
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  draw_border();
  draw_rocks();
  snake_draw();
  for (n = 0; n < 3; n++) {
    spawn_fruit(FRUIT_APPLE);
  }
  Hud::labels();
  present();
}

u8 Title::show() {
  u8 frames;
  u8 armed = 0;
  u8 blink = Pace::TitleBlink;
  u8 look = 0;
  phase = PhaseTitle;
  bbc::cursor(0);
  bbc::cls();
  Text::centre(3, ColWhite, "DAVECC SNAKE");
  paint(16, 8, k_sn_tail);
  paint(17, 8, k_sn_body);
  paint(18, 8, k_sn_body);
  paint(19, 8, k_sn_body);
  paint(20, 8, k_sn_head_r);
  paint(23, 8, k_sn_apple0);
  paint(26, 8, k_sn_bonus0);
  paint(14, 10, k_sn_rock);
  paint(28, 10, k_sn_rock);
  Text::centre(14, ColGreen, "EAT THE APPLES, DODGE THE ROCKS");
  Text::centre(16, ColGreen, "BONUS FRUIT IS WORTH MORE");
  Text::centre(22, ColWhite, "Z X / WASD / ARROWS STEER");
  Text::centre(23, ColWhite, "SPACE STARTS, ESCAPE QUITS");
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
    blink--;
    if (blink == 0) {
      blink = Pace::TitleBlink;
      look ^= 1;
      if (look != 0) {
        paint(23, 8, k_sn_apple1);
        paint(26, 8, k_sn_bonus1);
      } else {
        paint(23, 8, k_sn_apple0);
        paint(26, 8, k_sn_bonus0);
      }
    }
  }
  return 1;
}

namespace Game {

static void add_score(u16 points) {
  score = static_cast<unsigned>(score + points);
  if (score > best) {
    best = score;
  }
  move_gap = pace_for_score();
  hud_dirty = 1;
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
  paint(snake_col[snake_head], snake_row[snake_head], k_sn_rock);
  sound_death();
}

static void dying() {
  if (death_t > 0) {
    death_t--;
  }
  if (death_t != 0) {
    return;
  }
  if (lives == 0) {
    phase = PhaseOver;
    over_t = Pace::Over;
    Hud::banner("GAME OVER", ColRed, 12);
    Text::centre(15, ColWhite, "PRESS SPACE");
    return;
  }
  deploy();
}

void beat_input() {
  if (phase == PhasePlay) {
    sample_keys();
  }
}

void beat_snake() {
  u8 outcome;
  if (phase != PhasePlay) {
    return;
  }
  if (move_wait > 0) {
    move_wait--;
    return;
  }
  move_wait = move_gap;
  outcome = snake_step();
  if (outcome == StepDied) {
    kill_player();
    return;
  }
  if (outcome == StepAte) {
    add_score(10);
    sound_eat();
    ate_count++;
    spawn_fruit(FRUIT_APPLE);
    if (ate_count >= 8) {
      ate_count = 0;
      phase = PhaseWave;
      wave_t = Pace::Wave;
      Hud::banner("NEXT LEVEL", ColWhite, 14);
    }
  } else if (outcome == StepBonus) {
    add_score(50);
    sound_bonus();
  }
}

void beat_fruit() {
  u8 f;
  if (phase != PhasePlay) {
    return;
  }
  if (anim_wait > 0) {
    anim_wait--;
  } else {
    anim_wait = Pace::FruitAnim;
    fruit_look ^= 1;
    draw_fruits();
  }
  for (f = 0; f < SN_FRUITS; f++) {
    if (fruit_kind[f] == FRUIT_BONUS) {
      if (fruit_life[f] > 0) {
        fruit_life[f]--;
      }
      if (fruit_life[f] == 0) {
        wipe(fruit_col[f], fruit_row[f]);
        fruit_kind[f] = FRUIT_NONE;
      }
    }
  }
  if (bonus_wait > 0) {
    bonus_wait--;
  } else {
    bonus_wait = Pace::BonusGap;
    spawn_fruit(FRUIT_BONUS);
  }
}

static void play_frame() {
  if (jobs_ok) {
    input_job.h.resume();
    snake_job.h.resume();
    fruit_job.h.resume();
    return;
  }
  beat_input();
  beat_snake();
  beat_fruit();
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
      level++;
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
  level = 1;
  frames_played = 0;
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  deploy();
  play_used = 0;
  input_job = input_coro();
  snake_job = snake_coro();
  fruit_job = fruit_coro();
  jobs_ok = input_job.h && snake_job.h && fruit_job.h;
  while (tick() == 0) {
  }
  jobs_ok = 0;
  input_job = Play{};
  snake_job = Play{};
  fruit_job = Play{};
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
