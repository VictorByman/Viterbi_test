
void error(); // use own error function

#include "viterbi_rv.h"

#define uint32_t  unsigned int
#define uint8_t   unsigned char

void print_k(int x) {
    // show value x
    asm volatile("addi tp, %0, 0" ::"r"(x)); // just for simulation only
}


#define read_csr(reg) ({ unsigned long __tmp; \
    asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
    __tmp; })


#define rdtime()  read_csr(0xc00)


#define MAX_FRM_LEN 600
#define MAX_SR_LEN (((MAX_FRM_LEN+31)>>5)+1)


typedef struct
{
    unsigned int PRM;
    unsigned int nPRM;
    unsigned int PRM_len;
    unsigned int PRM_mask;
    unsigned int frm_len;
    unsigned int neg_G2;
    unsigned int T;
} vt_dec_atr;


typedef struct
{
    vt_dec_atr* atr;
    unsigned int* sr;
    unsigned int* frm;
    unsigned char* bits;
    unsigned char* frm_ready;
    unsigned char* bits_ready;
    //    unsigned char clearRq;
    unsigned char clearGlob;
    signed char prmReady;
} vt_dec_strg_type;


volatile unsigned int  data[MAX_FRM_LEN>>(1+2)];

vt_dec_strg_type  vt;

vt_dec_atr vit_a;

unsigned char vtbuf[200];



//******************************************************************************************
// CRC-24Q LookUp Table

const uint32_t crc24q_tbl[] = {
    0x000000,0x864CFB,0x8AD50D,0x0C99F6,0x93E6E1,0x15AA1A,0x1933EC,0x9F7F17,
    0xA18139,0x27CDC2,0x2B5434,0xAD18CF,0x3267D8,0xB42B23,0xB8B2D5,0x3EFE2E,
    0xC54E89,0x430272,0x4F9B84,0xC9D77F,0x56A868,0xD0E493,0xDC7D65,0x5A319E,
    0x64CFB0,0xE2834B,0xEE1ABD,0x685646,0xF72951,0x7165AA,0x7DFC5C,0xFBB0A7,
    0x0CD1E9,0x8A9D12,0x8604E4,0x00481F,0x9F3708,0x197BF3,0x15E205,0x93AEFE,
    0xAD50D0,0x2B1C2B,0x2785DD,0xA1C926,0x3EB631,0xB8FACA,0xB4633C,0x322FC7,
    0xC99F60,0x4FD39B,0x434A6D,0xC50696,0x5A7981,0xDC357A,0xD0AC8C,0x56E077,
    0x681E59,0xEE52A2,0xE2CB54,0x6487AF,0xFBF8B8,0x7DB443,0x712DB5,0xF7614E,
    0x19A3D2,0x9FEF29,0x9376DF,0x153A24,0x8A4533,0x0C09C8,0x00903E,0x86DCC5,
    0xB822EB,0x3E6E10,0x32F7E6,0xB4BB1D,0x2BC40A,0xAD88F1,0xA11107,0x275DFC,
    0xDCED5B,0x5AA1A0,0x563856,0xD074AD,0x4F0BBA,0xC94741,0xC5DEB7,0x43924C,
    0x7D6C62,0xFB2099,0xF7B96F,0x71F594,0xEE8A83,0x68C678,0x645F8E,0xE21375,
    0x15723B,0x933EC0,0x9FA736,0x19EBCD,0x8694DA,0x00D821,0x0C41D7,0x8A0D2C,
    0xB4F302,0x32BFF9,0x3E260F,0xB86AF4,0x2715E3,0xA15918,0xADC0EE,0x2B8C15,
    0xD03CB2,0x567049,0x5AE9BF,0xDCA544,0x43DA53,0xC596A8,0xC90F5E,0x4F43A5,
    0x71BD8B,0xF7F170,0xFB6886,0x7D247D,0xE25B6A,0x641791,0x688E67,0xEEC29C,
    0x3347A4,0xB50B5F,0xB992A9,0x3FDE52,0xA0A145,0x26EDBE,0x2A7448,0xAC38B3,
    0x92C69D,0x148A66,0x181390,0x9E5F6B,0x01207C,0x876C87,0x8BF571,0x0DB98A,
    0xF6092D,0x7045D6,0x7CDC20,0xFA90DB,0x65EFCC,0xE3A337,0xEF3AC1,0x69763A,
    0x578814,0xD1C4EF,0xDD5D19,0x5B11E2,0xC46EF5,0x42220E,0x4EBBF8,0xC8F703,
    0x3F964D,0xB9DAB6,0xB54340,0x330FBB,0xAC70AC,0x2A3C57,0x26A5A1,0xA0E95A,
    0x9E1774,0x185B8F,0x14C279,0x928E82,0x0DF195,0x8BBD6E,0x872498,0x016863,
    0xFAD8C4,0x7C943F,0x700DC9,0xF64132,0x693E25,0xEF72DE,0xE3EB28,0x65A7D3,
    0x5B59FD,0xDD1506,0xD18CF0,0x57C00B,0xC8BF1C,0x4EF3E7,0x426A11,0xC426EA,
    0x2AE476,0xACA88D,0xA0317B,0x267D80,0xB90297,0x3F4E6C,0x33D79A,0xB59B61,
    0x8B654F,0x0D29B4,0x01B042,0x87FCB9,0x1883AE,0x9ECF55,0x9256A3,0x141A58,
    0xEFAAFF,0x69E604,0x657FF2,0xE33309,0x7C4C1E,0xFA00E5,0xF69913,0x70D5E8,
    0x4E2BC6,0xC8673D,0xC4FECB,0x42B230,0xDDCD27,0x5B81DC,0x57182A,0xD154D1,
    0x26359F,0xA07964,0xACE092,0x2AAC69,0xB5D37E,0x339F85,0x3F0673,0xB94A88,
    0x87B4A6,0x01F85D,0x0D61AB,0x8B2D50,0x145247,0x921EBC,0x9E874A,0x18CBB1,
    0xE37B16,0x6537ED,0x69AE1B,0xEFE2E0,0x709DF7,0xF6D10C,0xFA48FA,0x7C0401,
    0x42FA2F,0xC4B6D4,0xC82F22,0x4E63D9,0xD11CCE,0x575035,0x5BC9C3,0xDD8538
};


