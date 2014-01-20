#include "lib2.h"
#define DEBUG
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
  assert(f != NULL);
  void* ALLOCATED[MAX_ALLOC];
  size_t last_alloc = 0;
  unsigned long long num,ptr;
  char* mem;
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
      case 'c':
//        printf("amount: ");
        fscanf(f,"%llu", &ptr);
        fscanf(f,"%llu", &num);
        if(NULL == (mem = calloc(ptr, num)) ) {
          printf("calloc error\n");
        } else {
          memset(mem, (unsigned char)try, num*ptr);
          printf(" calloc pointer: %p\n", mem);
          if(last_alloc < MAX_ALLOC) {
            ALLOCATED[last_alloc] = mem;
            last_alloc++;
          } else {
            fprintf(stderr, "too many bits, ptr not saved\n");
          }
        }
        break;
      case 'a':
//        printf("amount: ");
        fscanf(f,"%lld", &num);
        if(NULL == (mem = malloc( num)) ) {
          printf("malloc error\n");
        } else {
          memset(mem, (unsigned char)try, num);
          printf(" malloc pointer: %p\n", mem);
          if(last_alloc < MAX_ALLOC) {
            ALLOCATED[last_alloc] = mem;
            last_alloc++;
          } else {
            fprintf(stderr, "too many bits, ptr not saved\n");
          }
        }
        break;
      case 'r':
//        printf("amount: ");
        fscanf(f,"%llu", &ptr);
        fscanf(f,"%lld", &num);
        if(NULL == (mem = realloc(ALLOCATED[ptr], num)) ) {
          printf("realloc Error\n");
        } else {
          memset(mem, (unsigned char)try, num);
          printf(" realloc pointer: %p\n", mem);
          ALLOCATED[ptr] = mem;
        }
        break;
      case 'f':
//        printf("index for free: ");
        fscanf(f, "%llu", &num);
        if(num > last_alloc) {
          fprintf(stderr, "bad index\n");
        } else {
          free(ALLOCATED[num]);
          last_alloc = last_alloc - 1;
          ALLOCATED[num] = ALLOCATED[last_alloc];
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
        printf("[q]uit, [a]lloc <amount>, [f]ree <index>, [c]alloc <nmem> <size>, [r]ealloc <index> <amount>, [d]ebug or [h]elp?\n");
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
    FILE* f;
    if(argc < 3 || strcmp(argv[1], "-f") != 0) {
      f = fopen("2Test.txt", "r");
    } else {
      info("Using file:");
      info(argv[2]);
      f = fopen(argv[2], "r");
    }
    test(f);
  }
  return 0;
}

