/* First, path to index files. The order has to match the numbers in vankusconf.py. */
const char * files[40] = {
"/mnt/kraken/indexes/108.idx",
"/mnt/kraken/indexes/132.idx",
"/mnt/kraken/indexes/100.idx",
"/mnt/kraken/indexes/116.idx",
"/mnt/kraken/indexes/124.idx",
"/mnt/kraken/indexes/172.idx",
"/mnt/kraken/indexes/148.idx",
"/mnt/kraken/indexes/156.idx",
"/mnt/kraken/indexes/164.idx",
"/mnt/kraken/indexes/140.idx",
"/mnt1/kraken/indexes/220.idx",
"/mnt1/kraken/indexes/250.idx",
"/mnt1/kraken/indexes/196.idx",
"/mnt1/kraken/indexes/204.idx",
"/mnt1/kraken/indexes/260.idx",
"/mnt1/kraken/indexes/180.idx",
"/mnt1/kraken/indexes/188.idx",
"/mnt1/kraken/indexes/212.idx",
"/mnt1/kraken/indexes/230.idx",
"/mnt1/kraken/indexes/238.idx",
"/mnt2/kraken/indexes/324.idx",
"/mnt2/kraken/indexes/276.idx",
"/mnt2/kraken/indexes/268.idx",
"/mnt2/kraken/indexes/332.idx",
"/mnt2/kraken/indexes/292.idx",
"/mnt2/kraken/indexes/348.idx",
"/mnt2/kraken/indexes/340.idx",
"/mnt2/kraken/indexes/372.idx",
"/mnt2/kraken/indexes/356.idx",
"/mnt2/kraken/indexes/364.idx",
"/mnt3/kraken/indexes/388.idx",
"/mnt3/kraken/indexes/404.idx",
"/mnt3/kraken/indexes/396.idx",
"/mnt3/kraken/indexes/412.idx",
"/mnt3/kraken/indexes/380.idx",
"/mnt3/kraken/indexes/420.idx",
"/mnt3/kraken/indexes/436.idx",
"/mnt3/kraken/indexes/428.idx",
"/mnt3/kraken/indexes/492.idx",
"/mnt3/kraken/indexes/500.idx"
};

/* Offsets of beginning of tables specified in 4096B long blocks.
   Of course the order matters.
   E.g. the first number "102347869" tells us that the first table, 380, begins
   102347869*4096 = 419 216 871 424 bytes from the beginning of the device.
*/

const uint64_t offsets[40] = {
10228856,
40933745,
0,
20463167,
30695103,
40929700,
10237416,
20466911,
30699266,
0,
10230895,
30688643,
0,
20458654,
40928904,
30697677,
40926561,
10229941,
20463312,
0,
30694043,
10232220,
0,
40929055,
20461300,
10232322,
0,
40924849,
20461449,
30693163,
0,
30716396,
40948655,
20482783,
10234340,
40932533,
10229859,
30697447,
0,
20463952


};

/* Path to devices (or files) where the offsets are stored. */
const char * devpaths[8] = {
"/dev/indexes/lvol0",
"/dev/indexes/lvol1",
"/dev/indexes/lvol2",
"/dev/indexes/lvol3",
"/dev/indexes/lvol4",
"/dev/indexes/lvol5",
"/dev/indexes/lvol6",
"/dev/indexes/lvol7"
};

/* Which table has been stored on which device. E.g. "2" means that the table
   is on second (counting from 0) device from the devpaths array.
*/
//const int devs[40] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
const int devs[40] = {
0,0,0,0,0,1,1,1,1,1,
2,2,2,2,2,3,3,3,3,3,
4,4,4,4,4,5,5,5,5,5,
6,6,6,6,6,7,7,7,7,7
};
