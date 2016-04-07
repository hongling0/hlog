#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/fs.h>
#include <stdarg.h>
#include "hlog.h"
#include "hlock.h"
#include "plugin/hlogbuild.i"
#include "hconf.h"

struct hlogkey_t{
	int logid;
	char name[MAX_HLOG_KEY_LEN];
	struct hlogger_node_t* node;
	hlog_format format;
	int level;
};

struct hlogger_node_t{
	struct hlogger_t *logger;
	struct hlogger_node_t *prev;
	struct hlogger_node_t *next;
	struct hlogger_node_t *samenext;
};

struct hlogger_t{
	char name[MAX_HLOG_KEY_LEN];
	void *ctx;
	int level;
	struct hlog_opt *opt;
	struct hlogger_node_t *allnode;
};

struct hlogvec_t{
	struct h_rwlock lock;
	struct hlogkey_t *key_vec;
	unsigned int key_count;
	struct hlogger_t **logger_vec;
	unsigned int logger_count;
};

static __thread time_t tm_lasttime=0;
static __thread struct tm tm_last;
struct tm* hlog_getsystm(struct hlogev_t *ev){
	if(ev->time!=tm_lasttime){
		tm_lasttime=ev->time;
		localtime_r(&tm_lasttime,&tm_last);
	}
	return &tm_last;
}

static inline struct hlogkey_t * hlogvec_get_key(struct hlogvec_t* vec,const char* name,int remove){
	unsigned i=0;
	for(;i<vec->key_count;i++){
		struct hlogkey_t *key=&vec->key_vec[i];
		if(key->logid>=0&&strcmp(key->name,name)==0){
			if(remove){
				key->level=0;
			}
			return key;
		}
	}
	return NULL;
}

static inline int hlogvec_add_key(struct hlogvec_t* vec,const char* name,hlog_format format){
	struct hlogkey_t * oldkey,*curkey,*new_vec;
	unsigned int new_count,i,j;

	oldkey=hlogvec_get_key(vec,name,1);
	if(oldkey){
		oldkey->format=format;
		return oldkey->logid;
	}
addkey:
	for(i=0;i<vec->key_count;i++){
		curkey=&vec->key_vec[i];
		if(curkey->logid<0){
			curkey->logid=i;
			strncpy(curkey->name,name,sizeof(curkey->name));
			curkey->format=format;
			curkey->node=NULL;
			curkey->level=HLOG_MAXLVL;
			return i;
		}
	}
	new_count=vec->key_count?vec->key_count*2:8;
	new_vec=(struct hlogkey_t *)malloc(sizeof(*new_vec)*new_count);
	memset(new_vec,0,sizeof(*new_vec)*new_count);
	for(i=0;i<new_count;i++){
		curkey=&new_vec[i];
		curkey->logid=-1;
	}
	for(i=0,j=0;i<vec->key_count;i++,j++){
		oldkey=&vec->key_vec[i];
		curkey=&new_vec[j];
		curkey->logid=oldkey->logid;
		strncpy(curkey->name,oldkey->name,sizeof(curkey->name));
		curkey->format=oldkey->format;
		curkey->node=oldkey->node;
		curkey->level=oldkey->level;
	}
	free(vec->key_vec);
	vec->key_vec=new_vec;
	vec->key_count=new_count;
	goto addkey;
}

static inline struct hlogger_t * hlogvec_remove_logger(struct hlogvec_t* vec,const char* name){
	unsigned i=0;
	for(;i<vec->logger_count;i++){
		struct hlogger_t *logger;
		logger=vec->logger_vec[i];
		if(logger&&strcmp(logger->name,name)==0){
			vec->logger_vec[i]=NULL;
			return logger;
		}
	}
	return NULL;
}

static inline struct hlogger_t * hlogvec_add_logger(struct hlogvec_t* vec,struct hlogger_t *logger){
	struct hlogger_t * oldlogger,*curlogger,**new_vec;
	unsigned int new_count,i,j;

	oldlogger=hlogvec_remove_logger(vec,logger->name);
addlog:
	for(i=0;i<vec->logger_count;i++){
		curlogger=vec->logger_vec[i];
		if(!logger){
			vec->logger_vec[i]=logger;
			return oldlogger;
		}
	}
	new_count=vec->logger_count?vec->logger_count*2:8;
	new_vec=(struct hlogger_t **)malloc(sizeof(*new_vec)*new_count);
	memset(new_vec,0,sizeof(*new_vec)*new_count);
	for(i=0,j=0;i<vec->logger_count;i++){
		curlogger=vec->logger_vec[i];
		if(curlogger){
			new_vec[j++]=curlogger;
		}
	}
	free(vec->logger_vec);
	vec->logger_vec=new_vec;
	vec->logger_count=new_count;
	goto addlog;
}

static struct hlogvec_t* V;

static const char* LOG_LEVEL_NAME[]={"FATA","ERROR","WARN","INFO","DEBUG"};

