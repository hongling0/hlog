#ifndef _H_LOG_H
#define _H_LOG_H

#include <time.h>
#include "hconf.h"

#ifdef __cpluscplus
extern "C"{
#endif

#define HLOG_LEVEL_FATAL 0
#define HLOG_LEVEL_ERROR 1
#define HLOG_LEVEL_WARN 2
#define HLOG_LEVEL_INFO 3
#define HLOG_LEVEL_DEBUG 4
#define HLOG_LEVEL_MAXLVL HLOG_LEVEL_DEBUG

// flag
#define HLOG_FLAG_NOHEAD 	(1U<<1)
#define HLOG_FLAG_NOTIME 	(1U<<2)

struct hlogev_t{
	unsigned char level;
	int logid;
	int flag;
	int line;
	time_t time;
	const char* file;
};

typedef int (*hlog_format)(char *buf,size_t maxlen,const char* key,struct hlogev_t *ev,const char* fmt,...);

int hlog_initilize();
void hlog_release();

int hlogconf_loadconf(struct hconf *conf);
int hlog_keydeclare(const char* name,hlog_format format);
size_t hlog_interface(int logid,int level,int flag,const char* file,int line,const char* fmt,...);

#ifdef __cpluscplus
}
#endif


#define HLOG(logid,level,flag,__FILE__,__LINE__,...) hlog_interface(logid,level,flag,__FILE__,__LINE__,__VA_ARGS__)
#define HDEBUG(logid,...) HLOG(logid,HLOG_LEVEL_DEBUG,0,__FILE__,__LINE__,__VA_ARGS__)
#define HINFO(logid,...) HLOG(logid,HLOG_LEVEL_INFO,0,__FILE__,__LINE__,__VA_ARGS__)
#define HWARN(logid,...) HLOG(logid,HLOG_LEVEL_WARN,0,__FILE__,__LINE__,__VA_ARGS__)
#define HERROR(logid,...) HLOG(logid,HLOG_LEVEL_ERROR,0,__FILE__,__LINE__,__VA_ARGS__)
#define HFATAL(logid,...) HLOG(logid,HLOG_LEVEL_FATAL,0,__FILE__,__LINE__,__VA_ARGS__)

#endif
