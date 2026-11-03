// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  // Assignment 10 - Question 1: Counters to track physical memory allocation
  uint64 total_allocated;  // Total pages allocated since boot
  uint64 total_freed;      // Total pages freed since boot
  uint64 free_pages;       // Available physical pages in freelist
  int initialized;         // Flag indicating whether kinit has completed
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  // Assignment 10 - Question 1: Initialize allocation counters
  kmem.total_allocated = 0;
  kmem.total_freed = 0;
  kmem.free_pages = 0;
  kmem.initialized = 0;
  freerange(end, (void *)PHYSTOP);
  // Mark initialization complete so subsequent kalloc/kfree are tracked
  kmem.initialized = 1;
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  // Assignment 10 - Question 1: Track freed and available pages
  kmem.free_pages++;
  if (kmem.initialized) {
    kmem.total_freed++;
  }
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r) {
    kmem.freelist = r->next;
    // Assignment 10 - Question 1: Track allocated and available pages
    kmem.free_pages--;
    if (kmem.initialized) {
      kmem.total_allocated++;
    }
  }
  release(&kmem.lock);

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}

// Assignment 10 - Question 1b: System call implementation helper
// Displays total allocated pages, total freed pages,
// currently allocated pages, and available physical memory pages.
int
memstats(void)
{
  acquire(&kmem.lock);
  uint64 tot_alloc = kmem.total_allocated;
  uint64 tot_freed = kmem.total_freed;
  uint64 curr_alloc = kmem.total_allocated - kmem.total_freed;
  uint64 avail = kmem.free_pages;
  release(&kmem.lock);

  printk("\n================ Memory Statistics ================\n");
  printk("Total Allocated Pages       : %ld\n", tot_alloc);
  printk("Total Freed Pages           : %ld\n", tot_freed);
  printk("Currently Allocated Pages   : %ld\n", curr_alloc);
  printk("Available Physical Pages    : %ld\n", avail);
  printk("===================================================\n\n");
  return 0;
}
