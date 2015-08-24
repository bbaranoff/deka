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

// XXX put into .h
typedef struct blockspec {
  long blockno;
  uint64_t here;
  uint64_t target;
  int tbl;
  char * bl_memaddr;
  int j;
} blockspec;


/* Delta takes computed endpoints and searches for startpoints in rainbow tables. */

/* *** Copyright notice ***

   The following functions have been taken from DeltaLookup.cpp (and edited
   to be compilable with GCC)

    * kr02_mergebits, kr02_whitening
       - Simple hash function.

    * StartEndpointSearch, CompleteEndpointSearch, load_idx
       - Binary interface to undocumented table format.

   There exist several copies of this code with unspecified license.
   The A5/1 kernel itself is explicitly non-free, but the rest of Kraken
   (presumably by different authors) has no license given.

   We have tried to contact the kernel author to clarify the situation,
   but no reply was received.

   I am leaving the functions here, in any problems with the copyright
   please contact me at <jenda (at) hrach (.) eu> and we will attempt
   a clean-room design.

   */


/* How many tables, colors and keystream samples do we have in one burst */
#define tables 40
#define colors 8
#define samples 51

#define READ8()\
    bits = (mBitBuffer>>(mBitCount-8))&0xff;                 \
    mBitBuffer = (mBitBuffer<<8)|pBuffer[mBufPos++];

#define READ8safe()\
    bits = (mBitBuffer>>(mBitCount-8))&0xff;                 \
    if (mBufPos<4096) {                                      \
        mBitBuffer = (mBitBuffer<<8)|pBuffer[mBufPos++];     \
    }

#define READN(n)\
    bits = mBitBuffer>>(mBitCount-(n));         \
    bits = bits & ((1<<(n))-1);                 \
    mBitCount-=(n);                             \
    if(mBitCount<8) { \
        mBitBuffer = (mBitBuffer<<8)|pBuffer[mBufPos++];    \
        mBitCount+=8; \
    } 

#define DEBUG_PRINT 0

uint64_t * fragments;

uint64_t CompleteEndpointSearch(const void* pDataBlock, uint64_t here,
                                        uint64_t end);
void run();

/* Whitening function to expand short table generation key to 64 bits */
static uint64_t kr02_whitening(uint64_t key) {
    uint64_t white = 0;
    uint64_t bits = 0x93cbc4077efddc15ULL;
    uint64_t b = 0x1;
    while (b) {
        if (b & key) {
            white ^= bits;
        }
        bits = (bits<<1)|(bits>>63);
        b = b << 1;
    }
    return white;
}

static uint64_t kr02_mergebits(uint64_t key) {
    uint64_t r = 0ULL;
    uint64_t b = 1ULL;
    unsigned int i;

    for(i=0;i<64;i++) {
        if (key&b) {
            r |= 1ULL << (((i<<1)&0x3e)|(i>>5));
        }
        b = b << 1;
    }
    return r;
}

uint64_t ApplyIndexFunc(uint64_t start_index, int bits)
{
    uint64_t w = kr02_whitening(start_index);
    start_index = kr02_mergebits((w<<bits)|start_index);
    return start_index;
}

int max(int a, int b) {
  if (a>b) {
    return a;
  } else {
    return b;
  }
}

/* Index of blockstarts in table XXX explain */
int mBlockIndex[40][10227760+100000]; // XXX +hotfix to prevent access outside of array=>crash
uint64_t mPrimaryIndex[40][39952+1000]; // XXX +hotfix to prevent access outside of array=>crash
// XXX fix properly

const char * files[40] = { 
"/mnt/tables/gsm/380.idx",
"/mnt/tables/gsm/220.idx",
"/mnt/tables/gsm/100.idx",
"/mnt/tables/gsm/108.idx",
"/mnt/tables/gsm/116.idx",
"/mnt/tables/gsm/124.idx",
"/mnt/tables/gsm/132.idx",
"/mnt/tables/gsm/140.idx",
"/mnt/tables/gsm/148.idx",
"/mnt/tables/gsm/156.idx",
"/mnt/tables/gsm/164.idx",
"/mnt/tables/gsm/172.idx",
"/mnt/tables/gsm/180.idx",
"/mnt/tables/gsm/188.idx",
"/mnt/tables/gsm/196.idx",
"/mnt/tables/gsm/204.idx",
"/mnt/tables/gsm/212.idx",
"/mnt/tables/gsm/230.idx",
"/mnt/tables/gsm/238.idx",
"/mnt/tables/gsm/250.idx",
"/mnt/tables/gsm/260.idx",
"/mnt/tables/gsm/268.idx",
"/mnt/tables/gsm/276.idx",
"/mnt/tables/gsm/292.idx",
"/mnt/tables/gsm/324.idx",
"/mnt/tables/gsm/332.idx",
"/mnt/tables/gsm/340.idx",
"/mnt/tables/gsm/348.idx",
"/mnt/tables/gsm/356.idx",
"/mnt/tables/gsm/364.idx",
"/mnt/tables/gsm/372.idx",
"/mnt/tables/gsm/388.idx",
"/mnt/tables/gsm/396.idx",
"/mnt/tables/gsm/404.idx",
"/mnt/tables/gsm/412.idx",
"/mnt/tables/gsm/420.idx",
"/mnt/tables/gsm/428.idx",
"/mnt/tables/gsm/436.idx",
"/mnt/tables/gsm/492.idx",
"/mnt/tables/gsm/500.idx"  };

