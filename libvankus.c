#include <stdio.h>
#include <sys/time.h>
#include <assert.h>
#include <stdlib.h>

#include <inttypes.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#include <sys/ioctl.h>
#include <string.h>

#include <linux/fs.h>

#include "tables.h"

typedef struct __attribute__((__packed__)) fragment_s {
  uint64_t prng;
  uint64_t job;
  uint64_t pos;
  uint64_t iters;
  uint64_t table;
  uint64_t color;
  uint64_t start;
  uint64_t stop;
  uint64_t challenge;
} fragment;

typedef struct fragdbe {
  int burst;
  int pos;
} fragdbe;

#define BURSTFRAGS 16320
#define QSIZE 5
#define CLBLOBSIZE (10*BURSTFRAGS)
#define ONEFRAG 4
#define BMSIZE (BURSTFRAGS*sizeof(fragment))

fragment ** burstq[QSIZE];
fragdbe fragdb[CLBLOBSIZE];

int fragdbptr = 0;

int qstart, qstop;

uint64_t mytables[] = {380, 220, 100,108,116,124,132,140,148,156,164,172,180,188,196,204,212,230,238,250,260,268,276,292,324,332,340,348,356,364,372,388,396,404,412,420,428,436,492,500};

int getfirstfree() {
  int i;
  for(i=0; i<QSIZE; i++) {
    if(burstq[(i+qstop) % QSIZE] == NULL) {
      return i;
    }
  }
  return -1;
}

int getnumfree() {
  int fr = 0;
  for(int i=0; i<QSIZE; i++) {
    if(burstq[(i+qstop) % QSIZE] == NULL) {
      fr++;
    }
  }
  return fr;
}

int burst_load(char * cbuf, int size) {

  int idx = getfirstfree();

  if(idx == -1) {
    printf("Attempted to push burst to full queue!\n");
    return -1;
  }

  burstq[idx] = (fragment**) malloc(BMSIZE);

  memcpy(burstq[idx], cbuf, BMSIZE);

  printf("bload\n");

/*  for(int i = 0; i<16320*9; i++) {
    printf("%lx ", ((uint64_t*)burstq[idx])[i]);
    if(i%9) 
      printf("\n");
  }*/

  qstop = idx;

  return 0;

}

uint64_t getrf(uint64_t table, uint64_t color) {
#define offset -100
  //printf("t %li c %li\n", table, color);
  return rft[table + offset][color];
}

int fincond(fragment f) {
  if(f.color >= f.stop) {
    return 1;
  }
  if(f.prng == 0) {
    return 1;
  }
  return 0;
}

int frag_clblob(char * cbuf, int size) {

  fragdbptr = 0;

  uint64_t * clblob = (uint64_t *) cbuf;

  for(int i = 0; i<QSIZE; i++) {

    int bp = (i+qstart) % QSIZE;

    if(burstq[bp] != NULL) {
      fragment* arr = (fragment*) burstq[bp];

      for(int j = 0; j<BURSTFRAGS; j++) {

        if(fragdbptr > CLBLOBSIZE) {
          return fragdbptr;
        }

        fragment f = arr[j];

        if(f.table == 220 && f.pos == 5) {
          printf("frag %lu\n", f.prng);
        }

        if(fincond(f) == 0) {

          fragdbe e;
          e.burst = bp;
          e.pos = j;
          fragdb[fragdbptr] = e;

          clblob[ONEFRAG * fragdbptr + 0] = f.prng;
          clblob[ONEFRAG * fragdbptr + 1] = getrf(f.table, f.color);
          clblob[ONEFRAG * fragdbptr + 2] = f.challenge;

          fragdbptr++;

        } else {
          //printf("fragment %i:%i %lx %lx %lx %lx %lx\n", bp, j, f.prng, f.job, f.pos, f.table, f.color);
        }
      }
    }
  }

  return fragdbptr;
}

int getindex(uint64_t table) {

  int mlen = sizeof(mytables);

  int i = 0;
  while(mytables[i] != table) {
    if (i>mlen) {
      return -1;
    }
    i++;
  }

  return i;

}

void report(char * cbuf, int size) {

  uint64_t * a = (uint64_t *)cbuf;

  for(int i = 0; i<fragdbptr; i++) {

    fragdbe e = fragdb[i];

    fragment* arr = (fragment*) burstq[e.burst];

    if (a[i * ONEFRAG + 3] & 0x1) { // end of color

      /* if there is no next RF, no pre-reversing is used */
      if(arr[e.pos].color < arr[e.pos].stop - 1) {
        arr[e.pos].prng = a[i * ONEFRAG + 0] ^ getrf(arr[e.pos].table, arr[e.pos].color + 1);
      } else {
        arr[e.pos].prng = a[i * ONEFRAG + 0];
      }
      arr[e.pos].iters = 0;
      arr[e.pos].color++;

    } else { // resubmit

      arr[e.pos].prng = a[i * ONEFRAG + 0];
      arr[e.pos].iters++;

    }

    if (a[i * ONEFRAG + 3] & 0x2ULL) {
      printf("FOUND %lX (%lX) fdbpos %i\n",a[i * ONEFRAG + 0], a[i * ONEFRAG + 3], i);
    }

  }

  fragdbptr = 0;

}

int pop_result(char * cbuf, int size) {

  uint64_t * a = (uint64_t *)cbuf;

  printf("pop result\n");

  for(int i = 0; i < QSIZE; i++) {
    int missing = 0;

    printf("%i:%p ", i, burstq[i]);

    if(burstq[i] != 0) {

      fragment* arr = (fragment*) burstq[i];

      for(int j = 0; j < BURSTFRAGS; j++) {
        fragment f = arr[j];
        if((fincond(arr[j]) == 0) || (f.challenge != 0UL)) {
          //printf("No cond for %i:%i %lx %lx %lx %lx %lx\n", i, j, f.prng, f.job, f.pos, f.table, f.color);
          missing++;
        }
      }
      printf("Missing %i\n", missing);
      if(missing == 0) {

        fragment* arr = (fragment*) burstq[i];
        for(int b = 0; b < BURSTFRAGS; b++) {
          fragment f = arr[b];

          if(f.table == 220 && f.pos == 5) {
            printf("fin report %lu\n", f.prng);
          }

          a[b] = f.prng;
        }

        int jobnum = arr[0].job; // for historical reasons, there is a jobnum in every fragment

        free(burstq[i]);
        burstq[i] = 0;
        return jobnum;
      }
    }
  }

  return -1;

}




