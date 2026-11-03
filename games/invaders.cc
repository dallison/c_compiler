// Space Invaders for the BBC Master, linked with the sideways libc.
//
// The playfield is shadow mode 1 (VDU 22,129): 320x256, four colours, and
// HIMEM stays at &8000. The title is shadow mode 2, then main changes mode.
// Sprites below are already mode-1 bytes (logical 1 green, 2 red, 3 white
// after the palette remap). screen1.s, below &3000, copies one batch onto
// the shadow screen.
//
// Z X, A D, or the arrows move. Space or Return fires. Escape returns.

#include <bbc.h>

extern "C" void bbc_return(void);
extern "C" {
extern unsigned char batch_base[];
extern unsigned char batch_limit[];
}
extern "C" void batch_flush(const unsigned char* end);
extern "C" void note_acccon(void);

extern "C" {
unsigned score;
unsigned char lives;
unsigned char wave;
unsigned char phase;
unsigned char alive;
unsigned char acccon_copy;
}

enum { PhaseTitle = 1, PhasePlay = 2, PhaseDying = 3, PhaseWave = 4, PhaseOver = 5 };
enum { ColBlack = 0, ColGreen = 1, ColRed = 2, ColWhite = 3 };

// Set before mode 129. 1 is a human, 2 is the self-playing demo.
static unsigned char attract;

class Metrics {
 public:
  enum : int {
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

static const unsigned char k_squid0[] = {
    0x00, 0xcc, 0x33, 0x00, 0x11, 0xff, 0xff, 0x88, 0x33, 0x77, 0x77, 0x66,
    0x77, 0xff, 0xff, 0xee, 0x66, 0xbb, 0xff, 0x55, 0x00, 0xcc, 0x33, 0x00,
    0x11, 0x88, 0x11, 0x88, 0x22, 0x00, 0x00, 0x44};
static const unsigned char k_squid1[] = {
    0x00, 0xcc, 0x33, 0x00, 0x11, 0xff, 0xff, 0x88, 0x33, 0x77, 0x77, 0x66,
    0x77, 0xff, 0xff, 0xee, 0x66, 0xbb, 0xff, 0x55, 0x11, 0x44, 0x22, 0x88,
    0x00, 0xcc, 0x33, 0x00, 0x11, 0x00, 0x00, 0x88};
static const unsigned char k_crab0[] = {
    0x20, 0x00, 0x00, 0x40, 0x10, 0x00, 0x00, 0x80, 0x30, 0xf0, 0xf0, 0xc0,
    0x60, 0x70, 0xe0, 0x60, 0xf0, 0xf0, 0xf0, 0xf0, 0x30, 0x90, 0x90, 0xc0,
    0x40, 0x80, 0x10, 0x20, 0x20, 0x00, 0x00, 0x40};
static const unsigned char k_crab1[] = {
    0x20, 0x00, 0x00, 0x40, 0x50, 0x00, 0x00, 0xa0, 0x30, 0xf0, 0xf0, 0xc0,
    0x60, 0x70, 0xe0, 0x60, 0xf0, 0xf0, 0xf0, 0xf0, 0x10, 0x90, 0x90, 0x80,
    0x20, 0x80, 0x10, 0x40, 0x40, 0x40, 0x20, 0x20};
static const unsigned char k_oct0[] = {
    0x01, 0x00, 0x00, 0x08, 0x00, 0x08, 0x01, 0x00, 0x03, 0x0f, 0x0f, 0x0c,
    0x07, 0x06, 0x06, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x02, 0x0e, 0x07, 0x04,
    0x01, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x04};
static const unsigned char k_oct1[] = {
    0x01, 0x00, 0x00, 0x08, 0x08, 0x08, 0x01, 0x01, 0x03, 0x0f, 0x0f, 0x0c,
    0x07, 0x06, 0x06, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x0c, 0x03, 0x00,
    0x01, 0x04, 0x02, 0x08, 0x02, 0x00, 0x00, 0x04};
static const unsigned char k_cannon[] = {
    0x00, 0x03, 0x0c, 0x00, 0x00, 0x03, 0x0c, 0x00, 0x00, 0x03, 0x0c, 0x00,
    0x00, 0x0f, 0x0f, 0x00, 0x03, 0x0f, 0x0f, 0x0c, 0x07, 0x0f, 0x0f, 0x0e,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f};
static const unsigned char k_cannon_red[] = {
    0x00, 0x30, 0xc0, 0x00, 0x00, 0x30, 0xc0, 0x00, 0x00, 0x30, 0xc0, 0x00,
    0x00, 0xf0, 0xf0, 0x00, 0x30, 0xf0, 0xf0, 0xc0, 0x70, 0xf0, 0xf0, 0xe0,
    0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0};
static const unsigned char k_ufo[] = {
    0x00, 0x70, 0xe0, 0x00, 0x10, 0xf0, 0xf0, 0x80, 0x30, 0xf0, 0xf0, 0xc0,
    0x60, 0xb0, 0x60, 0xb0, 0x70, 0xf0, 0xf0, 0xe0, 0x10, 0xc0, 0x30, 0x80,
    0x20, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00};
static const unsigned char k_boom[] = {
    0x88, 0x88, 0x11, 0x11, 0x44, 0x44, 0x22, 0x22, 0x22, 0x22, 0x44, 0x44,
    0x11, 0x11, 0x88, 0x88, 0x22, 0x22, 0x44, 0x44, 0x44, 0x44, 0x22, 0x22,
    0x88, 0x88, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00};
static const unsigned char k_shot[] = {0x00, 0x88, 0x00, 0x88, 0x00, 0x88, 0x00, 0x88,
                                      0x00, 0x88, 0x00, 0x88, 0x00, 0x88, 0x00, 0x88};
static const unsigned char k_bomb0[] = {0xc0, 0x00, 0x30, 0x00, 0xc0, 0x00, 0x30, 0x00,
                                       0xc0, 0x00, 0x30, 0x00, 0xc0, 0x00, 0x30, 0x00};
static const unsigned char k_bomb1[] = {0x30, 0x00, 0xc0, 0x00, 0x30, 0x00, 0xc0, 0x00,
                                       0x30, 0x00, 0xc0, 0x00, 0x30, 0x00, 0xc0, 0x00};
static const unsigned char k_bomb2[] = {0x60, 0x00, 0x60, 0x00, 0x30, 0x00, 0x30, 0x00,
                                       0x10, 0x80, 0x10, 0x80, 0x30, 0x00, 0x60, 0x00};
static const unsigned char k_bunker[] = {
    0x07, 0x0f, 0x0f, 0x0f, 0x0f, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0x08, 0x00, 0x00, 0x01, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x0f,
    0x0e, 0x00, 0x00, 0x00, 0x00, 0x07, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x03,
    0x0c, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x03,
    0x0c, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x03};
static const unsigned char k_bit[6] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20};
static const unsigned ufo_prize[15] = {100, 50, 50,  100, 150, 100, 100, 50,
                                      300, 100, 100, 100, 50,  150, 100};

class Screen {
 public:
  static void close();
  static void fill(int x, int y, int width, int height, int colour);
  static void stamp(int x, int y, const unsigned char* bits, int width, int height);