uint32_t getbitu(const uint8_t* buff, uint32_t pos, uint32_t len)
{
    uint32_t i, bits = 0;
    for (i = pos; i < pos + len; i++) {
        bits = (bits << 1) + ((buff[i/8] >> (7 - i%8)) & 1u);
    }
    return bits;
}

int  crc_check(uint8_t* evn_page, uint8_t* odd_page)
{
    uint32_t i;
    uint32_t crc_c = 0;
    uint32_t crc_r = getbitu(odd_page, 82, 24); /* crc in message */

    // right alignment of data

    evn_page[14] |= evn_page[15] >> 2;
    evn_page[15] = evn_page[15]<<2 | evn_page[16]>>6;
    evn_page[16] = evn_page[16]<<2 | evn_page[17]>>6;
    evn_page[17] = evn_page[17]<<2 | evn_page[18]>>6;
    evn_page[18] = evn_page[18]<<2 | evn_page[19]>>6;
    evn_page[19] = evn_page[19]<<2 | evn_page[20]>>6;
    evn_page[20] = evn_page[20]<<2 | evn_page[21]>>6;
    evn_page[21] = evn_page[21]<<2 | evn_page[22]>>6;
    evn_page[22] = evn_page[22]<<2 | evn_page[23]>>6;
    evn_page[23] = evn_page[23]<<2 | evn_page[24]>>6;
    evn_page[24] = evn_page[24]<<2 | evn_page[25]>>6;


    evn_page[14] = evn_page[14]>>4 | evn_page[13]<<4;
    evn_page[13] = evn_page[13]>>4 | evn_page[12]<<4;
    evn_page[12] = evn_page[12]>>4 | evn_page[11]<<4;
    evn_page[11] = evn_page[11]>>4 | evn_page[10]<<4;
    evn_page[10] = evn_page[10]>>4 | evn_page[9]<<4;
    evn_page[9]  = evn_page[9]>>4  | evn_page[8]<<4;
    evn_page[8]  = evn_page[8]>>4  | evn_page[7]<<4;
    evn_page[7]  = evn_page[7]>>4  | evn_page[6]<<4;
    evn_page[6]  = evn_page[6]>>4  | evn_page[5]<<4;
    evn_page[5]  = evn_page[5]>>4  | evn_page[4]<<4;
    evn_page[4]  = evn_page[4]>>4  | evn_page[3]<<4;
    evn_page[3]  = evn_page[3]>>4  | evn_page[2]<<4;
    evn_page[2]  = evn_page[2]>>4  | evn_page[1]<<4;
    evn_page[1]  = evn_page[1]>>4  | evn_page[0]<<4;
    evn_page[0] >>= 4;

    for(i = 0; i < 25; i++) {
        crc_c=((crc_c<<8)&0xFFFFFF)^crc24q_tbl[(crc_c>>16)^evn_page[i]];
    }

    if (crc_r == crc_c)
    {
        // left alignment of data
        uint8_t* db = &evn_page[0];

        db[0]   = db[0]     << 6 | db[1]    >> 2;
        db[1]   = db[1]     << 6 | db[2]    >> 2;
        db[2]   = db[2]     << 6 | db[3]    >> 2;
        db[3]   = db[3]     << 6 | db[4]    >> 2;
        db[4]   = db[4]     << 6 | db[5]    >> 2;
        db[5]   = db[5]     << 6 | db[6]    >> 2;
        db[6]   = db[6]     << 6 | db[7]    >> 2;
        db[7]   = db[7]     << 6 | db[8]    >> 2;
        db[8]   = db[8]     << 6 | db[9]    >> 2;
        db[9]   = db[9]     << 6 | db[10]   >> 2;
        db[10]  = db[10]    << 6 | db[11]   >> 2;
        db[11]  = db[11]    << 6 | db[12]   >> 2;
        db[12]  = db[12]    << 6 | db[13]   >> 2;
        db[13]  = db[13]    << 6 | db[14]   >> 2;
        // rest of data 16 bits
        db[14] = getbitu(db, 120, 8);
        db[15] = getbitu(db, 128, 8);

        return 1;
    }
    else
        return 0;
}

