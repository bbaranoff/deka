
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
#define CLBLOBSIZE 4095*32
/* 4095 */

/* tables we have */
uint64_t mytables[] = {132, 108, 116, 124, 100, 140, 148, 156, 164, 172, 100, 108, 116, 124, 132, 140, 148, 156, 164, 172, 100, 108, 116, 124, 132, 140, 148, 156, 164, 172, 100, 108, 116, 124, 132, 140, 148, 156, 164, 172};