 private:
  static unsigned char* pen_;
  static int span_;
  static unsigned char* open(int x, int y, int width, int height);
};

unsigned char* Screen::pen_ = 0;
int Screen::span_ = 0;

unsigned char* Screen::open(int x, int y, int width, int height) {
  int across = width >> 2;
  int bytes = 0;
  int down = height;
  if (pen_ == 0) {
    pen_ = batch_base;
  }
  while (down > 0) {
    bytes += across;
    down--;
  }
  if (pen_ + 5 + bytes > batch_limit) {
    close();
  }
  span_ = bytes;
  pen_[0] = (unsigned char)x;
  pen_[1] = (unsigned char)(x >> 8);
  pen_[2] = (unsigned char)y;
  pen_[3] = (unsigned char)across;
  pen_[4] = (unsigned char)height;
  return pen_ + 5;
}

void Screen::close() {
  if (pen_ != 0 && pen_ != batch_base) {
    batch_flush(pen_);
  }
  pen_ = batch_base;
}

void Screen::fill(int x, int y, int width, int height, int colour) {
  unsigned char* dst;
  int paint = 0;
  int i;
  if (width == 0 || height == 0) {
    return;
  }
  if (colour == ColGreen) {
    paint = 0x0f;
  } else if (colour == ColRed) {
    paint = 0xf0;
  } else if (colour == ColWhite) {
    paint = 0xff;
  }
  dst = open(x, y, width, height);
  for (i = 0; i < span_; i++) {
    dst[i] = (unsigned char)paint;
  }
  pen_ = dst + span_;
}

void Screen::stamp(int x, int y, const unsigned char* bits, int width, int height) {
  unsigned char* dst;
  int i;
  if (width == 0 || height == 0) {
    return;
  }
  dst = open(x, y, width, height);
  for (i = 0; i < span_; i++) {
    dst[i] = bits[i];
  }
  pen_ = dst + span_;
}

class Keys {
 public:
  static int axis();
  static int fire();
  static int escape();

 private:
  static int down(unsigned char code);
};

class Tone {
 public:
  static void play(unsigned char channel, unsigned char pitch, unsigned char duration,
                   unsigned char volume);
  static void march(unsigned step);
  static void quiet(unsigned char channel);
};

class Text {
 public:
  static void write(const char* text);
  static void quad(unsigned value);
  static void pair(unsigned value);
};

class Rng {
 public:
  void seed();
  unsigned next();

