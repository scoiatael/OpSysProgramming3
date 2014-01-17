#include "common.h"

#define MEMORY_SIZE  ( 8 * 1 << (10 * 2) ) // 1<<(10*n) = 2^10^n = 1024^n hence 8Gb of virtual mem
#define PAGE_SIZE  4096
#define PAGE_GRAIN (2*sizeof(size_t)) 
#define PAGE_MAX_NUMBER ( MEMORY_SIZE / PAGE_SIZE )

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
  info(buf);
  sprintf(buf, " size : %lu", LX(node->size));
  info(buf);
  sprintf(buf, " prev : %lx", LX(node->prev));
  info(buf);
  sprintf(buf, " next : %lx", LX(node->next));
  info(buf);

}

void node_init(node_t* node, size_t size, node_t* prev, node_t* next)
{
  node->size = size;
  node->prev = prev;
  node->next = next;
  if(node->prev != NULL) {
    node->prev->next = node;
  }
  if(node->next != NULL) {
    node->next->prev = node;
  }
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
    if(node->size > size) {
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
  assert(node->size > size);

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

#define PAGE_FREE_SIZE ( PAGE_SIZE - sizeof(page_t))
#define NODE_MAX_NUMBER ( PAGE_SIZE / PAGE_GRAIN)

typedef struct page_desc {
  node_t* free_node;
} page_t;

#define PAGE_FREE_NODES(page) (page->free_node)
#define PAGE_DATA(page) ((node_t*)&page[1])

void page_debug(page_t* page)
{
  for (node_t* i = PAGE_FREE_NODES(page); i != NULL; i = i->next) {
    node_debug(i);
  }
}

void page_init(page_t* page)
{
  page->free_node = PAGE_DATA(page);
  node_init(PAGE_FREE_NODES(page), PAGE_FREE_SIZE, NULL, NULL);
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
  info("Creating new node");
  assert(node >= PAGE_FREE_NODES(page));
  char* page_end = (char*)page + PAGE_SIZE;
  char* node_end = (char*)node + size;
  assert(node_end <= page_end); // allocating on this page
  assert((next == NULL || node_end <= (char*)next));
  assert((prev == NULL || (char*)prev + prev->size <= (char*)node)); //node is valid location

  node_init(node, size, prev, next);
}

void page_update_free_node(page_t* page, node_t* node)
{
  if(node != NULL && node->prev == NULL) {
    page->free_node = node;
  }
}

void page_node_destroy(page_t* page, node_t* node)
{
  node_destroy(node);
  page_update_free_node(page, node->next);
}

#define ALLOC_SIZE(S) (S + sizeof(size_t))
void* page_find_space(page_t* page, size_t size)
{
  node_t* free_node = node_find_fitting(PAGE_FREE_NODES(page), size);
  if(free_node == NULL) {
    info("No fitting node found");
    return NULL;
  }
  node_decrease_size(free_node, ALLOC_SIZE(size));
  void* mem = (void*)free_node;
  free_node = node_move(free_node, ALLOC_SIZE(size));
  page_update_free_node(page, free_node);
  if(free_node -> size <= 1) {
    page_node_destroy(page, free_node);
  }
  return page_set_size(page, mem, size);
}

void page_node_merge_forward(page_t* page, node_t* node)
{
  for(unsigned int i=0; (node = node_merge_forward(node)) != NULL && i < NODE_MAX_NUMBER; i++ ) {
    page_node_destroy(page, node->next);
  }
}

page_t* page_free(page_t* page, void* ptr)
{
  ptr = (void*)((size_t*)ptr - 1);
  size_t size = page_get_size(page, ptr);
  if(size == 0) {
    return NULL;
  }
  node_t* prev_free_node = node_find_owner(PAGE_FREE_NODES(page), ptr);
  node_t* next_free_node;
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
  page_node_create(page, this_node, size, prev_free_node, next_free_node);
  page_node_merge_forward(page, prev_node);
  if(PAGE_FREE_NODES(page)->next == NULL) {
    return page;
  }
  return NULL;
}

typedef struct pagecontrol {
  char flags;
  size_t pages_size;
  page_t* pages[PAGE_MAX_NUMBER];

} pcontrol_t;

#define PCON_INITFLAG (0x1)

#define PCON_IS_INIT(X) ( ((X)->flags & PCON_INITFLAG) == 1 )
#define PCON_CHECKINIT(X) { if( ! PCON_IS_INIT(X) ) { info("Initializing pcon.."); pcontrol_init(X); } }

page_t* pcon_allocate_new_page(pcontrol_t* pcon);

void pcon_debug(pcontrol_t* pcon)
{
  for (size_t i = 0; i < pcon->pages_size; i++) {
    char buf[32];
    sprintf(buf, "---\npagenr : %lu\n", i);
    info(buf);
    page_debug(pcon->pages[i]);
  }
}

void pcon_cleanup();

void pcontrol_init(pcontrol_t* pcon)
{
  if(PCON_IS_INIT(pcon) ) {
    fatal("Subsequent initialization of pcontrol_t");
  }
  pcon->flags = PCON_INITFLAG;
  pcon->pages_size = 0;
  if(pcon_allocate_new_page(pcon) == NULL)
  {
    fatal("Cannot allocate first page");
  }
  atexit(&pcon_cleanup);
}

void* pcon_find_space(pcontrol_t* pcon, size_t size)
{
  void* mem = NULL;
  for (unsigned int i = 0; i < pcon->pages_size; i++) {
    if((mem = page_find_space(pcon->pages[i], size)) != NULL) {
      return mem;
    }
  }
  return NULL;
}

page_t* pcon_get_owner(pcontrol_t* pcon, void* ptr)
{
  for (unsigned int i = 0; i < pcon->pages_size; i++) {
    if( ((char*)ptr > (char*)pcon->pages[i]) && ((char*)ptr < ((char*)(pcon->pages[i]) + PAGE_SIZE))) {
      return pcon->pages[i];
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
  return page_get_size(page, ptr);
}

page_t* pcon_allocate_new_page(pcontrol_t* pcon)
{
  pcon->pages_size++;
  pcon->pages[pcon->pages_size-1] = (page_t*) mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(pcon->pages[pcon->pages_size-1] == NULL) {
    info("mmap failed");
    pcon->pages_size = pcon->pages_size - 1;
    return NULL;
  }
  page_init(pcon->pages[pcon->pages_size-1]);
  return pcon->pages[pcon->pages_size-1];
}

void pcon_dealloc_page(pcontrol_t* pcon, page_t* page)
{
  size_t i;
  for (i = 0; i < pcon->pages_size; i++) {
    if(pcon->pages[i] == page) {
      break;
    }
  }
  assert(i < pcon->pages_size );
  munmap(page, PAGE_SIZE);
  assert( pcon -> pages_size > 0);
  pcon -> pages_size = pcon->pages_size-1;
  pcon->pages[i] = pcon->pages[pcon->pages_size];
}

void* pcon_malloc(pcontrol_t* pcon, size_t size)
{
  size = (size < PAGE_GRAIN) ? PAGE_GRAIN : size;
  void* mem = pcon_find_space(pcon, size);
  if(mem == NULL) {
    info("Allocating new page");
    page_t* page_ptr = pcon_allocate_new_page(pcon);
    if(page_ptr  == NULL) {
      info("failed to allocate new page");
      return NULL;
    }
    mem = page_find_space(page_ptr, size);
  }
  return mem;
}

void pcon_free(pcontrol_t* pcon, void* ptr)
{
  page_t* page = pcon_get_owner(pcon, ptr);
  if(page == NULL) {
    return;
  }
  if(page_free(page, ptr) == page)
  {
    pcon_dealloc_page(pcon, page);
  }
}
void* pcon_realloc(pcontrol_t* pcon, void* ptr, size_t size)
{
  void* mem = pcon_malloc(pcon, size);
  if(mem == NULL) {
    return NULL;
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

pcontrol_t _pageinfo;

void _pcon_debug()
{
  pcon_debug(& _pageinfo);
}

void pcon_cleanup()
{
  info("Cleaning up..\n");
  for (size_t i = 0; i < _pageinfo.pages_size; i++) {
    munmap(_pageinfo.pages[i], PAGE_SIZE);
  }
}

void* my_malloc(size_t size)
{
  PCON_CHECKINIT(& _pageinfo);
  return pcon_malloc(&_pageinfo, size);
}

void* my_calloc(size_t count, size_t size)
{
  PCON_CHECKINIT(& _pageinfo);
  return pcon_malloc(&_pageinfo, size*count);
}


void* my_realloc(void* ptr, size_t size)
{
  if(! PCON_IS_INIT(&_pageinfo) ) {
    return NULL;
  }
  return pcon_realloc(& _pageinfo, ptr, size);
}

void my_free(void* ptr)
{
  if(! PCON_IS_INIT(&_pageinfo) ) {
    return;
  }
  pcon_free(& _pageinfo, ptr);
}
