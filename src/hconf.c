#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "hconf.h"

struct parse_ctx{
	const char *buffer;
	int line,word,over,last_line_word;
	int error;
	char errmsg[128];
	struct hm_pool *po;
};

static inline const char* goto_nextchar(struct parse_ctx *ctx){
	const char* c=ctx->buffer;
	if(*c=='\0'){
		ctx->over=1;
		return c;
	}
	c=ctx->buffer++;
	ctx->word++;
	switch(*c){
		case '\0':
			break;
		case '\n':{
				ctx->line++;
				ctx->last_line_word=ctx->word;
				ctx->word=0;
			}
			break;
		default:
			break;
	}
	return c;
};

static inline void goto_error(struct parse_ctx *ctx,const char* fmt,...){
	size_t len;
	va_list va;
	va_start(va,fmt);
	len=snprintf(ctx->errmsg,sizeof(ctx->errmsg),"@line %d: word:%d\n",ctx->line,ctx->word);
	len+=vsnprintf(ctx->errmsg+len,sizeof(ctx->errmsg)-len,fmt,va);
	snprintf(ctx->errmsg+len,sizeof(ctx->errmsg)-len,"%s\n",ctx->buffer-ctx->word-ctx->last_line_word);
	va_end(va);
	ctx->error=1;
}

static inline const char* trim_left(const char* start,const char* stop){
	for(;start<stop;start++){
		if(isgraph(*start)){
			break;
		}
	}
	return start;
}

static inline const char* trim_right(const char* start,const char* stop){
	for(stop--;start<=stop;stop--){
		if(isgraph(*stop)){
			++stop;
			break;
		}
	}
	return stop;
}

static inline const char * parse_key(struct parse_ctx *ctx){
	const char *buffer,*value_start=NULL;
	char c;
loop:
	buffer=goto_nextchar(ctx);
	c=*buffer;
	switch(c){
		case '}':{
				if(value_start){
					goto_error(ctx,"parse key error\n");

				}
				return NULL;
			}
			break;
		case '\n':
			if(value_start){
				goto_error(ctx,"parse key error\n");
				return NULL;
			}
			break;
		case '\0':
			if(value_start){
				goto_error(ctx,"parse key error\n");
				return NULL;
			}else{
				return NULL;
			}
			break;
		case ':':
			if(value_start){
				value_start=trim_left(value_start,buffer);
				if(value_start==buffer){
					goto_error(ctx,"parse key error\n");
					return NULL;
				}else{
					const char *value_stop=trim_right(value_start,buffer);
					return hm_pool_strlendup(ctx->po,value_start,value_stop-value_start);
				}
			}else{
				goto_error(ctx,"parse key error\n");
				return NULL;
			}
			break;
		case '{':{
				goto_error(ctx,"parse key error unexpected char {\n");
				return NULL;
			}
			break;
		default:
			if(isgraph(c)&&!value_start){
				value_start=buffer;
			}
			break;
	}
	goto loop;
}



static inline const char * try_parse_value(struct parse_ctx *ctx){
	const char *value_start,*buffer;
	char c;
	value_start=NULL;
loop:
	buffer=goto_nextchar(ctx);
	c=*buffer;
	if(!value_start){
		value_start=buffer;
	}
	switch(c){
		case EOF:
		case '\0':
			if(value_start){
				goto_error(ctx,"parse value error\n");
				return NULL;
			}else{
				return NULL;
			}
			break;
		case '{':{
				goto do_recover;
			}
		case '\n':{
				const char *value_stop=trim_right(value_start,buffer);
				value_start=trim_left(value_start,buffer);
				return hm_pool_strlendup(ctx->po,value_start,value_stop-value_start);
			}
			break;
	}
	goto loop;
do_recover:
	return NULL;
}


static struct hconf_node *parse_block(struct parse_ctx *ctx){
	struct hconf_node *head_node,*last_node,*new_node;
	const char* key,*value;

