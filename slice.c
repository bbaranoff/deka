
/* Bitsliced OpenCL kernel for fast A5/1 chain computation
   with challenge lookup.

   So far, it is hardcoded to use matrix of 64 longs, 
   i.e., running 64 A5/1 instances in parallel. */

kernel void krak(global ulong *buf, ulong slices) {
  
	/* kernel index */
	private size_t me = get_global_id(0); 

	/* pointer to memory "owned" by this kernel instance */
	private size_t myptr = me * 4 * 64;

	private long i,j,z,s;
	private ulong diff;

	if(me >= 2048) {
	  return;
	}

	//private ulong flags = buf[myptr + i*4 + 3];

	/* slice algorithm variables */

	private ulong r[64];         // a5/1 registers, serialized
	private ulong ch[64];        // challenge
	private ulong rf[64];        // reduction function
	private ulong keystream[64]; // generated keystream
	private ulong mask = 0;      // running slices

	private ulong prevlink[64];

	/* fill the machine */

	for(i=0; i<64; i++) {
		r[i] = 0;
		rf[i] = 0;
		ch[i] = 0;
	}

	/* initial register value */
	for(i=0; i<64; i++) {             // for each slice

    /* Is this slice active, i.e., non-zero? */
    if (buf[myptr + i*4] != 0) {
      mask |= 1ULL << i;
    }

		//mask |= (buf[myptr + i*4] != 0) << i;

		//printf("adding %llx %llx\n",buf[me + i*4], buf[myptr + i*4 + 1]);
		for(j=0; j<64; j++) {          // add each bit to the right place
			r[j] |= ((buf[myptr + i*4] >> j) & 1) << i;
		}
	}

	/* reduction function */
	for(i=0; i<64; i++) { // for each slice
		//printf("color %llx\n",buf[me + i*4 + 1]);
		for(j=0; j<64; j++) { // for each bit
			rf[j] |= ((buf[myptr + i*4 + 1] >> j) & 1) << i;
		}
	}

	/* challenge */
	for(i=0; i<64; i++) { // for each slice
		//printf("challenge %llx\n",buf[me + i*4 + 2]);
		for(j=0; j<64; j++) { // for each bit
			ch[j] |= ((buf[myptr + i*4 + 2] >> j) & 1) << i;
		}
	}

	/*printf("input:  ");
	for(i=63; i>=0; i--) {
		printf("%x", (r[i]&4)>>2);
	}
	printf("\n");*/

	private ulong res = mask;

	/* run the A5/1 slice machine */
	for(z=0; z<5000; z++) {

		/*if(z<10) {
			printf("input:  ");
			for(i=63; i>=0; i--) {
				printf("%x", (r[i]&4)>>2);
			}
			printf(" m %llX r %llX\n",mask, res);
		}*/


		/* apply reduction */
		for(i=0; i<64; i++) {
			r[i] = r[i] ^ (rf[i] & res);
		}

		res = 0;

		/* which instances reached distinguished point */
		for(i=63; i>=52; i--) {
			res |= r[i];
		}
		if (z == 0) {
			res = mask;
		}

		//res = mask;

		//if(res != 0xFFFFFFFFFFFFFFFFULL) {
		//  printf("0@%i, res %llX\n",z,res);
		//  break;
		//}
		//ulong res = 0xFFULL;

		/* save prevlink */

		for(i=0; i<64; i++) {
			prevlink[i] = r[i];
		}

		/* increment color here - now it is done in the parent python script
       it would probably require excessive branching, so it won't work on GPGPU */

		/* run 100 dummy clockings to collapse the keyspace + 64 keystream clockings */
		/* though, the cipher spits out the bit *before* the clocking, so 100 rounds are
		   effectively only 99 and hence only 99 + 64 = 163 rounds here */
		for(s=0; s<163; s++) {

			/* Shifting inspired by original a5_ati/brook/a5_slice.br.
			   Changed bit order and vectorizing should now do the compiler for us.
			   Also, compiler should handle loop unrolling.
			   And we are running in full 64 bits!

			   An A5/1 image or animation might be handy when understanding or
			   debugging this. Additionally, go read the bitslicing guide and
			   a Wikipedia article on Karnaugh maps.
			   https://en.wikipedia.org/wiki/A5/1#Description
			   https://www.youtube.com/watch?v=LgZAI3DdUA4
			   https://en.wikipedia.org/wiki/Karnaugh_map
			   http://archive.today/hTu5n */

			/* compute majority clocking */

			/* How does this work? When these bits are the same, then XOR
			   returns 0 and ~ returns 1; otherwise, it's 0 */
			ulong maj12 = ~ (r[8]  ^ r[29]);
			ulong maj13 = ~ (r[8]  ^ r[51]);
			ulong maj23 = ~ (r[29] ^ r[51]);

			ulong clock1 = maj12 | maj13;
			ulong clock2 = maj12 | maj23;
			ulong clock3 = maj13 | maj23;

			//printf("maj %x %x %x\n", maj12, maj13, maj23);

			/* do not clock instances in distinguished point */

			clock1 &= res;
			clock2 &= res;
			clock3 &= res;

			/* Precompute clock inverse */

			ulong iclk1 = ~clock1;
			ulong iclk2 = ~clock2;
			ulong iclk3 = ~clock3;

			/* LFSR feedback */

			ulong fb1 = (r[18] ^ r[17] ^ r[16] ^ r[13]) & clock1;
			ulong fb2 = (r[40] ^ r[39]) & clock2;
			ulong fb3 = (r[63] ^ r[62] ^ r[61] ^ r[48]) & clock3;


			/* the shifting itself */

			/* See https://en.wikipedia.org/wiki/File:A5-1_GSM_cipher.svg
			   We have the registers oriented like it's on that image,
			   so we are going "downwards", hence i-- here. */


			/* LFSR 1: bits 0-18 */
			for(i=18; i>0; i--) {
				r[i] = (r[i] & iclk1) | (r[i-1] & clock1);
			}
			r[0] = (r[0] & iclk1) | fb1;

			/* LFSR 2: bits 19-40 */
			for(i=40; i>19; i--) {
				r[i] = (r[i] & iclk2) | (r[i-1] & clock2);
			}
			r[19] = (r[19] & iclk2) | fb2;

			/* LFSR 3: bits 41-63 */
			for(i=63; i>41; i--) {
				r[i] = (r[i] & iclk3) | (r[i-1] & clock3);
			}
			r[41] = (r[41] & iclk3) | fb3;

			/* generate keystream, ugly modulo hack :-( */
			keystream[63 - ((s%99)&0x3f)] = (r[18] ^ r[40] ^ r[63]);

		}
		/* check for challenge */
		diff = ~res;
		for(i=0; i<64; i++) {
			diff |= ch[i] ^ keystream[i];
		}

		if(diff != 0xFFFFFFFFFFFFFFFFULL) {
			//printf("found %llx\n", diff);

			/* I don't really want to solve register restore here,
			   so just break the kernel execution and let our parent
			   handle this */

			for(i=0; i<64; i++) {
				r[i] = prevlink[i];
			}
			
			break;
		}
		//mask &= diff;
		

		/* use computed keystream as input in next round */
		for(i=0; i<64; i++) {
			r[i] = (keystream[i] & res) | (r[i] & ~res);
		}
	}

	/*printf("output: ");
	for(i=63; i>=0; i--) {
		printf("%x", (r[i]&4)>>2);
	}
	printf(" color: ");
	for(i=63; i>=0; i--) {
		printf("%x", (rf[i]&4)>>2);
	}
	printf("\n");
	printf("res %llX, diff %llX\n",res, diff);*/


	/* copy data back */
	for(i=0; i<64; i++) {             // for each slice
		buf[myptr + i*4] = 0;
		buf[myptr + i*4 + 3] |= ((~res >> i) & 1) << 0; // set color-end flag
		buf[myptr + i*4 + 3] |= ((~diff >> i) & 1) << 1; // set found flag
		for(j=63; j>=0; j--) {
			buf[myptr + i*4] |= ((r[j] >> i) & 1) << j;
		}
	}
}

