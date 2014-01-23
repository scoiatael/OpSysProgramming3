#define DEBUG
#include "common.h"
#include <malloc.h>
#include <pthread.h>

static void my_init();

void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook)(void) = my_init;

extern void* __libc_malloc(size_t size);
extern void* __libc_calloc(size_t num, size_t size);
extern void* __libc_realloc(void* ptr, size_t size);
extern void __libc_free(void* ptr);

#define IF_MALLOC_ACTIVE if(malloc_hook_active == 1)
#define ELSE_GLIBC } else {

static char malloc_hook_active = 1;

#define INFO(X) { malloc_hook_active = 0; info(X); malloc_hook_active = 1; }
#define FATAL(X) { malloc_hook_active = 0; fatal(X); }

#define MAX(X,Y) ((X > Y) ? (X) : (Y))
#define PAGE_GRAIN MAX((2*sizeof(size_t)), sizeof(node_t) + 10) 
#define ALIGN 4

typedef struct node_desc {
  size_t size;
  struct node_desc* prev;
  struct node_desc* next;
} node_t;

#define NODE_DATA(node) ((void*)&node[1])
#define NODE_OWNER(ptr) ((node_t*)((char*)ptr - sizeof(node_t)))

#define LX(X) ((long unsigned int)(X))
void node_debug(node_t* node)
{
  char buf[256];
  sprintf(buf, " this : %lx", LX(node));
  INFO(buf);
  sprintf(buf, " size : %lu", LX(node->size));
  INFO(buf);
  sprintf(buf, " prev : %lx", LX(node->prev));
  INFO(buf);
  sprintf(buf, " next : %lx", LX(node->next));
  INFO(buf);

}

void node_update_neighbours(node_t* node)
{
  if(node->prev != NULL) {
    node->prev->next = node;
  }
  if(node->next != NULL) {
    node->next->prev = node;
  }
}

void node_init(node_t* node, size_t size, node_t* prev, node_t* next)
{
  node->size = size;
  node->prev = prev;
  node->next = next;
  node_update_neighbours(node);
}

void node_destroy(node_t* node)
{
  if(node->prev != NULL) {
    node->prev->next = node->next;
  }
  if(node->next != NULL) {
    node->next->prev = node->prev;
  }
}

char node_owns(node_t* node, char* mem)
{
  char p1 = ((char*)node->next) > ((char*)mem);
  char p2 =  mem >= (char*)node;
  return p1 && p2;
}

char node_touches(node_t* node, node_t* neighbour)
{
  assert(node < neighbour);
  char p2 = (char*)NODE_DATA(node) + node->size >= (char*)neighbour;
  return p2;
}

node_t* node_merge_forward(node_t* node)
{
  if(node->next == NULL)
    return NULL;
  if(node_touches(node, node->next)) {
    node->size = node->next->size + node->size;

    return node;
  }
  return NULL;
}

node_t* node_find_fitting(node_t* node, size_t size)
{
  for(;node != NULL; node = node->next) {
    if(node->size >= size) {
      return node;
    }
  }
  return NULL;
}

node_t* node_find_owner(node_t* node, void* ptr)
{
  // assert( owner is on this list );
  if(ptr < (void*)node)
  {
    return NULL;
  }
  char* sptr = (char*) ptr;
  for(; node->next != NULL; node = node->next) {
    if(node_owns(node, sptr)) {
      return node;
    }
  }
  return node;
}

void node_decrease_size(node_t* node, size_t size)
{
  assert(node->size >= size);

  node->size = node->size - size;

}

node_t* node_move(node_t* node, size_t size)
{
  char* src = (char*)node;
  char* dest = src + size;
  for(int i=sizeof(node_t)-1;i>=0; i = i-1) {
    dest[i] = src[i];
  }
  return (node_t*)dest;
}

typedef struct page_desc {
  size_t size;
  node_t* free_node;
  struct page_desc* next_page;
} page_t;

#define PAGE_FREE_NODES(page) (page->free_node)
#define PAGE_DATA(page) ((node_t*)&page[1])

#define ALLOC_SIZE(S) (S + sizeof(size_t))

#define PAGE_FREE_SIZE(size) (size - sizeof(page_t))