	head_node=last_node=NULL;
	for(;;){
		key=parse_key(ctx);
		if(!key){
			goto do_return;
		}
		new_node=(struct hconf_node*)hm_pool_malloc(ctx->po,sizeof(*new_node));
		new_node->key=key;
		new_node->next=NULL;

		value=try_parse_value(ctx);
		if(value){
			new_node->type=HCONF_TYPE_STRING;
			new_node->value=value;
		}else if(ctx->error){
			goto do_return;
		}else{
			new_node->type=HCONF_TYPE_BLOCK;
			new_node->block_list=parse_block(ctx);
			if(!new_node->block_list){
				goto do_return;
			}
		}
		if(!head_node){
			head_node=last_node=new_node;
		}else{
			last_node->next=new_node;
			last_node=new_node;
		}
	}
do_return:
	return ctx->error?NULL:head_node;
}

int hconf_parse(struct hconf *conf,const char *buffer){
	struct parse_ctx c,*ctx;
	if(conf->po){
		hm_pool_destory(conf->po);
		conf->block_list=NULL;
	}
	conf->po=hm_pool_create(256);

	ctx=&c;
	ctx->po=conf->po;
	ctx->buffer=buffer;
	ctx->line=1;
	ctx->word=0;
	ctx->last_line_word=0;
	ctx->error=0;

	conf->block_list=parse_block(ctx);
	if(!conf->block_list){
		if(ctx->error){
			hm_pool_destory(conf->po);
			conf->po=NULL;
			fprintf(stderr, "parse error %s\n",ctx->errmsg);
			return -1;
		}

	}
	return 0;
}


struct hconf_node *hconf_get_node(struct hconf_node *block_list,const char* key){
	struct hconf_node *cur;
	const char *buf,*next;
	next=key;
	cur=block_list;
loop:
	buf=next;
	while(strchr(buf,'.')==buf){
		buf++;
	}
	next=strchr(buf,'.');
	if(next){
		while(cur)
			if(memcmp(cur->key,buf,next-buf)==0){
				if(cur->type!=HCONF_TYPE_BLOCK){
					fprintf(stderr, "%s [%s] is not a block\n",key,cur->key);
					return NULL;
				}else{
					cur=cur->block_list;
					goto loop;
				}
			}else{
				cur=cur->next;
			}
	}else{
		while(cur!=NULL){
			if(memcmp(cur->key,buf,strlen(cur->key))==0){
				return cur;
			}else{
				cur=cur->next;
			}
		}
	}
	fprintf(stderr, "%s %s is not found\n",key,buf);
	return NULL;
}


static inline double warp_strtod(const char* str,double def){
	char* endptr;
	double n=strtod(str,&endptr);
	if(*endptr!='\0'){
		fprintf(stderr, "can not convert to double %s\n",str);
		return def;
	}
	return n;
}

static inline long warp_strtol(const char *str,long def){\
	char* endptr;
	long n=strtol(str,&endptr,0);
	if(*endptr!='\0'){
		fprintf(stderr, "can not convert to long %s\n",str);
		return def;
	}
	return n;
}

const char* hconf_get_string(struct hconf_node *node,const char* key,const char* def){
	if(node){
		node=hconf_get_node(node->block_list,key);
	}
	if(node){
		if(node->type==HCONF_TYPE_STRING){
			return node->value;
		}else{
			fprintf(stderr, "%s %s is a block\n",key,node->key);
		}
	}
	return def;
}

long hconf_get_long(struct hconf_node *node,const char* key,long def){
	const char *value=hconf_get_string(node,key,NULL);
	if(value){
		return warp_strtol(value,def);
	}
	return def;
}
double hconf_get_double(struct hconf_node *node,const char* key,double def){
	const char *value=hconf_get_string(node,key,NULL);
	if(value){
		return warp_strtod(value,def);
	}
	return def;
}
