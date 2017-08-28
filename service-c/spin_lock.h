#ifndef SPIN_LOCK_H
#define SPIN_LOCK_H

#define SPIN_INIT(lock) 		spinlock_init(lock);
#define SPIN_LOCK(lock) 		spinlock_lock(lock);
#define SPIN_UNLOCK(lock) 		spinlock_unlock(lock);
#define SPIN_DESTROY(lock) 		spinlock_destroy(lock);

#ifdef USE_SPIN_LOCK  //spin_lock

typedef struct _spin_lock 
{
	int lock;
}spin_lock;

static inline void
spinlock_init(spin_lock *lock) 
{
	lock->lock = 0;
}

static inline void
spinlock_lock(spin_lock *lock) 
{
	while (__sync_lock_test_and_set(&lock->lock,1)) {}
}

static inline int
spinlock_trylock(spin_lock *lock) 
{
	return __sync_lock_test_and_set(&lock->lock,1) == 0;
}

static inline void
spinlock_unlock(spin_lock *lock) 
{
	__sync_lock_release(&lock->lock);
}

static inline void
spinlock_destory(spin_lock *lock) 
{
	;
}   

#else  //mutex_lock
	
#include <pthread.h>

typedef struct _spin_lock 
{
	pthread_mutex_t lock;
}spin_lock;

static inline void
spinlock_init(spin_lock *lock) {
	pthread_mutex_init(&lock->lock, NULL);
	//printf("use mutex_lock \n");
}

static inline void
spinlock_lock(spin_lock *lock) {
	pthread_mutex_lock(&lock->lock);
}

static inline int
spinlock_trylock(spin_lock *lock) {
	return pthread_mutex_trylock(&lock->lock) == 0;
}

static inline void
spinlock_unlock(spin_lock *lock) {
	pthread_mutex_unlock(&lock->lock);
}

static inline void
spinlock_destroy(spin_lock *lock) {
	pthread_mutex_destroy(&lock->lock);
}

#endif

#endif
