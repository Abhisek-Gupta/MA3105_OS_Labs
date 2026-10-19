#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "meminfo.h"

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
  int t = SBRK_LAZY;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else if (n < 0) {
    // Shrinking (n < 0) must deallocate pages that were really allocated
    if (growproc(n) < 0) {
      return -1;
    }
  } else if (n > 0) {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the process uses the
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

// Return physical address corresponding to virtual address va.
// Return -1 if page is not present or not user accessible (PTE_U not set).
uint64
sys_va2pa(void)
{
  uint64 va;
  argaddr(0, &va);

  if (va >= MAXVA)
    return -1;

  struct proc *p = myproc();
  pte_t *pte = walkpgdir(p->pagetable, va, 0);

  if (pte == 0)
    return -1;
  if ((*pte & PTE_V) == 0)
    return -1;
  if ((*pte & PTE_U) == 0)
    return -1;

  uint64 offset = va & (PGSIZE - 1);
  uint64 pa = PTE2PA(*pte) | offset;

  // Print translation details using cprintf:
  // page directory index, page table index, offset, and flag bits (P, W, U)
  cprintf("va2pa: va=0x%lx -> pdir_idx=%d, pt_idx=%d (L1=%d, L0=%d), offset=0x%x, flags: P=%d, W=%d, U=%d\n",
          va,
          (int)PX(2, va),
          (int)PX(0, va),
          (int)PX(1, va),
          (int)PX(0, va),
          (uint)offset,
          (*pte & PTE_V) ? 1 : 0,
          (*pte & PTE_W) ? 1 : 0,
          (*pte & PTE_U) ? 1 : 0);

  return pa;
}

// System call to populate physical and process memory statistics
uint64
sys_getmeminfo(void)
{
  uint64 addr;
  argaddr(0, &addr);

  if (addr == 0)
    return -1;

  struct meminfo mi;
  struct proc *p = myproc();

  mi.total_pages = kget_total_pages();
  mi.free_pages = kget_free_pages();
  mi.user_pages = p->sz / PGSIZE;
  mi.pagetable_pages = count_pagetable_pages(p->pagetable);

  if (copyout(p->pagetable, p->sz, addr, (char *)&mi, sizeof(mi)) < 0)
    return -1;

  return 0;
}

// System call to dump the page table hierarchy of the calling process
uint64
sys_vmprint(void)
{
  return vmprint_proc(myproc());
}
