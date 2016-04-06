#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/fs.h>
#include "hconf.h"
#include "hlog.h"
/*
"mainfilelog:{\n"
"	type:sizerotatefile\n"
"	level:1\n"
"	keys:MAIN DKM GAME INIT GM\n"
"	size:1024*1024 # 1mb\n"
"	file:Log/main.log\n"
"}\n"
*/

struct hfilelog_sizerotate_t{
	char path[PATH_MAX];
	FILE *file;
	struct{
		size_t w_curr;
		size_t w_total;
	}stats;
	size_t w_limit;
};

static int hfilelog_sizerotate_checkconf(struct hconf_node *node){
	if(hconf_get_long(node,"size",-1)==-1){
		fprintf(stderr, "undefined rotate size\n");
		return -1;
	}
	if(hconf_get_string(node,"file",NULL)==NULL){
		fprintf(stderr, "undefined rotate file name\n");
	}
	return 0;
}

static void* hfilelog_sizerotate_initilize(struct hconf_node *node){
	struct hfilelog_sizerotate_t *ro=(struct hfilelog_sizerotate_t*)malloc(sizeof(*ro));
	const char *file=hconf_get_string(node,"file",NULL);
	strncpy(ro->path,file,PATH_MAX);
	ro->file=NULL;
	ro->stats.w_curr=0;
	ro->stats.w_total=0;
	ro->w_limit=hconf_get_long(node,"size",0);
	return ro;
}

static void hfilelog_sizerotate_open(struct hfilelog_sizerotate_t *ro){
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
			fprintf(stderr, "hfilelog_sizerotate fopen(%s) failure %s\n",ro->path,strerror(errno));
		}else{
			struct stat st;
			if(fstat(fileno(ro->file),&st)==-1){
				fprintf(stderr,"hfilelog_sizerotate fstat(filenofileno(ro->file)) failure %s\n",strerror(errno));
				ro->stats.w_curr=0;
			}else{
				ro->stats.w_curr=st.st_size;
			}
		}
	}
}

static FILE* hfilelog_sizerotate_rotate(struct hfilelog_sizerotate_t *ro,struct hlogev_t *ev){
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
					fprintf(stderr,"hfilelog_sizerotate rename(%s,%s) failure %s\n",ro->path,path,strerror(errno));
				}else{
					hfilelog_sizerotate_open(ro);
				}
			}
		}else{
			hfilelog_sizerotate_open(ro);
		}
	}
	return ro->file;
}

static int hfilelog_sizerotate_logv(void* ctx,struct hlogev_t *ev,const char* buf,size_t len){
	struct hfilelog_sizerotate_t *ro=(struct hfilelog_sizerotate_t*)ctx;
	FILE *file=hfilelog_sizerotate_rotate(ro,ev);
	if(file){
		fwrite(buf,len,1,file);
		ro->stats.w_curr+=len;
		ro->stats.w_total+=len;
		fflush(file);
	}
	return 0;
}

static void hfilelog_sizerotate_release(void* ctx){
	struct hfilelog_sizerotate_t *ro=(struct hfilelog_sizerotate_t*)ctx;
	if(ro->file){
		if(ro->file!=stdout&&ro->file!=stderr){
			fclose(ro->file);
		}
	}
}

struct hlog_opt hfilelog_sizerotate_opt={
	.type="sizerotatefile",
	.checkconf=hfilelog_sizerotate_checkconf,
	.initilize=hfilelog_sizerotate_initilize,
	.log=hfilelog_sizerotate_logv,
	.release=hfilelog_sizerotate_release,
};
