#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "hlock.h"

struct counter_t{
	struct h_rwlock lock;
	int count;
	int running;
};


void* read_thread(void* args){
	struct counter_t *c=(struct counter_t*)args;
	int count=0;
	while(c->running){
		rwlock_rlock(&c->lock);
		if((count++)%10==0){
			printf("%p read_thread %d reader:%d\n",&c,count,c->count);
		}
		rwlock_runlock(&c->lock);
		usleep(10000);
	}
	return NULL;
}

void* write_thread(void* args){
	struct counter_t *c=(struct counter_t*)args;
	int count=0;
	while(c->running){
		rwlock_wlock(&c->lock);
		c->count++;
		printf("%p write_thread %d write:%d\n",&c,count++,c->count);
		usleep(1000000);
		rwlock_wunlock(&c->lock);
		usleep(0);
	}
	return NULL;
}

int main(int argc,char* argv[]){
	pthread_t tid[8];
	struct counter_t c;

	int i=0;

	rwlock_init(&c.lock);
	c.count=0;c.running=1;

	for(;i<5;i++){
		pthread_create(&tid[i],NULL,read_thread,&c);
	}
	for(;i<8;i++){
		pthread_create(&tid[i],NULL,write_thread,&c);
	}

	getchar();
	c.running=0;

	for(;i<8;i++){
		pthread_join(tid[i],NULL);
	}
	return 0;
}
