
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
			r[63-j] |= ((buf[myptr + i*4] >> j) & 1) << i;
		}
	}

	/* reduction function */
	for(i=0; i<64; i++) { // for each slice
		//printf("color %llx\n",buf[me + i*4 + 1]);
		for(j=0; j<64; j++) { // for each bit
			rf[63-j] |= ((buf[myptr + i*4 + 1] >> j) & 1) << i;
		}
	}

	/* challenge */
	for(i=0; i<64; i++) { // for each slice
		//printf("challenge %llx\n",buf[me + i*4 + 2]);
		for(j=0; j<64; j++) { // for each bit
			ch[63-j] |= ((buf[myptr + i*4 + 2] >> j) & 1) << i;
		}
	}

	/*printf("input:  ");
	for(i=63; i>=0; i--) {
		printf("%x", (r[i]&4)>>2);
	}
	printf("\n");*/

	private ulong res = mask;

        private ulong ctrl = mask;

    ulong4 reg1;
    ulong4 reg2;
  ulong4 reg3;
  ulong4 reg4;
  ulong4 reg5;
  ulong4 reg6;
  ulong4 reg7;
  ulong4 reg8;
    ulong4 reg9;
    ulong4 reg10;
  ulong4 reg11;
  ulong4 reg12;
  ulong4 reg13;
  ulong4 reg14;
  ulong4 reg15;
  ulong4 reg16;



    ulong4 out1;
    ulong4 out2;
    ulong4 out3;
    ulong4 out4;
    ulong4 out5;
    ulong4 out6;
    ulong4 out7;
    ulong4 out8;
    ulong4 out9;
    ulong4 out10;
    ulong4 out11;
    ulong4 out12;
    ulong4 out13;
    ulong4 out14;
    ulong4 out15;
    ulong4 out16;

    ulong4 fin1;
    ulong4 fin2;
    ulong4 fin3;
    ulong4 fin4;
    ulong4 fin5;
    ulong4 fin6;
    ulong4 fin7;
    ulong4 fin8;
    ulong4 fin9;
    ulong4 fin10;
    ulong4 fin11;
    ulong4 fin12;
    ulong4 fin13;
    ulong4 fin14;
    ulong4 fin15;
    ulong4 fin16;



