
/* How many bursts to load in parallel. The GPU should be fully saturated.
   Something like 50 is a good start, depending on number of computing units
   on your card.
   If the value is too low, "kernels" in oclvankus log will be lower than
   specified (and the performance would be of course impaired).
   If the value is too high, the cracker will have high latency.
*/
#define QSIZE 140
/* XXX 80 */

/* size of GPGPU buffer, kernels*slices */
#define CLBLOBSIZE 8191*32
/* 4095 */

/* tables we have */
uint64_t mytables[] = {132, 260, 172, 388, 140, 148, 324, 156,364, 164, 220, 356, 412, 428, 500, 436, 180, 188,492, 196, 204, 292, 268, 332, 372, 212, 420,348, 396, 100, 230, 340, 124, 108, 238,116, 404, 250, 380, 276};
