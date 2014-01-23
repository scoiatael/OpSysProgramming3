#define __need_IOV_MAX
#define DEBUG
#include "common.h"
#include <sys/uio.h>
#include <limits.h>
unsigned int DotSize;
char* Dots;

void dots_init(int N)
{
  DotSize = N*(N+1) / 2 + N - 1;
  Dots = malloc( DotSize + 1);
  memset(Dots, '*', DotSize);
  for (int i = 2, accum = 1; i <= N; i++, accum = accum + i ) {
    Dots[accum] = '\n';
  }
  Dots[DotSize] = '\0';
}

void print_triangle(unsigned int L, void (*init)(void), void(*print)(const char* string, int length))
{
  init();
  for (unsigned int i = 0; i < DotSize - DotSize/L + 1; i = i + DotSize/L) {
    print(Dots + i, DotSize/L);
  }
  print(Dots + DotSize - ( DotSize % L ) - 1, DotSize % L);
}

void null_init()
{
}

void write_pr(const char* str, int length)
{
  write(STDOUT_FILENO, str, length);
}


void writev_pr(const char* str , int length )
{
  struct iovec iov;
  iov.iov_base = (char*)str;
  iov.iov_len = length;
  writev(STDOUT_FILENO, &iov, 1);
}

void printf_init()
{
  setvbuf(stdout, NULL, _IOFBF, IOV_MAX);
}

void printf_pr(const char* str, int length )
{
  fwrite(str, length, 1, stdout);
}

const char* opts = "N:L:t:h";

void print_usage()
{
  puts(" options: ");
  puts(opts);
}

int main(int argc, char * const argv[])
{
  int N = 10, L = 10;
  int opt;
  int def = 0;
  while( (opt = getopt(argc, argv, opts)) != -1) {
    switch(opt) {
      case 'N':
        N = atoi(optarg);
        break;
      case 'L':
        L = atoi(optarg);
        break;
      case 't':
        def = atoi(optarg);
        break;
      case'h':
        print_usage();
        exit(EXIT_SUCCESS);
        break;
    }
  } 
  dots_init(N);
//  printf(" N: %d L: %d t:%d\n", N,L,def);
  switch(def) {
    case 0:
      print_triangle(L, null_init, write_pr);
      break;
    case 1:
      print_triangle(L, null_init, writev_pr);
      break;
    case 2:
      print_triangle(L, null_init, printf_pr);
      break;
    case 3:
      print_triangle(L, printf_init, printf_pr);
      break;
  }
  return 0;
}
