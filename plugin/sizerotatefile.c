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
"	type:sizerotatefile\n"
"	level:1\n"
"	keys:MAIN DKM GAME INIT GM\n"
"	size:1024*1024 # 1mb\n"
"	file:Log/main.log\n"
"}\n"
*/

struct sizerotatefilelog_t{
	char path[PATH_MAX];
	FILE *file;
	struct{
		size_t w_curr;
		size_t w_total;
	}stats;
	struct h_lock lock;
	size_t w_limit;
};

static int sizerotatefilelog_checkconf(struct hconf_node *node){
	if(hconf_get_long(node,"size",-1)==-1){
		fprintf(stderr, "undefined rotate size\n");
		return -1;
	}
	if(hconf_get_string(node,"file",NULL)==NULL){
		fprintf(stderr, "undefined rotate file name\n");
	}
	return 0;
}

static void* sizerotatefilelog_initilize(struct hconf_node *node){
	struct sizerotatefilelog_t *ro=(struct sizerotatefilelog_t*)malloc(sizeof(*ro));
	const char *file=hconf_get_string(node,"file",NULL);
	strncpy(ro->path,file,PATH_MAX);
	ro->file=NULL;
	ro->stats.w_curr=0;
	ro->stats.w_total=0;
	ro->w_limit=hconf_get_long(node,"size",0);
	hlock_init(&ro->lock);
	return ro;
}

static void sizerotatefilelog_open(struct sizerotatefilelog_t *ro){
	if(strcmp(ro->path,"stdout")==0){
		ro->file=stdout;
		ro->w_limit=0;
	}else if(strcmp(ro->path,"stderr")==0){
		ro->file=stderr;
		ro->w_limit=0;
	}else{
		assert(!ro->file);
		ro->file=fopen(ro->path,"a+");
		if(!ro->file){
			fprintf(stderr, "sizerotatefilelog fopen(%s) failure %s\n",ro->path,strerror(errno));
		}else{
			struct stat st;
			if(fstat(fileno(ro->file),&st)==-1){
				fprintf(stderr,"sizerotatefilelog fstat(filenofileno(ro->file)) failure %s\n",strerror(errno));
				ro->stats.w_curr=0;
			}else{
				ro->stats.w_curr=st.st_size;
			}
		}
	}
}

static FILE* sizerotatefilelog_rotate(struct sizerotatefilelog_t *ro,struct hlogev_t *ev){
	if(!ro->file||(ro->w_limit>0&&ro->stats.w_curr>ro->w_limit)){
		if(ro->file){
			if(ro->file!=stdout&&ro->file!=stderr){
				char path[PATH_MAX];
				struct tm*tm;
				fclose(ro->file);
				ro->file=NULL;
				tm=hlog_getsystm(ev);
				snprintf(path,sizeof(path),"%s.%04d%02d%02d%02d%02d%02d",ro->path,
					tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,
					tm->tm_min,tm->tm_sec);
				if(rename(ro->path,path)==-1){
					fprintf(stderr,"sizerotatefilelog rename(%s,%s) failure %s\n",ro->path,path,strerror(errno));
				}else{
					sizerotatefilelog_open(ro);
				}
			}
		}else{
			sizerotatefilelog_open(ro);
		}
	}
	return ro->file;
}

static int sizerotatefilelog_logv(void* ctx,struct hlogev_t *ev,const char* buf,size_t len){
	FILE *file;
	struct sizerotatefilelog_t *ro=(struct sizerotatefilelog_t*)ctx;
	hlock_lock(&ro->lock);
	file=sizerotatefilelog_rotate(ro,ev);
	if(file){
		fwrite(buf,len,1,file);
		ro->stats.w_curr+=len;
		ro->stats.w_total+=len;
		fflush(file);
	}
	hlock_unlock(&ro->lock);
	return 0;
}

static void sizerotatefilelog_release(void* ctx){
	struct sizerotatefilelog_t *ro=(struct sizerotatefilelog_t*)ctx;
	if(ro->file){
		if(ro->file!=stdout&&ro->file!=stderr){
			fclose(ro->file);
		}
	}
}

struct hlog_opt sizerotatefilelog_opt={
	.type="sizerotatefile",
	.checkconf=sizerotatefilelog_checkconf,
	.initilize=sizerotatefilelog_initilize,
	.log=sizerotatefilelog_logv,
	.release=sizerotatefilelog_release,
};