void page_debug(page_t* page)
{
  for (node_t* i = PAGE_FREE_NODES(page); i != NULL; i = i->next) {
    node_debug(i);
  }
}

void page_init(page_t* page, size_t size)
{
  page->size = size;
  page->free_node = PAGE_DATA(page);
  node_init(PAGE_FREE_NODES(page), PAGE_FREE_SIZE(size), NULL, NULL);
}

char page_empty(page_t* page)
{
  size_t s1 = PAGE_FREE_SIZE(page->size);
  size_t s2 = PAGE_FREE_NODES(page)->size;
  return s1 == s2;
}

void* page_set_size(page_t* page __attribute__((unused)), void* ptr, size_t size)
{
  *(size_t*)ptr = size;
  return (void*)((size_t*)ptr + 1);
}

size_t page_get_size(page_t* page __attribute__((unused)), void* ptr)
{
  return *(size_t*)ptr;
}

void page_node_create(page_t* page, node_t* node, size_t size, node_t* prev, node_t* next)
{
//  INFO("Creating new node");
  assert(node >= PAGE_FREE_NODES(page));
  char* page_end = (char*)page + page->size;
  char* node_end = (char*)node + size;
  assert(node_end <= page_end); // allocating on this page
  assert((next == NULL || node_end <= (char*)next));
  assert((prev == NULL || (char*)prev + prev->size <= (char*)node)); //node is valid location

  node_init(node, size, prev, next);
}

void page_update_free_node(page_t* page, node_t* node)
{
  assert(node != NULL);
  node_t* i;
  for (i = node; i->prev != NULL ; i = i->prev) {
  }
  page->free_node = i;
  node_update_neighbours(node);
}

void page_node_destroy(page_t* page, node_t* node)
{
  node_destroy(node);
  if(node -> next != NULL) {
    page_update_free_node(page, node->next);
  } else {
    if(node->prev != NULL) {
      page_update_free_node(page, node->prev);
    } else {
      page->free_node = NULL;
    }
  }
}

void* page_find_space(page_t* page, size_t size)
{
  node_t* free_node = node_find_fitting(PAGE_FREE_NODES(page), ALLOC_SIZE(size));
  if(free_node == NULL) {
//    INFO("No fitting node found");
    return NULL;
  }
  node_decrease_size(free_node, ALLOC_SIZE(size));
  void* mem = (void*)free_node;
  if(free_node -> size <= sizeof(node_t)-1) {
    //INFO("destroying node");
    page_node_destroy(page, free_node);
  } else {
    free_node = node_move(free_node, ALLOC_SIZE(size));
    page_update_free_node(page, free_node);
  }
  return page_set_size(page, mem, size);
}

void page_node_merge_forward(page_t* page, node_t* node)
{
  for(unsigned int i=0; (node = node_merge_forward(node)) != NULL; i++ ) {
    page_node_destroy(page, node->next);
  }
}

#define PAGE_REAL_PTR(ptr) ((void*)((size_t*)ptr - 1))

page_t* page_free(page_t* page, void* ptr)
{
  ptr = PAGE_REAL_PTR(ptr);
  size_t size = page_get_size(page, ptr);
  if(size == 0) {
    return NULL;
  }
  node_t* prev_free_node = NULL, *next_free_node;
  if(PAGE_FREE_NODES(page) != NULL) {
    prev_free_node = node_find_owner(PAGE_FREE_NODES(page), ptr);
  }
  node_t* this_node, *prev_node;
  this_node = (node_t*) ptr; 
  if(prev_free_node == NULL) {
    next_free_node = PAGE_FREE_NODES(page);
    prev_node = this_node;
    page->free_node = this_node;
  } else {
    next_free_node = prev_free_node->next;
    prev_node = prev_free_node;
  }
  page_node_create(page, this_node, ALLOC_SIZE(size), prev_free_node, next_free_node);
  page_node_merge_forward(page, prev_node);
  if(page_empty(page)) {
    return page;
  }
  return NULL;
}

typedef struct pagecontrol {
  char flags;
  page_t* pages;
  pthread_mutex_t lock;

} pcontrol_t;

