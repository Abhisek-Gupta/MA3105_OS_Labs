# OS Assignment 10 Report: Memory Management & Paging in xv6-riscv

## 1. Introduction
This report provides a concise summary of the implementation, modifications, algorithms, and experimental observations for **Operating Systems Assignment 10** implemented in the `xv6-riscv` operating system.

---

## 2. Question 1 — Tracking Physical Memory Allocation

### Implementation & Modifications
- **Physical Page Counters** (`kernel/kalloc.c`): Added `total_allocated`, `total_freed`, `free_pages`, and `initialized` fields to `struct kmem`.
  - In `kinit()`, `freerange()` initializes the free pool. The `initialized` flag is set upon completion so that only dynamic runtime allocations and deallocations are tracked.
  - In `kalloc()`, `free_pages` is decremented and `total_allocated` is incremented.
  - In `kfree()`, `free_pages` is incremented and `total_freed` is incremented.
- **System Call `memstats()`** (`SYS_memstats` 23): Displays total allocated, total freed, currently allocated (`total_allocated - total_freed`), and available physical pages.
- **Test Program (`user/memtest.c`)**: Allocates 10 pages using `sbrk(40960)`, verifies counter changes, deallocates via `sbrk(-40960)`, and repeats with 5 pages.

### Output & Observations
- **Initial**: Allocated: 240, Freed: 40, Active: 200, Available: 32535.
- **After +10 Pages**: Allocated: 250 (+10), Active: 210 (+10), Available: 32525 (-10).
- **After -10 Pages**: Freed: 50 (+10), Active: 200 (-10), Available: 32535 (+10).
- **Observation**: Physical page accounting remains strictly conserved across all allocations and deallocations.

---

## 3. Question 2 — Virtual-to-Physical Address Translation

### Implementation & Modifications
- **Page Table Walking** (`kernel/vm.c`): Created `walkpgdir()` as a wrapper around Sv39's 3-level `walk()`.
- **System Call `vtop(va)`** (`SYS_vtop` 24):
  - Validates that `va < myproc()->sz` and `va < MAXVA`.
  - Locates the PTE and checks that both `PTE_V` (Valid) and `PTE_U` (User accessible) are set.
  - Combines physical page base (`PTE2PA(*pte)`) with the 12-bit offset (`va & 0xFFF`). Returns `(uint64)-1` if unmapped or invalid.
- **Test Program (`user/vttest.c`)**: Translates code (`main`), stack (`&local_var`), and heap pages with offsets (`0x512`, `0x1024`).

### Output & Observations
- Code VA `0x52` translates to PA `0x87F22052` (Offset `0x52`).
- Heap VA `0x4200` translates to PA `0x87F26200` (Page base `0x87F26000` + offset `0x200`).
- Unmapped addresses and deallocated heap pages correctly return `[UNMAPPED / INVALID]`.

---

## 4. Question 3 — Page Table Management & Page Protection

### Implementation & Modifications
- **System Call `pageinfo(va)`** (`SYS_pageinfo` 25): Inspects the target PTE and prints its Physical Frame Number (PFN) and permission flags (`P=1`, `W=1`, `U=1`).
- **System Call `setreadonly(va)`** (`SYS_setreadonly` 26): Clears the write permission bit (`*pte &= ~PTE_W`) and executes `sfence_vma()` to flush the Translation Lookaside Buffer (TLB).
- **Test Program (`user/pagetest.c`)**:
  - Allocates a page and writes to it (`P=1, W=1, U=1`).
  - Child process marks page read-only via `setreadonly()` (`P=1, W=0, U=1`).
  - Child reads successfully, then attempts to write (`*p = 'Z'`).
  - The RISC-V MMU hardware traps immediately with a Store Page Fault (`scause 0xf` / 15), terminating the process.

### Output & Observations
```
[Child] Displaying page-table information after setreadonly:
--- Page Table Info ---
Virtual Address       : 0x5000
Physical Frame Number : 0x87f2d
Page-Table Flags      : P=1, W=0, U=1
-----------------------
[Child] Reading from protected page: p[0] = 'A' (Read OK!)
[Child] Attempting to WRITE to protected page (*p = 'Z')...
usertrap(): unexpected scause 0xf pid=4 sepc=0x10a stval=0x5000
[Parent] Child process terminated with status -1. Page protection verified!
```

---

## 5. Question 4 — Page Replacement Algorithms (FIFO, LRU, Optimal)

### Implementation & Methodology
`user/pagereplace.c` implements and compares three algorithms using the 20-page reference string:
`7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1`

1. **FIFO**: Replaces the oldest loaded page using a circular pointer.
2. **LRU**: Tracks access timestamps and evicts the page with the minimum timestamp.
3. **Optimal**: Looks ahead into future references and evicts the page that will not be accessed for the longest time.

### Comparison Table (Across 3, 4, and 5 Frames)
| Algorithm | Frames | Page Faults | Page Hits | Hit Ratio (%) | Fault Ratio (%) |
|:---|:---:|:---:|:---:|:---:|:---:|
| **FIFO** | 3 | 15 | 5 | 25.00% | 75.00% |
| **LRU** | 3 | 12 | 8 | 40.00% | 60.00% |
| **Optimal** | 3 | 9 | 11 | 55.00% | 45.00% |
| **FIFO** | 4 | 10 | 10 | 50.00% | 50.00% |
| **LRU** | 4 | 8 | 12 | 60.00% | 40.00% |
| **Optimal** | 4 | 8 | 12 | 60.00% | 40.00% |
| **FIFO** | 5 | 9 | 11 | 55.00% | 45.00% |
| **LRU** | 5 | 7 | 13 | 65.00% | 35.00% |
| **Optimal** | 5 | 7 | 13 | 65.00% | 35.00% |

### Key Observations & Analysis
1. **Optimal Algorithm**: Achieves the theoretical minimum number of page faults across all configurations (9 faults for 3 frames, 8 for 4 frames, 7 for 5 frames).
2. **LRU vs. FIFO**: LRU consistently outperforms FIFO:
   - At 3 frames, LRU reduces faults by 20% (12 vs. 15).
   - At 4 and 5 frames, LRU **matches Optimal performance** (8 and 7 faults).
   - FIFO suffers because it disregards access recency, repeatedly evicting high-frequency pages like `0` and `2`.
3. **Conclusion**: Belady's Optimal is the best theoretical algorithm; LRU is the best practical online algorithm for the given reference string.

---

## 6. Summary of Files
- **Kernel Modified**: `kernel/kalloc.c`, `kernel/vm.c`, `kernel/defs.h`, `kernel/syscall.h`, `kernel/syscall.c`, `kernel/sysproc.c`.
- **User Modified / Generated**: `user/user.h`, `user/usys.pl`, `user/usys.S`, `Makefile`.
- **User Programs Created**: `user/memtest.c`, `user/vttest.c`, `user/pagetest.c`, `user/pagereplace.c`.
