#include "common.h"

#ifdef LTL

#include "libP1.h"
#include "libP2.h"
#include "libP3.h"

#endif // LTL

#ifdef RTL

#include <dlfcn.h>

#define LIB_PATH "lib1.so"
#define ERR_SYM "error"
#define DEL_SYM "delay"
#define NOP_SYM "NOP"

#define DLSYM(X,Y) {       \
  (X) = (typeof (X)) dlsym(dlib, Y);     \
  err = dlerror();         \
  if(err != NULL) {        \
    fatal(err); \
  }}

#endif //RTL

#define USAGE { fatal("compile with either -DRTL (make 1RTL) or -DLTL (make 1LTL)"); }

int main(void)
{
#ifndef LTL
#ifndef RTL
  USAGE;
#endif // RTL
#endif //LTL

#if defined ( LTL ) || ( RTL )

  void (*errPtr)(const char*, int);
  void (*delPtr)(int);
  void (*NOPtr)();

#endif //defined LTL || RTL

#ifdef RTL
#ifdef LTL
  USAGE;
#endif // LTL

  char* dlib = dlopen(LIB_PATH, RTLD_NOW);
  if(dlib == NULL) {
    info("dlopen failed");
    fatal(dlerror());
  }
  dlerror();
  char* err;
  DLSYM(errPtr, ERR_SYM);
  DLSYM(delPtr, DEL_SYM);
  DLSYM(NOPtr, NOP_SYM);

#endif // RTL

#ifdef LTL

  errPtr = &error;
  delPtr = &delay;
  NOPtr = &NOP;

#endif //LTL

#if defined ( LTL ) || ( RTL )

  (*errPtr)("TEST: Info error", 1); 
  (*delPtr)(100);
  (*errPtr)("TEST: Info error2", 1); 
  (*NOPtr)();
  (*NOPtr)();
  (*errPtr)("TEST: Fatal error", 0); 
  return 0;

#endif //defined LTL || RTL
}
