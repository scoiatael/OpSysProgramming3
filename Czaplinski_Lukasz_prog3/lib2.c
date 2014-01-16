#include "common.h"

#define MEMORY_SIZE  ( 8 * 1 << (10 * 2) ) // 1<<(10*n) = 2^10^n = 1024^n hence 8Gb of virtual mem
#define PAGE_SIZE  4096
#define PAGE_GRAIN 32
#define PAGE_MAX_NUMBER ( MEMORY_SIZE / PAGE_SIZE )

typedef struct node_desc {
  size_t size;
  void* mem;
  struct node_desc* prev;
  struct node_desc* next;
  size_t index;
} node_t;

#define LX(X) ((long unsigned int)(X))
void node_debug(node_t* node)
{
  char buf[256];
  sprintf(buf, " this : %lx", LX(node));
  info(buf);
  sprintf(buf, " size : %lu", LX(node->size));
  info(buf);
  sprintf(buf, " mem  : %lx", LX(node->mem));
  info(buf);
  sprintf(buf, " prev : %lx", LX(node->prev));
  info(buf);
  sprintf(buf, " next : %lx", LX(node->next));
  info(buf);
  sprintf(buf, " index: %lu", LX(node->index));
  info(buf);

}

void node_init(node_t* node, size_t size, void* mem, node_t* prev, node_t* next, size_t index)
{
  node->size = size;
  node->mem = mem;
  node->prev = prev;
  node->next = next;
  node->index = index;
}

char node_owns(node_t* node, char* mem)
{
  return ((char*)node->mem + node->size) > ((char*)node->next->mem) && mem >= (char*)node->mem;
}

node_t* node_merge_forward(node_t* node)
{
  if(node->next == NULL)
    return NULL;
  if(node_owns(node, (char*)node->next->mem - 1)) {
    node->next->prev = node->prev;
    if (node->prev != NULL) {
      node->prev->next = node->next;
    }
    node->next->mem = node->mem;
    node->next->size = node->next->size + node->size;
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
  char* sptr = (char*) ptr;
  for(; node != NULL; node = node->next) {
    if(node_owns(node, sptr)) {
      return node;
    }
  }
  return NULL;
}

void node_decrease_size(node_t* node, size_t size)
{
  assert(node->size >= size);

  node->mem = node->mem + size;
  node->size = node->size - size;

  // can't do anything about nodes with size 0
}

#define NODE_MAX_NUMBER ( PAGE_SIZE / PAGE_GRAIN / 2)

typedef struct page_desc {
  size_t free_nodes_size;
  node_t free_nodes[NODE_MAX_NUMBER];
  size_t occupied_nodes[NODE_MAX_NUMBER];
} page_t;

#define PAGE_OCCUPIED_INDEX(page_ptr, ptr) (((char*)ptr - (char*)(page_ptr - 1)) / PAGE_GRAIN)
#define PAGE_FREE_SIZE ( PAGE_SIZE - sizeof(page_t))
#define PAGE_DATA(P) ((void*) &(P[1]) )

void page_node_create(page_t* page, size_t size, void* mem, node_t* prev, node_t* next);

void page_debug(page_t* page)
{
  for (size_t i = 0; i < page->free_nodes_size; i++) {
    node_debug(&page->free_nodes[i]);
  }
  char buf[32];
  info("---");
  for (size_t i = 0; i < NODE_MAX_NUMBER; i++) {
    if(page->occupied_nodes[i] != 0) {
      sprintf(buf, "%lu : %lu\n", i, page->occupied_nodes[i]);
      info(buf);
    }
  }
}

void page_init(page_t* page)
{
  page->free_nodes_size = 0;
  page_node_create(page, PAGE_FREE_SIZE, PAGE_DATA(page), NULL, NULL);
}

size_t page_get_size(page_t* page, void* ptr)
{
  return page->occupied_nodes[PAGE_OCCUPIED_INDEX(page, ptr)];
}

void page_node_create(page_t* page, size_t size, void* mem, node_t* prev, node_t* next)
{
  info("Creating new node");
  page->free_nodes_size++;
  assert(page->free_nodes_size < NODE_MAX_NUMBER);
  node_init(&page->free_nodes[page->free_nodes_size-1], size, mem, prev, next, page->free_nodes_size-1);
  page->occupied_nodes[PAGE_OCCUPIED_INDEX(page, mem)] = 0;
}

void page_node_destroy(page_t* page, node_t* node)
{
  page->free_nodes_size = page->free_nodes_size - 1;
  page->free_nodes[node->index] = page->free_nodes[page->free_nodes_size];
  page->free_nodes[node->index].index = node->index;
  if(page->free_nodes[node->index].prev != NULL) {
    page->free_nodes[node->index].prev->next = &page->free_nodes[node->index];
  }
  if(page->free_nodes[node->index].next != NULL) {
    page->free_nodes[node->index].next->prev = &page->free_nodes[node->index];
  }
}

void* page_find_space(page_t* page, size_t size)
{
  node_t* free_node = node_find_fitting(&page->free_nodes[0], size);
  if(free_node == NULL) {
    info("No fitting node found");
    return NULL;
  }
  void* mem = free_node->mem;
  node_decrease_size(free_node, size);
  if(free_node -> size == 0) {
    page_node_destroy(page, free_node);
  }
  page->occupied_nodes[PAGE_OCCUPIED_INDEX(page, mem)] = size;
  return mem;
}

void page_node_merge_forward(page_t* page, node_t* node)
{
  for(int i=0; (node = node_merge_forward(node)) != NULL && i < NODE_MAX_NUMBER; i++ ) {
    page_node_destroy(page, node);
  }
}

page_t* page_free(page_t* page, void* ptr)
{
  size_t size = page_get_size(page, ptr);
  if(size == 0) {
    return NULL;
  }
  page->occupied_nodes[PAGE_OCCUPIED_INDEX(page, ptr)] = -1;
  node_t* prev_free_node = node_find_owner(&page->free_nodes[0], ptr);
  page_node_create(page, size, ptr, prev_free_node, prev_free_node->next);
  page_node_merge_forward(page, prev_free_node);
  if(&page->free_nodes[0].next == NULL) {
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
