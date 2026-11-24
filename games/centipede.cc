// Centipede for the BBC Master, linked with the sideways libc.
//
// Shadow mode 1 (VDU 22,129). A mushroom field, a crawling centipede that
// drops and reverses at edges and mushrooms, a spider in the player zone,
// and a blaster confined to the bottom rows. centipede_fx.s does the
// per-frame crawl, shot, and spider work; this file keeps waves, scoring,
// and the coroutine play loop.
//
// Arrows / WASD / Z X move. Space or Return fires. Escape returns.

#include <bbc.h>
#include <coroutine>
#include <stddef.h>

#include "centipede.h"

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
enum { ColBlack = 0, ColGreen = 1, ColRed = 2, ColWhite = 3 };

class Pace {
 public:
  enum : unsigned {
    Title = 200,
    TitleStep = 20,
    Crawl = 3,
    Spider = 2,
    SpiderIn = 200,
    Boom = 10,
    Death = 80,
    Wave = 90,
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
  static void fill(int x, int y, int width, int height, int colour);
};

extern "C" {
void frame_wait();
void keys_read();
void hud_digits();
void player_step();
void player_draw();
void player_image();
void player_erase();
void cell_draw();
void cell_wipe();
void mush_draw();
void crawl_step();
void shot_step();
void spider_step();
void sound_boom();
void sound_death();

extern const unsigned char k_ce_head_l[];
extern const unsigned char k_ce_head_r[];
extern const unsigned char k_ce_body[];
extern const unsigned char k_ce_mush0[];
extern const unsigned char k_ce_mush1[];
extern const unsigned char k_ce_mush2[];
extern const unsigned char k_ce_mush3[];
extern const unsigned char k_ce_spider0[];
extern const unsigned char k_ce_spider1[];
extern const unsigned char k_ce_ship[];
extern const unsigned char k_ce_ship_red[];
extern const unsigned char k_blast[];
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
enum { ShotFlying = 0, ShotSeg = 1, ShotMush = 2, ShotSpent = 3, ShotSpider = 4 };

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
enum { EnvShot = 1, EnvBoom = 2, EnvDeath = 3 };

static const Envelope envelopes[3] = {
    {EnvShot, NoRepeat | 1, {-5, 0, 0}, {12, 0, 0}, 126, -6, 0, -126, 110, 0},
    {EnvBoom, NoRepeat | 1, {0, 0, 0}, {0, 0, 0}, 126, -5, 0, -126, 120, 0},
    {EnvDeath, NoRepeat | 2, {0, 0, 0}, {0, 0, 0}, 126, -2, 0, -126, 126, 0}};

extern "C" {
u8 key_bits;

u8 ce_col;
u8 ce_row;
const unsigned char* ce_src;

u8 mush[CE_COLS * CE_ROWS];

u8 seg_on[CE_SEGS];
u8 seg_x[CE_SEGS];
u8 seg_y[CE_SEGS];
u8 seg_dir[CE_SEGS];
u8 seg_count;
u8 crawl_event;

u16 player_x;
u8 player_row;
u8 fire_want;

u8 shot_on;
u16 shot_x;
u8 shot_col;
u8 shot_y;
u8 shot_event;
u8 shot_hit_seg;

u8 spider_on;
u8 spider_x;
u8 spider_y;
i8 spider_vx;
i8 spider_vy;
u8 spider_anim;
u8 spider_hit;

extern const Note snd_shot = {Flush | 2, EnvShot, 160, 3};
extern const Note snd_boom = {Flush | 0, EnvBoom, 5, 5};
extern const Note snd_death = {Flush | 0, EnvDeath, 6, 25};

extern const unsigned char* const mush_spr[4] = {
    k_ce_mush0, k_ce_mush1, k_ce_mush2, k_ce_mush3};
extern const unsigned char* const spider_spr[2] = {k_ce_spider0, k_ce_spider1};

extern const u8 key_codes[KEY_COUNT] = {0x61, 0x41, 0x19, 0x42, 0x32, 0x79,
                                        0x39, 0x21, 0x48, 0x29, 0x51, 0x68,
                                        0x62, 0x49, 0x70};
extern const u8 key_masks[KEY_COUNT] = {
    KeyLeft, KeyLeft,  KeyLeft,  KeyRight, KeyRight, KeyRight,
    KeyUp,   KeyUp,    KeyUp,    KeyDown,  KeyDown,  KeyDown,
    KeyFire, KeyFire,  KeyEscape};
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
  enum : unsigned { Cols = 40 };
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
void beat_player();
void beat_laser();
void beat_crawl();
void beat_spider();
}  // namespace Game

static u16 rng_state;
static u8 hud_dirty;
static u8 frames_played;
static u8 crawl_t;
static u8 spider_t;
static u8 spider_in;
static u8 death_t;
static u8 wave_t;
static u8 over_t;
static u8 alive_segs;

static Play player_job;
static Play laser_job;
static Play crawl_job;
static Play spider_job;
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

Play crawl_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_crawl();
  }
}