static __thread char timestr[64];
static __thread time_t timestr_lasttime=0;
static __thread size_t timestr_len=0;
static int hlog_default_formater(char *buf,size_t maxlen,const char* key,struct hlogev_t *ev,const char* fmt,...){
	va_list ap;
	int headlen=0;

	if(timestr_lasttime!=ev->time){
		struct tm *tm=hlog_getsystm(ev);
		timestr_len=strftime(timestr,sizeof(timestr),"%c ",tm);
		timestr_lasttime=ev->time;
	}
	memcpy(buf,timestr,timestr_len);
	headlen=timestr_len;
	if(!(ev->flag&HLOG_FLAG_NOHEAD)){
		headlen+=snprintf(buf+headlen,maxlen-headlen,"[%s][%s] ",key,LOG_LEVEL_NAME[ev->level]);
		assert(headlen<maxlen);
	}
	va_start(ap,fmt);
	headlen+=vsnprintf(buf+headlen,maxlen-headlen,fmt,ap);
	va_end(ap);
	return headlen;
}

int hlog_initilize(){
	if(V){
		fprintf(stderr, "hlog is already initilized\n");
		return -1;
	}else{
		struct hlogvec_t *v=malloc(sizeof(*v));
		rwlock_init(&v->lock);
		v->key_vec=NULL;
		v->key_count=0;
		v->logger_vec=NULL;
		v->logger_count=0;
		hlogvec_add_key(v,"*",hlog_default_formater);
		V=v;
		return 0;
	}
}

void hlog_release(){
	if(V){
		struct hlogvec_t *v=V;
		V=NULL;
		free(v->key_vec);
		free(v->logger_vec);
		free(v);
	}
}

static int hlog_keydeclare_impl(struct hlogvec_t *v,const char* name,hlog_format format){
	int logid;

	if(strlen(name)>=MAX_HLOG_KEY_LEN){
		fprintf(stderr, "hlog_keydeclare %s failure KEY too long than %d\n",name,(int)MAX_HLOG_KEY_LEN);
		return -1;
	}
	if(!format){
		format=hlog_default_formater;
	}
	logid=hlogvec_add_key(v,name,format);
	return logid;
}

int hlog_keydeclare(const char* name,hlog_format format){
	int logid;
	struct hlogvec_t *v=V;
	rwlock_wlock(&v->lock);
	logid=hlog_keydeclare_impl(v,name,format);
	rwlock_wunlock(&v->lock);
	return logid;
}

static inline struct hlogkey_t* hlog_keygrub(struct hlogvec_t *v,int id){
	struct hlogkey_t *key=&v->key_vec[id];
	if(id<v->key_count){
		if(key->logid>0||id==0){
			return key;
		}
	}
	fprintf(stderr, "hlog_keygrub %d failure not register log key\n",(int)id);
	return NULL;
}

size_t hlog_interface(int logid,int level,int flag,const char* file,int line,const char* fmt,...){
	struct hlogkey_t *key;
	struct hlogvec_t *vec;
	size_t maxlen,len;
	char *buf,*ptr,cache[2048];

	if(level<0||level>HLOG_MAXLVL){
		fprintf(stderr, "hlog_interface log failure log level %d error id %d\n",level,logid);
		return 0;
	}
	ptr=NULL;
	len=0;
	maxlen=sizeof(cache);
	buf=cache;
	vec=V;
	rwlock_rlock(&vec->lock);
logstart:
	key=hlog_keygrub(vec,logid);
	if(key&&key->node&&key->level>=level){
		struct hlogger_node_t *l;
		struct hlogev_t ev={
			.logid=logid,.level=level,
			.flag=flag,.time=time(NULL),
			.file=file,.line=line
		};
		if(!ptr){
			va_list va;

			va_start(va,fmt);
			len=key->format(buf,maxlen,key->name,&ev,fmt,va);
			va_end(va);
			if(len>maxlen){
				buf=NULL;
				maxlen*=2;
				do{
					free(buf);
					buf=(char*)malloc(maxlen);
					va_start(va,fmt);
					len=key->format(buf,maxlen,key->name,&ev,fmt,va);
					va_end(va);
				}
				while(len>maxlen);
			}
			ptr=buf;
		}
		l=key->node;
		while(l){
			l->logger->opt->log(l->logger->ctx,&ev,ptr,len);
			l=l->next;
		}
	}
	if(logid!=0){
		logid=0;
		goto logstart;
	}
	if(ptr!=cache){
		free(ptr);
	}
	rwlock_runlock(&vec->lock);
	return len;
}

static void hlog_logger_remove(struct hlogvec_t *vec,const char* name){
	struct hlogger_t *logger=hlogvec_remove_logger(vec,name);
	if(logger){
		struct hlogger_node_t *node,*next;
		node=logger->allnode;
		while(node){
			next=node->next;
			if(node->prev){
				node->prev->next=node->next;
			}
			if(node->next){
				node->next->prev=node->prev;
			}
			free(node);
			node=next;
		}
		logger->opt->release(logger->ctx);
		free(logger);
	}
}

