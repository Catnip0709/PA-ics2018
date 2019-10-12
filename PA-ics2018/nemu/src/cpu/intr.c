#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  //TODO();
	//模拟i386中断机制处理过程
	//1、当前状态压栈
	rtl_push((rtlreg_t *)&cpu.eflags);
	cpu.eflags.IF=0;
	rtl_push((rtlreg_t *)&cpu.cs);
	rtl_push((rtlreg_t *)&ret_addr);

	//2、从idtr中读出首地址
	uint32_t idtr_base = cpu.idtr.base;

	//3、索引，找到门描述符
	uint32_t eip_low, eip_high, offset;
	eip_low = vaddr_read(idtr_base + NO * 8, 4) & 0x0000ffff;
	eip_high = vaddr_read(idtr_base + NO * 8 + 4, 4) & 0xffff0000;

	//4、将门描述符中的offset域组合成目标地址
	offset = eip_low | eip_high;

	//5、跳转到目标地址
	decoding.jmp_eip = offset;
	decoding.is_jmp = 1;
}

void dev_raise_intr() {
}
