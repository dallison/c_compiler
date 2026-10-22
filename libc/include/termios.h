#ifndef davecc_termios_h
#define davecc_termios_h

typedef unsigned tcflag_t;
typedef unsigned cc_t;
typedef unsigned speed_t;

#define NCCS 32

struct termios {
  tcflag_t c_iflag;
  tcflag_t c_oflag;
  tcflag_t c_cflag;
  tcflag_t c_lflag;
  cc_t c_cc[NCCS];
};

#define ECHO 0000010
#define ICANON 0000002
#define ISIG 0000001
#define VMIN 6
#define VTIME 5

#ifdef __cplusplus
extern "C" {
#endif

int tcgetattr(int fd, struct termios* termios);
int tcsetattr(int fd, int actions, const struct termios* termios);

#ifdef __cplusplus
}
#endif

#endif
