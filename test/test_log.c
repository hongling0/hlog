
#include <pthread.h>
#include "hlog.h"

const char * conf_string=""
"gmfilelog:{\n"
"	type:timerotatefile\n"
"	level:4\n"
"	keys:MAIN INIT\n"
"	file:tmp/gm%Y%m%d_%H%M%S.log\n"
"}\n"
"initfilelog:{\n"
"	type:sizerotatefile\n"
"	level:4\n"
"	keys:INIT\n"
"	size:10485760\n"
"	file:tmp/main.log\n"
"}\n";

/*
int hlog_keydeclare(const char* name,hlog_format format);
size_t hlog_interface(int logid,int level,int flag,const char* file,int line,const char* fmt,...);
int hlogconf_loadconf(struct hconf *conf);
*/
int LOG_INIT;
int LOG_MAIN;

void* rthread(void* args){
	int count=0;
	for(count=0;count<1000;count++){
		HINFO(LOG_INIT,"log_init_ok_%p\n",args);
		HINFO(LOG_MAIN,"log_main_ok_%p\n",args);
	}
	return NULL;
}

int main(int argc,char* argv[]){
	int i;
	hlog_initilize();
	LOG_INIT=hlog_keydeclare("INIT",NULL);
	LOG_MAIN=hlog_keydeclare("MAIN",NULL);
	struct hconf conf;
	hconf_init(&conf);
	if(hconf_parse(&conf,conf_string)!=0){
		hconf_destory(&conf);
		return -1;
	}
	if(hlogconf_loadconf(&conf)!=0){
		hconf_destory(&conf);
		return -1;
	}
	hconf_destory(&conf);
	pthread_t tid[8];
	for(i=0;i<8;i++){
		pthread_create(&tid[i],NULL,rthread,&tid[i]);
	}

	for(i=0;i<8;i++){
		pthread_join(tid[i],NULL);
	}
	hlog_release();
	return 0;
}
