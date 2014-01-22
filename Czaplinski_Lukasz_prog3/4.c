#include "common.h"
#include <sys/ioctl.h>

volatile int termsize = -1;
volatile char should_print_termsize = 1 < 0;

void get_termsize()
{
  struct winsize arg;
  memset(&arg, 0, sizeof(struct winsize));
  ioctl(STDIN_FILENO, TIOCGWINSZ, &arg);
  termsize = arg.ws_row * arg.ws_col;
  should_print_termsize = 1 > 0;
}

void print_termsize()
{
  assert(should_print_termsize);
  int ts = termsize;
  printf("Termsize: %d\n", ts);
  should_print_termsize = 1 < 0;
}

void sigint_handler(int sig)
{
  switch(sig) {
    case SIGINT:
      exit(EXIT_SUCCESS);
    default:
      break;
  }
}

void sigwinch_handler(int sig)
{
  switch(sig) {
    case SIGWINCH:
      get_termsize();
      break;
    default:
      break;
  }
}

void set_handlers()
{
  struct sigaction act;
  memset(&act, 0, sizeof(struct sigaction));
  act.sa_handler = sigint_handler;
  if(sigaction(SIGINT, &act, NULL) == -1) {
    fatal("SIGINT sigaction");
  }
  act.sa_handler = sigwinch_handler;
  if(sigaction(SIGWINCH, &act, NULL) == -1) {
    fatal("SIGWINCH sigaction");
  }

}

int main(void)
{
  if(! isatty( STDIN_FILENO )) {
    fprintf(stderr, "Run on term only!\n");
    exit(EXIT_FAILURE);
  }
  set_handlers();
  get_termsize();
  while(1 > 0) {
    if(should_print_termsize) {
      print_termsize();
    } else {
      sleep(1);
    }
  }
  return 0;
}
