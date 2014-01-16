#include "lib2.h"
#include "common.h"

#define MAX_ALLOC 100

void sigint_handler(int sig)
{
  char buf[256];
  sprintf(buf, "Caught %d", sig);
  if(sig == SIGINT)
    exit(EXIT_SUCCESS);
}

void test(FILE* f)
{
  void* ALLOCATED[MAX_ALLOC];
  size_t last_alloc = 0;
  unsigned long long num;
  for(int try=0;;try++) {
    putc('#', stdout);
    fflush(stdout);
    char ch = ' ';
    while(ch == ' ' || ch == '\n') {
      if(fscanf(f,"%c", &ch) == EOF) {
        exit(EXIT_SUCCESS);
      }
    }
    switch(ch) {
      case 'a':
//        printf("amount: ");
        fscanf(f,"%lld", &num);
        char* mem;
        if(NULL == (mem = my_malloc( num)) ) {
          printf("malloc Error\n");
        } else {
          memset(mem, (unsigned char)try, num);
          printf(" malloc pointer: %p\n", mem);
          if(last_alloc < MAX_ALLOC) {
            ALLOCATED[last_alloc] = mem;
            last_alloc++;
          } else {
            fprintf(stderr, "Too many bits, ptr not saved\n");
          }
        }
        break;
      case 'f':
//        printf("index for free: ");
        fscanf(f, "%llu", &num);
        if(num > last_alloc) {
          fprintf(stderr, "bad index\n");
        } else {
          my_free(ALLOCATED[num]);
          ALLOCATED[num] = ALLOCATED[last_alloc];
          last_alloc = last_alloc - 1;
        }
        break;
      case 'd':
        for(unsigned int i = 0; i<last_alloc; i++) {
          printf("%d : %p\n", i, ALLOCATED[i]);
        }
        _pcon_debug();
        break;
      case 'q':
        exit(EXIT_SUCCESS);
      case 'h':
      default:
        printf("[q]uit, [a]lloc <amount>, [f]ree <index>, [d]ebug or [h]elp?\n");
    }
  }

}

int main(int argc, const char *argv[])
{
  if(signal(SIGINT, &sigint_handler) == SIG_ERR)
    info("signal(SIGINT) failed");
  printf("PAGESIZE: %ld\n", sysconf(_SC_PAGESIZE));
  if(argc != 1 && strcmp(argv[1],"-i")==0) {
    info("interactive mode");
    test(stdin);
  } else {
    FILE* f = fopen("2Test.txt", "r");
    test(f);
  }
  return 0;
}

