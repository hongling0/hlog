#ifndef _H_LOG_H
#define _H_LOG_H

#include <time.h>

#define MAX_HLOG_KEY_LEN 24

#define HLOG_FATA 0
#define HLOG_ERROR 1
#define HLOG_WARN 2
#define HLOG_INFO 3
#define HLOG_DEBUG 4
#define HLOG_MAXLVL HLOG_DEBUG

// flag
#define HLOG_FLAG_TMOK 		(1U<<0)
#define HLOG_FLAG_NOHEAD 	(1U<<1)
#define HLOG_FLAG_NOTIME 	(1U<<2)

struct hlogev_t{
	unsigned char level;
	int logid;
	int flag;
	struct tm tm;
	int line;
	const char* file;
};

static inline struct tm* hlog_getsystm(struct hlogev_t *ev){
	if(!(ev->flag&HLOG_FLAG_TMOK)){
		time_t now=time(NULL);
		localtime_r(&now,&ev->tm);
		ev->flag|=HLOG_FLAG_TMOK;
	}
	return &ev->tm;
}

typedef int (*hlog_format)(char *buf,size_t maxlen,const char* key,struct hlogev_t *ev,const char* fmt,...);

struct hconf_node;
struct hconf;
struct hlog_opt{
	const char* type;
	int (*checkconf)(struct hconf_node *conf);
	void* (*initilize)(struct hconf_node *conf);
	int (*log)(void* ctx,struct hlogev_t *ev,const char* buf,size_t len);
	void (*release)(void* ctx);
};

int hlog_initilize();
void hlog_release();

int hlog_keydeclare(const char* name,hlog_format format);
size_t hlog_interface(int logid,int level,int flag,const char* file,int line,const char* fmt,...);
int hlogconf_loadconf(struct hconf *conf);

#endif
