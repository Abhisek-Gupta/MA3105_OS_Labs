#ifndef _MEMINFO_H_
#define _MEMINFO_H_

struct meminfo {
  int total_pages;      // Total physical memory pages managed by the system
  int free_pages;       // Number of free physical pages currently available
  union {
    int user_pages;     // Pages in calling process's user address space (sz / PGSIZE)
    int proc_pages;     // Alias for user_pages
  };
  union {
    int pagetable_pages;// Number of pages used by the process's page tables
    int pt_pages;       // Alias for pagetable_pages
  };
};

#endif
