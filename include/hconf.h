#ifndef _H_CONF_H_
#define _H_CONF_H_
#include <stdint.h>
#include "hpool.h"

#define HCONF_TYPE_STRING	0
#define HCONF_TYPE_BLOCK	1

struct hconf_node{
	uint8_t type;
	const char * key;
	union {
		const char * value;
		struct hconf_node * block_list;
	};
	struct hconf_node *next;
};

struct hconf{
	struct hm_pool *po;
	struct hconf_node *block_list;
};

static inline void hconf_init(struct hconf *conf){
	conf->po=NULL;;
	conf->block_list=NULL;
}

static inline void hconf_destory(struct hconf *conf){
	hm_pool_destory(conf->po);
	conf->po=NULL;
	conf->block_list=NULL;
}
int hconf_parse(struct hconf *conf,const char *buffer);

struct hconf_node *hconf_get_node(struct hconf_node *block_list,const char* key);
const char* hconf_get_string(struct hconf_node *node,const char* key,const char* def);
long hconf_get_long(struct hconf_node *node,const char* key,long def);
double hconf_get_double(struct hconf_node *node,const char* key,double def);

#endif //_H_CONF_H_
