#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/fs.h>
#include "hdev.h"

/*
"mainfilelog:{\n"
"	type:timerotatefile\n"
"	level:1\n"
"	keys:MAIN DKM GAME INIT GM\n"
"	size:1024*1024 # 1mb\n"
"	file:Log/%Y-%m-%d_main.log\n"
"}\n"
*/

struct timerotatefilelog_t{
	char path[PATH_MAX];
	char realpath[PATH_MAX];
	time_t realpath_lasttime;
	FILE *file;
	struct h_lock lock;
};

static int timerotatefilelog_checkconf(struct hconf_node *node){
	if(hconf_get_string(node,"file",NULL)==NULL){
		fprintf(stderr, "undefined rotate file name\n");
	}
	return 0;
}

static void* timerotatefilelog_initilize(struct hconf_node *node){
	struct timerotatefilelog_t *ro=(struct timerotatefilelog_t*)malloc(sizeof(*ro));
	const char *file=hconf_get_string(node,"file",NULL);
	strncpy(ro->path,file,PATH_MAX);
	ro->file=NULL;
	ro->realpath_lasttime=0;
	hlock_init(&ro->lock);
	return ro;
}

static void timerotatefilelog_open(struct timerotatefilelog_t *ro){
	if(strcmp(ro->path,"stdout")==0){
		ro->file=stdout;
	}else if(strcmp(ro->path,"stderr")==0){
		ro->file=stderr;
	}else{
		assert(!ro->file);
		ro->file=fopen(ro->realpath,"a+");
		if(!ro->file){
			fprintf(stderr, "timerotatefilelog fopen(%s) failure %s\n",ro->realpath,strerror(errno));
		}
	}
}

static FILE* timerotatefilelog_rotate(struct timerotatefilelog_t *ro,struct hlogev_t *ev){
	if(!ro->file){
		goto openfile;
	}else if(ro->file==stderr||ro->file==stdout){
		goto retfile;
	}else if(ro->realpath_lasttime!=ev->time){
		fclose(ro->file);
		ro->file=NULL;
		goto openfile;
	}else{
		goto retfile;
	}
openfile:
	ro->realpath_lasttime=ev->time;
	strftime(ro->realpath,sizeof(ro->realpath),ro->path,hlog_getsystm(ev));
	timerotatefilelog_open(ro);
retfile:
	return ro->file;
}

static int timerotatefilelog_logv(void* ctx,struct hlogev_t *ev,const char* buf,size_t len){
	FILE *file;
	struct timerotatefilelog_t *ro=(struct timerotatefilelog_t*)ctx;
	hlock_lock(&ro->lock);
	file=timerotatefilelog_rotate(ro,ev);
	if(file){
		fwrite(buf,len,1,file);
		fflush(file);
	}
	hlock_unlock(&ro->lock);
	return 0;
}

static void timerotatefilelog_release(void* ctx){
	struct timerotatefilelog_t *ro=(struct timerotatefilelog_t*)ctx;
	if(ro->file){
		if(ro->file!=stdout&&ro->file!=stderr){
			fclose(ro->file);
		}
	}
}

struct hlog_opt timerotatefilelog_opt={
	.type="timerotatefile",
	.checkconf=timerotatefilelog_checkconf,
	.initilize=timerotatefilelog_initilize,
	.log=timerotatefilelog_logv,
	.release=timerotatefilelog_release,
};
