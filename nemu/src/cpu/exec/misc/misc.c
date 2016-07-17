#include "cpu/exec/helper.h"
#include "cpu/decode/modrm.h"

make_helper(nop) {
	print_asm("nop");
	return 1;
}

make_helper(int3) {
	void do_int3();
	do_int3();
	print_asm("int3");

	return 1;
}

make_helper(lea) {
	ModR_M m;
	m.val = instr_fetch(eip + 1, 1);
	int len = load_addr(eip + 1, &m, op_src);
	reg_l(m.reg) = op_src->addr;

	print_asm("leal %s,%%%s", op_src->str, regsl[m.reg]);
	return 1 + len;
}

make_helper(clc) {
	cpu.EFLAGS.CF = 0;
	print_asm("clc");
	return 1;
}

make_helper(cld){
	cpu.EFLAGS.DF = 0;
	print_asm("cld");
	return 1;
}

make_helper(stc) {
	cpu.EFLAGS.CF = 1;
	print_asm("stc");
	return 1;
}

make_helper(sti) {
	cpu.EFLAGS.IF = 1;
	print_asm("sti");
	return 1;
}

make_helper(cli) {
	cpu.EFLAGS.IF = 0;
	print_asm("cli");
	return 1;
}

/*make_helper(cltd)
{
	uint8_t first = cpu.eax >> 31;
	int i = 0;
	print_asm("cltd");
	for(;i<32;i++)
		cpu.edx = cpu.edx & (first<<i);
	return 1;
}*/

make_helper(std)
{
	cpu.EFLAGS.DF = 1;
	print_asm("std");
	return 1;
}