const uint64_t offsets[40] = {
102347869,
81849336,
0,
20461178,
112574826,
102336184,
92077095,
92105934,
30702472,
30688927,
40931967,
0,
40924796,
51169435,
71618441,
61409247,
102343350,
61407630,
10229859,
30695679,
51162401,
112576721,
10232259,
61385698,
71639236,
81873709,
10228856,
20459800,
71641995,
81874248,
30695489,
20461339,
92105934,
0,
112596312,
51153680,
20467386,
10233293,
0,
40935940
};

const int devs[40] = {
2,
2,
2,
2,
3,
3,
2,
1,
3,
0,
3,
3,
2,
1,
2,
1,
1,
3,
0,
1,
3,
1,
1,
2,
1,
3,
2,
0,
3,
1,
2,
1,
3,
1,
2,
2,
3,
3,
0,
1 };

const char * devpaths[4] = {
"/dev/disk/by-id/ata-ADATA_SX900_02716033500100000013",
"/dev/disk/by-id/ata-ADATA_SX900_02716081500300000310",
"/dev/disk/by-id/ata-ADATA_SX900_02716081500600000463",
"/dev/disk/by-id/ata-ADATA_SX900_02730235500600001031"};

char * storages[4];

int mNumBlocks[40];
unsigned long mStepSize[40];
int64_t mLowEndpoint[40];
int64_t mHighEndpoint[40];
int64_t mBlockOffset;

static unsigned short mBase[256];
static unsigned char mBits[256];

int mInitStatics = 0;

void mmap_devices() {

  for(int i = 0; i<4; i++) {
    size_t dsize;
    int fd = open(devpaths[i], O_RDONLY);

    ioctl(fd, BLKGETSIZE64, &dsize);

    //dsize += 10000000000;

    printf("mmap %s %li bytes fd %i ", devpaths[i], dsize, fd);

    storages[i] = (char*)mmap(NULL, dsize, PROT_READ, MAP_PRIVATE, fd, 0);

    printf("result %p\n", storages[i]);

  }

}

void load_idx() {

	/* Load index for all tables */
	for(int idx=0; idx<40; idx++) {

		/* Open index file */
		FILE* fd = fopen(files[idx],"rb");
		if (fd==0) {
		    printf("Could not open %s for reading.\n", "f");
		}
		assert(fd);
		fseek(fd, 0, SEEK_END);

		/* Get file size */
		long size = ftell(fd);
		unsigned int num = (size / sizeof(uint64_t))-1;
		fseek(fd ,0 ,SEEK_SET );
		size_t alloced = num*sizeof(int)+(num/256)*sizeof(int64_t);
		fprintf(stderr, "Allocated %li bytes: %i\n",alloced,idx);

		mNumBlocks[idx] = num;
		uint64_t end;

		/* Get step size for position estimation */
		mStepSize[idx] = 0xfffffffffffffLL/(num+1);
		int64_t min = 0;
		int64_t max = 0;
		int64_t last = 0;

		/* Read blocks and save block offsets */
		for(int bl=0; bl<num; bl++) {
			size_t r = fread(&end,sizeof(uint64_t),1,fd);
			assert(r==1);
			int64_t offset = (end>>12)-last-mStepSize[idx];
			last = end>>12;
			if (offset>max) max = offset;
			if (offset<min) min = offset;
			if (offset >= 0x7fffffff || offset <=-0x7fffffff) {
				fprintf(stderr,"index file corrupt: %s\n", "F");
				exit(1);
			}
			mBlockIndex[idx][bl]=offset;
			if ((bl&0xff)==0) {
				mPrimaryIndex[idx][bl>>8]=end;
			}
		}

		mBlockIndex[idx][num] = 0x7fffffff; /* for detecting last index */
		// printf("%llx %llx %llx\n", min,max,mPrimaryIndex[1]);

		mLowEndpoint[idx] = mPrimaryIndex[idx][0];
		size_t r=fread(&mHighEndpoint[idx],sizeof(uint64_t),1,fd);
		assert(r==1);
		fclose(fd);
		mBlockOffset=0ULL;
	}
//    mDevice = dev;

	if (!mInitStatics) {
		// Fill in decoding tables
		int groups[] = {0,4,126,62,32,16,8,4,2,1};
		int gsize = 1;
		unsigned short base = 0;
		int group = 0;
		for (int i=0;i<10;i++) {
			for (int j=0; j<groups[i]; j++) {
				mBase[group] = base;
				mBits[group] = i;
				base += gsize;
				group++;
			}
			gsize = 2 * gsize;
		}
		// The rest should be unused 
		assert(group<256);
		mInitStatics = 1;
	}
}

