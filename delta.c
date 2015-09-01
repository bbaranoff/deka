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

#include "revbits.h"

#include "delta_config.h"

#include "delta.h"

#include "delta_binary.h"

typedef struct blockspec {
  long blockno;
  uint64_t here;
  uint64_t target;
  int tbl;
  char * bl_memaddr;
  int j;
} blockspec;


/* How many tables, colors and keystream samples do we have in one burst */
#define tables 40
#define colors 8
#define samples 51

int max(int a, int b) {
  if (a>b) {
    return a;
  } else {
    return b;
  }
}


#define devices (sizeof(devpaths) / sizeof(devpaths[0]))

char * storages[devices];

void mmap_devices() {

  for(int i = 0; i<devices; i++) {
    size_t dsize;
    int fd = open(devpaths[i], O_RDONLY);

    ioctl(fd, BLKGETSIZE64, &dsize);

    printf("mmap %s %li bytes fd %i ", devpaths[i], dsize, fd);

    storages[i] = (char*)mmap(NULL, dsize, PROT_READ, MAP_PRIVATE, fd, 0);

    printf("result %p\n", storages[i]);

  }

}

/* Block metadata */
blockspec blockq[16320];
int blockqptr = 0;

/* Save block metadata and advise the kernel to cache the block for us.

  blockno: number of the 4KiB block in the table
  here:    delta-encoding offset of the block
  target:  endpoint we want to find
  tbl:     table id
  j:       fragment position in burst
 */

void MineABlockNCQ(long blockno, uint64_t here, uint64_t target, int tbl, int j) {

  // printf("Searching for endpoint, block %lx, blockstart %lx, endpoint %lX, table %i, idx %i\n", blockno, here, target, tbl, j);

  blockno += offsets[tbl];

  blockq[blockqptr].blockno = blockno;
  blockq[blockqptr].here = here;
  blockq[blockqptr].target = target;
  blockq[blockqptr].tbl = tbl;
  blockq[blockqptr].j = j;

  int64_t a = blockno*4096;

  char * maddr = storages[devs[tbl]] + a;

  madvise(maddr, 4096, MADV_WILLNEED|MADV_RANDOM);

  blockq[blockqptr].bl_memaddr = maddr;

  blockqptr++;
}

/* Process all blocks from burstq. Hash the generation keys and write
    the result to the buffer. */

void MineBlocksMmap(uint64_t * fragments) {

  int mined = 0;

  for(int i = 0; i<blockqptr; i++) {
    char * maddr = blockq[i].bl_memaddr;

    int j = blockq[i].j;

    uint64_t re = CompleteEndpointSearch(maddr, blockq[i].here, blockq[i].target);

    if(re) {
      re=rev(ApplyIndexFunc(re, 34));
      mined++;
    }

    fragments[j] = re;

    madvise(maddr, 4096, MADV_DONTNEED);
  }

  printf("mined %i\n", mined);

  blockqptr = 0;
}

/* Init the machine. This is to be called once on the library load. */
void delta_init() {
  mmap_devices();
  load_idx();

}

/* Prepare block for all fragments from burst in cbuf. */

void ncq_submit(char * cbuf, int size) {

  uint64_t * fragments = (uint64_t *)cbuf;

	for(int tbl=0; tbl<tables; tbl++) {
		for(int i=0; i<(colors*samples); i++) {
			int j = i+(tbl*colors*samples);
			uint64_t tg = fragments[j];
			tg = rev(tg);

			//printf("Search for %llx, table %i, i %i, j %i\n", tg, tbl, i, j);

			StartEndpointSearch(tg, tbl, j);
		}
	}

}

/* Wrapper for MineBlocksMmap */
void ncq_read(char * cbuf, int size) {
  MineBlocksMmap((uint64_t *)cbuf);
}