Play spider_coro() {
  for (;;) {
    co_await Frame{};
    Game::beat_spider();
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
  bbc::tab(18, 0);
  Text::write("WAVE");
  bbc::tab(33, 0);
  Text::write("LIVES");
}

void Hud::banner(const char* line, u8 colour, u8 row) {
  bbc::colour(128);
  Text::centre(row, colour, line);
}

static void add_score(u16 points) {
  score = static_cast<unsigned>(score + points);
  hud_dirty = 1;
}

static void present() {
  hud_digits();
  hud_dirty = 0;
}

static void paint_mushrooms() {
  u8 x;
  u8 y;
  for (y = 0; y < CE_ROWS; y++) {
    for (x = 0; x < CE_COLS; x++) {
      if (mush[static_cast<unsigned>(y) * CE_COLS + x] != 0) {
        ce_col = x;
        ce_row = y;
        mush_draw();
      }
    }
  }
}

static void seed_mushrooms() {
  u8 n;
  u8 i;
  u8 x;
  u8 y;
  u16 cell;
  for (cell = 0; cell < static_cast<u16>(CE_COLS * CE_ROWS); cell++) {
    mush[cell] = 0;
  }
  n = static_cast<u8>(30 + wave * 4);
  if (n > 80) {
    n = 80;
  }
  for (i = 0; i < n; i++) {
    x = static_cast<u8>((rng_next() >> 8) % CE_COLS);
    y = static_cast<u8>((rng_next() >> 8) % (CE_ROWS - 5));
    if (y < 1) {
      y = 1;
    }
    mush[static_cast<unsigned>(y) * CE_COLS + x] = CE_MUSH_MAX;
  }
}

static void spawn_centipede() {
  u8 i;
  u8 len = static_cast<u8>(CE_SEGS - (wave > 4 ? 0 : 4 - wave));
  if (len < 4) {
    len = 4;
  }
  if (len > CE_SEGS) {
    len = CE_SEGS;
  }
  seg_count = len;
  alive_segs = len;
  for (i = 0; i < CE_SEGS; i++) {
    seg_on[i] = 0;
  }
  for (i = 0; i < len; i++) {
    seg_on[i] = 1;
    seg_x[i] = static_cast<u8>(len - 1 - i);
    seg_y[i] = 0;
    seg_dir[i] = DIR_RIGHT;
    ce_col = seg_x[i];
    ce_row = seg_y[i];
    if (i == 0) {
      ce_src = k_ce_head_r;
    } else {
      ce_src = k_ce_body;
    }
    cell_draw();
  }
}

static void spawn_spider() {
  spider_on = 1;
  spider_hit = 0;
  spider_x = static_cast<u8>((rng_next() >> 8) % CE_COLS);
  spider_y = CE_PLAYER_MIN_ROW;
  spider_vx = (rng_next() & 0x8000) != 0 ? 1 : -1;
  spider_vy = 1;
  spider_anim = 0;
}

static void clear_shot() {
  if (shot_on != 0) {
    shot_on = 0;
    fx_col = shot_col;
    fx_y = shot_y;
    fx_len = CE_SHOT_LEN;
    fx_blank();
  }
}

static void clear_spider() {
  if (spider_on != 0) {
    spider_on = 0;
    ce_col = spider_x;
    ce_row = spider_y;
    mush_draw();
  }
}

static void clear_centipede() {
  u8 i;
  for (i = 0; i < seg_count; i++) {
    if (seg_on[i] != 0) {
      ce_col = seg_x[i];
      ce_row = seg_y[i];
      mush_draw();
      seg_on[i] = 0;
    }
  }
  seg_count = 0;
  alive_segs = 0;
}

static u8 Title_show() {
  u8 frames;
  u8 armed = 0;
  u8 look = 0;
  u8 step_t = Pace::TitleStep;
  phase = PhaseTitle;
  bbc::cursor(0);
  bbc::cls();
  Text::centre(2, ColWhite, "DAVECC CENTIPEDE");
  ce_col = 13;
  ce_row = 8;
  ce_src = k_ce_head_r;
  cell_draw();
  ce_col = 14;
  ce_src = k_ce_body;
  cell_draw();
  ce_col = 15;
  cell_draw();
  ce_col = 16;
  cell_draw();
  ce_col = 12;
  ce_row = 12;
  ce_src = k_ce_mush0;
  cell_draw();
  ce_col = 15;
  ce_src = k_ce_spider0;
  cell_draw();
  Text::centre(18, ColGreen, "ARROWS / WASD MOVE");
  Text::centre(19, ColGreen, "SPACE FIRES, ESCAPE QUITS");
  Text::centre(22, ColWhite, "SHOOT THE CENTIPEDE");
  Text::centre(28, ColRed, "WRITTEN IN C++20 WITH DAVECC");
  Text::centre(29, ColRed, "BY DAVE ALLISON");
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
      ce_col = 15;
      ce_row = 12;
      ce_src = look ? k_ce_spider1 : k_ce_spider0;
      cell_draw();
    }
  }
  return 1;
}

