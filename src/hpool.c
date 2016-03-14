#include <stdlib.h>
#include <stdint.h>
#include "hpool.h"

#define max(a,b) ( ((a)>(b)) ? (a):(b) )

struct hm_block{
	struct hm_block * next;
	char *ptr,*max;
};

struct hm_pool{
    struct hm_block *head,*cur;
    size_t default_blk_size,used_mem,blk_count;
};

struct hm_pool *hm_pool_create(size_t default_blk_size){
	struct hm_pool *po=(struct hm_pool *)malloc(sizeof(*po));
	po->head=po->cur=NULL;
	if(default_blk_size==0){
		default_blk_size=4096-sizeof(*po->head);
	}
	po->default_blk_size=default_blk_size;
	po->used_mem=0;
	po->blk_count=0;
	return po;
}

void hm_pool_destory(struct hm_pool *po){
	struct hm_block *blk,*next;
	blk=po?po->head:NULL;
	while(blk){
		next=blk->next;
		free(blk);
		blk=next;
	}
}

#define align_ptr(p,a) (((uintptr_t)(p)+((uintptr_t)(a)-1))&~((uintptr_t)(a)-1))

static struct hm_block * hm_block_create(struct hm_pool *po,size_t sz){
	struct hm_block *blk=malloc(sizeof(*blk)+sz);
	blk->next=NULL;
	blk->ptr=(char*)(blk+1);
	blk->max=((char*)blk->ptr)+sz;
	if(!po->cur){
		po->cur=po->head=blk;
	}else{
		po->cur->next=blk;
		po->cur=blk;
	}
	po->blk_count++;
	return blk;
}

void * hm_pool_malloc(struct hm_pool *po,size_t sz){
	char* ret;
	struct hm_block *cur;

	cur=po->cur;
	if(!cur||cur->max-cur->ptr<sz){
reblock:
		cur=hm_block_create(po,max(sz,po->default_blk_size));
	}
	ret=(char*)align_ptr(cur->ptr,sizeof(unsigned long));
	if(ret+sz<=cur->max){
		cur->ptr=ret+sz;
	}else{
		goto reblock;
	}
	po->used_mem+=sz;
	return ret;
}
