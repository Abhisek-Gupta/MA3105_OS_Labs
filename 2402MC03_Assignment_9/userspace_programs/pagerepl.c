#include "types.h"
#include "stat.h"
#include "user.h"

#define MAX_FRAMES 32
#define MAX_REFS 128

// Helper to parse space-separated integers from a string
int
parse_string(char *s, int *refs)
{
  int n = 0;
  while(*s && n < MAX_REFS){
    while(*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
      s++;
    if(!*s)
      break;
    refs[n++] = atoi(s);
    while(*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r')
      s++;
  }
  return n;
}

// Helper to print integer with right-aligned padding
void
print_padded(int val, int width)
{
  int len = 0;
  int temp = val;
  if(temp == 0){
    len = 1;
  } else {
    if(temp < 0){
      len++;
      temp = -temp;
    }
    while(temp > 0){
      len++;
      temp /= 10;
    }
  }
  while(len < width){
    printf(1, " ");
    len++;
  }
  printf(1, "%d", val);
}

// Run FIFO page replacement
int
run_fifo(int num_frames, int *refs, int n, int verbose)
{
  int frames[MAX_FRAMES];
  int count = 0;
  int ptr = 0;
  int faults = 0;
  int hits = 0;
  int i, j;

  for(i = 0; i < num_frames; i++)
    frames[i] = -1;

  if(verbose)
    printf(1, "\n=== FIFO Page Replacement (Frames: %d) ===\n", num_frames);

  for(i = 0; i < n; i++){
    int p = refs[i];
    int hit = 0;
    for(j = 0; j < count; j++){
      if(frames[j] == p){
        hit = 1;
        break;
      }
    }

    if(hit){
      hits++;
    } else {
      faults++;
      if(count < num_frames){
        frames[count++] = p;
      } else {
        frames[ptr] = p;
        ptr = (ptr + 1) % num_frames;
      }
    }

    if(verbose){
      printf(1, "Step ");
      print_padded(i + 1, 2);
      printf(1, " : Ref %d | Frames: [", p);
      for(j = 0; j < num_frames; j++){
        if(j < count)
          printf(1, " %d", frames[j]);
        else
          printf(1, " -");
      }
      printf(1, " ] | %s\n", hit ? "HIT" : "FAULT");
    }
  }

  int ratio_int = (hits * 100) / n;
  int ratio_dec = ((hits * 10000) / n) % 100;
  if(verbose){
    printf(1, "FIFO Result: References = %d, Faults = %d, Hits = %d, Hit Ratio = %d.%d%d%%\n",
           n, faults, hits, ratio_int, ratio_dec / 10, ratio_dec % 10);
  }
  return faults;
}

// Run LRU page replacement
int
run_lru(int num_frames, int *refs, int n, int verbose)
{
  int frames[MAX_FRAMES];
  int last_used[MAX_FRAMES];
  int count = 0;
  int faults = 0;
  int hits = 0;
  int i, j;

  for(i = 0; i < num_frames; i++){
    frames[i] = -1;
    last_used[i] = -1;
  }

  if(verbose)
    printf(1, "\n=== LRU Page Replacement (Frames: %d) ===\n", num_frames);

  for(i = 0; i < n; i++){
    int p = refs[i];
    int hit_idx = -1;
    for(j = 0; j < count; j++){
      if(frames[j] == p){
        hit_idx = j;
        break;
      }
    }

    if(hit_idx != -1){
      hits++;
      last_used[hit_idx] = i;
    } else {
      faults++;
      if(count < num_frames){
        frames[count] = p;
        last_used[count] = i;
        count++;
      } else {
        int lru_idx = 0;
        int min_time = last_used[0];
        for(j = 1; j < num_frames; j++){
          if(last_used[j] < min_time){
            min_time = last_used[j];
            lru_idx = j;
          }
        }
        frames[lru_idx] = p;
        last_used[lru_idx] = i;
      }
    }

    if(verbose){
      printf(1, "Step ");
      print_padded(i + 1, 2);
      printf(1, " : Ref %d | Frames: [", p);
      for(j = 0; j < num_frames; j++){
        if(j < count)
          printf(1, " %d", frames[j]);
        else
          printf(1, " -");
      }
      printf(1, " ] | %s\n", hit_idx != -1 ? "HIT" : "FAULT");
    }
  }

  int ratio_int = (hits * 100) / n;
  int ratio_dec = ((hits * 10000) / n) % 100;
  if(verbose){
    printf(1, "LRU Result:  References = %d, Faults = %d, Hits = %d, Hit Ratio = %d.%d%d%%\n",
           n, faults, hits, ratio_int, ratio_dec / 10, ratio_dec % 10);
  }
  return faults;
}

// Print comparison table for given frames list
void
print_table(int *frame_counts, int num_counts, int *refs, int n)
{
  int k;
  printf(1, "\n======================================================================\n");
  printf(1, " Frames | Algorithm | References | Faults | Hits | Hit Ratio\n");
  printf(1, "----------------------------------------------------------------------\n");

  for(k = 0; k < num_counts; k++){
    int f = frame_counts[k];

    int fifo_faults = run_fifo(f, refs, n, 0);
    int fifo_hits = n - fifo_faults;
    int fifo_ratio_int = (fifo_hits * 100) / n;
    int fifo_ratio_dec = ((fifo_hits * 10000) / n) % 100;

    int lru_faults = run_lru(f, refs, n, 0);
    int lru_hits = n - lru_faults;
    int lru_ratio_int = (lru_hits * 100) / n;
    int lru_ratio_dec = ((lru_hits * 10000) / n) % 100;

    printf(1, "      %d | FIFO      |         %d |     ", f, n);
    print_padded(fifo_faults, 2);
    printf(1, " |   ");
    print_padded(fifo_hits, 2);
    printf(1, " |   %d.%d%d%%\n", fifo_ratio_int, fifo_ratio_dec / 10, fifo_ratio_dec % 10);

    printf(1, "      %d | LRU       |         %d |     ", f, n);
    print_padded(lru_faults, 2);
    printf(1, " |   ");
    print_padded(lru_hits, 2);
    printf(1, " |   %d.%d%d%%\n", lru_ratio_int, lru_ratio_dec / 10, lru_ratio_dec % 10);

    if(k < num_counts - 1)
      printf(1, "----------------------------------------------------------------------\n");
  }
  printf(1, "======================================================================\n");
}

int
main(int argc, char *argv[])
{
  // Reference string from assignment:
  // 7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1
  int default_refs[] = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};
  int default_n = sizeof(default_refs) / sizeof(default_refs[0]);

  if(argc > 1){
    if(strcmp(argv[1], "-i") == 0){
      char buf[256];
      printf(1, "Enter number of frames: ");
      gets(buf, sizeof(buf));
      int f = atoi(buf);
      if(f <= 0 || f > MAX_FRAMES){
        printf(1, "Invalid frame count\n");
        exit();
      }
      printf(1, "Enter reference string (space-separated integers): ");
      gets(buf, sizeof(buf));
      int refs[MAX_REFS];
      int n = parse_string(buf, refs);
      if(n <= 0){
        printf(1, "Invalid reference string\n");
        exit();
      }
      run_fifo(f, refs, n, 1);
      run_lru(f, refs, n, 1);
      exit();
    } else {
      int f = atoi(argv[1]);
      if(f <= 0 || f > MAX_FRAMES){
        printf(1, "Usage: pagerepl [-i] | [<frames> [p1 p2 ...]]\n");
        exit();
      }
      int refs[MAX_REFS];
      int n = 0;
      int i;
      for(i = 2; i < argc && n < MAX_REFS; i++){
        refs[n++] = atoi(argv[i]);
      }
      if(n == 0){
        run_fifo(f, default_refs, default_n, 1);
        run_lru(f, default_refs, default_n, 1);
      } else {
        run_fifo(f, refs, n, 1);
        run_lru(f, refs, n, 1);
      }
      exit();
    }
  }

  // Full assignment test suite
  printf(1, "=======================================================\n");
  printf(1, " Assignment 9 - Question 3: FIFO & LRU Page Replacement\n");
  printf(1, " Reference String: 7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1\n");
  printf(1, "=======================================================\n");

  printf(1, "\n[Task b]: Testing with 3 Frames (Expected: FIFO=15, LRU=12)\n");
  int fifo_f3 = run_fifo(3, default_refs, default_n, 1);
  int lru_f3 = run_lru(3, default_refs, default_n, 1);

  printf(1, "\nVerification against expected results:\n");
  printf(1, "FIFO (3 frames): %d faults (Expected: 15) -> %s\n",
         fifo_f3, fifo_f3 == 15 ? "MATCH" : "MISMATCH");
  printf(1, "LRU  (3 frames): %d faults (Expected: 12) -> %s\n",
         lru_f3, lru_f3 == 12 ? "MATCH" : "MISMATCH");

  printf(1, "\n[Task c]: Running for 3, 4, and 5 Frames\n");
  int test_frames[] = {3, 4, 5};
  print_table(test_frames, 3, default_refs, default_n);

  exit();
}
