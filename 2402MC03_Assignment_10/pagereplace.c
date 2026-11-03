#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_FRAMES 10
#define REF_LEN 20

static int ref_string[REF_LEN] = {
  7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
};

// Helper to print 2-digit number with leading space if needed
static void
print2d(int n)
{
  if (n >= 0 && n < 10)
    printf(" %d", n);
  else
    printf("%d", n);
}

// Helper to print percentage with two decimals using integer arithmetic
static void
print_ratio(int count, int total)
{
  int pct = (count * 10000) / total;
  int whole = pct / 100;
  int frac = pct % 100;
  if (frac < 10)
    printf("%d.0%d%%", whole, frac);
  else
    printf("%d.%d%%", whole, frac);
}

// Print the current frame contents
static void
print_frames(int frames[], int num_frames)
{
  printf("[ ");
  for (int i = 0; i < num_frames; i++) {
    if (frames[i] == -1)
      printf("- ");
    else
      printf("%d ", frames[i]);
  }
  printf("]");
}

// ============================================================================
// 1. First-In First-Out (FIFO) Page Replacement
// ============================================================================
void
run_fifo(int num_frames, int verbose, int *out_faults, int *out_hits)
{
  int frames[MAX_FRAMES];
  int faults = 0;
  int hits = 0;
  int fifo_ptr = 0;

  for (int i = 0; i < num_frames; i++)
    frames[i] = -1;

  if (verbose) {
    printf("\n--- FIFO Page Replacement (Frames = %d) ---\n", num_frames);
    printf("Step | Ref | Frames State | Result\n");
    printf("-----+-----+--------------+-----------\n");
  }

  for (int i = 0; i < REF_LEN; i++) {
    int page = ref_string[i];
    int hit = 0;

    for (int j = 0; j < num_frames; j++) {
      if (frames[j] == page) {
        hit = 1;
        break;
      }
    }

    if (hit) {
      hits++;
      if (verbose) {
        printf(" ");
        print2d(i + 1);
        printf("  |  %d  | ", page);
        print_frames(frames, num_frames);
        printf(" | Page HIT\n");
      }
    } else {
      faults++;
      frames[fifo_ptr] = page;
      fifo_ptr = (fifo_ptr + 1) % num_frames;
      if (verbose) {
        printf(" ");
        print2d(i + 1);
        printf("  |  %d  | ", page);
        print_frames(frames, num_frames);
        printf(" | Page FAULT\n");
      }
    }
  }

  if (verbose) {
    printf("-----+-----+--------------+-----------\n");
    printf("FIFO Summary: Faults = %d, Hits = %d, Hit Ratio = ", faults, hits);
    print_ratio(hits, REF_LEN);
    printf(", Fault Ratio = ");
    print_ratio(faults, REF_LEN);
    printf("\n");
  }

  if (out_faults) *out_faults = faults;
  if (out_hits) *out_hits = hits;
}

// ============================================================================
// 2. Least Recently Used (LRU) Page Replacement
// ============================================================================
void
run_lru(int num_frames, int verbose, int *out_faults, int *out_hits)
{
  int frames[MAX_FRAMES];
  int last_used[MAX_FRAMES];
  int faults = 0;
  int hits = 0;

  for (int i = 0; i < num_frames; i++) {
    frames[i] = -1;
    last_used[i] = -1;
  }

  if (verbose) {
    printf("\n--- LRU Page Replacement (Frames = %d) ---\n", num_frames);
    printf("Step | Ref | Frames State | Result\n");
    printf("-----+-----+--------------+-----------\n");
  }

  for (int i = 0; i < REF_LEN; i++) {
    int page = ref_string[i];
    int hit_index = -1;

    for (int j = 0; j < num_frames; j++) {
      if (frames[j] == page) {
        hit_index = j;
        break;
      }
    }

    if (hit_index != -1) {
      hits++;
      last_used[hit_index] = i;
      if (verbose) {
        printf(" ");
        print2d(i + 1);
        printf("  |  %d  | ", page);
        print_frames(frames, num_frames);
        printf(" | Page HIT\n");
      }
    } else {
      faults++;
      int replace_idx = -1;
      for (int j = 0; j < num_frames; j++) {
        if (frames[j] == -1) {
          replace_idx = j;
          break;
        }
      }

      if (replace_idx == -1) {
        int min_time = last_used[0];
        replace_idx = 0;
        for (int j = 1; j < num_frames; j++) {
          if (last_used[j] < min_time) {
            min_time = last_used[j];
            replace_idx = j;
          }
        }
      }

      frames[replace_idx] = page;
      last_used[replace_idx] = i;
      if (verbose) {
        printf(" ");
        print2d(i + 1);
        printf("  |  %d  | ", page);
        print_frames(frames, num_frames);
        printf(" | Page FAULT\n");
      }
    }
  }

  if (verbose) {
    printf("-----+-----+--------------+-----------\n");
    printf("LRU Summary: Faults = %d, Hits = %d, Hit Ratio = ", faults, hits);
    print_ratio(hits, REF_LEN);
    printf(", Fault Ratio = ");
    print_ratio(faults, REF_LEN);
    printf("\n");
  }

  if (out_faults) *out_faults = faults;
  if (out_hits) *out_hits = hits;
}