#define PCON_INITFLAG (0x1)

#define PCON_IS_INIT(X) ( ((X)->flags & PCON_INITFLAG) == 1 )
#define PCON_CHECKINIT(X) { if( ! PCON_IS_INIT(X) ) { /*INFO("Initializing pcon.."); */pcontrol_init(X); } }
#define PCON_LOCK(P) { if(pthread_mutex_lock(&P->lock) != 0) { fatal("mutex lock"); } }
#define PCON_ULOCK(P) { if(pthread_mutex_unlock(&P->lock) != 0) { fatal("mutex unlock"); } }

#define PAGE_SIZE 4096

#define PAGE_NEXT(P) ((P)->next_page)
#define PAGE_SET_NEXT(P,Ptr) { (P)->next_page = Ptr; }

page_t* pcon_allocate_new_page(pcontrol_t* pcon, size_t size);

void pcon_debug(pcontrol_t* pcon)
{
  size_t i=0;
  for (page_t* p = pcon->pages; p!=NULL; p = PAGE_NEXT(p), i++) {
    char buf[32];
    sprintf(buf, "---\npagenr : %lu\n", i);
    //INFO(buf);
    page_debug(p);
  }
}

 void pcon_cleanup();

void pcontrol_init(pcontrol_t* pcon)
{
  if(PCON_IS_INIT(pcon) ) {
    fatal("Subsequent initialization of pcontrol_t");
  }
  pcon->flags = PCON_INITFLAG;
  if(pcon_allocate_new_page(pcon, 1) == NULL)
  {
    fatal("Cannot allocate first page");
  }
  if(pthread_mutex_init(&pcon->lock,NULL) != 0) {
    fatal("mutex init");
  }
 // atexit(&pcon_cleanup);
}

void* pcon_find_space(pcontrol_t* pcon, size_t size)
{
  void* mem = NULL;
  for (page_t* p = pcon->pages; p != NULL; p = PAGE_NEXT(p)) {
    if((mem = page_find_space(p, size)) != NULL) {
      return mem;
    }
  }
  return NULL;
}

page_t* pcon_get_owner(pcontrol_t* pcon, void* ptr)
{
  for (page_t* p = pcon->pages; p != NULL; p = PAGE_NEXT(p)) {
    if( ((char*)ptr > (char*)p) && ((char*)ptr < ((char*)(p) + p->size))) {
      return p;
    }
  }
  return NULL;
}

size_t pcon_get_size(pcontrol_t* pcon, void* ptr)
{
  page_t* page = pcon_get_owner(pcon, ptr);
  if(page == NULL) {
    return 0;
  }
  return page_get_size(page, PAGE_REAL_PTR(ptr));
}

page_t* pcon_allocate_new_page(pcontrol_t* pcon, size_t size)
{
  //INFO("Allocating new page");
  char buf[256];
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  size_t psize = PAGE_SIZE;
  while(size >= PAGE_FREE_SIZE(psize)) {
//    flags = flags | MAP_HUGETLB;
    psize = psize + PAGE_SIZE;
  }
  sprintf(buf, " size: %lu", psize);
  //INFO(buf);
  page_t* p = (page_t*) mmap(NULL, psize, PROT_READ | PROT_WRITE | PROT_EXEC, flags, -1, 0);
  if(p == MAP_FAILED) {
    //INFO("mmap failed");
    return NULL;
  }
  PAGE_SET_NEXT(p,pcon->pages);
  pcon->pages = p;
  page_init(p, psize);
  return p;
}

void pcon_dealloc_page(pcontrol_t* pcon, page_t* page)
{
  if(page == pcon->pages && PAGE_NEXT(page) == NULL)
    return; // don't deallocate last page
  //INFO("deallocating page");
  page_t* next = PAGE_NEXT(page);
  page_t* p;
  for (p = pcon->pages; p != NULL; p = PAGE_NEXT(p)) {
    if(PAGE_NEXT(p) == page)
      break;
  }
  if(p != NULL) {
    PAGE_SET_NEXT(p, next);
  }
  if(pcon->pages == page) {
    pcon->pages = next;
  }
  if(munmap(page, page->size) == -1)
  {
    fatal("munmap failed");
  }
}

