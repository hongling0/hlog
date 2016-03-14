#include <stdio.h>
#include "hconf.h"


const char *buffer=
"console:{\n"
"	type:console\n"
"	level:1\n"
"	keys:MAIN DKM GAME INIT GM\n"
"}\n"
"mainfilelog:{\n"
"	type:sizerotatefile\n"
"	level:1\n"
"	keys:MAIN DKM GAME INIT GM\n"
"	size:1024*1024 # 1mb\n"
"	file:Log/main.log\n"
"}\n"
"gmfilelog:{\n"
"	type:sizerotatefile\n"
"	level:1\n"
"	keys:MAIN DKM GAME INIT GM\n"
"	size:1024*1024 # 1mb\n"
"	file:Log/gm.log\n"
"}\n"
"sqlfilelog:{\n"
"	type:timerotatefile\n"
"	level:1\n"
"	keys:MAIN DKM GAME INIT GM\n"
"	file:Log/%Y-%m-%d_main.log\n"
"}\n";

int main(int argc,char *argv[]){
	struct hconf_node * node;
	struct hconf conf;
	hconf_init(&conf);
	if(hconf_parse(&conf,buffer)!=0){
		return -1;
	}
	for(node=conf.block_list;node;node=node->next){
		printf("NAME %s TYPE:%d ADDR:%p\n",node->key,(int)node->type,node);
		printf("	type %s\n",hconf_get_string(node,".type","empty string"));
		printf("	level %d\n",(int)hconf_get_long(node,"level",0));
		printf("	keys %s\n",hconf_get_string(node,".keys","empty string"));
		printf("	size %d\n",(int)hconf_get_long(node,"size",0));
		printf("	file %s\n",hconf_get_string(node,".file","empty string"));
	}
	hconf_destory(&conf);
	return 0;
}
