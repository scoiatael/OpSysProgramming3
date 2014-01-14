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

#define timediff(t1,t2) ((t2.tv_sec - t1.tv_sec)*1000LL+(t2.tv_usec - t1.tv_usec))

#define DIFF(X,Y) { struct timeval t1,t2; gettimeofday(&t1,NULL); X; gettimeofday(&t2,NULL); Y=timediff(t1,t2);}

#ifndef NOFUNC

void fatal(const char* msg) 
{
  fprintf(stderr, msg);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

void info(const char* msg)
{
  fprintf(stderr, msg);
  fprintf(stderr, "\n");
}
#endif //NOFUNC

#define ERRF 
#endif /* end of include guard: COMMON_H */
