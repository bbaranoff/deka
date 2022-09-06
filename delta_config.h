
/* First, path to index files. The order has to match the numbers in vankusconf.py. */
const char * files[40] = {
"/home/nirvana/kraken/indexes/132.idx",
"/home/nirvana/kraken/indexes/260.idx",
"/home/nirvana/kraken/indexes/172.idx",
"/home/nirvana/kraken/indexes/388.idx",
"/home/nirvana/kraken/indexes/140.idx",
"/home/nirvana/kraken/indexes/148.idx",
"/home/nirvana/kraken/indexes/324.idx",
"/home/nirvana/kraken/indexes/156.idx",
"/home/nirvana/kraken/indexes/364.idx",
"/home/nirvana/kraken/indexes/164.idx",
"/home/nirvana/kraken/indexes/220.idx",
"/home/nirvana/kraken/indexes/356.idx",
"/home/nirvana/kraken/indexes/412.idx",
"/home/nirvana/kraken/indexes/428.idx",
"/home/nirvana/kraken/indexes/500.idx",
"/home/nirvana/kraken/indexes/436.idx",
"/home/nirvana/kraken/indexes/180.idx",
"/home/nirvana/kraken/indexes/188.idx",
"/home/nirvana/kraken/indexes/492.idx",
"/home/nirvana/kraken/indexes/196.idx",
"/home/nirvana/kraken/indexes/204.idx",
"/home/nirvana/kraken/indexes/292.idx",
"/home/nirvana/kraken/indexes/268.idx",
"/home/nirvana/kraken/indexes/332.idx",
"/home/nirvana/kraken/indexes/372.idx",
"/home/nirvana/kraken/indexes/212.idx",
"/home/nirvana/kraken/indexes/420.idx",
"/home/nirvana/kraken/indexes/348.idx",
"/home/nirvana/kraken/indexes/396.idx",
"/home/nirvana/kraken/indexes/100.idx",
"/home/nirvana/kraken/indexes/230.idx",
"/home/nirvana/kraken/indexes/340.idx",
"/home/nirvana/kraken/indexes/124.idx",
"/home/nirvana/kraken/indexes/108.idx",
"/home/nirvana/kraken/indexes/238.idx",
"/home/nirvana/kraken/indexes/116.idx",
"/home/nirvana/kraken/indexes/404.idx",
"/home/nirvana/kraken/indexes/250.idx",
"/home/nirvana/kraken/indexes/380.idx",
"/home/nirvana/kraken/indexes/276.idx"
};

/* Offsets of beginning of tables specified in 4096B long blocks.
   Of course the order matters.
   E.g. the first number "102347869" tells us that the first table, 380, begins
   102347869*4096 = 419 216 871 424 bytes from the beginning of the device.
*/

const uint64_t offsets[40] = {
337704706,
153511760,
163756989,
317240425,
0,
81872479,
194451345,
92101974,
368445836,
133047015,
388906829,
347975480,
10237416,
30711290,
307006930,
51177271,
286545303,
235382931,
276315444,
40946376,
225152942,
296774187,
214920722,
122814790,
378677522,
255851823,
102334329,
173990282,
266085194,
71643623,
204686357,
399134588,
358207194,
143277449,
327474765,
184219409,
61411364,
20471029,
112566347,
245622743
};

/* Path to devices (or files) where the tables are stored. */
const char * devpaths[] = {
"/dev/sdd"};

/* Which table has been stored on which device. E.g. "2" means that the table
   is on second (counting from 0) device from the devpaths array.
*/
//const int devs[40] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
const int devs[40] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

