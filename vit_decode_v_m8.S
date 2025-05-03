#
# version for 8 bits metric size
#

 .globl  v27_update_vasm_m8

#void v27_update(v27_t *v,
#  a0 unsigned int decision dw[2]
#  a1 const unsigned char *syms,
#  a2 int nbits
#  a3 unsigned char  *poly,
#  a4 PK

v27_update_vasm_m8:


      beqz	a2,loop_exit

#preload poly_c[]
 	  li a5, 32
      vsetvli x0, a5, e8,m2 #
      vle8.v v2, (a3)       # v->poly->c0[0..31] 32 const
      addi  a3,a3,32
      vle8.v v4, (a3)       # v->poly->c1[0..31] 32 const

#  init metrics here
      srai t0,a4,1 # PK/2
      vsetvli x0, a5, e8,m2 #
      vmv.v.x v16, t0 #  m0 = v->old_metrics[i] ==> 19-16
      vmv.v.x v18, t0 #  m1 = v->old_metrics[i+32] ==> 23-20
      li t0, 0
      vmv.s.x v16, t0

      add   t1,a2,a2
      add	t5,t1,a1  # last symb

      li   a5, 32
      slli a4,a4,1  # PK*2
      addi t6,a4,0  # PK*2

.balign 8
rep_i:
      lbu	a4,0(a1) #  sym0 = *syms++;
      lbu	a2,1(a1) #  sym1 = *syms++;
      addi	   a1,a1,2

      vsetvli x0, a5, e8,m2 #  32 SEW8
      vxor.vx  v6,v2,a4
      vxor.vx  v8,v4,a2
      vadd.vv  v10,v6,v8  #   metric
      vrsub.vx  v8,v10,t6 #  (PK-metric)

      vadd.vv  v24,v16,v10 # m0
      vadd.vv  v28,v18,v8  # m1
      vadd.vv  v16,v16,v8  # m0 += (PK - metric);
      vadd.vv  v18,v18,v10 # m1 +=  metric;

      vmsltu.vv v0,v28,v24   #  decision = (signed int)(m0-m1) > 0;
      vminu.vv  v28,v28,v24  # v->new_metrics[2*i] = decision ? m1 : m0;
      vmsltu.vv v1,v18,v16   #  decision = (signed int)(m0-m1) > 0;
      vminu.vv  v24,v18,v16  # v->new_metrics[2*i+1] = decision ? m1 : m0;
# new metrics with mixed words
# VLEN >= 128 bits
      li t2,16
      vsetvli  x0, t2, e8,m2
 	  vslidedown.vx v8,  v28, t2
 	  vslidedown.vx v12, v24, t2
      vsetvli  x0, t2, e8, m1
      vwaddu.vv  v16,  v28, v24
      vwaddu.vv  v18,  v8,  v12
      li  t0,-1
      vwmaccu.vx v16, t0, v24  # vd[i] = +(x[rs1] * vs2[i]) + vd[i]
      vwmaccu.vx v18, t0, v12
# VLEN 128 bits only!!!16.17,18.19
#       vwaddu.vv  v16,  v28, v24
#       li  t0,-1
#       vwmaccu.vx v16, t0, v24  # vd[i] = +(x[rs1] * vs2[i]) + vd[i]
#end mix
      vmv.x.s  t0, v28 #  if(v->new_metrics[0] > (1<<30)) {

      vsetivli x0, 1, e32,m1
      vse32.v  v0,(a0)
      addi     a0,a0,4
      vse32.v  v1,(a0)
      addi     a0,a0,4

      beq	a1,t5,loop_exit	 #  while(nbits--) {

      li	 t1,0x40
      bge 	 t1,t0,rep_i  #  if(v->new_metrics[0] > (1<<6)) {

# new metrics = metrics - minimum
# scale_metric:
      vsetvli  x0, a5, e8,m2
      vmv.s.x  v28,t1
      vredminu.vs v28, v16, v28
      vredminu.vs v28, v18, v28
      vmv.x.s  t1,v28
      vsub.vx  v16,v16,t1
      vsub.vx  v18,v18,t1
       j	rep_i


loop_exit:
     ret

