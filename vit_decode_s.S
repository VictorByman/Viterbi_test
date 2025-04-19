.globl  v27_update_sasm

#void v27_update(v27_t *v, const unsigned char *syms, int nbits)
#                  a0,                          a1,        a2

v27_update_sasm:


#  while(nbits--) {
      beqz	a2,abn_exit
      addi	sp,sp,-44

      sw	s0,12(sp)
      sw	s1,8(sp)
      sw	s2,4(sp)
      sw	s3,16(sp)
      sw	s4,20(sp)
      sw	s5,24(sp)
      sw	s6,28(sp)
      sw	s7,32(sp)
      sw	s8,36(sp)
      sw	s9,40(sp)

#  v27_decision_t *d = &v->decisions[v->decisions_index];
      lw	t0,524(a0)
      lw	a7,520(a0)
      lw	a5,528(a0)
      lw	a6,512(a0)
 	  lw	a3,516(a0)

      sh1add	t5,a2,a1  # last symb

      j	jump_lab1


rep_z:
      lw	a5,528(a0)
      lw	a4,532(a0)
#  v->decisions_index = 0;

#    /* Swap pointers to old and new metrics */
#    tmp = v->old_metrics;
#    v->old_metrics = v->new_metrics;

      sw	a3,512(a0)

#  i f(++v->decisions_index >= v->decisions_count)
      addi	a5,a5,1

#  v->decisions_index = 0;
      sltu	a4,a5,a4
      neg	a4,a4
      and	a5,a5,a4
      sw	a5,528(a0)
#  v->new_metrics = tmp;
      sw	a6,516(a0)


      mv	a4,a3
      mv	a3,a6
      beq	a1,t5,loop_exit	 #  while(nbits--) {
      mv	a6,a4



jump_lab1:
      sh3add	a5,a5,t0 #  v27_decision_t *d = &v->decisions[v->decisions_index];

      li    s6,0 #  d->w[0] = d->w[1] = 0;
      li    s7,0

      lbu	a4,0(a1) #  sym0 = *syms++;
      li    s5,32 # 0-31 btfly

      lbu	a2,1(a1) #  sym1 = *syms++;
      addi	a1,a1,2	 #  sym1 = *syms++;

      li    s3,0 # <<  (i)&31

rep_btf:

# a7 v->poly->c0
# a7 v->poly->c1
# a6 v->old_metrics
# a3 v->new_metrics
# a5 d->w  &v->decisions[v->decisions_index]

      lbu	t1,0(a7)   # v->poly->c0[i]
      lbu	s0,32(a7)  # v->poly->c1[i]
      lw	t2,128(a6) # m1 = v->old_metrics[i+32]
      lw	t3,0(a6)   # m0 = v->old_metrics[i]

.align 3  // align next code to 8 bytes
rep_btf_i:
      xor	s0,s0,a2   # (v->poly->c1[i] ^ sym1)
      xor	t1,a4,t1   # (v->poly->c0[i] ^ sym0)

      add	t1,t1,s0   #   metric
      addi  s5,s5,-1   # can be delete!!!

      add   t4,t1,-510 # -(510-metric)
      addi	a7,a7,1	   # v->poly->c_[i] next state

      add	s9,t3,t1   # m0 = v->old_metrics[i] + metric;
      sub	s8,t2,t4   # m1 = v->old_metrics[i+32] + (510 - metric);

      sub	t3,t3,t4   # m0 += (510-metric);
      slt   s2,s8,s9   # decision = (signed int)(m0-m1) > 0;

      add	t1,t1,t2   # m1 +=  metric;
      sll 	s2,s2,s3   # decision << ((2*i)&31) 0,2,4,,

      lbu	s0,32(a7)  # v->poly->c1[i]
      or    s6,s6,s2   # d->w[0] |= decision << ((2*i)&31);

      min   s1,s8,s9   # v->new_metrics[2*i] = decision ? m1 : m0;

      sw	s1,0(a3)   # v->new_metrics[2*i] = decision ? m1 : m0;
      slt   t2,t1,t3   # decision = (signed int)(m0-m1) > 0;

      min  t3,t1,t3   #  v->new_metrics[2*i+1] = decision ? m1 : m0;

      lbu  t1,0(a7)   # v->poly->c0[i]
      addi	a6,a6,4   # v->old_metrics[i]

      sw	t3,4(a3)      # v->new_metrics[2*i+1] = decision ? m1 : m0;
      sll 	t3,t2,s3  # << (2*i+1) 1,3,5,

      lw	t2,128(a6)    # m1 = v->old_metrics[i+32]
      or    s7,s7,t3 # d->w[0] |= decision << ((2*i+1)&31);

	  lw	t3,0(a6)   # m0 = v->old_metrics[i]
	  addi	a3,a3,8   # v->new_metrics[2*i]

      addi  s3,s3,1
      bnez	s5,rep_btf_i

      sw	s6,0(a5)  # d->w[0] =  low 32 bits
      sw	s7,4(a5)  # d->w[1] =  hig 32 bits


	  addi  a7,a7,-32
	  addi	a6,a6,-128
	  addi	a3,a3,-256

#  i f(v->new_metrics[0] > (1<<30)) {
     lw	    a5,0(a3)
     lui	t6,0x40000
     bgeu	t6,a5,rep_z

     mv	a5,a3
     addi	t1,a3,256
     mv	a4,a3
#  unsigned int minmetric = 1UL<<31;
     lui	a2,0x80000
#   i f(v->new_metrics[i] < minmetric)
lab_16b0:
     lw	t3,0(a4)
#  for(i=0; i<64; i++) {
     addi	a4,a4,4
#  i f(v->new_metrics[i] < minmetric)
     minu	a2,a2,t3
#   for(i=0; i<64; i++) {
     bne 	a4,t1,lab_16b0


#   v->new_metrics[i] -= minmetric;
lab_16be:
        lw	a4,0(a5)
#  for(i=0; i<64; i++)
     addi	a5,a5,4
#  v->new_metrics[i] -= minmetric;
     sub	a4,a4,a2
     sw	a4,-4(a5)
#  for(i=0; i<64; i++)
     beq	a5,t1,rep_z
#  v->new_metrics[i] -= minmetric;
     lw	a4,0(a5)
#  for(i=0; i<64; i++)
     addi	a5,a5,4
#  v->new_metrics[i] -= minmetric;
     sub	a4,a4,a2
     sw	a4,-4(a5)
#  for(i=0; i<64; i++)
     bne	a5,t1,lab_16be
     j	rep_z


loop_exit:
     lw	s0,12(sp)
     lw	s1,8(sp)
     lw	s2,4(sp)
     lw	s3,16(sp)
     lw	s4,20(sp)
     lw	s5,24(sp)
     lw	s6,28(sp)
     lw	s7,32(sp)
     lw	s8,36(sp)
     lw	s9,40(sp)
     addi	sp,sp,44
     ret
abn_exit:
     ret
