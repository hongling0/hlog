#ifndef _H_POOL_H_
#define _H_POOL_H_
#include <stddef.h>

struct hm_pool *hm_pool_create(size_t default_blk_size);
void hm_pool_destory(struct hm_pool *po);
void *hm_pool_malloc(struct hm_pool *po,size_t sz);

#endif //_H_POOL_H_
