// Snake for the BBC Micro, linked with the sideways libc.
//
// The 16-colour screen is mode 2 (160x256). Mode 5 has the same pixels
// and only four colours. A Model B mode 2 screen starts at &3000, and
// this program does not fit under it, so main selects mode 130: mode 2
// drawn from the Master's shadow RAM, with HIMEM left at &8000.
//
//   ZX:/ or arrows or WASD steer. Space starts. Escape returns to BASIC.
//
// Build:
//   DAVECC_LIB_DIR=<paged-libc> davecc -target 6502-acorn-bbc-davecc \
//     -std=c++20 -fno-exceptions -Wl,-T -Wl,bbc_mode2.ld games/snake.cc

#include <bbc.h>

extern "C" void bbc_return(void);

// Plain names so a RAM dump can see how far a headless run got.
extern "C" unsigned score;
extern "C" unsigned char lives;
extern "C" unsigned char snake_state;

enum class Direction : unsigned char { Up = 0, Right = 1, Down = 2, Left = 3 };

enum class Step : unsigned char { Moved = 0, Ate = 1, Died = 2 };

class Metrics {
 public:
  enum : int {
    Cell = 32,
    Cols = 24,
    Rows = 22,
    Max = 128,
    OriginX = 256,
    OriginY = 160,
    Heart = 224
  };
};

class Keys {
 public:
  static int pressed(unsigned char code);
  static int escape();
};

class Sound {
 public:
  static void vsync();
  static void tone(unsigned char pitch, unsigned char duration);
  static void wait(int frames);
};

class Text {
 public:
  static void put(const char* text);
  static void put4(unsigned value);
  static void spaces(int count);
};

class Board {
 public:
  static int column(unsigned char col);
  static int row(unsigned char line);
  static void rect(int x0, int y0, int x1, int y1);
  static void erase(unsigned char col, unsigned char line);
  static void body(int x, int y);
  static void head(int x, int y, Direction facing, int colour);
  static void apple(int x, int y);
  static void frame();
  static void title_word();
  static void title_preview();

 private:
  static void blit(int x, int y, int index, int pix, int colour);
};

class Rng {
 public:
  unsigned next();

 private:
  unsigned state_;
};

class Snake {
 public:
  void reset();
  void draw() const;
  void sample();
  int covers(unsigned char col, unsigned char line, int ignore_tail) const;
  Step step(unsigned char apple_col, unsigned char apple_row);
  void paint_dead() const;
  void paint_move(unsigned char previous) const;

  unsigned char head_index() const { return head_; }

 private:
  void queue(Direction turn);
  unsigned char col_[Metrics::Max];
  unsigned char row_[Metrics::Max];
  unsigned char head_;
  unsigned char length_;
  Direction dir_;
  Direction next_;
};

class Apple {
 public:
  void reset();
  void place(const Snake& snake, Rng& rng);
  void draw() const;
  unsigned char column() const { return col_; }
  unsigned char line() const { return row_; }

 private:
  unsigned char col_;
  unsigned char row_;
};

class Hud {
 public:
  static void install();
  static void show(unsigned points, unsigned best, unsigned char left);
  static void hint();
  static void game_over();
};

class Title {
 public:
  static int show();
};

class Game {
 public:
  void run();

 private:
  void deal();
  int pace() const;
  int tick();
  void over();
  void match();

  Snake snake_;
  Apple apple_;
  Rng rng_;
  unsigned best_;
};

int Keys::pressed(unsigned char code) {
  bbc::os_regs regs = bbc::osbyte(bbc::osbyte_command::inkey,
                                  (unsigned char)(0 - code), 0xff);
  return regs.x != 0;
}

int Keys::escape() { return pressed(113); }

void Sound::vsync() { bbc::osbyte(bbc::osbyte_command::wait_vsync, 0, 0); }

