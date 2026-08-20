
/* How many bursts to load in parallel. The GPU should be fully saturated.
   Something like 50 is a good start, depending on number of computing units
   on your card.
   If the value is too low, "kernels" in oclvankus log will be lower than
   specified (and the performance would be of course impaired).
   If the value is too high, the cracker will have high latency.
*/
#define QSIZE 40
/* XXX 80 */

/* size of GPGPU buffer, kernels*slices */
#define CLBLOBSIZE 4095*64
/* 4095 */

/* tables we have */
uint64_t mytables[] = {108, 132, 100, 116, 124, 172, 148, 156, 164, 140, 220, 250, 196, 204, 260, 180, 188, 212, 230, 238, 324, 276, 268, 332, 292, 348, 340, 372, 356, 364, 388, 404, 396, 412, 380, 420, 436, 428, 492, 500};