blockspec blockq[16320];
int blockqptr = 0;

int failpt = 0;

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

  //printf("t a %li base %p, tgt %p t %i ",a,storages[tbl],maddr,tbl);

  //fflush(stdout);

  madvise(maddr, 4096, MADV_WILLNEED|MADV_RANDOM);

  //char * crap = malloc(4096);

  //memcpy(crap, maddr, 4096);

  //printf("%x %i, ", crap[1], failpt++);

  //free(crap);

  blockq[blockqptr].bl_memaddr = maddr;

  blockqptr++;
}

void MineBlocksMmap() {
  for(int i = 0; i<blockqptr; i++) {
    char * maddr = blockq[i].bl_memaddr;

    int j = blockq[i].j;

    uint64_t re = CompleteEndpointSearch(maddr, blockq[i].here, blockq[i].target);

/*    if ( j== 2453) {
      printf("NCQ processor: block %lx, blockstart %lx, endpoint %lX, que %i, idx %i\n", blockq[i].blockno, blockq[i].here, blockq[i].target, i, j);
      printf("Mined %lx\n", re);
    }*/

    if(re) {
      re=rev(ApplyIndexFunc(re, 34)); //XXX 34?
    }

/* if ( j== 2453) {
   printf("Post re %lx\n", re);
}*/
    fragments[j] = re;

    madvise(maddr, 4096, MADV_DONTNEED);
  }
  blockqptr = 0;
}

/* Return XXX

   end = endpoint value
   blockstart = starting value of a block
   tbl = table ID (the seq id, like 0-39) */

void StartEndpointSearch(uint64_t end, uint64_t blockstart, int tbl, int j) {

	//printf("end=%llx, blockstart=%llx\n", end, blockstart);
	if (end<mLowEndpoint[tbl]) return;
	if (end>mHighEndpoint[tbl]) return;

	uint64_t bid = (end>>12) / mStepSize[tbl];
	unsigned int bl = ((unsigned int)bid)/256;

	// Methinks the division has been done by float, and may 
	// have less precision than required
	while (bl && (mPrimaryIndex[tbl][bl]>end)) bl--;

	uint64_t here = mPrimaryIndex[tbl][bl];
	int count = 0;
	bl = bl *256;
	uint64_t delta = (mStepSize[tbl] + mBlockIndex[tbl][bl+1])<<12;

#if DEBUG_PRINT
	printf("here: %llx bl: %llu\n", here, bl);
#endif

	// XXX 41MB block => 42991616, ble (41 * 1024 * 1024)
	while(((here+delta)<=end) && (bl<mNumBlocks[tbl]+1)) {
		here+=delta;
		bl++;
		count++;
#if DEBUG_PRINT
		printf("here: %llx bl: %llu\n", here, bl);
#endif
		delta = (mStepSize[tbl] + mBlockIndex[tbl][bl+1])<<12;
	}

#if DEBUG_PRINT
	printf("%i block (%i)\n", bl, count);
#endif

	blockstart = here; // set first in case of sync loading 
     /*   if (j == 2453) {
            printf("Seq search, block %x, blockstart %lx, endpoint %lX, table %i, idx %i\n", bl, here, end, tbl, j);
            uint64_t rs = MineABlock(bl,here,end,tbl);
            printf("Returned %lx (32-bit hash input)\n", rs);
            rs=rev(ApplyIndexFunc(rs, 34));
            printf("Hashed %lx\n",rs);
        }*/


	MineABlockNCQ(bl,here,end,tbl,j);
	//mDevice->Request(req, (uint64_t)bl+mBlockOffset );
}