/*
 *
 *
 *
 * Viterbi test  for RISC-V
 *
 *
 *
 */


int  my_main()
{

    unsigned char initial_state = 0;
    unsigned int decisions_count = MAX_FRM_LEN>>1;
    int bit, i, k;
    volatile unsigned int input_message[8];

    v27_decision_t decisions[MAX_FRM_LEN>>1];
    unsigned char encoded_bits[MAX_FRM_LEN];
    v27_t fec;

    uint8_t *dec;
    dec = (uint8_t *) data;
    vt.bits = vtbuf;

    for(i=0;i<32;i++)  vt.bits[i] = 0;


    for(int kk=0;kk<2;kk++)
    {


        if(kk==0)
        {

            input_message[0] =  0xe6f9f8b0;
            input_message[1] =  0x77c34b39;
            input_message[2] =  0xf70bf11a;
            input_message[3] =  0xd8242225;
            input_message[4] =  0xab318b6e;
            input_message[5] =  0xb3a2935c;
            input_message[6] =  0x5f17fb57;
            input_message[7] =  0x00002e80;

        }


        if(kk==1)
        {
            input_message[0] =  0x5dd67f3c;
            input_message[1] =  0x772f4071;
            input_message[2] =  0x9e594031;
            input_message[3] =  0xc07e5fff;
            input_message[4] =  0x1db3a3fc;
            input_message[5] =  0x3a7cb480;
            input_message[6] =  0xf2a0ebec;
            input_message[7] =  0x0000eef2;
        }


        volatile int mt = rdtime();

        int Nenc = 240;

        bit = Nenc;
        i = 1; k = 0;

#if 0
        while (bit-- > 0)
        {
            encoded_bits[k] = (((input_message[bit>>5] >> (bit&0x1f))^k) & 1)*PK;
            k += 8;
            if (k >=Nenc) k = i++;
        }
#else

        bit--;
        for(k=0;k<Nenc;k+=8) //while (bit-- > 0)
        {
            encoded_bits[k]   = (((input_message[bit>>5] >> (bit&0x1f)) ) & 1)*PK;
            encoded_bits[k+1] = (((input_message[(bit-30)>>5] >> ((bit-30)&0x1f))^1)   & 1)*PK;
            encoded_bits[k+2] = (((input_message[(bit-60)>>5] >> ((bit-60)&0x1f)))     & 1)*PK;
            encoded_bits[k+3] = (((input_message[(bit-90)>>5] >> ((bit-90)&0x1f))^1)   & 1)*PK;
            encoded_bits[k+4] = (((input_message[(bit-120)>>5] >> ((bit-120)&0x1f)))   & 1)*PK;
            encoded_bits[k+5] = (((input_message[(bit-150)>>5] >> ((bit-150)&0x1f))^1) & 1)*PK;
            encoded_bits[k+6] = (((input_message[(bit-180)>>5] >> ((bit-180)&0x1f)))   & 1)*PK;
            encoded_bits[k+7] = (((input_message[(bit-210)>>5] >> ((bit-210)&0x1f))^1) & 1)*PK;
            bit--;
        }
#endif


        Nenc = Nenc >> 1;


        v27_init(&fec,
                 decisions,
                 decisions_count,
                 v27_get_poly(),
                 initial_state);

        volatile int nt = rdtime();

#ifdef _C_VERSION_
        v27_update_C(&fec, encoded_bits, Nenc);
#else

#ifdef _RVV_ON_

        v27_update_vasm ( fec.decisions[0].w,
                encoded_bits,
                Nenc,
                fec.poly->c0,
                PK);

        fec.decisions_count = Nenc;
        fec.decisions_index = Nenc;

#else
        v27_update_sasm(&fec, encoded_bits, Nenc);
#endif

#endif

        print_k(rdtime() - nt); // print cycles

        v27m_chainback_fixed(&fec, dec, Nenc-6, 0);


        if ((dec[0]>>7  & 1) == 1)
        {
            for(int n=0;n<15;n++)  vt.bits[n+15] = dec[n];

            int  vok = crc_check(&vt.bits[0], &vt.bits[15]);


            if (vok)
            {
                print_k(rdtime() - mt); // print cycles

                unsigned int sum = 0;
                for(i=0;i<16;i++) sum += vt.bits[i];

                if(sum!=1652) {
                    print_k(sum);
                    error();
                }

                print_k(5);   //  crc succeed
            }
            else
            {
                print_k(2); //  crc FAILED
            }
        }
        else
        {
            for(int n=0;n<15;n++)  vt.bits[n] = dec[n];

            print_k(rdtime() - mt); // print cycles

        }

    }

}