void Sound::tone(unsigned char pitch, unsigned char duration) {
  unsigned char block[8];
  block[0] = 1;
  block[1] = 0;
  block[2] = 0xf1;
  block[3] = 0xff;
  block[4] = pitch;
  block[5] = 0;
  block[6] = duration;
  block[7] = 0;
  bbc::osword(7, block);
}

void Sound::wait(int frames) {
  while (frames > 0) {
    vsync();
    frames--;
  }
}

void Text::put(const char* text) {
  while (*text != 0) {
    bbc::oswrch((unsigned char)*text);
    text++;
  }
}

void Text::put4(unsigned value) {
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

void Text::spaces(int count) {
  while (count > 0) {
    bbc::oswrch(' ');
    count--;
  }
}

int Board::column(unsigned char col) {
  return Metrics::OriginX + (col << 5);
}

int Board::row(unsigned char line) {
  return Metrics::OriginY + (line << 5);
}

void Board::rect(int x0, int y0, int x1, int y1) {
  bbc::move(x0, y0);
  bbc::move(x1, y0);
  bbc::triangle(x0, y1);
  bbc::move(x1, y0);
  bbc::move(x1, y1);
  bbc::triangle(x0, y1);
}

void Board::erase(unsigned char col, unsigned char line) {
  bbc::gcol(BBC_GCOL_SET, 0);
  rect(column(col), row(line), column(col) + Metrics::Cell - 1,
       row(line) + Metrics::Cell - 1);
}

void Board::body(int x, int y) {
  bbc::gcol(BBC_GCOL_SET, 2);
  rect(x + 4, y + 4, x + Metrics::Cell - 5, y + Metrics::Cell - 5);
  bbc::gcol(BBC_GCOL_SET, 3);
  rect(x + 12, y + 12, x + Metrics::Cell - 13, y + Metrics::Cell - 13);
}

void Board::head(int x, int y, Direction facing, int colour) {
  int cx = x + 16;
  int cy = y + 16;
  int ax = cx;
  int ay = cy;
  bbc::gcol(BBC_GCOL_SET, colour);
  rect(x + 2, y + 2, x + Metrics::Cell - 3, y + Metrics::Cell - 3);
  if (facing == Direction::Up) {
    ay += 4;
  } else if (facing == Direction::Down) {
    ay -= 4;
  } else if (facing == Direction::Left) {
    ax -= 4;
  } else {
    ax += 4;
  }
  bbc::gcol(BBC_GCOL_SET, 0);
  if (facing == Direction::Left || facing == Direction::Right) {
    rect(ax, ay - 8, ax + 6, ay - 2);
    rect(ax, ay + 2, ax + 6, ay + 8);
  } else {
    rect(ax - 8, ay, ax - 2, ay + 6);
    rect(ax + 2, ay, ax + 8, ay + 6);
  }
  bbc::gcol(BBC_GCOL_SET, 7);
  rect(ax - 2, ay - 2, ax + 2, ay + 2);
}

void Board::apple(int x, int y) {
  int cx = x + 16;
  int cy = y + 13;
  bbc::gcol(BBC_GCOL_SET, 1);
  bbc::move(cx, cy + 11);
  bbc::move(cx + 9, cy);
  bbc::triangle(cx, cy - 9);
  bbc::move(cx + 9, cy);
  bbc::move(cx, cy - 9);
  bbc::triangle(cx - 9, cy);
  bbc::gcol(BBC_GCOL_SET, 7);
  rect(cx - 7, cy + 1, cx - 2, cy + 5);
  bbc::gcol(BBC_GCOL_SET, 2);
  rect(cx - 1, cy + 9, cx + 2, cy + 15);
  bbc::move(cx, cy + 11);
  bbc::move(cx + 9, cy + 15);
  bbc::triangle(cx + 1, cy + 8);
}

void Board::frame() {
  int x0 = Metrics::OriginX - 24;
  int y0 = Metrics::OriginY - 24;
  int x1 = Metrics::OriginX + Metrics::Cols * Metrics::Cell + 23;
  int y1 = Metrics::OriginY + Metrics::Rows * Metrics::Cell + 23;
  bbc::gcol(BBC_GCOL_SET, 6);
  rect(x0, y0, x1, y1);
  bbc::gcol(BBC_GCOL_SET, 4);
  rect(x0 + 8, y0 + 8, x1 - 8, y1 - 8);
  bbc::gcol(BBC_GCOL_SET, 0);
  rect(Metrics::OriginX, Metrics::OriginY,
       Metrics::OriginX + Metrics::Cols * Metrics::Cell - 1,
       Metrics::OriginY + Metrics::Rows * Metrics::Cell - 1);
  bbc::gcol(BBC_GCOL_SET, 3);
  rect(x0 + 4, y1 - 16, x0 + 16, y1 - 4);
  rect(x1 - 16, y1 - 16, x1 - 4, y1 - 4);
  rect(x0 + 4, y0 + 4, x0 + 16, y0 + 16);
  rect(x1 - 16, y0 + 4, x1 - 4, y0 + 16);
}

void Board::blit(int x, int y, int index, int pix, int colour) {
  static const unsigned char glyphs[25] = {
      0x0e, 0x10, 0x0e, 0x01, 0x1e,  // S
      0x11, 0x19, 0x15, 0x13, 0x11,  // N
      0x0e, 0x11, 0x1f, 0x11, 0x11,  // A
      0x11, 0x12, 0x1c, 0x12, 0x11,  // K
      0x1f, 0x10, 0x1e, 0x10, 0x1f   // E
  };
  int line;
  bbc::gcol(BBC_GCOL_SET, colour);
  for (line = 0; line < 5; line++) {
    unsigned char bits = glyphs[index + line];
    int col;
    for (col = 0; col < 5; col++) {
      if ((bits & (0x10 >> col)) != 0) {
        int px = x + col * pix;
        int py = y + (4 - line) * pix;
        rect(px, py, px + pix - 4, py + pix - 4);
      }
    }
  }
}

void Board::title_word() {
  int i;
  int x = 360;
  for (i = 0; i < 5; i++) {
    blit(x + 8, 672, i * 5, 20, 3);
    blit(x, 680, i * 5, 20, 2);
    x += 112;
  }
}

void Board::title_preview() {
  int i;
  int x = 488;
  for (i = 0; i < 6; i++) {
    body(x, 440);
    x += Metrics::Cell;
  }
  head(x, 440, Direction::Right, 3);
  apple(x + Metrics::Cell + 16, 440);
}

unsigned Rng::next() {
  state_ = (unsigned)(state_ * 25173 + 13849);
  return state_;
}

void Snake::reset() {
  unsigned char i;
  length_ = 5;
  head_ = 4;
  dir_ = Direction::Right;
  next_ = Direction::Right;
  for (i = 0; i < length_; i++) {
    col_[i] = (unsigned char)(4 + i);
    row_[i] = 10;
  }
}

void Snake::draw() const {
  unsigned char i;
  for (i = length_ - 1; i > 0; i--) {
    unsigned char index = (unsigned char)((head_ - i) & (Metrics::Max - 1));
    Board::body(Board::column(col_[index]), Board::row(row_[index]));
  }
  Board::head(Board::column(col_[head_]), Board::row(row_[head_]), dir_, 3);
}

void Snake::queue(Direction turn) {
  unsigned char dir = (unsigned char)dir_;
  unsigned char next = (unsigned char)next_;
  unsigned char want = (unsigned char)turn;
  if (((dir + 2) & 3) == want) {
    return;
  }
  if (((next + 2) & 3) == want) {
    return;
  }
  next_ = turn;
}

void Snake::sample() {
  if (Keys::pressed(58) || Keys::pressed(34) || Keys::pressed(73)) {
    queue(Direction::Up);
  } else if (Keys::pressed(42) || Keys::pressed(82) || Keys::pressed(105)) {
    queue(Direction::Down);
  } else if (Keys::pressed(26) || Keys::pressed(66) || Keys::pressed(98)) {
    queue(Direction::Left);
  } else if (Keys::pressed(122) || Keys::pressed(50) || Keys::pressed(67)) {
    queue(Direction::Right);
  }
}

int Snake::covers(unsigned char col, unsigned char line, int ignore_tail) const {
  unsigned char count = length_;
  unsigned char i;
  if (ignore_tail && count > 0) {
    count--;
  }
  for (i = 0; i < count; i++) {
    unsigned char index = (unsigned char)((head_ - i) & (Metrics::Max - 1));
    if (col_[index] == col && row_[index] == line) {
      return 1;
    }
  }
  return 0;
}

Step Snake::step(unsigned char apple_col, unsigned char apple_row) {
  unsigned char x = col_[head_];
  unsigned char y = row_[head_];
  int eat;
  dir_ = next_;
  if (dir_ == Direction::Up) {
    y++;
  } else if (dir_ == Direction::Right) {
    x++;
  } else if (dir_ == Direction::Down) {
    y--;
  } else {
    x--;
  }
  eat = x == apple_col && y == apple_row;
  if (x >= Metrics::Cols || y >= Metrics::Rows ||
      covers(x, y, eat ? 0 : 1)) {
    return Step::Died;
  }
  head_ = (unsigned char)((head_ + 1) & (Metrics::Max - 1));
  col_[head_] = x;
  row_[head_] = y;
  if (eat) {
    if (length_ < Metrics::Max - 1) {
      length_++;
    }
    return Step::Ate;
  }
  {
    unsigned char tail = (unsigned char)((head_ - length_) & (Metrics::Max - 1));
    Board::erase(col_[tail], row_[tail]);
  }
  return Step::Moved;
}

void Snake::paint_dead() const {
  Board::head(Board::column(col_[head_]), Board::row(row_[head_]), dir_, 1);
}

void Snake::paint_move(unsigned char previous) const {
  Board::body(Board::column(col_[previous]), Board::row(row_[previous]));
  Board::head(Board::column(col_[head_]), Board::row(row_[head_]), dir_, 3);
}

void Apple::reset() {
  col_ = 14;
  row_ = 10;
}

void Apple::place(const Snake& snake, Rng& rng) {
  unsigned char col;
  unsigned char line;
  int guard = 0;
  do {
    col_ = (unsigned char)(rng.next() & 31);
    row_ = (unsigned char)(rng.next() & 31);
    guard++;
  } while ((col_ >= Metrics::Cols || row_ >= Metrics::Rows ||
            snake.covers(col_, row_, 0)) &&
           guard < 40);
  if (col_ < Metrics::Cols && row_ < Metrics::Rows &&
      !snake.covers(col_, row_, 0)) {
    return;
  }
  for (line = 0; line < Metrics::Rows; line++) {
    for (col = 0; col < Metrics::Cols; col++) {
      if (!snake.covers(col, line, 0)) {
        col_ = col;
        row_ = line;
        return;
      }
    }
  }
}

void Apple::draw() const {
  Board::apple(Board::column(col_), Board::row(row_));
}

void Hud::install() {
  static const unsigned char bits[8] = {0x6c, 0xfe, 0xfe, 0x7c,
                                        0x38, 0x10, 0x00, 0x00};
  bbc::defchar(Metrics::Heart, bits);
}

void Hud::show(unsigned points, unsigned best, unsigned char left) {
  int i;
  bbc::colour(128);
  bbc::tab(0, 0);
  bbc::colour(3);
  Text::put("SCORE ");
  bbc::colour(7);
  Text::put4(points);
  bbc::colour(6);
  Text::put(" BEST ");
  bbc::colour(7);
  Text::put4(best);
  bbc::tab(0, 1);
  bbc::colour(7);
  Text::put("LIVES ");
  for (i = 0; i < 3; i++) {
    bbc::colour(i < left ? 1 : 4);
    bbc::oswrch(Metrics::Heart);
  }
  Text::spaces(11);
}

void Hud::hint() {
  bbc::colour(128);
  bbc::tab(0, 31);
  bbc::colour(6);
  // Mode 2 is 20 columns. A 20th character on the last row scrolls the
  // bitmap by one text line and leaves the opening snake behind.
  Text::put("ZX:/ ARROWS WASD");
}

void Hud::game_over() {
  bbc::colour(128);
  bbc::tab(5, 14);
  bbc::colour(1);
  Text::put("GAME OVER");
  bbc::tab(4, 16);
  bbc::colour(3);
  Text::put("SPACE PLAYS");
}

int Title::show() {
  int frames = 0;
  bbc::cls();
  bbc::cursor(0);
  bbc::colour(128);
  snake_state = 1;
  Board::title_word();
  Board::title_preview();
  bbc::gcol(BBC_GCOL_SET, 6);
  Board::rect(360, 636, 908, 644);
  bbc::tab(1, 28);
  bbc::colour(3);
  Text::put("SPACE STARTS");
  bbc::tab(1, 29);
  bbc::colour(7);
  Text::put("ARROWS OR ZX:/");
  bbc::tab(1, 30);
  bbc::colour(6);
  Text::put("WASD STEERS");
  bbc::tab(1, 31);
  bbc::colour(1);
  Text::put("ESCAPE QUITS");
  for (;;) {
    Sound::vsync();
    if (Keys::escape()) {
      return 0;
    }
    if (Keys::pressed(99) || Keys::pressed(58) || Keys::pressed(42) ||
        Keys::pressed(26) || Keys::pressed(122)) {
      return 1;
    }
    frames++;
    if (frames >= 90) {
      return 1;
    }
  }
}

void Game::deal() {
  snake_.reset();
  apple_.reset();
  bbc::cls();
  bbc::cursor(0);
  Board::frame();
  snake_.draw();
  apple_.draw();
  Hud::show(score, best_, lives);
  Hud::hint();
}

int Game::pace() const {
  if (score >= 150) {
    return 3;
  }
  if (score >= 70) {
    return 5;
  }
  if (score >= 30) {
    return 6;
  }
  return 8;
}

int Game::tick() {
  unsigned char previous = snake_.head_index();
  Step outcome = snake_.step(apple_.column(), apple_.line());
  if (outcome == Step::Died) {
    snake_state = 3;
    if (lives > 0) {
      lives--;
    }
    Hud::show(score, best_, lives);
    snake_.paint_dead();
    Sound::tone(28, 12);
    Sound::wait(20);
    return 0;
  }
  if (outcome == Step::Ate) {
    score += 10;
    if (score > best_) {
      best_ = score;
    }
    Sound::tone(140, 2);
    Sound::tone(180, 2);
    apple_.place(snake_, rng_);
    apple_.draw();
    Hud::show(score, best_, lives);
  }
  snake_.paint_move(previous);
  return 1;
}

void Game::over() {
  snake_state = 4;
  Hud::game_over();
  for (;;) {
    Sound::vsync();
    if (Keys::escape()) {
      bbc_return();
    }
    if (Keys::pressed(99)) {
      return;
    }
  }
}

void Game::match() {
  snake_state = 2;
  score = 0;
  lives = 3;
  deal();
  Sound::tone(100, 2);
  Sound::tone(140, 2);
  for (;;) {
    int frames = pace();
    while (frames > 0) {
      Sound::vsync();
      if (Keys::escape()) {
        bbc_return();
      }
      snake_.sample();
      frames--;
    }
    if (!tick()) {
      if (lives == 0) {
        over();
        return;
      }
      deal();
    }
  }
}

void Game::run() {
  Hud::install();
  for (;;) {
    if (!Title::show()) {
      return;
    }
    match();
  }
}

static Game game;

int main() {
  bbc::mode(130);
  game.run();
  bbc_return();
  return 0;
}
