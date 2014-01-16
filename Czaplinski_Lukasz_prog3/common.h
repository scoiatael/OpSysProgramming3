#ifndef COMMON_H

#define COMMON_H

#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>

#define timediff(t1,t2) ((t2.tv_sec - t1.tv_sec)*1000LL+(t2.tv_usec - t1.tv_usec))

#define DIFF(X,Y) { struct timeval t1,t2; gettimeofday(&t1,NULL); X; gettimeofday(&t2,NULL); Y=timediff(t1,t2);}

#ifndef NO_COM_DEF

void info(const char* msg)
{
  fprintf(stderr, msg);
  fprintf(stderr, "\n");
}

void fatal(const char* msg) 
{
  info(msg);
  perror(": ");
  exit(EXIT_FAILURE);
}
#endif //NOFUNC

#define ERRF 
#endif /* end of include guard: COMMON_H */