static void deploy() {
  phase = PhasePlay;
  fire_want = 0;
  clear_shot();
  clear_spider();
  spider_hit = 0;
  crawl_t = Pace::Crawl;
  spider_t = Pace::Spider;
  spider_in = Pace::SpiderIn;
  player_x = 152;
  player_row = CE_ROWS - 2;
  bbc::cls();
  bbc::cursor(0);
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  Screen::fill(0, 16, 320, 2, ColGreen);
  seed_mushrooms();
  paint_mushrooms();
  spawn_centipede();
  player_draw();
  Hud::labels();
  present();
}

static void kill_player() {
  if (phase != PhasePlay) {
    return;
  }
  clear_shot();
  phase = PhaseDying;
  death_t = Pace::Death;
  if (lives > 0) {
    lives--;
  }
  hud_dirty = 1;
  fx_src = k_blast;
  player_image();
  sound_death();
}

static void dying() {
  if ((death_t & 7) == 0) {
    fx_src = (death_t & 8) != 0 ? k_ce_ship_red : k_blast;
    player_image();
  }
  if (death_t > 0) {
    death_t--;
  }
  if (death_t != 0) {
    return;
  }
  player_erase();
  if (lives == 0) {
    phase = PhaseOver;
    over_t = Pace::Over;
    Hud::banner("GAME OVER", ColRed, 12);
    Text::centre(15, ColWhite, "PRESS SPACE");
    return;
  }
  clear_centipede();
  clear_spider();
  spawn_centipede();
  player_x = 152;
  player_row = CE_ROWS - 2;
  fire_want = 0;
  spider_in = Pace::SpiderIn;
  phase = PhasePlay;
  player_draw();
}

