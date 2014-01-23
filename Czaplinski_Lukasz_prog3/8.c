#define DEBUG
#define _BSD_SOURCE
#include "common.h"

#define FILENAME "8_random_name"
#define MODE (S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH | S_IRGRP | S_IWGRP)

#define SEVERAL 128

int fd = -1;
char* mem = NULL;
int memsize = -1;
char init = 0;

void _print_mincore_helper(char c, int count)
{
  switch(count) {
    default:
      printf(":%d%c:", count,c);
      break;
    case 3:
      putchar(c);
    case 2:
      putchar(c);
    case 1:
      putchar(c);
      break;
  }
}
void print_mincore()
{
  unsigned char* coredump = malloc(SEVERAL + 1);
  if(mincore(mem, memsize, coredump) == -1) {
    fatal("mincore");
  }
  int s = 0, m = 0;
  for (int i = 0; i < SEVERAL; i++) {
    switch(coredump[i] & 1) {
      case 0:
        if(m != 0) {
          _print_mincore_helper('M', m);
          m = 0;
        }
        s++;
        break;
      case 1:
        if(s != 0) {
          _print_mincore_helper('S', s);
          s = 0;
        }
        m++;
        break;
    }
  }
  if(s != 0) {
    _print_mincore_helper('S', s);
    s = 0;
  }
  if(m != 0) {
    _print_mincore_helper('M', m);
    m = 0;
  }
  putchar('\n');
}

void mem_advise(int advise)
{
  if(madvise(mem, memsize, advise) == -1) {
    fatal("madvise");
  }
}


void close_descriptor(int* i)
{
  if(*i != -1) {
    close(*i);
  }
}

void free_fd()
{
  close_descriptor(&fd);
}

void free_mem()
{
  munmap(mem, memsize);
}

void unlink_f()
{
  unlink(FILENAME);
}

void mem_touch()
{
  int index = (memsize >> 1) -1;
  mem[index] = 'b';
}

void mem_lock()
{
  if(mlock(mem, memsize >> 1) != 0) {
    fatal("mlock");
  }
}

void mem_init()
{
  memsize = SEVERAL * sysconf(_SC_PAGESIZE);
  if((fd = open(FILENAME,O_RDWR | O_CREAT | O_TRUNC ,MODE)) < 0) {
    fatal("creat");
  }
  if((ftruncate(fd, memsize)) != 0) {
    fatal("ftruncate");
  }
  if((mem = mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_PRIVATE , fd, 0)) == MAP_FAILED) {
    fatal("mmap");
  }
  if(init == 0) {
    atexit(free_fd);
    atexit(free_mem);
    atexit(unlink_f);
    init = 1;
  }
}

void mem_reset()
{
  if(init == 0){
    return;
  }
  free_fd();
  free_mem();
  mem_init();
}

void DO_STH_PRODUCTIVE()
{
    puts("  ( pages: [M]emory, [S]wapped out )");
    puts("start..");
    print_mincore();
    puts(" ..after dontneed:");
  mem_advise(MADV_DONTNEED);
    print_mincore();
  mem_reset();
    puts(" ..after willneed:");
  mem_advise(MADV_WILLNEED);
    print_mincore();
  mem_reset();
    puts(" ..after random:");
  mem_advise(MADV_RANDOM);
  mem_touch();
    print_mincore();
  mem_reset();
    puts(" ..after sequential"); 
  mem_advise(MADV_SEQUENTIAL);
  mem_touch();
    print_mincore();
  mem_reset();
    puts(" ..locking");
    mem_lock();
  mem_advise(MADV_DONTNEED);
// Never get here -> can't advise dontneed on locked pages
  puts("after lock & swapout");
  print_mincore();
}

int main()
{
  mem_init();
  DO_STH_PRODUCTIVE();
  return 0;
}
