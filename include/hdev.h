#ifndef _H_DEV_H_
#define _H_DEV_H_
#include "hlog.h"
#include "hlock.h"

#ifdef __cpluscplus
extern "C"{
#endif

struct hlogev_t{
	unsigned char level;
	int logid;
	int flag;
	int line;
	time_t time;
	const char* file;
};

struct hlog_opt{
	const char* type;
	int (*checkconf)(struct hconf_node *conf);
	void* (*initilize)(struct hconf_node *conf);
	int (*log)(void* ctx,struct hlogev_t *ev,const char* buf,size_t len);
	void (*release)(void* ctx);
};

struct tm* hlog_getsystm(struct hlogev_t *ev);

#ifdef __cpluscplus
}
#endif

#endif //_H_DEV_H_
