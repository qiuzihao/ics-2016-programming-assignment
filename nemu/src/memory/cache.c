#include "common.h"
#include "burst.h"
#include "misc.h"
#include "memory.h"

void dram_write(hwaddr_t , size_t , uint32_t);
static void l2_readread(hwaddr_t , void *);
static void l1_readread(hwaddr_t , void *);
void l2_writewrite(hwaddr_t , void * , uint8_t *);
void l1_writewrite(hwaddr_t , void * , uint8_t *) ;
uint32_t l2_cache_read(hwaddr_t , size_t);
uint32_t l1_cache_read(hwaddr_t , size_t);
void l1_cache_write(hwaddr_t , size_t , uint32_t);
void l2_cache_write(hwaddr_t , void * , uint8_t *);


// 1 vaild-bit; 2^(11+x) symbol; 2^6 content;
// 2^10 row

#define L1_ASSOCIATE 3
#define L1_ROW 10
#define L1_COL 6

#define L2_ASSOCIATE 4
#define L2_ROW 16
#define L2_COL 6

//adddress : 
//symbol (11+x); cache-group (10-x); offset 6;

#define l1_row (1 << L1_ROW)
#define l1_col (1 << L1_COL)
#define l1_associate (1 << L1_ASSOCIATE)

#define l2_row (1 << L2_ROW)
#define l2_col (1 << L2_COL)
#define l2_associate (1 << L2_ASSOCIATE)

typedef union 
{
	struct 
	{
		uint32_t offset : 6;
		uint32_t groupnum : 10 - L1_ASSOCIATE;
		uint32_t symbol : 11 + L1_ASSOCIATE;
	};
	uint32_t addr;
} l1_phy_addr;

typedef union
{
	struct
	{
		uint32_t offset : 6;
		uint32_t groupnum : 16 - L2_ASSOCIATE;
		uint32_t symbol : 5 + L2_ASSOCIATE;
	};
	uint32_t addr;
} l2_phy_addr;

typedef struct
{
	uint8_t content[l1_col];
	uint32_t symbol;
	bool valid;
} L1_CACHE ;

typedef struct
{
	uint8_t content[l2_col];
	uint32_t symbol;
	bool valid;
	bool dirty;
} L2_CACHE ;

typedef union {
	struct {
		uint32_t acol	: 10 ;
		uint32_t arow	: 10 ;
		uint32_t abank	: 3 ;
		uint32_t arank	: 4 ;
	};
	uint32_t addr;
} dram_addr;

typedef struct
{
	uint8_t buf[1<<10];
	uint32_t row_idx;
	bool valid;
} RB;

L1_CACHE l1_cache[l1_row];
L2_CACHE l2_cache[l2_row];

extern uint8_t dram[(1<<4)][(1 << 3)][(1 << 10)][(1 << 10)];
extern RB rowbufs[1<<4][1<<3];

void init_cache()
{
	int i;
	for(i = 0; i< l1_row ;i++)
		l1_cache[i].valid = false;
	for(i = 0; i < l2_row ;i++)
	{
		l2_cache[i].valid = false;
		l2_cache[i].dirty = false;
	}
}