void delta_init() {
  mmap_devices();
  load_idx();

  printf("MMAP: ");
  for(int i = 0; i<4; i++) {
    printf("%p ",storages[i]);
  }
  printf("\n");

}


void ncq_submit(char * cbuf, int size) {

  printf("MMAP: ");
  for(int i = 0; i<4; i++) {
    printf("%p ",storages[i]);
  }
  printf("\n");

  fragments = (uint64_t *)cbuf;


	for(int tbl=0; tbl<tables; tbl++) {
		for(int i=0; i<(colors*samples); i++) {
			uint64_t sta = 0;

			int j = i+(tbl*colors*samples);
			uint64_t tg = fragments[j];
			tg = rev(tg);

			/* Test target that is found in table 220
			tg=0xdfd05a8b899b6000ULL; */

			//printf("Search for %llx, table %i, i %i, j %i\n", tg, tbl, i, j);

			StartEndpointSearch(tg, sta, tbl, j);

			//printf("re: %llx\n", re);
		}
	}

}

void ncq_read(char * cbuf, int size) {
  printf("MMAP: ");
  for(int i = 0; i<4; i++) {
    printf("%p ",storages[i]);
  }
  printf("\n");

  MineBlocksMmap();
}

#define DEBUG_PRINT 0

/*

   *pDataBlock - pointer to mined block contents
   here - XXX
   end - XXX */
uint64_t CompleteEndpointSearch(const void* pDataBlock, uint64_t here,
                                        uint64_t end) {
    const unsigned char* pBuffer = (const unsigned char*)pDataBlock;
    unsigned int mBufPos = 0;
    unsigned int mBitBuffer = pBuffer[mBufPos++];
    unsigned int mBitCount = 8;
    unsigned char bits;
    uint64_t index;
    uint64_t tmp, result;
    uint64_t delta;


    // read generating index for first chain in block 
    READ8();
    tmp = bits;
    READ8();
    tmp = (tmp<<8)|bits;
    READ8();
    tmp = (tmp<<8)|bits;
    READ8();
    tmp = (tmp<<8)|bits;
    READN(2);
    tmp = (tmp<<2)|bits;

#if DEBUG_PRINT
    printf("%llx %llx\n", here, tmp);
#endif

    if (here==end) {
        result = tmp;
        return result;
    }

    for(;;) {
        int available = (4096-mBufPos)*8 + mBitCount;
        if (available<51) {
#if DEBUG_PRINT
            printf("End of block (%i bits left)\n", available);
#endif
            break;
        }
        READ8();
        if (bits==0xff) {
            if (available<72) {
#if DEBUG_PRINT
                printf("End of block (%i bits left)\n", available);
#endif
                break;
            }
            // Escape code 
            READ8();
            tmp = bits;
            READ8();
            tmp = (tmp<<8)|bits;
            READ8();
            tmp = (tmp<<8)|bits;
            READ8();
            tmp = (tmp<<8)|bits;
            READ8();
            tmp = (tmp<<8)|bits;
            READ8();
            tmp = (tmp<<8)|bits;
            READ8();
            tmp = (tmp<<8)|bits;
            READ8safe();
            tmp = (tmp<<8)|bits;
            delta = tmp >> 34;
            index = tmp & 0x3ffffffffULL;
        } else {
            unsigned int code = bits;
            unsigned int rb = mBits[code];
            //printf("%02x - %i - %x ",code,rb,mBase[code]);
            delta = mBase[code];
            unsigned int d2 = 0;
            if (rb>=8) {
                READ8();
                d2 = bits;
                rb-=8;
            }
            if (rb) {
                READN(rb);
                d2 = (d2<<rb)|bits;
            }
            //printf("%llx %x\n",delta,d2);
            delta+=d2;
            READ8();
            delta = (delta<<8)|bits;
            READN(2);
            delta = (delta<<2)|bits;

            READN(1);
            uint64_t idx = bits;
            READ8();
            idx = (idx<<8)|bits;
            READ8();
            idx = (idx<<8)|bits;
            READ8();
            idx = (idx<<8)|bits;
            READ8safe();
            index = (idx<<8)|bits;
        }
        here += delta<<12;
#if DEBUG_PRINT
        printf("%llx %llx\n", here, index);
#endif
        if (here==end) {
           result = index;
           return result;
        }

        if (here>end) {
#if DEBUG_PRINT
            printf("passed: %llx %llx\n", here, end);
#endif
            break;
        }
    }

    return 0;
}


