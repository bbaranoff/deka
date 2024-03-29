
/* First, path to index files. The order has to match the numbers in vankusconf.py. */
const char * files[40] = {
"/mnt1/kraken/indexes/132.idx",
"/mnt1/kraken/indexes/140.idx",
"/mnt1/kraken/indexes/108.idx",
"/mnt1/kraken/indexes/116.idx",
"/mnt1/kraken/indexes/100.idx",
"/mnt1/kraken/indexes/124.idx",


"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx",
"/mnt1/test.idx"
};


/* Offsets of beginning of tables specified in 4096B long blocks.
   Of course the order matters.
   E.g. the first number "102347869" tells us that the first table, 380, begins
   102347869*4096 = 419 216 871 424 bytes from the beginning of the device.
*/

const uint64_t offsets[40] = {
40933745,
51204519,
10228856,
20463167,
0,
30695103,


0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0
};

/* Path to devices (or files) where the offsets are stored. */
const char * devpaths[1] = {
"/dev/sdb4"
};

/* Which table has been stored on which device. E.g. "2" means that the table
   is on second (counting from 0) device from the devpaths array.
*/
//const int devs[40] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
const int devs[40] = {
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0
};
