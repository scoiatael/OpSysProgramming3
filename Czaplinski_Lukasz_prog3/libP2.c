#include "libP2.h"

#define otM (1000000000)

void delay(int r)
{
  struct timespec nSleepTime;
  memset(&nSleepTime, 0, sizeof(struct timespec));
  nSleepTime.tv_nsec = r % otM;
  nSleepTime.tv_sec = r / otM; 
  nanosleep(&nSleepTime, &nSleepTime);
}


