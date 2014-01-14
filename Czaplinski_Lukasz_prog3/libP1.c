#include "libP1.h"

void error(const char* message, int infoLevel)
{
  switch(infoLevel) {
    case 0:
      fprintf(stderr, "%s\n", message);
      exit(EXIT_FAILURE);
    case 1:
      fprintf(stdout, "%s\n", message);
      break;
    default:
      break;
  }
}

