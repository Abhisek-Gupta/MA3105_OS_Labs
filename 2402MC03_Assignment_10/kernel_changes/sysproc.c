#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// Assignment 10 - Question 1: System call to display physical memory statistics
uint64
sys_memstats(void)
{
  return memstats();
}

// Assignment 10 - Question 2: System call to translate virtual address to physical address
uint64
sys_vtop(void)
{
  uint64 va;
  argaddr(0, &va);
  return vtop_trans(myproc()->pagetable, va);
}

// Assignment 10 - Question 3a: System call to display physical frame number and flags (P, W, U)
uint64
sys_pageinfo(void)
{
  uint64 va;
  argaddr(0, &va);

  if (va >= MAXVA || va >= myproc()->sz) {
    printk("pageinfo: Virtual address 0x%lx is unmapped (exceeds process size)\n", va);
    return -1;
  }

  pte_t *pte = walkpgdir(myproc()->pagetable, va, 0);
  if (pte == 0 || (*pte & PTE_V) == 0 || (*pte & PTE_U) == 0) {
    printk("pageinfo: Virtual address 0x%lx is not mapped or PTE is invalid\n", va);
    return -1;
  }

  uint64 pfn = PTE2PA(*pte) >> 12;
  int p = (*pte & PTE_V) ? 1 : 0;
  int w = (*pte & PTE_W) ? 1 : 0;
  int u = (*pte & PTE_U) ? 1 : 0;

  printk("--- Page Table Info ---\n");
  printk("Virtual Address       : 0x%lx\n", va);
  printk("Physical Frame Number : 0x%lx (%ld)\n", pfn, pfn);
  printk("Page-Table Flags      : P=%d, W=%d, U=%d\n", p, w, u);
  printk("-----------------------\n");
  return 0;
}

// Assignment 10 - Question 3b: System call to remove write permission (PTE_W) from page-table entry
uint64
sys_setreadonly(void)
{
  uint64 va;
  argaddr(0, &va);

  if (va >= MAXVA || va >= myproc()->sz)
    return -1;

  pte_t *pte = walkpgdir(myproc()->pagetable, va, 0);
  if (pte == 0 || (*pte & PTE_V) == 0 || (*pte & PTE_U) == 0)
    return -1;

  // Clear write flag
  *pte &= ~PTE_W;

  // Invalidate TLB so updated permissions take immediate effect
  sfence_vma();

  return 0;
}