static struct hlogger_t * hlog_logger_initilize(struct hlog_opt *opt,const char *name,int level,struct hconf_node *node){
	struct hlogger_t *logger;
	void *ctx;
	if(level<0||level>HLOG_MAXLVL){
		fprintf(stderr, "hlog_logger_initilize %s failure log level %d error\n",name,level);
		return NULL;
	}
	if(strlen(name)>=MAX_HLOG_KEY_LEN){
		fprintf(stderr, "hlog_logger_initilize %s failure name too long than %d\n",name,(int)MAX_HLOG_KEY_LEN);
		return NULL;
	}
	ctx=opt->initilize(node);
	if(!ctx){
		return NULL;
	}
	logger=(struct hlogger_t *)malloc(sizeof(*logger));
	logger->level=level;
	strncpy(logger->name,name,sizeof(logger->name));
	logger->opt=opt;
	logger->allnode=NULL;
	logger->ctx=ctx;
	return logger;
}

static struct hlogger_node_t * hlog_hlogger_t_refkey(struct hlogvec_t *vec,const char* name,int level){
	int logid=hlog_keydeclare_impl(vec,name,NULL);
	if(logid<0){
		return NULL;
	}else{
		struct hlogger_node_t *node=(struct hlogger_node_t*)malloc(sizeof(*node));
		struct hlogkey_t* key=hlog_keygrub(vec,logid);
		node->next=key->node;
		if(node->next){
			node->next->prev=node;
		}
		node->prev=NULL;
		key->node=node;
		if(key->level<level){
			key->level=level;
		}
		return node;
	}
}

struct hlog_opt *get_logopt(const char* type){
	int i=0;
	for(;i<sizeof(HLOG_OPT_LIST)/sizeof(HLOG_OPT_LIST[0]);i++){
		if(strcmp(type,HLOG_OPT_LIST[i]->type)==0){
			return HLOG_OPT_LIST[i];
		}
	}
	fprintf(stderr, "undefined logopt type %s\n",type);
	return NULL;
}

static int hlogconf_check(struct hconf_node *node){
	//long level;
	struct hlog_opt *opt;
	const char *type,*keys;
	type=hconf_get_string(node,"type",NULL);
	if(!type) return -1;
	// level=hconf_get_long(node,"level",0);
	keys=hconf_get_string(node,"type",NULL);
	if(!keys) return -1;
	opt=get_logopt(type);
	if(!opt) return -1;
	return opt->checkconf(node);
}

static const char** parse_keys(struct hm_pool *po,const char* keystr){
	char *ptr,str[strlen(keystr)+1];
	const char **keys,*key_start;
	int count,i;

	strcpy(str,keystr);
	key_start=NULL;
	count=0;
	ptr=str;
	while(*ptr){
		if(isgraph(*ptr)){
			if(!key_start){
				key_start=ptr;
				count++;
			}
		}else{
			if(key_start){
				*ptr='\0';
				key_start=NULL;
			}
		}
		ptr++;
	}
	keys=(const char**)hm_pool_malloc(po,sizeof(*keys)*(count+1));
	ptr=str;
	key_start=NULL;
	for(i=0;i<count;){
		if(isgraph(*ptr)){
			if(!key_start){
				keys[i++]=hm_pool_strdup(po,str);
				key_start=ptr;
			}
		}else{
			if(key_start){
				key_start=NULL;
			}
		}
		ptr++;
	}
	keys[i]=NULL;
	return keys;
}


static int hlog_hlogger_start(struct hlog_opt *opt,const char *name,int level,const char** keys,struct hconf_node *conf){
	struct hlogger_t *logger;
	struct hlogvec_t *v;
	const char **cur;
	int r=0;
	v=V;

	rwlock_wlock(&v->lock);
	hlog_logger_remove(v,name);
	logger=hlog_logger_initilize(opt,name,level,conf);
	if(logger==0){
		r=-1;
	}else{
		for(cur=keys;*cur;cur++){
			struct hlogger_node_t *node=hlog_hlogger_t_refkey(v,*cur,level);
			if(node==NULL){
				r=-1;
				break;
			}else{
				node->samenext=logger->allnode;
				logger->allnode=node;
				node->logger=logger;
			}
		}
	}
	rwlock_wunlock(&v->lock);
	return r;
}

int hlogconf_loadconf(struct hconf *conf){
	struct hconf_node *node;

	for(node=conf->block_list;node;node=node->next){
		if(hlogconf_check(node)!=0){
			return -1;
		}
	}
	for(node=conf->block_list;node;node=node->next){
		const char *name=node->key;
		const char* type=hconf_get_string(node,"type",NULL);
		struct hlog_opt *opt=get_logopt(type);
		int level=hconf_get_long(node,"level",0);
		const char* keys=hconf_get_string(node,"keys",NULL);
		const char** keyvec=parse_keys(conf->po,keys);
		if(hlog_hlogger_start(opt,name,level,keyvec,node)!=0){
			return -1;
		}
	}
	return 0;
}