// ============================================================================
// 3. Optimal Page Replacement
// ============================================================================
void
run_optimal(int num_frames, int verbose, int *out_faults, int *out_hits)
{
  int frames[MAX_FRAMES];
  int faults = 0;
  int hits = 0;

  for (int i = 0; i < num_frames; i++)
    frames[i] = -1;

  if (verbose) {
    printf("\n--- Optimal Page Replacement (Frames = %d) ---\n", num_frames);
    printf("Step | Ref | Frames State | Result\n");
    printf("-----+-----+--------------+-----------\n");
  }

  for (int i = 0; i < REF_LEN; i++) {
    int page = ref_string[i];
    int hit = 0;

    for (int j = 0; j < num_frames; j++) {
      if (frames[j] == page) {
        hit = 1;
        break;
      }
    }

    if (hit) {
      hits++;
      if (verbose) {
        printf(" ");
        print2d(i + 1);
        printf("  |  %d  | ", page);
        print_frames(frames, num_frames);
        printf(" | Page HIT\n");
      }
    } else {
      faults++;
      int replace_idx = -1;
      for (int j = 0; j < num_frames; j++) {
        if (frames[j] == -1) {
          replace_idx = j;
          break;
        }
      }

      if (replace_idx == -1) {
        int furthest_use = -1;
        for (int j = 0; j < num_frames; j++) {
          int next_use = 999999;
          for (int k = i + 1; k < REF_LEN; k++) {
            if (ref_string[k] == frames[j]) {
              next_use = k;
              break;
            }
          }
          if (next_use > furthest_use) {
            furthest_use = next_use;
            replace_idx = j;
          }
        }
      }

      frames[replace_idx] = page;
      if (verbose) {
        printf(" ");
        print2d(i + 1);
        printf("  |  %d  | ", page);
        print_frames(frames, num_frames);
        printf(" | Page FAULT\n");
      }
    }
  }

  if (verbose) {
    printf("-----+-----+--------------+-----------\n");
    printf("Optimal Summary: Faults = %d, Hits = %d, Hit Ratio = ", faults, hits);
    print_ratio(hits, REF_LEN);
    printf(", Fault Ratio = ");
    print_ratio(faults, REF_LEN);
    printf("\n");
  }

  if (out_faults) *out_faults = faults;
  if (out_hits) *out_hits = hits;
}

