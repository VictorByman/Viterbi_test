/* K=7 r=1/2 Viterbi decoder in portable C
 * Copyright Feb 2004, Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */

#include <stdlib.h>

#include "viterbi_rv.h"


void v27_poly4F6D_init(v27_poly_t *p)
{
    int state;
    unsigned char x;

    for(state = 0; state < 32; state++) {
        p->c0[state] =  ((0x96696996 >> (state & (V27POLYA/2)))&1) ? PK : 0;
        p->c1[state] =  ((0x96696996 >> (state & (V27POLYB/2)))&1) ? PK : 0;
    }
}


/*  Initialize a v27_t struct for Viterbi decoding.
 */
void v27_init(v27_t *v, v27_decision_t *decisions, unsigned int decisions_count,
              const v27_poly_t *poly, unsigned char initial_state)
{
    int i;

    v->old_metrics = v->metrics1;
    v->new_metrics = v->metrics2;
    v->poly = poly;
    v->decisions = decisions;
    v->decisions_index = 0;
    v->decisions_count = decisions_count;

#ifdef _RVV_ON_
#else
    for(i = 0; i < 64; i++) {
        v->old_metrics[i] = PK>>2;
    }

    v->old_metrics[initial_state & 63] = 0; /* Bias known start state */
#endif
}

/* C-language butterfly */
#define BFLY(i) {\
    unsigned int metric,m0,m1,decision;\
    metric = (v->poly->c0[i] ^ sym0) + (v->poly->c1[i] ^ sym1);\
    m0 = v->old_metrics[i] + metric;\
    m1 = v->old_metrics[i+32] + (PK*2 - metric);\
    decision = (signed int)(m0-m1) > 0;\
    v->new_metrics[2*i] = decision ? m1 : m0;\
    wl |= decision << i;\
    m0 -= (metric+metric-PK*2);\
    m1 += (metric+metric-PK*2);\
    decision = (signed int)(m0-m1) > 0;\
    v->new_metrics[2*i+1] = decision ? m1 : m0;\
    wh |= decision << i;\
    }

/*  Update a v27_t decoder with a block of symbols.
 */
void v27_update_C(v27_t *v, const unsigned char *syms, int nbits)
{
    unsigned char sym0, sym1;
    metric_size *tmp;
    int normalize = 0;

    while(nbits--) {
        unsigned int wl = 0;
        unsigned int wh = 0;
        sym0 = *syms++;
        sym1 = *syms++;

        BFLY(0);
        BFLY(1);
        BFLY(2);
        BFLY(3);
        BFLY(4);
        BFLY(5);
        BFLY(6);
        BFLY(7);
        BFLY(8);
        BFLY(9);
        BFLY(10);
        BFLY(11);
        BFLY(12);
        BFLY(13);
        BFLY(14);
        BFLY(15);
        BFLY(16);
        BFLY(17);
        BFLY(18);
        BFLY(19);
        BFLY(20);
        BFLY(21);
        BFLY(22);
        BFLY(23);
        BFLY(24);
        BFLY(25);
        BFLY(26);
        BFLY(27);
        BFLY(28);
        BFLY(29);
        BFLY(30);
        BFLY(31);

        v27_decision_t *d = &v->decisions[v->decisions_index];
        d->w[0] = wl;
        d->w[1] = wh;

        /* Normalize metrics if they are nearing overflow */
        if(v->new_metrics[0] > (1<<30)) {
            int i;
            unsigned int minmetric = 1UL<<31;

            for(i=0; i<64; i++) {
                if(v->new_metrics[i] < minmetric)
                    minmetric = v->new_metrics[i];
            }

            for(i=0; i<64; i++)
                v->new_metrics[i] -= minmetric;

            normalize += minmetric;
        }

        /* Advance decision index */
        if(++v->decisions_index >= v->decisions_count)
            v->decisions_index = 0;

        /* Swap pointers to old and new metrics */
        tmp = v->old_metrics;
        v->old_metrics = v->new_metrics;
        v->new_metrics = tmp;
    }
}

/*  Retrieve the most likely output bit sequence with known final state from
    a v27_t decoder.
 */

void v27m_chainback_fixed(v27_t *v, unsigned char *data, unsigned int nbits,
                          unsigned char final_state)
{
    int k;
    unsigned int decisions_index = v->decisions_index;
    unsigned int t0,t1,shv;

    v27_decision_t *d;
    final_state %= 64;
    final_state <<= 2;

    while(nbits-- != 0) {

        decisions_index = (decisions_index == 0) ?
                    v->decisions_count-1 : decisions_index-1;

        d = &v->decisions[decisions_index];

        t0 = d->w[0];
        t1 = d->w[1];
        if((final_state>>2)/32) { // top half
            t0 >>= 16;
            t1 >>= 16;
        }
        shv = (final_state>>2)%32;
        if(shv&1) { // odd
            shv >>= 1;
            k = (t1>>shv) & 1;
        }
        else {
            shv >>= 1;
            k = (t0>>shv) & 1;
        }

        data[nbits>>3] = final_state = (final_state >> 1) | (k << 7);
    }
}


/*
 Provides a singleton polynomial object.
 */
const v27_poly_t* v27_get_poly(void)
{
    static v27_poly_t instance;
    static int initialized = 0;

    if (!initialized) {
        v27_poly4F6D_init(&instance);
        initialized = 1;
    }
    return &instance;
}