static void l2_readread(hwaddr_t addr, void *data)
{
	Assert(addr < (1 << 27), "Physical address %x is outside of the physical memory!", addr);
	l2_phy_addr tempaddr;
	tempaddr.addr = addr & ~BURST_MASK;
	int i = 0;
	int flag = 0;
	dram_addr temp;
	temp.addr = addr & ~BURST_MASK;
	uint32_t arank = temp.arank;
	uint32_t abank = temp.abank;
	uint32_t arow = temp.arow;
	uint32_t acol = temp.acol;
	
	for(i = tempaddr.groupnum * l2_associate ; i< (tempaddr.groupnum + 1) * l2_associate ; i++)
	{
		if(l2_cache[i].valid == true && l2_cache[i].symbol == tempaddr.symbol)
		{
			flag = 1;
			//判断是否发生越界
			//if(tempaddr.offset > ((1 << 6) - 8))
			//	break;
			memcpy(data, l2_cache[i].content + tempaddr.offset , BURST_LEN);
			break;
		}
	}
	if (flag == 0) {
	int newplace = 0;
	for(i = tempaddr.groupnum * l2_associate; i < (tempaddr.groupnum + 1) * l2_associate ; i++)
	{
		if(l2_cache[i].valid == false)
		{
			newplace = i;
			break;
		}
	}
	
	if(newplace == 0)
		//随机生成新位置
		newplace = tempaddr.groupnum * l2_associate + 1;// + ((addr << (32 - L2_ASSOCIATE)) >> (32 - L2_ASSOCIATE));
	//write allocate
	if(l2_cache[newplace].dirty == true)
	{
		uint32_t changeaddr = 0;
		changeaddr = changeaddr | (tempaddr.groupnum << 6);
		changeaddr = changeaddr | (l2_cache[newplace].symbol << (22 - L2_ASSOCIATE));
		int j = 0;
		for(; j < 16 ; j++)
		 {
			 dram_write(changeaddr + 4 * j , 4, *(l2_cache[newplace].content + 4 *j ));
		 }
	}

	l2_cache[newplace].valid = true;
	l2_cache[newplace].symbol = tempaddr.symbol;
	l2_cache[newplace].dirty = false;	
	//不能调用 dram_read() 否则需要读取16次主存，应当只读取一次 
	//判断是否在行缓冲中
	if(!(rowbufs[arank][abank].valid && rowbufs[arank][abank].row_idx == arow) ) 
	{
		memcpy(rowbufs[arank][abank].buf, dram[arank][abank][arow], (1 << 10));
		rowbufs[arank][abank].row_idx = arow;
		rowbufs[arank][abank].valid = true;
	}

	//memcpy(l2_cache[newplace].content, rowbufs[arank][abank].buf + acol - tempaddr.offset , (1 << 6));
	memcpy(l2_cache[newplace].content, dram[arank][abank][arow] + acol- tempaddr.offset, (1 << 6));
	memcpy(data, l2_cache[newplace].content + tempaddr.offset, BURST_LEN);

	}
}

static void l1_readread(hwaddr_t addr, void *data)
{
	Assert(addr < (1 << 27), "Physical address %x is outside of the physical memory!", addr);
	l1_phy_addr tempaddr;
	tempaddr.addr = addr & ~BURST_MASK;
	int i = 0;
	int flag = 0;
	
	for(i = tempaddr.groupnum*l1_associate ; i< (tempaddr.groupnum + 1) * l1_associate ; i++)
	{
		if(l1_cache[i].valid == true && l1_cache[i].symbol == tempaddr.symbol)
		{
			flag = 1;
			//判断是否发生越界
			//if(tempaddr.offset > ((1 << 6) - 8))
			//	break;
			memcpy(data, l1_cache[i].content + tempaddr.offset , BURST_LEN);
			break;
		}
	}
	if (flag == 0){
	int newplace = 0;
	for(i = tempaddr.groupnum * l1_associate; i < (tempaddr.groupnum + 1) * l1_associate ; i++)
	{
		if(l1_cache[i].valid == false)
		{
			newplace = i;
			break;
		}
	}
	
	if(newplace == 0)
		//随机生成新位置
		newplace = tempaddr.groupnum * l1_associate + 1;//((addr << (32 - L1_ASSOCIATE)) >> (32 - L1_ASSOCIATE));
	
	l1_cache[newplace].valid = true;
	l1_cache[newplace].symbol = tempaddr.symbol;
	
	uint32_t start = tempaddr.addr - tempaddr.offset;
	int j = 0;
	for(i = 0; i < 16 ;i++)
	{
		uint32_t num = l2_cache_read(start + 4*i , 4);
		for(j = 0; j < 4; j++)
			l1_cache[newplace].content[4 * i + j] = num >> (8 * j) << 24 >> 24;
	}
	
	memcpy(data, l1_cache[newplace].content + tempaddr.offset, BURST_LEN);

}
}