// ============================================================================
// Main
// ============================================================================
int
main(int argc, char *argv[])
{
  printf("====================================================================\n");
  printf(" QUESTION 4: PAGE REPLACEMENT ALGORITHMS (FIFO, LRU, OPTIMAL)       \n");
  printf("====================================================================\n");
  printf("Reference String: ");
  for (int i = 0; i < REF_LEN; i++)
    printf("%d ", ref_string[i]);
  printf("\nTotal References : %d\n", REF_LEN);

  // Tasks 4a & 4b: Detailed step-by-step trace with 3 frames
  printf("\n====================================================================\n");
  printf(" PART 1: DETAILED STEP-BY-STEP TRACE (3 FRAMES)\n");
  printf("====================================================================\n");
  int f_faults, f_hits, l_faults, l_hits, o_faults, o_hits;
  run_fifo(3, 1, &f_faults, &f_hits);
  run_lru(3, 1, &l_faults, &l_hits);
  run_optimal(3, 1, &o_faults, &o_hits);

  printf("\n====================================================================\n");
  printf(" PART 2: 3-FRAME COMPARISON SUMMARY\n");
  printf("====================================================================\n");
  printf("Algorithm | Total Faults | Total Hits | Hit Ratio | Fault Ratio\n");
  printf("----------+--------------+------------+-----------+------------\n");

  printf("FIFO      |      ");
  print2d(f_faults);
  printf("      |     ");
  print2d(f_hits);
  printf("     |   ");
  print_ratio(f_hits, REF_LEN);
  printf("  |   ");
  print_ratio(f_faults, REF_LEN);
  printf("\n");

  printf("LRU       |      ");
  print2d(l_faults);
  printf("      |     ");
  print2d(l_hits);
  printf("     |   ");
  print_ratio(l_hits, REF_LEN);
  printf("  |   ");
  print_ratio(l_faults, REF_LEN);
  printf("\n");

  printf("Optimal   |      ");
  print2d(o_faults);
  printf("      |     ");
  print2d(o_hits);
  printf("     |   ");
  print_ratio(o_hits, REF_LEN);
  printf("  |   ");
  print_ratio(o_faults, REF_LEN);
  printf("\n");

  // Task 4c: Comparison table for 3, 4, and 5 frames
  printf("\n====================================================================\n");
  printf(" PART 3: COMPARISON TABLE ACROSS 3, 4, AND 5 FRAMES (TASK 4c)\n");
  printf("====================================================================\n");
  printf("+-----------+--------+-------------+------------+-----------+-------------+\n");
  printf("| Algorithm | Frames | Page Faults | Page Hits  | Hit Ratio | Fault Ratio |\n");
  printf("+-----------+--------+-------------+------------+-----------+-------------+\n");

  int frame_counts[] = {3, 4, 5};
  for (int idx = 0; idx < 3; idx++) {
    int f = frame_counts[idx];
    int flt, hit;

    // FIFO
    run_fifo(f, 0, &flt, &hit);
    printf("| FIFO      |   %d    |     ", f);
    print2d(flt);
    printf("      |     ");
    print2d(hit);
    printf("     |   ");
    print_ratio(hit, REF_LEN);
    printf("  |   ");
    print_ratio(flt, REF_LEN);
    printf("   |\n");

    // LRU
    run_lru(f, 0, &flt, &hit);
    printf("| LRU       |   %d    |     ", f);
    print2d(flt);
    printf("      |     ");
    print2d(hit);
    printf("     |   ");
    print_ratio(hit, REF_LEN);
    printf("  |   ");
    print_ratio(flt, REF_LEN);
    printf("   |\n");

    // Optimal
    run_optimal(f, 0, &flt, &hit);
    printf("| Optimal   |   %d    |     ", f);
    print2d(flt);
    printf("      |     ");
    print2d(hit);
    printf("     |   ");
    print_ratio(hit, REF_LEN);
    printf("  |   ");
    print_ratio(flt, REF_LEN);
    printf("   |\n");

    printf("+-----------+--------+-------------+------------+-----------+-------------+\n");
  }

  printf("\n====================================================================\n");
  printf(" ANALYSIS & OBSERVATIONS (TASK 4c):\n");
  printf("====================================================================\n");
  printf("1. Optimal Algorithm Performance:\n");
  printf("   - Optimal page replacement produces the fewest page faults in all cases\n");
  printf("     (9 faults for 3 frames, 8 for 4 frames, 7 for 5 frames).\n");
  printf("   - It serves as the theoretical benchmark because it has future knowledge\n");
  printf("     and evicts the page that will not be used for the longest period.\n\n");
  printf("2. LRU vs. FIFO Performance:\n");
  printf("   - LRU consistently outperforms FIFO across all frame configurations:\n");
  printf("     * At 3 frames: LRU has 12 faults vs FIFO 15 faults (20%% fewer faults).\n");
  printf("     * At 4 frames: LRU has 8 faults vs FIFO 10 faults (matches Optimal!).\n");
  printf("     * At 5 frames: LRU has 7 faults vs FIFO 9 faults (matches Optimal!).\n");
  printf("   - FIFO suffers from ignoring page access recency, frequently evicting heavily\n");
  printf("     used pages (such as page 0 and page 2) purely because they entered early.\n\n");
  printf("3. Conclusion:\n");
  printf("   - Belady's Optimal algorithm performs BEST overall for the reference string.\n");
  printf("   - Among practical online algorithms (without future knowledge), LRU performs\n");
  printf("     best and matches Optimal performance at 4 and 5 frames.\n");
  printf("====================================================================\n");

  exit(0);
}
