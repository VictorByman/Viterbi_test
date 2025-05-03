/* User include file for libfec
 * Copyright 2004, Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */

#ifndef _FEC_H_
#define _FEC_H_

//#define _C_VERSION_

#ifdef _C_VERSION_
#else
// assembler : scalar or vector
#define _RVV_ON_
#endif


#ifdef _RVV_ON_
#define  PK    7
#define metric_size unsigned short
#else
#define  PK    255
#define metric_size unsigned int
#endif

/* r=1/2 k=7 convolutional encoder polynomials
 */
#define V27POLYA  0x4f
#define V27POLYB  0x6d

typedef struct {
  unsigned char c0[32];
  unsigned char c1[32];
} v27_poly_t;

typedef struct {
  unsigned int w[2];
} v27_decision_t;

/* State info for instance of r=1/2 k=7 Viterbi decoder
 */

typedef struct {
	metric_size metrics1[64];      /* Path metric buffer 1 */
	metric_size metrics2[64];      /* Path metric buffer 2 */
  /* Pointers to path metrics, swapped on every bit */
	metric_size *old_metrics, *new_metrics;
  const v27_poly_t *poly;         /* Polynomial to use */
  v27_decision_t *decisions;      /* Beginning of decisions for block */
  unsigned int decisions_index;   /* Index of current decision */
  unsigned int decisions_count;   /* Number of decisions in history */
} v27_t;


void v27_poly_init(v27_poly_t *poly, const signed char polynomial[2]);
void v27_init(v27_t *v, v27_decision_t *decisions, unsigned int decisions_count,
              const v27_poly_t *poly, unsigned char initial_state);
void v27_update_C(v27_t *v, const unsigned char *syms, int nbits);

void v27m_chainback_fixed(v27_t *v, unsigned char *data, unsigned int nbits,
                         unsigned char final_state);

const v27_poly_t* v27_get_poly(void);

void v27_update_sasm(v27_t *v, const unsigned char *syms, int nbits);
void v27_poly4F6D_init(v27_poly_t *p);

void v27_update_vasm(unsigned int *dw, const unsigned char *syms, int nbits,
		unsigned char *poly, int k);

void v27_update_vasm_m8(unsigned int *dw, const unsigned char *syms, int nbits,
		unsigned char *poly, int k);

#endif