fin1.x = rf[0];
fin1.y = rf[1];
fin1.z = rf[2];
fin1.w = rf[3];
fin2.x = rf[4];
fin2.y = rf[5];
fin2.z = rf[6];
fin2.w = rf[7];
fin3.x = rf[8];
fin3.y = rf[9];
fin3.z = rf[10];
fin3.w = rf[11];
fin4.x = rf[12];
fin4.y = rf[13];
fin4.z = rf[14];
fin4.w = rf[15];
fin5.x = rf[16];
fin5.y = rf[17];
fin5.z = rf[18];
fin5.w = rf[19];
fin6.x = rf[20];
fin6.y = rf[21];
fin6.z = rf[22];
fin6.w = rf[23];
fin7.x = rf[24];
fin7.y = rf[25];
fin7.z = rf[26];
fin7.w = rf[27];
fin8.x = rf[28];
fin8.y = rf[29];
fin8.z = rf[30];
fin8.w = rf[31];
fin9.x = rf[32];
fin9.y = rf[33];
fin9.z = rf[34];
fin9.w = rf[35];
fin10.x = rf[36];
fin10.y = rf[37];
fin10.z = rf[38];
fin10.w = rf[39];
fin11.x = rf[40];
fin11.y = rf[41];
fin11.z = rf[42];
fin11.w = rf[43];
fin12.x = rf[44];
fin12.y = rf[45];
fin12.z = rf[46];
fin12.w = rf[47];
fin13.x = rf[48];
fin13.y = rf[49];
fin13.z = rf[50];
fin13.w = rf[51];
fin14.x = rf[52];
fin14.y = rf[53];
fin14.z = rf[54];
fin14.w = rf[55];
fin15.x = rf[56];
fin15.y = rf[57];
fin15.z = rf[58];
fin15.w = rf[59];
fin16.x = rf[60];
fin16.y = rf[61];
fin16.z = rf[62];
fin16.w = rf[63];



  ulong clock1;
  ulong clock2;
  ulong clock3;
    ulong iclk1;
    ulong iclk2;
    ulong iclk3;

  ulong cr1r2;
  ulong cr1r3;
  ulong cr2r3;

    ulong fb1;
    ulong fb2;
    ulong fb3;


	/* run the A5/1 slice machine */
	for(z=0; z<5000; z++) {


out1.x = r[0];
out1.y = r[1];
out1.z = r[2];
out1.w = r[3];
out2.x = r[4];
out2.y = r[5];
out2.z = r[6];
out2.w = r[7];
out3.x = r[8];
out3.y = r[9];
out3.z = r[10];
out3.w = r[11];
out4.x = r[12];
out4.y = r[13];
out4.z = r[14];
out4.w = r[15];
out5.x = r[16];
out5.y = r[17];
out5.z = r[18];
out5.w = r[19];
out6.x = r[20];
out6.y = r[21];
out6.z = r[22];
out6.w = r[23];
out7.x = r[24];
out7.y = r[25];
out7.z = r[26];
out7.w = r[27];
out8.x = r[28];
out8.y = r[29];
out8.z = r[30];
out8.w = r[31];
out9.x = r[32];
out9.y = r[33];
out9.z = r[34];
out9.w = r[35];
out10.x = r[36];
out10.y = r[37];
out10.z = r[38];
out10.w = r[39];
out11.x = r[40];
out11.y = r[41];
out11.z = r[42];
out11.w = r[43];
out12.x = r[44];
out12.y = r[45];
out12.z = r[46];
out12.w = r[47];
out13.x = r[48];
out13.y = r[49];
out13.z = r[50];
out13.w = r[51];
out14.x = r[52];
out14.y = r[53];
out14.z = r[54];
out14.w = r[55];
out15.x = r[56];
out15.y = r[57];
out15.z = r[58];
out15.w = r[59];
out16.x = r[60];
out16.y = r[61];
out16.z = r[62];
out16.w = r[63];




		/*if(z<10) {
			printf("input:  ");
			for(i=63; i>=0; i--) {
				printf("%x", (r[i]&4)>>2);
			}
			printf(" m %llX r %llX\n",mask, res);
		}*/


		/* apply reduction */
		/*for(i=0; i<64; i++) {
			r[i] = r[i] ^ (rf[i] & res);
		}*/

        reg1 = out1 ^ fin1;
    reg2 = out2 ^ fin2;
    reg3 = out3 ^ fin3;
    reg4 = out4 ^ fin4;
    reg5 = out5 ^ fin5;
    reg6 = out6 ^ fin6;
    reg7 = out7 ^ fin7;
        reg8 = out8 ^ fin8;
    reg9 = out9 ^ fin9;
    reg10 = out10 ^ fin10;
    reg11 = out11 ^ fin11;
        reg12 = out12 ^ fin12;
    reg13 = out13 ^ fin13;
    reg14 = out14 ^ fin14;
        reg15 = out15 ^ fin15;
        reg16 = out16 ^ fin16;



		res = 0;

		/* which instances reached distinguished point */
		/*for(i=63; i>=52; i--) {
			res |= r[i];
		}
		if (z == 0) {
			res = mask;
		}*/


        res = reg1.x | reg1.y | reg1.z | reg1.w |
            reg2.x | reg2.y | reg2.z | reg2.w |
            reg3.x | reg3.y | reg3.z | reg3.w |
            reg4.x | reg4.y | ctrl;
           if(res != 0xFFFFFFFFFFFFFFFFULL) {
             //printf("0@%i, res %llX\n",z,res);
             break;
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

ctrl = 0u;

        for (j = 0; j<99; j++) {
            /* Extra (discarded clockings) */
            cr1r2 = ~(reg14.w ^ reg9.z);
            cr1r3 = ~(reg14.w ^ reg4.x);
            cr2r3 = ~(reg9.z  ^ reg4.x);
            clock1 = cr1r2 | cr1r3;
            clock2 = cr1r2 | cr2r3;
            clock3 = cr1r3 | cr2r3;
            fb1 = (reg12.y ^ reg12.z ^ reg12.w ^ reg13.z) & clock1;
            fb2 = (reg6.w ^ reg7.x) & clock2;
            fb3 = (reg1.x ^ reg1.y ^ reg1.z ^ reg4.w) & clock3;
            iclk1 = ~clock1;
            iclk2 = ~clock2;
            iclk3 = ~clock3;
            reg1.x = (reg1.x & iclk3) | (reg1.y & clock3);
            reg1.y = (reg1.y & iclk3) | (reg1.z & clock3);
            reg1.z = (reg1.z & iclk3) | (reg1.w & clock3);
            reg1.w = (reg1.w & iclk3) | (reg2.x & clock3);
            reg2.x = (reg2.x & iclk3) | (reg2.y & clock3);
            reg2.y = (reg2.y & iclk3) | (reg2.z & clock3);
            reg2.z = (reg2.z & iclk3) | (reg2.w & clock3);
            reg2.w = (reg2.w & iclk3) | (reg3.x & clock3);
            reg3.x = (reg3.x & iclk3) | (reg3.y & clock3);
            reg3.y = (reg3.y & iclk3) | (reg3.z & clock3);
            reg3.z = (reg3.z & iclk3) | (reg3.w & clock3);
            reg3.w = (reg3.w & iclk3) | (reg4.x & clock3);
            reg4.x = (reg4.x & iclk3) | (reg4.y & clock3);
            reg4.y = (reg4.y & iclk3) | (reg4.z & clock3);
            reg4.z = (reg4.z & iclk3) | (reg4.w & clock3);
            reg4.w = (reg4.w & iclk3) | (reg5.x & clock3);
            reg5.x = (reg5.x & iclk3) | (reg5.y & clock3);
            reg5.y = (reg5.y & iclk3) | (reg5.z & clock3);
            reg5.z = (reg5.z & iclk3) | (reg5.w & clock3);
            reg5.w = (reg5.w & iclk3) | (reg6.x & clock3);
            reg6.x = (reg6.x & iclk3) | (reg6.y & clock3);
            reg6.y = (reg6.y & iclk3) | (reg6.z & clock3);
            reg6.z = (reg6.z & iclk3) | fb3;
            reg6.w = (reg6.w & iclk2) | (reg7.x & clock2);;
            reg7.x = (reg7.x & iclk2) | (reg7.y & clock2);
            reg7.y = (reg7.y & iclk2) | (reg7.z & clock2);
            reg7.z = (reg7.z & iclk2) | (reg7.w & clock2);
            reg7.w = (reg7.w & iclk2) | (reg8.x & clock2);
            reg8.x = (reg8.x & iclk2) | (reg8.y & clock2);
            reg8.y = (reg8.y & iclk2) | (reg8.z & clock2);
            reg8.z = (reg8.z & iclk2) | (reg8.w & clock2);
            reg8.w = (reg8.w & iclk2) | (reg9.x & clock2);
            reg9.x = (reg9.x & iclk2) | (reg9.y & clock2);
            reg9.y = (reg9.y & iclk2) | (reg9.z & clock2);
            reg9.z = (reg9.z & iclk2) | (reg9.w & clock2);
            reg9.w = (reg9.w & iclk2) | (reg10.x & clock2);
            reg10.x = (reg10.x & iclk2) | (reg10.y & clock2);
            reg10.y = (reg10.y & iclk2) | (reg10.z & clock2);
            reg10.z = (reg10.z & iclk2) | (reg10.w & clock2);
            reg10.w = (reg10.w & iclk2) | (reg11.x & clock2);
            reg11.x = (reg11.x & iclk2) | (reg11.y & clock2);
            reg11.y = (reg11.y & iclk2) | (reg11.z & clock2);
            reg11.z = (reg11.z & iclk2) | (reg11.w & clock2);
            reg11.w = (reg11.w & iclk2) | (reg12.x & clock2);
            reg12.x = (reg12.x & iclk2) | fb2;
            reg12.y = (reg12.y & iclk1) | (reg12.z & clock1);
            reg12.z = (reg12.z & iclk1) | (reg12.w & clock1);
            reg12.w = (reg12.w & iclk1) | (reg13.x & clock1);
            reg13.x = (reg13.x & iclk1) | (reg13.y & clock1);
            reg13.y = (reg13.y & iclk1) | (reg13.z & clock1);
            reg13.z = (reg13.z & iclk1) | (reg13.w & clock1);
            reg13.w = (reg13.w & iclk1) | (reg14.x & clock1);
            reg14.x = (reg14.x & iclk1) | (reg14.y & clock1);
            reg14.y = (reg14.y & iclk1) | (reg14.z & clock1);
            reg14.z = (reg14.z & iclk1) | (reg14.w & clock1);
            reg14.w = (reg14.w & iclk1) | (reg15.x & clock1);
            reg15.x = (reg15.x & iclk1) | (reg15.y & clock1);
            reg15.y = (reg15.y & iclk1) | (reg15.z & clock1);
            reg15.z = (reg15.z & iclk1) | (reg15.w & clock1);
            reg15.w = (reg15.w & iclk1) | (reg16.x & clock1);
            reg16.x = (reg16.x & iclk1) | (reg16.y & clock1);
            reg16.y = (reg16.y & iclk1) | (reg16.z & clock1);
            reg16.z = (reg16.z & iclk1) | (reg16.w & clock1);
            reg16.w = (reg16.w & iclk1) | fb1;
        }

        for(j = 0; j < 16; j++) {
            cr1r2 = ~(reg14.w ^ reg9.z);
            cr1r3 = ~(reg14.w ^ reg4.x);
            cr2r3 = ~(reg9.z  ^ reg4.x);
            clock1 = cr1r2 | cr1r3;
            clock2 = cr1r2 | cr2r3;
            clock3 = cr1r3 | cr2r3;
            fb1 = (reg12.y ^ reg12.z ^ reg12.w ^ reg13.z) & clock1;
            fb2 = (reg6.w ^ reg7.x) & clock2;
            fb3 = (reg1.x ^ reg1.y ^ reg1.z ^ reg4.w) & clock3;
            iclk1 = ~clock1;
            iclk2 = ~clock2;
            iclk3 = ~clock3;
            reg1.x = (reg1.x & iclk3) | (reg1.y & clock3);
            reg1.y = (reg1.y & iclk3) | (reg1.z & clock3);
            reg1.z = (reg1.z & iclk3) | (reg1.w & clock3);
            reg1.w = (reg1.w & iclk3) | (reg2.x & clock3);
            reg2.x = (reg2.x & iclk3) | (reg2.y & clock3);
            reg2.y = (reg2.y & iclk3) | (reg2.z & clock3);
            reg2.z = (reg2.z & iclk3) | (reg2.w & clock3);
            reg2.w = (reg2.w & iclk3) | (reg3.x & clock3);
            reg3.x = (reg3.x & iclk3) | (reg3.y & clock3);
            reg3.y = (reg3.y & iclk3) | (reg3.z & clock3);
            reg3.z = (reg3.z & iclk3) | (reg3.w & clock3);
            reg3.w = (reg3.w & iclk3) | (reg4.x & clock3);
            reg4.x = (reg4.x & iclk3) | (reg4.y & clock3);
            reg4.y = (reg4.y & iclk3) | (reg4.z & clock3);
            reg4.z = (reg4.z & iclk3) | (reg4.w & clock3);
            reg4.w = (reg4.w & iclk3) | (reg5.x & clock3);
            reg5.x = (reg5.x & iclk3) | (reg5.y & clock3);
            reg5.y = (reg5.y & iclk3) | (reg5.z & clock3);
            reg5.z = (reg5.z & iclk3) | (reg5.w & clock3);
            reg5.w = (reg5.w & iclk3) | (reg6.x & clock3);
            reg6.x = (reg6.x & iclk3) | (reg6.y & clock3);
            reg6.y = (reg6.y & iclk3) | (reg6.z & clock3);
            reg6.z = (reg6.z & iclk3) | fb3;
            reg6.w = (reg6.w & iclk2) | (reg7.x & clock2);;
            reg7.x = (reg7.x & iclk2) | (reg7.y & clock2);
            reg7.y = (reg7.y & iclk2) | (reg7.z & clock2);
            reg7.z = (reg7.z & iclk2) | (reg7.w & clock2);
            reg7.w = (reg7.w & iclk2) | (reg8.x & clock2);
            reg8.x = (reg8.x & iclk2) | (reg8.y & clock2);
            reg8.y = (reg8.y & iclk2) | (reg8.z & clock2);
            reg8.z = (reg8.z & iclk2) | (reg8.w & clock2);
            reg8.w = (reg8.w & iclk2) | (reg9.x & clock2);
            reg9.x = (reg9.x & iclk2) | (reg9.y & clock2);
            reg9.y = (reg9.y & iclk2) | (reg9.z & clock2);
            reg9.z = (reg9.z & iclk2) | (reg9.w & clock2);
            reg9.w = (reg9.w & iclk2) | (reg10.x & clock2);
            reg10.x = (reg10.x & iclk2) | (reg10.y & clock2);
            reg10.y = (reg10.y & iclk2) | (reg10.z & clock2);
            reg10.z = (reg10.z & iclk2) | (reg10.w & clock2);
            reg10.w = (reg10.w & iclk2) | (reg11.x & clock2);
            reg11.x = (reg11.x & iclk2) | (reg11.y & clock2);
            reg11.y = (reg11.y & iclk2) | (reg11.z & clock2);
            reg11.z = (reg11.z & iclk2) | (reg11.w & clock2);
            reg11.w = (reg11.w & iclk2) | (reg12.x & clock2);
            reg12.x = (reg12.x & iclk2) | fb2;
            reg12.y = (reg12.y & iclk1) | (reg12.z & clock1);
            reg12.z = (reg12.z & iclk1) | (reg12.w & clock1);
            reg12.w = (reg12.w & iclk1) | (reg13.x & clock1);
            reg13.x = (reg13.x & iclk1) | (reg13.y & clock1);
            reg13.y = (reg13.y & iclk1) | (reg13.z & clock1);
            reg13.z = (reg13.z & iclk1) | (reg13.w & clock1);
            reg13.w = (reg13.w & iclk1) | (reg14.x & clock1);
            reg14.x = (reg14.x & iclk1) | (reg14.y & clock1);
            reg14.y = (reg14.y & iclk1) | (reg14.z & clock1);
            reg14.z = (reg14.z & iclk1) | (reg14.w & clock1);
            reg14.w = (reg14.w & iclk1) | (reg15.x & clock1);
            reg15.x = (reg15.x & iclk1) | (reg15.y & clock1);
            reg15.y = (reg15.y & iclk1) | (reg15.z & clock1);
            reg15.z = (reg15.z & iclk1) | (reg15.w & clock1);
            reg15.w = (reg15.w & iclk1) | (reg16.x & clock1);
            reg16.x = (reg16.x & iclk1) | (reg16.y & clock1);
            reg16.y = (reg16.y & iclk1) | (reg16.z & clock1);
            reg16.z = (reg16.z & iclk1) | (reg16.w & clock1);
            reg16.w = (reg16.w & iclk1) | fb1;
            out1.x   = (reg1.x ^ reg6.w ^ reg12.y);

            cr1r2 = ~(reg14.w ^ reg9.z);
            cr1r3 = ~(reg14.w ^ reg4.x);
            cr2r3 = ~(reg9.z  ^ reg4.x);
            clock1 = cr1r2 | cr1r3;
            clock2 = cr1r2 | cr2r3;
            clock3 = cr1r3 | cr2r3;
            fb1 = (reg12.y ^ reg12.z ^ reg12.w ^ reg13.z) & clock1;
            fb2 = (reg6.w ^ reg7.x) & clock2;
            fb3 = (reg1.x ^ reg1.y ^ reg1.z ^ reg4.w) & clock3;
            iclk1 = ~clock1;
            iclk2 = ~clock2;
            iclk3 = ~clock3;
            reg1.x = (reg1.x & iclk3) | (reg1.y & clock3);
            reg1.y = (reg1.y & iclk3) | (reg1.z & clock3);
            reg1.z = (reg1.z & iclk3) | (reg1.w & clock3);
            reg1.w = (reg1.w & iclk3) | (reg2.x & clock3);
            reg2.x = (reg2.x & iclk3) | (reg2.y & clock3);
            reg2.y = (reg2.y & iclk3) | (reg2.z & clock3);
            reg2.z = (reg2.z & iclk3) | (reg2.w & clock3);
            reg2.w = (reg2.w & iclk3) | (reg3.x & clock3);
            reg3.x = (reg3.x & iclk3) | (reg3.y & clock3);
            reg3.y = (reg3.y & iclk3) | (reg3.z & clock3);
            reg3.z = (reg3.z & iclk3) | (reg3.w & clock3);
            reg3.w = (reg3.w & iclk3) | (reg4.x & clock3);
            reg4.x = (reg4.x & iclk3) | (reg4.y & clock3);
            reg4.y = (reg4.y & iclk3) | (reg4.z & clock3);
            reg4.z = (reg4.z & iclk3) | (reg4.w & clock3);
            reg4.w = (reg4.w & iclk3) | (reg5.x & clock3);
            reg5.x = (reg5.x & iclk3) | (reg5.y & clock3);
            reg5.y = (reg5.y & iclk3) | (reg5.z & clock3);
            reg5.z = (reg5.z & iclk3) | (reg5.w & clock3);
            reg5.w = (reg5.w & iclk3) | (reg6.x & clock3);
            reg6.x = (reg6.x & iclk3) | (reg6.y & clock3);
            reg6.y = (reg6.y & iclk3) | (reg6.z & clock3);
            reg6.z = (reg6.z & iclk3) | fb3;
            reg6.w = (reg6.w & iclk2) | (reg7.x & clock2);;
            reg7.x = (reg7.x & iclk2) | (reg7.y & clock2);
            reg7.y = (reg7.y & iclk2) | (reg7.z & clock2);
            reg7.z = (reg7.z & iclk2) | (reg7.w & clock2);
            reg7.w = (reg7.w & iclk2) | (reg8.x & clock2);
            reg8.x = (reg8.x & iclk2) | (reg8.y & clock2);
            reg8.y = (reg8.y & iclk2) | (reg8.z & clock2);
            reg8.z = (reg8.z & iclk2) | (reg8.w & clock2);
            reg8.w = (reg8.w & iclk2) | (reg9.x & clock2);
            reg9.x = (reg9.x & iclk2) | (reg9.y & clock2);
            reg9.y = (reg9.y & iclk2) | (reg9.z & clock2);
            reg9.z = (reg9.z & iclk2) | (reg9.w & clock2);
            reg9.w = (reg9.w & iclk2) | (reg10.x & clock2);
            reg10.x = (reg10.x & iclk2) | (reg10.y & clock2);
            reg10.y = (reg10.y & iclk2) | (reg10.z & clock2);
            reg10.z = (reg10.z & iclk2) | (reg10.w & clock2);
            reg10.w = (reg10.w & iclk2) | (reg11.x & clock2);
            reg11.x = (reg11.x & iclk2) | (reg11.y & clock2);
            reg11.y = (reg11.y & iclk2) | (reg11.z & clock2);
            reg11.z = (reg11.z & iclk2) | (reg11.w & clock2);
            reg11.w = (reg11.w & iclk2) | (reg12.x & clock2);
            reg12.x = (reg12.x & iclk2) | fb2;
            reg12.y = (reg12.y & iclk1) | (reg12.z & clock1);
            reg12.z = (reg12.z & iclk1) | (reg12.w & clock1);
            reg12.w = (reg12.w & iclk1) | (reg13.x & clock1);
            reg13.x = (reg13.x & iclk1) | (reg13.y & clock1);
            reg13.y = (reg13.y & iclk1) | (reg13.z & clock1);
            reg13.z = (reg13.z & iclk1) | (reg13.w & clock1);
            reg13.w = (reg13.w & iclk1) | (reg14.x & clock1);
            reg14.x = (reg14.x & iclk1) | (reg14.y & clock1);
            reg14.y = (reg14.y & iclk1) | (reg14.z & clock1);
            reg14.z = (reg14.z & iclk1) | (reg14.w & clock1);
            reg14.w = (reg14.w & iclk1) | (reg15.x & clock1);
            reg15.x = (reg15.x & iclk1) | (reg15.y & clock1);
            reg15.y = (reg15.y & iclk1) | (reg15.z & clock1);
            reg15.z = (reg15.z & iclk1) | (reg15.w & clock1);
            reg15.w = (reg15.w & iclk1) | (reg16.x & clock1);
            reg16.x = (reg16.x & iclk1) | (reg16.y & clock1);
            reg16.y = (reg16.y & iclk1) | (reg16.z & clock1);
            reg16.z = (reg16.z & iclk1) | (reg16.w & clock1);
            reg16.w = (reg16.w & iclk1) | fb1;
            out1.y   = (reg1.x ^ reg6.w ^ reg12.y);

            cr1r2 = ~(reg14.w ^ reg9.z);
            cr1r3 = ~(reg14.w ^ reg4.x);
            cr2r3 = ~(reg9.z  ^ reg4.x);
            clock1 = cr1r2 | cr1r3;
            clock2 = cr1r2 | cr2r3;
            clock3 = cr1r3 | cr2r3;
            fb1 = (reg12.y ^ reg12.z ^ reg12.w ^ reg13.z) & clock1;
            fb2 = (reg6.w ^ reg7.x) & clock2;
            fb3 = (reg1.x ^ reg1.y ^ reg1.z ^ reg4.w) & clock3;
            iclk1 = ~clock1;
            iclk2 = ~clock2;
            iclk3 = ~clock3;
            reg1.x = (reg1.x & iclk3) | (reg1.y & clock3);
            reg1.y = (reg1.y & iclk3) | (reg1.z & clock3);
            reg1.z = (reg1.z & iclk3) | (reg1.w & clock3);
            reg1.w = (reg1.w & iclk3) | (reg2.x & clock3);
            reg2.x = (reg2.x & iclk3) | (reg2.y & clock3);
            reg2.y = (reg2.y & iclk3) | (reg2.z & clock3);
            reg2.z = (reg2.z & iclk3) | (reg2.w & clock3);
            reg2.w = (reg2.w & iclk3) | (reg3.x & clock3);
            reg3.x = (reg3.x & iclk3) | (reg3.y & clock3);
            reg3.y = (reg3.y & iclk3) | (reg3.z & clock3);
            reg3.z = (reg3.z & iclk3) | (reg3.w & clock3);
            reg3.w = (reg3.w & iclk3) | (reg4.x & clock3);
            reg4.x = (reg4.x & iclk3) | (reg4.y & clock3);
            reg4.y = (reg4.y & iclk3) | (reg4.z & clock3);
            reg4.z = (reg4.z & iclk3) | (reg4.w & clock3);
            reg4.w = (reg4.w & iclk3) | (reg5.x & clock3);
            reg5.x = (reg5.x & iclk3) | (reg5.y & clock3);
            reg5.y = (reg5.y & iclk3) | (reg5.z & clock3);
            reg5.z = (reg5.z & iclk3) | (reg5.w & clock3);
            reg5.w = (reg5.w & iclk3) | (reg6.x & clock3);
            reg6.x = (reg6.x & iclk3) | (reg6.y & clock3);
            reg6.y = (reg6.y & iclk3) | (reg6.z & clock3);
            reg6.z = (reg6.z & iclk3) | fb3;
            reg6.w = (reg6.w & iclk2) | (reg7.x & clock2);;
            reg7.x = (reg7.x & iclk2) | (reg7.y & clock2);
            reg7.y = (reg7.y & iclk2) | (reg7.z & clock2);
            reg7.z = (reg7.z & iclk2) | (reg7.w & clock2);
            reg7.w = (reg7.w & iclk2) | (reg8.x & clock2);
            reg8.x = (reg8.x & iclk2) | (reg8.y & clock2);
            reg8.y = (reg8.y & iclk2) | (reg8.z & clock2);
            reg8.z = (reg8.z & iclk2) | (reg8.w & clock2);
            reg8.w = (reg8.w & iclk2) | (reg9.x & clock2);
            reg9.x = (reg9.x & iclk2) | (reg9.y & clock2);
            reg9.y = (reg9.y & iclk2) | (reg9.z & clock2);
            reg9.z = (reg9.z & iclk2) | (reg9.w & clock2);
            reg9.w = (reg9.w & iclk2) | (reg10.x & clock2);
            reg10.x = (reg10.x & iclk2) | (reg10.y & clock2);
            reg10.y = (reg10.y & iclk2) | (reg10.z & clock2);
            reg10.z = (reg10.z & iclk2) | (reg10.w & clock2);
            reg10.w = (reg10.w & iclk2) | (reg11.x & clock2);
            reg11.x = (reg11.x & iclk2) | (reg11.y & clock2);
            reg11.y = (reg11.y & iclk2) | (reg11.z & clock2);
            reg11.z = (reg11.z & iclk2) | (reg11.w & clock2);
            reg11.w = (reg11.w & iclk2) | (reg12.x & clock2);
            reg12.x = (reg12.x & iclk2) | fb2;
            reg12.y = (reg12.y & iclk1) | (reg12.z & clock1);
            reg12.z = (reg12.z & iclk1) | (reg12.w & clock1);
            reg12.w = (reg12.w & iclk1) | (reg13.x & clock1);
            reg13.x = (reg13.x & iclk1) | (reg13.y & clock1);
            reg13.y = (reg13.y & iclk1) | (reg13.z & clock1);
            reg13.z = (reg13.z & iclk1) | (reg13.w & clock1);
            reg13.w = (reg13.w & iclk1) | (reg14.x & clock1);
            reg14.x = (reg14.x & iclk1) | (reg14.y & clock1);
            reg14.y = (reg14.y & iclk1) | (reg14.z & clock1);
            reg14.z = (reg14.z & iclk1) | (reg14.w & clock1);
            reg14.w = (reg14.w & iclk1) | (reg15.x & clock1);
            reg15.x = (reg15.x & iclk1) | (reg15.y & clock1);
            reg15.y = (reg15.y & iclk1) | (reg15.z & clock1);
            reg15.z = (reg15.z & iclk1) | (reg15.w & clock1);
            reg15.w = (reg15.w & iclk1) | (reg16.x & clock1);
            reg16.x = (reg16.x & iclk1) | (reg16.y & clock1);
            reg16.y = (reg16.y & iclk1) | (reg16.z & clock1);
            reg16.z = (reg16.z & iclk1) | (reg16.w & clock1);
            reg16.w = (reg16.w & iclk1) | fb1;
            out1.z   = (reg1.x ^ reg6.w ^ reg12.y);

            cr1r2 = ~(reg14.w ^ reg9.z);
            cr1r3 = ~(reg14.w ^ reg4.x);
            cr2r3 = ~(reg9.z  ^ reg4.x);
            clock1 = cr1r2 | cr1r3;
            clock2 = cr1r2 | cr2r3;
            clock3 = cr1r3 | cr2r3;
            fb1 = (reg12.y ^ reg12.z ^ reg12.w ^ reg13.z) & clock1;
            fb2 = (reg6.w ^ reg7.x) & clock2;
            fb3 = (reg1.x ^ reg1.y ^ reg1.z ^ reg4.w) & clock3;
            iclk1 = ~clock1;
            iclk2 = ~clock2;
            iclk3 = ~clock3;
            reg1.x = (reg1.x & iclk3) | (reg1.y & clock3);
            reg1.y = (reg1.y & iclk3) | (reg1.z & clock3);
            reg1.z = (reg1.z & iclk3) | (reg1.w & clock3);
            reg1.w = (reg1.w & iclk3) | (reg2.x & clock3);
            reg2.x = (reg2.x & iclk3) | (reg2.y & clock3);
            reg2.y = (reg2.y & iclk3) | (reg2.z & clock3);
            reg2.z = (reg2.z & iclk3) | (reg2.w & clock3);
            reg2.w = (reg2.w & iclk3) | (reg3.x & clock3);
            reg3.x = (reg3.x & iclk3) | (reg3.y & clock3);
            reg3.y = (reg3.y & iclk3) | (reg3.z & clock3);
            reg3.z = (reg3.z & iclk3) | (reg3.w & clock3);
            reg3.w = (reg3.w & iclk3) | (reg4.x & clock3);
            reg4.x = (reg4.x & iclk3) | (reg4.y & clock3);
            reg4.y = (reg4.y & iclk3) | (reg4.z & clock3);
            reg4.z = (reg4.z & iclk3) | (reg4.w & clock3);
            reg4.w = (reg4.w & iclk3) | (reg5.x & clock3);
            reg5.x = (reg5.x & iclk3) | (reg5.y & clock3);
            reg5.y = (reg5.y & iclk3) | (reg5.z & clock3);
            reg5.z = (reg5.z & iclk3) | (reg5.w & clock3);
            reg5.w = (reg5.w & iclk3) | (reg6.x & clock3);
            reg6.x = (reg6.x & iclk3) | (reg6.y & clock3);
            reg6.y = (reg6.y & iclk3) | (reg6.z & clock3);
            reg6.z = (reg6.z & iclk3) | fb3;
            reg6.w = (reg6.w & iclk2) | (reg7.x & clock2);;
            reg7.x = (reg7.x & iclk2) | (reg7.y & clock2);
            reg7.y = (reg7.y & iclk2) | (reg7.z & clock2);
            reg7.z = (reg7.z & iclk2) | (reg7.w & clock2);
            reg7.w = (reg7.w & iclk2) | (reg8.x & clock2);
            reg8.x = (reg8.x & iclk2) | (reg8.y & clock2);
            reg8.y = (reg8.y & iclk2) | (reg8.z & clock2);
            reg8.z = (reg8.z & iclk2) | (reg8.w & clock2);
            reg8.w = (reg8.w & iclk2) | (reg9.x & clock2);
            reg9.x = (reg9.x & iclk2) | (reg9.y & clock2);
            reg9.y = (reg9.y & iclk2) | (reg9.z & clock2);
            reg9.z = (reg9.z & iclk2) | (reg9.w & clock2);
            reg9.w = (reg9.w & iclk2) | (reg10.x & clock2);
            reg10.x = (reg10.x & iclk2) | (reg10.y & clock2);
            reg10.y = (reg10.y & iclk2) | (reg10.z & clock2);
            reg10.z = (reg10.z & iclk2) | (reg10.w & clock2);
            reg10.w = (reg10.w & iclk2) | (reg11.x & clock2);
            reg11.x = (reg11.x & iclk2) | (reg11.y & clock2);
            reg11.y = (reg11.y & iclk2) | (reg11.z & clock2);
            reg11.z = (reg11.z & iclk2) | (reg11.w & clock2);
            reg11.w = (reg11.w & iclk2) | (reg12.x & clock2);
            reg12.x = (reg12.x & iclk2) | fb2;
            reg12.y = (reg12.y & iclk1) | (reg12.z & clock1);
            reg12.z = (reg12.z & iclk1) | (reg12.w & clock1);
            reg12.w = (reg12.w & iclk1) | (reg13.x & clock1);
            reg13.x = (reg13.x & iclk1) | (reg13.y & clock1);
            reg13.y = (reg13.y & iclk1) | (reg13.z & clock1);
            reg13.z = (reg13.z & iclk1) | (reg13.w & clock1);
            reg13.w = (reg13.w & iclk1) | (reg14.x & clock1);
            reg14.x = (reg14.x & iclk1) | (reg14.y & clock1);
            reg14.y = (reg14.y & iclk1) | (reg14.z & clock1);
            reg14.z = (reg14.z & iclk1) | (reg14.w & clock1);
            reg14.w = (reg14.w & iclk1) | (reg15.x & clock1);
            reg15.x = (reg15.x & iclk1) | (reg15.y & clock1);
            reg15.y = (reg15.y & iclk1) | (reg15.z & clock1);
            reg15.z = (reg15.z & iclk1) | (reg15.w & clock1);
            reg15.w = (reg15.w & iclk1) | (reg16.x & clock1);
            reg16.x = (reg16.x & iclk1) | (reg16.y & clock1);
            reg16.y = (reg16.y & iclk1) | (reg16.z & clock1);
            reg16.z = (reg16.z & iclk1) | (reg16.w & clock1);
            reg16.w = (reg16.w & iclk1) | fb1;
            out1.w   = (reg1.x ^ reg6.w ^ reg12.y);

r[4*j+0] = out1.x;
r[4*j+1] = out1.y;
r[4*j+2] = out1.z;
r[4*j+3] = out1.w;


        }
	


if(res != 0xFFFFFFFFFFFFFFFFULL) {

        out1 = reg1 ^ fin1;
        out2 = reg2 ^ fin2;
        out3 = reg3 ^ fin3;
        out4 = reg4 ^ fin4;
        out5 = reg5 ^ fin5;
        out6 = reg6 ^ fin6;
        out7 = reg7 ^ fin7;
        out8 = reg8 ^ fin8;
        out9 = reg9 ^ fin9;
        out10 = reg10 ^ fin10;
        out11 = reg11 ^ fin11;
        out12 = reg12 ^ fin12;
        out13 = reg13 ^ fin13;
        out14 = reg14 ^ fin14;
        out15 = reg15 ^ fin15;
        out16 = reg16 ^ fin16;

}



		/* increment color here - now it is done in the parent python script
       it would probably require excessive branching, so it won't work on GPGPU */

		/* run 100 dummy clockings to collapse the keyspace + 64 keystream clockings */
		/* though, the cipher spits out the bit *before* the clocking, so 100 rounds are
		   effectively only 99 and hence only 99 + 64 = 163 rounds here */
		for(s=0; s<0; s++) {

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

