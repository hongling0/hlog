#include <pthread.h>

#ifdef __cpluscplus
extern "C"{
#endif

struct h_rwlock{
	pthread_rwlock_t lock;
};

static inline void rwlock_init(struct h_rwlock * l){
	pthread_rwlock_init(&l->lock, NULL);
}

static inline void rwlock_rlock(struct h_rwlock * l){
	pthread_rwlock_rdlock(&l->lock);
}

static inline void rwlock_runlock(struct h_rwlock * l){
	pthread_rwlock_unlock(&l->lock);
}

static inline void rwlock_wlock(struct h_rwlock * l){
	pthread_rwlock_wrlock(&l->lock);
}

static inline void rwlock_wunlock(struct h_rwlock * l){
	pthread_rwlock_unlock(&l->lock);
}

struct h_lock{
	pthread_mutex_t lock;
};

static inline void hlock_init(struct h_lock * l){
	pthread_mutex_init(&l->lock, NULL);
}

static inline void hlock_lock(struct h_lock * l){
	pthread_mutex_lock(&l->lock);
}

static inline void hlock_unlock(struct h_lock * l){
	pthread_mutex_unlock(&l->lock);
}

#ifdef __cpluscplus
}
#endif