void* pcon_malloc(pcontrol_t* pcon, size_t size)
{
  if(size < 1) {
    errno = EINVAL;
    return NULL;
  }
  size = (size < PAGE_GRAIN) ? PAGE_GRAIN : size;
  if(size % ALIGN) {
    size = size + ALIGN - size % ALIGN;
  }
  PCON_LOCK(pcon);
  void* mem = pcon_find_space(pcon, size);
  if(mem == NULL) {
    page_t* page_ptr = pcon_allocate_new_page(pcon, size);
    if(page_ptr  == NULL) {
      //INFO("failed to allocate new page");
      PCON_ULOCK(pcon);
      errno = ENOMEM;
      return NULL;
    }
    mem = page_find_space(page_ptr, size);
  }
  if(mem == NULL)
    //INFO("malloc failed");
  PCON_ULOCK(pcon);
  return mem;
}

void pcon_free(pcontrol_t* pcon, void* ptr)
{
  if(ptr == NULL) {
    return;
  }
  PCON_LOCK(pcon);
  page_t* page = pcon_get_owner(pcon, ptr);
  if(page == NULL) {
    PCON_ULOCK(pcon);
    return;
  }
  if(page_free(page, ptr) == page)
  {
    pcon_dealloc_page(pcon, page);
  }
  PCON_ULOCK(pcon);
}

void* pcon_realloc(pcontrol_t* pcon, void* ptr, size_t size)
{
  void* mem = pcon_malloc(pcon, size);
  if(mem == NULL) {
    return NULL;
  }
  if(ptr == NULL) {
    return mem;
  }
  size_t oldsize = pcon_get_size(pcon, ptr);
  if (oldsize == 0) {
    pcon_free(pcon, mem);
    return NULL;
  }
  memcpy(mem, ptr, oldsize);
  pcon_free(pcon, ptr);
  return mem;
}

static pcontrol_t _pageinfo;

 void my_init()
{
  PCON_CHECKINIT(&_pageinfo);
}

void* pcon_calloc(pcontrol_t* pcon, size_t count, size_t size)
{
  void* mem = pcon_malloc(pcon, count*size + 1);
  if(mem == NULL){
    return NULL;
  }
  memset(mem, 0, count*size+1);
  return mem;
}

 void _pcon_debug()
{
  pcon_debug(& _pageinfo);
}

 void pcon_cleanup()
{
  //INFO("Cleaning up..\n");
  for (page_t* p = _pageinfo.pages, *t = p; p != NULL; p = t ) {
    t = PAGE_NEXT(p);
    munmap(p, p->size);
  }
}

 void* malloc(size_t size)
{
  IF_MALLOC_ACTIVE {
    PCON_CHECKINIT(& _pageinfo);
    char buf[256];
    sprintf(buf, " a %lu", size);
    INFO(buf);
    char* mem = pcon_malloc(&_pageinfo, size);
    sprintf(buf, "p %p", mem);
    INFO(buf);
    return mem;
  ELSE_GLIBC
    return __libc_malloc(size);
  }
}


 void* calloc(size_t count, size_t size)
{
  IF_MALLOC_ACTIVE {
    PCON_CHECKINIT(& _pageinfo);
    char buf[256];
    sprintf(buf, " c %lu %lu", count, size);
    INFO(buf);
    return pcon_calloc(&_pageinfo, count, size);
  ELSE_GLIBC
    return __libc_calloc(count,size);
  }
}

 void* realloc(void* ptr, size_t size)
{
  IF_MALLOC_ACTIVE {
    if(! PCON_IS_INIT(&_pageinfo) ) {
      return NULL;
    }
    return pcon_realloc(& _pageinfo, ptr, size);
  ELSE_GLIBC
    return __libc_realloc(ptr, size);
  }
}

 void free(void* ptr)
{
  IF_MALLOC_ACTIVE {
    if(! PCON_IS_INIT(&_pageinfo) ) {
      return;
    }
    char buf[256];
    sprintf(buf, " free: %p",ptr);
    //INFO(buf);
    pcon_free(& _pageinfo, ptr);
  ELSE_GLIBC
    __libc_free(ptr);
  }
}
