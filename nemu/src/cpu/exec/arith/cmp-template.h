#include "cpu/exec/template-start.h" 

#define instr cmp

static void do_execute()
{
	uint32_t leng=(DATA_BYTE << 3)-1;
	DATA_TYPE result;
	result= op_dest->val - op_src->val;
	//printf("%d ",result);
	if(((result >> leng == 1) && (op_dest->val >>leng == 0) && (op_src->val>>leng == 1))||((result >> leng ==0 ) &&(op_dest->val >> leng == 1)&&(op_src->val >>leng==0)))                       //OF
		cpu.EFLAGS.OF =1;
	else cpu.EFLAGS.OF=0;
	//printf("%d %d %d\n",op_dest->val>>leng,op_src->val>>leng,result>>leng);
	if(result == 0)	
		cpu.EFLAGS.ZF= 1;               //ZF
	else
		cpu.EFLAGS.ZF = 0;
	cpu.EFLAGS.SF = MSB(result) &0x1;			//SF
	//printf("%d ",cpu.EFLAGS.SF);
	if(((result+op_src->val) & 0xf) < (result & 0xf))	//AF
		cpu.EFLAGS.AF=1;
	else cpu.EFLAGS.AF=0;
	if((DATA_TYPE)op_dest->val < (DATA_TYPE)op_src->val)
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
	print_asm_template2();
} 

make_instr_helper(i2a)
make_instr_helper(i2rm)
make_instr_helper(r2rm)
make_instr_helper(rm2r)

#if DATA_BYTE ==2 ||DATA_BYTE ==4
make_instr_helper(si2rm)
#endif

#include "cpu/exec/template-end.h"
