#include "cpu/exec.h"
extern void raise_intr(uint8_t NO, vaddr_t ret_addr);
void diff_test_skip_qemu();
void diff_test_skip_nemu();

make_EHelper(lidt) {
  rtl_li(&t0,id_dest->val);
  rtl_li(&cpu.idtr.i_limit,vaddr_read(t0,2));
  rtl_li(&cpu.idtr.i_base,vaddr_read(t0+2,4));
  print_asm_template1(lidt);
}

make_EHelper(mov_r2cr) {
  TODO();

  print_asm("movl %%%s,%%cr%d", reg_name(id_src->reg, 4), id_dest->reg);
}

make_EHelper(mov_cr2r) {
  TODO();

  print_asm("movl %%cr%d,%%%s", id_src->reg, reg_name(id_dest->reg, 4));

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(int) {
  //raise_intr(id_dest->val,cpu.eip);
  //raise_intr(id_dest->addr,cpu.eip);

  rtl_push(&cpu.eflags.val);

  //rtl_li(&t0,1);
  //rtl_set_IF(&t0);

  rtl_push(&cpu.cs);
  rtl_push(&cpu.eip);
  //rtl_push(&ret_addr);
  
  //cpu.cs = ??
  rtl_li(&t0,vaddr_read(cpu.idtr.i_base+4*id_dest->addr,4));
  if((t0 & 0x00008000) == 0)
      assert(0);
  //cpu.eip = ret_addr + (t0 & 0x00001111);
  rtl_addi(&decoding.jmp_eip,&cpu.eip,t0);
  decoding.is_jmp = 1;
  print_asm("int %s", id_dest->str);

#ifdef DIFF_TEST
  diff_test_skip_nemu();
#endif
}

make_EHelper(iret) {
  TODO();

  print_asm("iret");
}

uint32_t pio_read(ioaddr_t, int);
void pio_write(ioaddr_t, int, uint32_t);

make_EHelper(in) {
  t2 = pio_read(id_src->val,id_dest->width);
  operand_write(id_dest,&t2);

  print_asm_template2(in);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(out) {
  pio_write(id_dest->val,id_dest->width,id_src->val);

  print_asm_template2(out);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}