void l2_writewrite(hwaddr_t addr , void *data , uint8_t * mask)
{
	Assert(addr < (1 << 27), "Physical address %x is outside of the physical address !" , addr);
	l2_phy_addr tempaddr;
	tempaddr.addr = addr & ~BURST_MASK;
	int i = 0;
	
	//赋值从后向前
	
	//写命中
	//write back
	for(i = tempaddr.groupnum*l2_associate ; i< (tempaddr.groupnum + 1) * l2_associate ; i++)
	{
		if(l2_cache[i].valid == true && l2_cache[i].symbol == tempaddr.symbol)
		{
			//int j = 0;
			//for( ; j < len ; j++)
			//{
//				if (tempaddr.offset + j > ((1 << 6) - 1))
//					break;
//				if(j < 4)
//					l2_cache[i].content[tempaddr.offset + j] = *data >> ( 8*j) << 24 >> 24;
//				else l2_cache[i].content[tempaddr.offset + j] = 0; 
					memcpy_with_mask(l2_cache[i].content + tempaddr.offset, data,BURST_LEN, mask );
//				else l2_cache[i].content[tempaddr.offset + j] = 0;
			//}
			l2_cache[i].dirty = true;
			return ;
		}
	}
	//写不命中
	//write allocate
	l2_cache_read(addr , 4);
	l2_writewrite(addr , data , mask);
}

void l1_writewrite(hwaddr_t addr , void * data , uint8_t *mask)
{
	Assert(addr < (1 << 27), "Physical address %x is outside of the physical address !" , addr);
	l1_phy_addr tempaddr;
	tempaddr.addr = addr & ~BURST_MASK;
	int i = 0;
	
	//赋值从后向前
	for(i = tempaddr.groupnum*l1_associate ; i< (tempaddr.groupnum + 1) * l1_associate ; i++)
	{
		if(l1_cache[i].valid == true && l1_cache[i].symbol == tempaddr.symbol)
		{
//			int j = 0;
//			for( ; j < len ; j++)
//			{
//				if (tempaddr.offset + j > ((1 << 6) - 1))
//					break;
//				if(j < 4)
//					l1_cache[i].content[tempaddr.offset + j] = *data >> ( 8*j) << 24 >> 24;
				memcpy_with_mask(l1_cache[i].content + tempaddr.offset, data,BURST_LEN, mask );
//								else l1_cache[i].content[tempaddr.offset + j] = 0; 
//			}
		}
	}
}



uint32_t l2_cache_read(hwaddr_t addr, size_t len)
{
	uint32_t offset = addr & BURST_MASK;
	uint8_t temp[2 * BURST_LEN];
//printf("%x ",addr);
	l2_readread (addr , temp);
	if(offset + len > BURST_LEN)
	{
		l2_readread( addr + BURST_LEN, temp + BURST_LEN);
	}

	return unalign_rw(temp + offset, 4);
}

uint32_t l1_cache_read(hwaddr_t addr, size_t len)
{
	uint32_t offset = addr & BURST_MASK;
	uint8_t temp[2 * BURST_LEN];

	l1_readread(addr, temp);
	if(offset + len > BURST_LEN)
	{
		l1_readread(addr + BURST_LEN, temp + BURST_LEN);
	}

	return unalign_rw(temp + offset, 4);
}

void l1_cache_write(hwaddr_t addr, size_t len , uint32_t data)
{
	uint32_t offset = addr & BURST_MASK;
	uint8_t temp[2 * BURST_LEN];
	uint8_t mask[2 * BURST_LEN];
	memset(mask, 0, 2 * BURST_LEN);

	*(uint32_t *)(temp + offset) = data;
	memset(mask + offset, 1, len);

	//write through
	l2_cache_write( addr, temp, mask);
	//not write allocate
	l1_writewrite(addr , temp, mask);
	
	if(offset + len > BURST_LEN)
	{
		l2_cache_write(addr +BURST_LEN , temp + BURST_LEN, mask + BURST_LEN);
		l1_writewrite(addr+ BURST_LEN, temp + BURST_LEN, mask + BURST_LEN);
	}
	 //l1_writewrite(addr , temp, mask);
}

void l2_cache_write(hwaddr_t addr, void *data, uint8_t *mask)
{
	l2_writewrite(addr, data, mask);
}