static void split_at(u8 hit) {
  u8 i;
  u8 tail = 0;
  // Everything behind the hit becomes a new head chain; the hit dies and
  // leaves a mushroom.
  ce_col = seg_x[hit];
  ce_row = seg_y[hit];
  mush[static_cast<unsigned>(ce_row) * CE_COLS + ce_col] = CE_MUSH_MAX;
  mush_draw();
  seg_on[hit] = 0;
  if (alive_segs > 0) {
    alive_segs--;
  }
  // Compact: keep 0..hit-1, then append hit+1.. as a continuation that
  // already has positions; promote hit+1 to a head by leaving it in place.
  for (i = static_cast<u8>(hit + 1); i < seg_count; i++) {
    if (seg_on[i] != 0) {
      tail = 1;
      break;
    }
  }
  (void)tail;
  // Count remaining live segments for wave clear.
  alive_segs = 0;
  for (i = 0; i < seg_count; i++) {
    if (seg_on[i] != 0) {
      alive_segs++;
    }
  }
  // Rebuild packed chain from live segments in order.
  {
    u8 n = 0;
    u8 xs[CE_SEGS];
    u8 ys[CE_SEGS];
    u8 ds[CE_SEGS];
    for (i = 0; i < seg_count; i++) {
      if (seg_on[i] != 0) {
        xs[n] = seg_x[i];
        ys[n] = seg_y[i];
        ds[n] = seg_dir[i];
        n++;
      }
    }
    seg_count = n;
    for (i = 0; i < CE_SEGS; i++) {
      seg_on[i] = 0;
    }
    for (i = 0; i < n; i++) {
      seg_on[i] = 1;
      seg_x[i] = xs[i];
      seg_y[i] = ys[i];
      seg_dir[i] = ds[i];
    }
    alive_segs = n;
  }
}

namespace Game {

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
  if (shot_event == ShotSeg) {
    add_score(100);
    sound_boom();
    split_at(shot_hit_seg);
    if (alive_segs == 0) {
      phase = PhaseWave;
      wave_t = Pace::Wave;
      clear_spider();
    }
  } else if (shot_event == ShotMush) {
    add_score(1);
  } else if (shot_event == ShotSpider) {
    add_score(300);
    sound_boom();
    clear_spider();
    spider_in = Pace::SpiderIn;
  }
}

void beat_crawl() {
  if (phase != PhasePlay) {
    return;
  }
  if (crawl_t > 0) {
    crawl_t--;
    return;
  }
  crawl_t = Pace::Crawl;
  if (wave > 3) {
    crawl_t = 2;
  }
  crawl_step();
  if (crawl_event) {
    kill_player();
  }
}

void beat_spider() {
  if (phase != PhasePlay) {
    return;
  }
  if (!spider_on) {
    if (spider_in > 0) {
      spider_in--;
    } else {
      spawn_spider();
    }
    return;
  }
  if (spider_t > 0) {
    spider_t--;
    return;
  }
  spider_t = Pace::Spider;
  spider_hit = 0;
  spider_step();
  if (spider_hit) {
    kill_player();
  }
}

static void play_frame() {
  if (jobs_ok) {
    player_job.h.resume();
    laser_job.h.resume();
    crawl_job.h.resume();
    spider_job.h.resume();
    return;
  }
  beat_player();
  beat_laser();
  beat_crawl();
  beat_spider();
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
  frames_played = 0;
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  deploy();
  play_used = 0;
  player_job = player_coro();
  laser_job = laser_coro();
  crawl_job = crawl_coro();
  spider_job = spider_coro();
  jobs_ok = player_job.h && laser_job.h && crawl_job.h && spider_job.h;
  while (tick() == 0) {
  }
  jobs_ok = 0;
  player_job = Play{};
  laser_job = Play{};
  crawl_job = Play{};
  spider_job = Play{};
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
    bbc::osword(bbc::osword_command::envelope, const_cast<Envelope*>(&envelopes[i]));
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
  Title_show();
  bbc::osbyte(bbc::osbyte_command::ack_escape, 0, 0);
  Game::run();
  return 0;
}
