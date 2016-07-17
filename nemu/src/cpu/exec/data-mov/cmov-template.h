#include "cpu/exec/template-start.h"

#define instr cmov

static void do_execute()
{
	DATA_TYPE opcode = ops_decoded.opcode & 0xff; 
	if(opcode == 0x44 && cpu.EFLAGS.ZF ==1)
		OPERAND_W(op_dest ,op_src->val);
	else if(opcode == 0x41 && cpu.EFLAGS.OF==0)
        OPERAND_W(op_dest,op_src->val);
	else if(opcode == 0x42 && cpu.EFLAGS.CF==1)
		OPERAND_W(op_dest,op_src->val);
	else if(opcode == 0x43 && cpu.EFLAGS.CF==0)
		OPERAND_W(op_dest,op_src->val);
	else if(opcode == 0x45 && cpu.EFLAGS.ZF==0)
		OPERAND_W(op_dest,op_src->val);
	else if(opcode == 0x46 && (cpu.EFLAGS.ZF==1 || cpu.EFLAGS.CF == 1))
		OPERAND_W(op_dest,op_src->val);
	else if(opcode == 0x47 && cpu.EFLAGS.ZF==0 && cpu.EFLAGS.CF == 0)
		OPERAND_W(op_dest,op_src->val);
	else if(opcode == 0x4b && cpu.EFLAGS.PF==0)
		OPERAND_W(op_dest,op_src->val);
	else if(opcode == 0x4c && cpu.EFLAGS.SF!= cpu.EFLAGS.OF)
		OPERAND_W(op_dest,op_src->val);
	else if(opcode == 0x4e && (cpu.EFLAGS.ZF == 1 ||(cpu.EFLAGS.SF != cpu.EFLAGS.OF)))
		OPERAND_W(op_dest,op_src->val);
	else if(opcode == 0x4f && cpu.EFLAGS.ZF==0 && cpu.EFLAGS.SF == cpu.EFLAGS.OF)
		OPERAND_W(op_dest,op_src->val);
	else if(opcode == 0x48 && cpu.EFLAGS.SF==1)
		OPERAND_W(op_dest,op_src->val);
	else if (opcode == 0x4d && cpu.EFLAGS.SF ==cpu.EFLAGS.OF)
		OPERAND_W(op_dest , op_src->val);
	else if(opcode == 0x49 && cpu.EFLAGS.SF == 0)
		OPERAND_W(op_dest,op_src->val);
	else if(opcode == 0x40 && cpu.EFLAGS.OF == 1)
		OPERAND_W(op_dest,op_src->val);
	else if(opcode == 0x4a && cpu.EFLAGS.PF == 1)
		OPERAND_W(op_dest,op_src->val);
	else
		;
	print_asm_template1();
}

make_instr_helper(rm2r)

#include "cpu/exec/template-end.h"
