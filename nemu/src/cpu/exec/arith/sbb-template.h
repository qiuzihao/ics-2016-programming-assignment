#include "cpu/exec/template-start.h"

#define instr sbb

static void do_execute()
{
	uint32_t leng=(DATA_BYTE << 3)-1;
	DATA_TYPE result;
	result= op_dest->val - op_src->val - cpu.EFLAGS.CF;
	if(((result >> leng == 1) && (op_dest->val >>leng == 0) && (op_src->val) == 1)||((result >> leng ==0 ) &&(op_dest->val >> leng == 1)&&(    op_src->val ==0) ))                                     //OF
		cpu.EFLAGS.OF =1;
	else cpu.EFLAGS.OF=0;
	cpu.EFLAGS.ZF= !(result);               //ZF
	cpu.EFLAGS.SF = result >> leng;			//SF
	if(((result+op_src->val) & 0xf) < (result & 0xf))	//AF
		cpu.EFLAGS.AF=1;
	else cpu.EFLAGS.AF=0;
	if(op_dest->val < op_src->val)
		cpu.EFLAGS.CF=1;                     //CF
	else cpu.EFLAGS.CF=0;
	int i=0,sum=0;				             //PF
	for(;i<8;i++)
	{
		if(((result >> i) & 0x1) ==1)
			sum++;
	}
	if(sum%2==0)
		cpu.EFLAGS.PF=1;
	else cpu.EFLAGS.PF=0;	
	OPERAND_W(op_dest,result);
	
	print_asm_template2();

}
make_instr_helper(i2a)
make_instr_helper(i2rm)
make_instr_helper(rm2r)
make_instr_helper(r2rm)

#if DATA_BYTE==2 ||DATA_BYTE == 4
make_instr_helper(si2rm)
#endif

#include "cpu/exec/template-end.h"