 private:
  unsigned state_;
};

class Shelters;

class Fleet {
 public:
  void reset(int origin_y);
  void erase();
  void draw();
  int step(Shelters* shelters);
  int shoot(int x, int y);
  int drop_at(int col, int* x, int* y) const;
  int column_x(int col) const;
  int column_alive(int col) const;
  int pace(int wave_number) const;
  int last_x_;
  int last_y_;

 private:
  void render(int draw_it);
  void tighten();
  int rank_alive(int rank) const;
  int rank_y(int rank) const;
  int cell(int rank, int col) const;
  const unsigned char* picture(int rank) const;
  int points(int rank) const;
  int origin_x_;
  int origin_y_;
  int dir_;
  int lo_;
  int hi_;
  int bot_;
  unsigned char frame_;
  unsigned char on_[55];
};

class Shelters {
 public:
  void reset();
  void draw();
  int strike(int x, int y);

 private:
  int left(int index) const;
  unsigned char solid_[4];
};

class Saucer {
 public:
  void reset();
  void erase();
  void draw();
  void launch(int from_left);
  int step();
  int contains(int x, int y) const;
  int x_;
  int on_;
  int score_x_;
  unsigned char score_t_;
  unsigned score_n_;

 private:
  int dir_;
  unsigned char tick_;
};

class Cannon {
 public:
  void reset();
  void erase();
  void draw();
  void move(int dir);
  int x_;
};

class Volley {
 public:
  void reset();
  void erase();
  void draw();
  int fire(int x, unsigned slot);
  void drop(int x, int y);
  int move_shot(Fleet* fleet, Shelters* shelters, Saucer* saucer);
  int move_bombs(Shelters* shelters, int px);
  unsigned slot() const;

 private:
  const unsigned char* bomb_bits(int frame) const;
  int shot_x_;
  int shot_y_;
  int shot_on_;
  unsigned char shot_i_;
  int bomb_x_[3];
  int bomb_y_[3];
  unsigned char bomb_on_[3];
  unsigned char bomb_frame_[3];
};

class Hud {
 public:
  static void text(unsigned best, int demo);
  static void cannons();
  static void banner(const char* line, unsigned char colour, unsigned char row);
};

class Title {
 public:
  static int show();
};

class Game {
 public:
  void run();

