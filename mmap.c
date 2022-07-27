#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <math.h>

static size_t page_size;

// align_down - rounds a value down to an alignment
// @x: the value
// @a: the alignment (must be power of 2)
//
// Returns an aligned value.
#define align_down(x, a) ((x) & ~((typeof(x))(a) - 1))

#define AS_LIMIT	(1 << 25) // Maximum limit on virtual memory bytes
#define MAX_SQRTS	(1 << 27) // Maximum limit on sqrt table entries
static double *sqrts;
static double *new_sqrts = NULL;

// Use this helper function as an oracle for square root values.
// start is the start value you want to calculate the sqrt for
static void
calculate_sqrts(double *sqrt_pos, int start, int nr)
{
  //printf("sqrt_pos %p   start   %d     nr    %d\n", sqrt_pos, start, nr);
  int i;

  for (i = 0; i < nr; i++)
    sqrt_pos[i] = sqrt((double)(start + i));
}

/**
 * @brief handles SIGSEGV (segfault) signal whenever there is a page fault
 * Unmaps an existing page, maps a new page, and populates that page with sqrts.
 *
 * note: the fist time handle_sigsegv is called munmap() will be passed NULL.
 * on subsequent calls the page to be unmapped will be the last page mapped
 */
static void
handle_sigsegv(int sig, siginfo_t *si, void *ctx)
{
  uintptr_t fault_addr = (uintptr_t)si->si_addr; // where the pagefault occurred

  // Step 1) free up space
  if(munmap(new_sqrts, page_size) == -1){ // NULL lets the OS choose a page
    fprintf(stderr, "Unmapping failed in handle_sigsegv!\n");
  }

  // Step 2) allocate map memory
  new_sqrts = mmap((void*)si->si_addr, page_size, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1 , 0);
  if(new_sqrts == MAP_FAILED){
    fprintf(stderr, "Mapping in handle_sigsegv failed.\n");
    exit(EXIT_FAILURE);
  }

  // Cast starter and new pointer to longs so difference is calculated in bytes
  long diff = ((long)new_sqrts) - ((long)sqrts);

  // Step 3) populate square roots in newly mapped memory
  calculate_sqrts(new_sqrts, diff/sizeof(double), page_size/sizeof(double));
}

static void
setup_sqrt_region(void)
{
  struct rlimit lim = {AS_LIMIT, AS_LIMIT};
  struct sigaction act;

  // Only mapping to find a safe location for the table.
  sqrts = mmap(NULL, MAX_SQRTS * sizeof(double) + AS_LIMIT, PROT_NONE, // pointer to the start of the SQRT table
	       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); 
  if (sqrts == MAP_FAILED) {
    fprintf(stderr, "Couldn't mmap() region for sqrt table; %s\n",
	    strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Now release the virtual memory to remain under the rlimit.
  if (munmap(sqrts, MAX_SQRTS * sizeof(double) + AS_LIMIT) == -1) {
    fprintf(stderr, "Couldn't munmap() region for sqrt table; %s\n",
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Set a soft rlimit on virtual address-space bytes.
  if (setrlimit(RLIMIT_AS, &lim) == -1) {
    fprintf(stderr, "Couldn't set rlimit on RLIMIT_AS; %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Register a signal handler to capture SIGSEGV.
  act.sa_sigaction = handle_sigsegv;
  act.sa_flags = SA_SIGINFO;
  sigemptyset(&act.sa_mask);
  if (sigaction(SIGSEGV, &act, NULL) == -1) {
    fprintf(stderr, "Couldn't set up SIGSEGV handler;, %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static void
test_sqrt_region(void)
{
  int i, pos = rand() % (MAX_SQRTS - 1);
  double correct_sqrt;

  printf("Validating square root table contents...\n");
  srand(0xDEADBEEF);

  for (i = 0; i < 500000; i++) {
    if (i % 2 == 0)
      pos = rand() % (MAX_SQRTS - 1);
    else
      pos += 1;
    calculate_sqrts(&correct_sqrt, pos, 1);
    if (sqrts[pos] != correct_sqrt) {
      fprintf(stderr, "Square root is incorrect. Expected %f, got %f.\n",
              correct_sqrt, sqrts[pos]);
      exit(EXIT_FAILURE);
    }
  }

  printf("All tests passed!\n");
}

int
main(int argc, char *argv[])
{
  page_size = sysconf(_SC_PAGESIZE);
  printf("page_size is %ld\n", page_size);
  setup_sqrt_region();
  test_sqrt_region();
  return 0;
}

