

struct h_rwlock{
	int read,write;
};

static inline void rwlock_init(struct h_rwlock * l){
	l->write=l->read=0;
}

static inline void rwlock_rlock(struct h_rwlock * l){
	for(;;){
		while(l->write){
			__sync_synchronize();
		}
		__sync_add_and_fetch(&l->read,1);
		if(l->write){
			__sync_sub_and_fetch(&l->read,1);
		}else{
			break;
		}
	}
}

static inline void rwlock_runlock(struct h_rwlock * l){
	__sync_sub_and_fetch(&l->read,1);
}

static inline void rwlock_wlock(struct h_rwlock * l){
	while(__sync_lock_test_and_set(&l->write,1)){}
	while(l->read){
		__sync_synchronize();
	}
}

static inline void rwlock_wunlock(struct h_rwlock * l){
	__sync_lock_release(&l->write);
}