 private:
  int session();
  int tick();
  void deploy();
  void present();
  void add_score(unsigned points);
  void kill_player();
  void play();
  void steer(int* dir, int* do_fire);
  void march();
  void launch_bomb();
  void launch_ufo();
  void score_hit(int scored);
  Rng rng_;
  Fleet fleet_;
  Shelters shelters_;
  Saucer saucer_;
  Cannon cannon_;
  Volley volley_;
  unsigned best_;
  unsigned ufo_wait_;
  int boom_x_;
  int boom_y_;
  unsigned char demo_;
  unsigned char extra_;
  unsigned char landed_;
  unsigned char hud_dirty_;
  unsigned char lives_draw_;
  unsigned char clock_;
  unsigned char step_t_;
  unsigned char bomb_t_;
  unsigned char boom_t_;
  unsigned char death_t_;
  unsigned char wave_t_;
  unsigned char over_t_;
  unsigned char ufo_i_;
  unsigned char march_i_;
  unsigned char hunt_;
  int boom_on_;
};

static Game game;

int Keys::down(unsigned char code) {
  bbc::os_regs regs = bbc::osbyte(bbc::osbyte_command::inkey, (unsigned char)(0 - code), 0xff);
  return regs.x != 0;
}

int Keys::axis() {
  if (down(26) || down(66) || down(98)) {
    return -1;
  }
  if (down(122) || down(50) || down(67)) {
    return 1;
  }
  return 0;
}

int Keys::fire() { return down(99) || down(74); }

int Keys::escape() { return down(113); }

void Tone::play(unsigned char channel, unsigned char pitch, unsigned char duration,
                unsigned char volume) {
  unsigned char block[8];
  block[0] = channel;
  block[1] = 0;
  block[2] = 0;
  block[3] = 0;
  if (volume != 0) {
    block[2] = (unsigned char)(0x100 - volume);
    block[3] = 0xff;
  }
  block[4] = pitch;
  block[5] = 0;
  block[6] = duration;
  block[7] = 0;
  bbc::osword(7, block);
}

void Tone::march(unsigned step) {
  static const unsigned char pitch[4] = {52, 40, 32, 24};
  play(1, pitch[step & 3], 1, 15);
}

void Tone::quiet(unsigned char channel) { play(channel, 0, 1, 0); }

void Text::write(const char* text) {
  while (*text != 0) {
    bbc::oswrch((unsigned char)*text);
    text++;
  }
}

void Text::quad(unsigned value) {
  static const unsigned place_value[4] = {1000, 100, 10, 1};
  int i;
  if (value > 9999) {
    value = 9999;
  }
  for (i = 0; i < 4; i++) {
    unsigned place = place_value[i];
    unsigned char digit = '0';
    while (value >= place) {
      value -= place;
      digit++;
    }
    bbc::oswrch(digit);
  }
}

void Text::pair(unsigned value) {
  unsigned char tens = '0';
  if (value > 99) {
    value = 99;
  }
  while (value >= 10) {
    value -= 10;
    tens++;
  }
  bbc::oswrch(tens);
  bbc::oswrch((unsigned char)('0' + value));
}

void Rng::seed() { state_ = 1; }

unsigned Rng::next() {
  state_ = (unsigned)(state_ + (state_ << 5) + 13849);
  return state_;
}

int Fleet::cell(int rank, int col) const { return (rank << 3) + (rank << 1) + rank + col; }

int Fleet::column_x(int col) const { return origin_x_ + (col << 4) + (col << 2); }

int Fleet::rank_y(int rank) const { return origin_y_ + (rank << 4); }

const unsigned char* Fleet::picture(int rank) const {
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

int Fleet::points(int rank) const {
  if (rank == 0) {
    return 30;
  }
  if (rank < 3) {
    return 20;
  }
  return 10;
}

int Fleet::column_alive(int col) const {
  int rank;
  for (rank = 0; rank < Metrics::Ranks; rank++) {
    if (on_[cell(rank, col)]) {
      return 1;
    }
  }
  return 0;
}

int Fleet::rank_alive(int rank) const {
  int col;
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

void Fleet::reset(int origin_y) {
  int i;
  origin_x_ = Metrics::OriginX;
  origin_y_ = origin_y;
  dir_ = 1;
  frame_ = 0;
  lo_ = 0;
  hi_ = Metrics::Columns - 1;
  bot_ = Metrics::Ranks - 1;
  last_x_ = 0;
  last_y_ = 0;
  for (i = 0; i < 55; i++) {
    on_[i] = 1;
  }
  alive = 55;
}

void Fleet::render(int draw_it) {
  int rank;
  int col;
  for (rank = 0; rank < Metrics::Ranks; rank++) {
    for (col = 0; col < Metrics::Columns; col++) {
      int x;
      int y;
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

int Fleet::step(Shelters* shelters) {
  int dx = 0;
  int dy = 0;
  int edge;
  int col;
  if (alive == 0) {
    return 0;
  }
  if (dir_ > 0) {
    edge = column_x(hi_) + Metrics::AlienW;
    if (edge + 4 > 312) {
      dy = 8;
      dir_ = -1;
    } else {
      dx = 4;
    }
  } else {
    edge = column_x(lo_);
    if (edge < 12) {
      dy = 8;
      dir_ = 1;
    } else {
      dx = -4;
    }
  }
  if (rank_y(bot_) + dy + Metrics::AlienH >= Metrics::PlayerY) {
    return 2;
  }
  erase();
  origin_x_ += dx;
  origin_y_ += dy;
  frame_ ^= 1;
  for (col = lo_; col <= hi_; col++) {
    if (on_[cell(bot_, col)]) {
      shelters->strike(column_x(col) + 8, rank_y(bot_) + 4);
    }
  }
  draw();
  return 1;
}

int Fleet::shoot(int x, int y) {
  int rank;
  int col;
  for (rank = Metrics::Ranks - 1; rank >= 0; rank--) {
    for (col = 0; col < Metrics::Columns; col++) {
      int ax;
      int ay;
      if (!on_[cell(rank, col)]) {
        continue;
      }
      ax = column_x(col);
      ay = rank_y(rank);
      if (x >= ax + Metrics::AlienW) {
        continue;
      }
      if (ax >= x + Metrics::Shot) {
        continue;
      }
      if (y >= ay + Metrics::AlienH) {
        continue;
      }
      if (ay >= y + Metrics::Shot) {
        continue;
      }
        on_[cell(rank, col)] = 0;
        if (alive > 0) {
          alive--;
        }
        last_x_ = ax;
        last_y_ = ay;
        Screen::fill(ax, ay, Metrics::AlienW, Metrics::AlienH, ColBlack);
        if (col == lo_ || col == hi_ || rank == bot_) {
          tighten();
        }
        return points(rank);
    }
  }
  return 0;
}

int Fleet::drop_at(int col, int* x, int* y) const {
  int rank;
  if (col < 0 || col >= Metrics::Columns) {
    return 0;
  }
  for (rank = Metrics::Ranks - 1; rank >= 0; rank--) {
    if (!on_[cell(rank, col)]) {
      continue;
    }
    *x = column_x(col) + 4;
    *y = rank_y(rank) + Metrics::AlienH;
    return 1;
  }
  return 0;
}

int Fleet::pace(int wave_number) const {
  int gap = 1;
  if (alive > 40) {
    gap = 16;
  } else if (alive > 28) {
    gap = 11;
  } else if (alive > 16) {
    gap = 7;
  } else if (alive > 8) {
    gap = 4;
  } else if (alive > 2) {
    gap = 2;
  }
  if (wave_number > 2 && gap > 2) {
    gap--;
  }
  return gap;
}

int Shelters::left(int index) const { return 32 + (index << 6) + (index << 3); }

void Shelters::reset() {
  int i;
  for (i = 0; i < 4; i++) {
    solid_[i] = 0x2f;
  }
}

void Shelters::draw() {
  int i;
  for (i = 0; i < 4; i++) {
    int bx = left(i);
    int row;
    int col;
    Screen::stamp(bx, Metrics::BunkerY, k_bunker, 24, 16);
    for (row = 0; row < 2; row++) {
      for (col = 0; col < 3; col++) {
        int bit = k_bit[(row << 1) + row + col];
        if ((solid_[i] & bit) == 0) {
          Screen::fill(bx + (col << 3), Metrics::BunkerY + (row << 3), 8, 8, ColBlack);
        }
      }
    }
  }
}

int Shelters::strike(int x, int y) {
  int i;
  if (y < Metrics::BunkerY || y >= Metrics::BunkerY + 16) {
    return 0;
  }
  for (i = 0; i < 4; i++) {
    int bx = left(i);
    int col;
    int row;
    int bit;
    if (x < bx || x >= bx + 24) {
      continue;
    }
    col = (x - bx) >> 3;
    row = (y - Metrics::BunkerY) >> 3;
    if (col > 2 || row > 1) {
      continue;
    }
    bit = k_bit[(row << 1) + row + col];
    if ((solid_[i] & bit) == 0) {
      continue;
    }
    solid_[i] = (unsigned char)(solid_[i] & (unsigned char)~bit);
    Screen::fill(bx + (col << 3), Metrics::BunkerY + (row << 3), 8, 8, ColBlack);
    return 1;
  }
  return 0;
}

void Saucer::reset() {
  on_ = 0;
  score_t_ = 0;
  score_n_ = 0;
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

void Saucer::launch(int from_left) {
  on_ = 1;
  score_t_ = 0;
  tick_ = 0;
  if (from_left) {
    x_ = 4;
    dir_ = 4;
  } else {
    x_ = 300;
    dir_ = -4;
  }
}

int Saucer::step() {
  if (!on_) {
    return 0;
  }
  tick_++;
  if (tick_ & 1) {
    return 1;
  }
  x_ += dir_;
  if (x_ < 0 || x_ > 304) {
    on_ = 0;
    Tone::quiet(3);
    return 0;
  }
  Tone::play(3, (tick_ & 8) ? 156 : 144, 2, 10);
  return 1;
}

int Saucer::contains(int x, int y) const {
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
  if (dir < 0 && x_ >= 12) {
    x_ -= 4;
  } else if (dir > 0 && x_ <= 292) {
    x_ += 4;
  }
}

void Volley::reset() {
  int i;
  shot_on_ = 0;
  for (i = 0; i < 3; i++) {
    bomb_on_[i] = 0;
  }
}

void Volley::erase() {
  int i;
  if (shot_on_) {
    Screen::fill(shot_x_, shot_y_, Metrics::Shot, Metrics::Shot, ColBlack);
  }
  for (i = 0; i < 3; i++) {
    if (bomb_on_[i]) {
      Screen::fill(bomb_x_[i], bomb_y_[i], Metrics::Shot, Metrics::Shot, ColBlack);
    }
  }
}

const unsigned char* Volley::bomb_bits(int frame) const {
  if (frame == 1) {
    return k_bomb1;
  }
  if (frame == 2) {
    return k_bomb2;
  }
  return k_bomb0;
}

void Volley::draw() {
  int i;
  if (shot_on_) {
    Screen::stamp(shot_x_, shot_y_, k_shot, Metrics::Shot, Metrics::Shot);
  }
  for (i = 0; i < 3; i++) {
    if (bomb_on_[i]) {
      Screen::stamp(bomb_x_[i], bomb_y_[i], bomb_bits(bomb_frame_[i]), Metrics::Shot,
                    Metrics::Shot);
    }
  }
}

int Volley::fire(int x, unsigned slot) {
  if (shot_on_) {
    return 0;
  }
  shot_on_ = 1;
  shot_i_ = (unsigned char)slot;
  shot_x_ = x + 4;
  shot_y_ = Metrics::PlayerY - Metrics::Shot;
  Tone::play(2, 210, 1, 12);
  return 1;
}

unsigned Volley::slot() const { return shot_i_; }

void Volley::drop(int x, int y) {
  int i;
  for (i = 0; i < 3; i++) {
    if (bomb_on_[i]) {
      continue;
    }
    bomb_on_[i] = 1;
    bomb_x_[i] = x;
    bomb_y_[i] = y;
    bomb_frame_[i] = (unsigned char)i;
    return;
  }
}

int Volley::move_shot(Fleet* fleet, Shelters* shelters, Saucer* saucer) {
  int scored;
  if (!shot_on_) {
    return 0;
  }
  shot_y_ -= 4;
  if (shot_y_ < 28) {
    shot_on_ = 0;
    return 0;
  }
  scored = fleet->shoot(shot_x_, shot_y_);
  if (scored) {
    shot_on_ = 0;
    return scored;
  }
  if (saucer->contains(shot_x_, shot_y_)) {
    shot_on_ = 0;
    saucer->score_x_ = saucer->x_;
    saucer->score_t_ = 40;
    saucer->on_ = 0;
    Tone::quiet(3);
    Tone::play(0, 6, 4, 15);
    Screen::fill(saucer->score_x_, Metrics::UfoY, 16, 8, ColBlack);
    return 1000;
  }
  if (shelters->strike(shot_x_ + 4, shot_y_ + 4)) {
    shot_on_ = 0;
  }
  return 0;
}

int Volley::move_bombs(Shelters* shelters, int px) {
  int i;
  int killed = 0;
  for (i = 0; i < 3; i++) {
    int bx;
    int by;
    if (!bomb_on_[i]) {
      continue;
    }
    bx = bomb_x_[i];
    by = bomb_y_[i] + 4;
    bomb_y_[i] = by;
    bomb_frame_[i]++;
    if (bomb_frame_[i] > 2) {
      bomb_frame_[i] = 0;
    }
    if (by >= 224) {
      bomb_on_[i] = 0;
      continue;
    }
    if (shot_on_) {
      if (shot_x_ < bx + Metrics::Shot) {
        if (bx < shot_x_ + Metrics::Shot) {
          if (shot_y_ < by + Metrics::Shot) {
            if (by < shot_y_ + Metrics::Shot) {
              bomb_on_[i] = 0;
              shot_on_ = 0;
              Tone::play(0, 4, 2, 12);
              continue;
            }
          }
        }
      }
    }
    if (shelters->strike(bx + 4, by + 4)) {
      bomb_on_[i] = 0;
      continue;
    }
    if (bx < px + 14) {
      if (px + 2 < bx + Metrics::Shot) {
        if (by < Metrics::PlayerY + 8) {
          if (Metrics::PlayerY + 1 < by + Metrics::Shot) {
            bomb_on_[i] = 0;
            killed = 1;
          }
        }
      }
    }
  }
  return killed;
}

void Hud::text(unsigned best, int demo) {
  bbc::colour(128);
  bbc::tab(0, 0);
  bbc::colour(ColGreen);
  Text::write("SCORE ");
  bbc::colour(ColWhite);
  Text::quad(score);
  bbc::colour(ColGreen);
  Text::write("  HI ");
  bbc::colour(ColWhite);
  Text::quad(best);
  bbc::colour(ColRed);
  Text::write("  WAVE ");
  bbc::colour(ColWhite);
  Text::pair(wave);
  Text::write("    ");
  bbc::tab(0, 1);
  if (demo) {
    bbc::colour(ColRed);
    Text::write("DEMO");
  }
}

void Hud::cannons() {
  int n = lives;
  int i;
  int x;
  if (n > 4) {
    n = 4;
  }
  Screen::fill(224, 8, 96, 8, ColBlack);
  x = 304 - (n << 4);
  for (i = 0; i < n; i++) {
    Screen::stamp(x, 8, k_cannon, 16, 8);
    x += 16;
  }
}

void Hud::banner(const char* line, unsigned char colour, unsigned char row) {
  bbc::colour(128);
  bbc::tab(12, row);
  bbc::colour(colour);
  Text::write(line);
}

int Title::show() {
  static const unsigned char squid[8] = {0x3c, 0x7e, 0xdb, 0xff, 0x66, 0x24, 0x42, 0x81};
  static const unsigned char crab[8] = {0x42, 0x24, 0x7e, 0xdb, 0xff, 0x66, 0xa5, 0x42};
  static const unsigned char octo[8] = {0x24, 0x18, 0x7e, 0xdb, 0xff, 0x66, 0xa5, 0x42};
  int frames = 0;
  int note = 0;
  int beat = 0;
  phase = PhaseTitle;
  attract = 2;
  bbc::cursor(0);
  bbc::defchar(224, squid);
  bbc::defchar(225, crab);
  bbc::defchar(226, octo);
  bbc::cls();
  bbc::tab(2, 4);
  bbc::colour(3);
  Text::write("SPACE");
  bbc::tab(1, 6);
  bbc::colour(2);
  Text::write("INVADERS");
  bbc::tab(2, 18);
  bbc::colour(1);
  bbc::oswrch(224);
  Text::write("  30 POINTS");
  bbc::tab(2, 20);
  bbc::colour(7);
  bbc::oswrch(225);
  Text::write("  20 POINTS");
  bbc::tab(2, 22);
  bbc::colour(2);
  bbc::oswrch(226);
  Text::write("  10 POINTS");
  bbc::tab(2, 24);
  bbc::colour(5);
  Text::write("MYSTERY  50-300");
  bbc::tab(4, 27);
  bbc::colour(3);
  Text::write("PRESS SPACE");
  bbc::tab(3, 29);
  bbc::colour(6);
  Text::write("Z X OR ARROWS");
  bbc::tab(4, 31);
  bbc::colour(7);
  Text::write("ESCAPE QUITS");
  for (;;) {
    bbc::osbyte(bbc::osbyte_command::wait_vsync, 0, 0);
    if (Keys::escape()) {
      return 0;
    }
    if (Keys::fire() || Keys::axis() != 0) {
      attract = 1;
      return 1;
    }
    frames++;
    beat++;
    if (beat >= 14) {
      beat = 0;
      Tone::march((unsigned)note);
      note++;
    }
    if (frames >= 90) {
      return 1;
    }
  }
}

void Game::add_score(unsigned points) {
  score = (unsigned)(score + points);
  if (score > best_) {
    best_ = score;
  }
  if (!extra_ && score >= 1500 && lives < 4) {
    extra_ = 1;
    lives++;
    lives_draw_ = 1;
    Tone::play(2, 160, 6, 12);
    Tone::play(3, 200, 6, 10);
  }
  hud_dirty_ = 1;
}

void Game::present() {
  int score_col = saucer_.score_x_ >> 3;
  if (lives_draw_) {
    Hud::cannons();
    lives_draw_ = 0;
  }
  if (score_col < 0) {
    score_col = 0;
  }
  if (score_col > 35) {
    score_col = 35;
  }
  Screen::close();
  if (hud_dirty_) {
    Hud::text(best_, demo_);
    hud_dirty_ = 0;
  }
  if (saucer_.score_t_ > 0) {
    bbc::colour(128);
    bbc::tab((unsigned char)score_col, 4);
    Text::write("    ");
    bbc::tab((unsigned char)score_col, 4);
    bbc::colour(ColRed);
    Text::quad(saucer_.score_n_);
  } else if (saucer_.score_n_ != 0) {
    bbc::colour(128);
    bbc::tab((unsigned char)score_col, 4);
    Text::write("    ");
    saucer_.score_n_ = 0;
  }
}

void Game::deploy() {
  int drop = (wave - 1) << 3;
  if (drop > 40) {
    drop = 40;
  }
  phase = PhasePlay;
  saucer_.reset();
  volley_.reset();
  boom_on_ = 0;
  fleet_.reset(Metrics::OriginY + drop);
  step_t_ = (unsigned char)fleet_.pace(wave);
  bomb_t_ = 30;
  ufo_wait_ = 180;
  Tone::quiet(3);
  Screen::close();
  bbc::cls();
  bbc::cursor(0);
  Screen::fill(0, Metrics::LineY, 320, 2, ColGreen);
  Screen::fill(0, Metrics::GroundY, 320, 2, ColGreen);
  shelters_.draw();
  fleet_.draw();
  cannon_.draw();
  lives_draw_ = 1;
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
  lives_draw_ = 1;
  hud_dirty_ = 1;
  boom_on_ = 1;
  boom_x_ = cannon_.x_;
  boom_y_ = Metrics::PlayerY;
  boom_t_ = 36;
  volley_.reset();
  Tone::play(0, 6, 14, 15);
  Tone::play(2, 36, 10, 15);
}

void Game::steer(int* dir, int* do_fire) {
  int tx;
  *do_fire = 0;
  *dir = 0;
  if (!fleet_.column_alive(hunt_)) {
    hunt_++;
    if (hunt_ >= Metrics::Columns) {
      hunt_ = 0;
    }
  }
  tx = fleet_.column_x(hunt_);
  if (cannon_.x_ + 6 < tx) {
    *dir = 1;
  } else if (cannon_.x_ > tx + 6) {
    *dir = -1;
  } else {
    *do_fire = 1;
  }
}

void Game::score_hit(int scored) {
  if (scored == 1000) {
    unsigned prize = ufo_prize[volley_.slot()];
    saucer_.score_n_ = prize;
    add_score(prize);
    return;
  }
  add_score((unsigned)scored);
  Tone::play(0, 6, 3, 15);
  shelters_.strike(fleet_.last_x_ + 8, fleet_.last_y_ + 4);
  boom_on_ = 1;
  boom_x_ = fleet_.last_x_;
  boom_y_ = fleet_.last_y_;
  boom_t_ = 6;
}

void Game::march() {
  int stepped;
  if (phase != PhasePlay) {
    return;
  }
  if (step_t_ > 0) {
    step_t_--;
    return;
  }
  stepped = fleet_.step(&shelters_);
  Tone::march(march_i_);
  march_i_++;
  step_t_ = (unsigned char)fleet_.pace(wave);
  if (stepped == 2) {
    landed_ = 1;
    lives = 0;
    kill_player();
  }
}

void Game::launch_bomb() {
  int guard = 0;
  int col;
  if (phase != PhasePlay || alive == 0) {
    return;
  }
  if (bomb_t_ > 0) {
    bomb_t_--;
    return;
  }
  bomb_t_ = 20;
  if (alive < 16) {
    bomb_t_ = 10;
  }
  if (wave > 2 && bomb_t_ > 8) {
    bomb_t_ = (unsigned char)(bomb_t_ - 4);
  }
  col = (int)(rng_.next() & 15);
  while (guard < Metrics::Columns) {
    int bx;
    int by;
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
  if (phase != PhasePlay || saucer_.on_ || saucer_.score_t_ != 0) {
    return;
  }
  if (ufo_wait_ > 0) {
    ufo_wait_--;
    return;
  }
  saucer_.launch(rng_.next() & 1);
  ufo_wait_ = 280;
}

void Game::play() {
  int dir = 0;
  int do_fire = 0;
  int scored;
  if (boom_on_) {
    Screen::fill(boom_x_, boom_y_, 16, 8, ColBlack);
    if (boom_t_ == 0) {
      boom_on_ = 0;
    }
  }
  if (demo_) {
    steer(&dir, &do_fire);
  } else {
    dir = Keys::axis();
    if (Keys::fire()) {
      do_fire = 1;
    }
  }
  cannon_.erase();
  volley_.erase();
  saucer_.erase();
  cannon_.move(dir);
  if (do_fire && volley_.fire(cannon_.x_, ufo_i_)) {
    ufo_i_++;
    if (ufo_i_ >= 15) {
      ufo_i_ = 0;
    }
  }
  scored = volley_.move_shot(&fleet_, &shelters_, &saucer_);
  if (scored > 0) {
    score_hit(scored);
  }
  if (phase == PhasePlay && ((clock_ & 1) == 0 || wave > 2)) {
    if (volley_.move_bombs(&shelters_, cannon_.x_)) {
      kill_player();
    }
  }
  saucer_.step();
  march();
  launch_bomb();
  launch_ufo();
  if (phase == PhasePlay && alive == 0) {
    phase = PhaseWave;
    wave_t_ = 24;
    Tone::play(2, 120, 4, 12);
    Tone::play(3, 160, 4, 12);
  }
  if (phase == PhasePlay) {
    cannon_.draw();
    volley_.draw();
    saucer_.draw();
  }
  if (boom_on_) {
    Screen::stamp(boom_x_, boom_y_, k_boom, 16, 8);
    if (boom_t_ > 0) {
      boom_t_--;
    }
  }
  present();
  if (saucer_.score_t_ > 0) {
    saucer_.score_t_--;
  }
}

int Game::tick() {
  clock_++;
  bbc::osbyte(bbc::osbyte_command::wait_vsync, 0, 0);
  if (Keys::escape()) {
    bbc_return();
  }
  if (phase == PhaseOver) {
    if (Keys::fire() || (demo_ && Keys::axis() != 0)) {
      return 1;
    }
    if (demo_) {
      if (over_t_ > 0) {
        over_t_--;
      }
      if (over_t_ == 0) {
        return 3;
      }
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
        Tone::play(0, 2, 20, 15);
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
  if (demo_ && (Keys::fire() || Keys::axis() != 0)) {
    return 1;
  }
  play();
  return 0;
}

int Game::session() {
  score = 0;
  lives = 3;
  wave = 1;
  extra_ = 0;
  landed_ = 0;
  ufo_i_ = 0;
  march_i_ = 0;
  clock_ = 0;
  hunt_ = 5;
  hud_dirty_ = 1;
  lives_draw_ = 1;
  cannon_.reset();
  shelters_.reset();
  deploy();
  for (;;) {
    int result = tick();
    if (result != 0) {
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
  note_acccon();
  rng_.seed();
  demo_ = attract == 2;
  for (;;) {
    int again = session();
    demo_ = again == 3;
  }
}

int main() {
  bbc::mode(130);
  if (Title::show() == 0) {
    bbc_return();
  }
  bbc::mode(129);
  game.run();
  bbc_return();
  return 0;
}
