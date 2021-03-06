//
// Created by @Zhylkaaa on 25/10/2020.
//

#ifndef SYSTEMPROGRAMMING_MANY_READERS_ONE_WRITER_H
#define SYSTEMPROGRAMMING_MANY_READERS_ONE_WRITER_H
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <zconf.h>

#define CHECK_ERROR(ret_code, action) {if(ret_code == -1){fprintf(stderr, "Error occured during %s semaphore.\n", action); exit (-1);}}

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

typedef struct rk_sema {
#ifdef __APPLE__
    dispatch_semaphore_t    sem;
#else
    sem_t                   sem;
#endif
} rk_sema;


static inline int rk_sema_init(struct rk_sema *s, uint32_t value){
#ifdef __APPLE__
    dispatch_semaphore_t *sem = &s->sem;

    *sem = dispatch_semaphore_create(value);
    return *sem == NULL;
#else
    return sem_init(&s->sem, 0, value);
#endif
}

static inline int rk_sema_wait(struct rk_sema *s){
#ifdef __APPLE__
    dispatch_semaphore_wait(s->sem, DISPATCH_TIME_FOREVER);
    return 0;
#else
    return sem_wait(&s->sem)
#endif
}

static inline int rk_sema_post(struct rk_sema *s) {
#ifdef __APPLE__
    dispatch_semaphore_signal(s->sem);
    return 0;
#else
    return sem_post(&s->sem);
#endif
}

static inline void rk_sema_destroy(struct rk_sema *s){
#ifdef __APPLE__
    dispatch_release(s->sem);
#else
    sem_destroy(&s->sem);
#endif
}

#endif //SYSTEMPROGRAMMING_MANY_READERS_ONE_WRITER_H
