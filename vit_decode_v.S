

 .globl  v27_update_vasm

#void v27_update(v27_t *v,
#  a0 unsigned int decision dw[2]
#  a1 const unsigned char *syms,
#  a2 int nbits
#  a3 unsigned char  *poly,
#  a4 PK

v27_update_vasm:


      beqz	a2,loop_exit

#preload poly_c[]
 	  li a5, 32
      vsetvli x0, a5, e8,m2 #
      vle8.v v2, (a3)       # v->poly->c0[0..31] 32 const
      addi  a3,a3,32
      vle8.v v4, (a3)       # v->poly->c1[0..31] 32 const

#  init metrics here
      srai t0,a4,1 # PK/2
      vsetvli x0, a5, e16,m4 #
      vmv.v.x v16, t0 #8t m0 = v->old_metrics[i] ==> 19-16
      vmv.v.x v20, t0 #8t m1 = v->old_metrics[i+32] ==> 23-20
      li t0, 0
      vmv.s.x v16, t0

      add   t1,a2,a2
      add	t5,t1,a1  # last symb

      li   a5, 32
      slli a4,a4,1   # PK*2
      sub  t6,t0,a4  #-PK*2

.balign 8
rep_i:
      lbu	a4,0(a1) #  sym0 = *syms++;
      lbu	a2,1(a1) #  sym1 = *syms++;
      addi	   a1,a1,2

      vsetvli x0, a5, e8,m2 #  32 SEW8
      vxor.vx  v6,v2,a4
      vxor.vx  v8,v4,a2
      vadd.vv  v10,v6,v8  #   metric
      vadd.vx  v8,v10,t6  # -(PK-metric)

      vwadd.wv  v24,v16,v10 # m0
      vwsub.wv  v28,v20,v8  # m1
      vwsub.wv  v16,v16,v8  # m0 -= -(PK - metric);
      vwadd.wv  v20,v20,v10 # m1 +=  metric;

      vsetvli  x0, a5, e16,m4 # 32 SEW16
      vmslt.vv v0,v28,v24   #  decision = (signed int)(m0-m1) > 0;
      vmin.vv  v28,v28,v24  # v->new_metrics[2*i] = decision ? m1 : m0;
      vmslt.vv v1,v20,v16   #  decision = (signed int)(m0-m1) > 0;
      vmin.vv  v24,v20,v16  # v->new_metrics[2*i+1] = decision ? m1 : m0;
# new metrics with mixed words
# VLEN >= 128 bits
      li t2,16
      vsetvli  x0, t2, e16,m4
 	  vslidedown.vx v8,  v28, t2
 	  vslidedown.vx v12, v24, t2
      vsetvli  x0, t2, e16, m2
      vwaddu.vv  v16,  v28, v24
      vwaddu.vv  v20,  v8,  v12
      li  t0,-1
      vwmaccu.vx v16, t0, v24  # vd[i] = +(x[rs1] * vs2[i]) + vd[i]
      vwmaccu.vx v20, t0, v12
# VLEN 128 bits only!!!
#     vwaddu.vv  v16,  v28, v24
#     li  t0,-1
#     vwmaccu.vx v16, t0, v24  # vd[i] = +(x[rs1] * vs2[i]) + vd[i]
#end mix
      vmv.x.s  t0, v28 #  if(v->new_metrics[0] > (1<<30)) {

      vsetivli x0, 1, e32,m1
      vse32.v  v0,(a0)
      addi     a0,a0,4
      vse32.v  v1,(a0)
      addi     a0,a0,4

      beq	a1,t5,loop_exit	 #  while(nbits--) {

      slli   t0,t0,16
      lui	 t1,0x40000
      bge 	 t1,t0,rep_i  #  if(v->new_metrics[0] > (1<<30)) {

# here an error if overflow
scale_err:
     j	scale_err


     j	rep_i


loop_exit:
     ret

/*
      li t2,16
      vsetvli  x0, t2, e16,m4
 	  vslidedown.vx v8,  v28, t2
 	  vslidedown.vx v12, v24, t2
      vsetvli  x0, t2, e32, m4 # 16 SEW32
      vzext.vf2  v20,  v8     # 8
      vzext.vf2  v16,  v28    # 8
      li  t0,0x8000
      vsetvli  x0, t2, e16, m2 # 16 SEW16
      vwmaccus.vx v16, t0, v24  # vd[i] = +(x[rs1] * vs2[i]) + vd[i]
      vwmaccus.vx v16, t0, v24
      vwmaccus.vx v20, t0, v12
      vwmaccus.vx v20, t0, v12
#  4*2+1+4*2+1+4*4 = 34
# VLEN 128 bits only!!!
#      vsetvli  x0, a5, e32, m8 # 32 SEW32
#      vzext.vf2  v16,  v28     #8
#      li  t0,0x8000
#      vsetvli  x0, a5, e16, m4 # 32 SEW16
#      vwmaccus.vx v16, t0, v24  # vd[i] = +(x[rs1] * vs2[i]) + vd[i]
#      vwmaccus.vx v16, t0, v24  #
#   8+1+8*2 = 25
*/

