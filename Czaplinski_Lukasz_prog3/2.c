#include "lib2.h"
#include "common.h"


int main(int argc, const char *argv[])
{
  printf("PAGESIZE: %ld\n", sysconf(_SC_PAGESIZE));
  if(argc == 1 || strcmp(argv[1],"-i")==0) {
    unsigned long long num;
    for(int try=0;;try++) {
      printf("[a]lloc, [f]ree or [d]ebug?\n");
      char ch = ' ';
      while(ch == ' ' || ch == '\n') {
        scanf("%c", &ch);
      }
      switch(ch) {
        case 'a':
          printf("amount: ");
          scanf("%lld", &num);
          char* mem;
          if(NULL == (mem = my_malloc( num)) ) {
            printf("malloc Error\n");
          } else {
            memset(mem, (unsigned char)try, num);
            printf(" malloc pointer: %p\n", mem);
          }
          break;
        case 'f':
          printf("pointer for free: ");
          scanf("%llx", &num);
          my_free((void*)num);
          break;
        case 'd':
          _pcon_debug();
          break;
        default:
          printf("only [a] or [f] please\n");
      }
    }
  } else {
    /* ..TODO.. */
  }
  return 0;
}

