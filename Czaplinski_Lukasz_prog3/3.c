#include "common.h"

#define PAGE_SIZE 4096
#define SEVERAL 4*

void* mem = NULL;
int memsize = -1;
volatile siginfo_t SI;
volatile int sig_error = 0;

void init_mem()
{
  int pagesize = sysconf( _SC_PAGESIZE);
  memsize = SEVERAL pagesize;
  mem = mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  if(mem == NULL) {
    fatal("mmap");
  }
}

void mem_set(char a)
{
  for (int i = 0; sig_error == 0 && i < memsize; i++) {
    ((char*)mem)[i] = a;
  }
}

void mem_set_protread()
{
  if(mprotect(mem, memsize, PROT_READ) == -1) {
    fatal("mprotect");
  }
}

void simple_sigsegv_handler(int sig __attribute__((unused)) ) 
{
  sig_error = -1;
}

void write_siginfo()
{
  fprintf(stderr, "si_addr: %p\n", SI.si_addr);
  fprintf(stderr, "si_code: %d\n", SI.si_code);
}

void sigsegv_handler(int sig, siginfo_t* siginfo, void* ptr __attribute__((unused)) )
{
  switch(sig) {
    case SIGSEGV:
      SI = *siginfo;
      sig_error = 1;
      atexit(write_siginfo);
      exit(EXIT_SUCCESS);
      break;
    default:
      sig_error = -1;
  }
}

void set_sigsegvhandler()
{
  struct sigaction act;
  memset(&act, 0, sizeof(struct sigaction));
  act.sa_flags = SA_SIGINFO ;
  act.sa_sigaction = sigsegv_handler;
  if(sigaction(SIGSEGV, &act, NULL) == -1) {
    fatal("sigaction");
  }
}

int main(void)
{
  init_mem();
  char* cmem = (char*)mem;
  set_sigsegvhandler();
  mem_set('a');
  printf(" in mem: %c%c%c\n", cmem[0], cmem[4098], cmem[memsize - 1]); 
  if(sig_error != 0) {
    switch(sig_error) {
      case -1:
        fatal("something unpredicted..");
        break;
      case 1:
        write_siginfo();
        fatal("SIGSEGV too soon!");
    }
  }
  mem_set_protread();
  while(sig_error == 0) {
    printf("Trying to write..\n");
    mem_set('b');
    printf(" in mem: %c%c%c\n", cmem[0], cmem[4098], cmem[memsize - 1]); 
  }
  fatal("...shouldn't be here...");
  return 0;
}
