//
// Created by @Zhylkaaa on 27/10/2020.
//

#include "readers_writers_critic.h"

#define READER_TURNS 7
#define READERS_COUNT 5
#define WRITERS_COUNT 5
#define WRITER_TURNS 7

rk_sema read_sem;
rk_sema write_sem;
pthread_mutex_t critic_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t critic_cond = PTHREAD_COND_INITIALIZER;

int num_readers = 0;
int num_writers = 0;

//returns random amount of time from [1, max]
int GetRandomNumber(int max){
    return (rand() % max) + 1;
}

// Writer thread function
int Writer(void* data) {
    int i;
    int threadId = *(int*)data;

    for (i = 0; i < WRITER_TURNS; i++) {
        CHECK_ERROR(rk_sema_wait(&write_sem), "locking write semaphore")

        // Write
        printf("(W) Writer %d started writing...\n", threadId);
        fflush(stdout);
        usleep(GetRandomNumber(800));
        printf("(W) finished\n");
        fflush(stdout);

        CHECK_ERROR(rk_sema_post(&write_sem), "unlocking write semaphore")

        // Think, think, think, think
        usleep(GetRandomNumber(1000));

        if(i == 0){
            CHECK_ERROR(pthread_mutex_lock(&critic_mutex), "locking critic mutex")
            num_writers++;
            if(num_writers == WRITERS_COUNT) {
                CHECK_ERROR(pthread_cond_signal(&critic_cond), "signaling critic")
            }
            CHECK_ERROR(pthread_mutex_unlock(&critic_mutex), "unlocking critic mutex")
        }
    }

    free(data);
    return 0;
}

// Writer thread function
int Critic(void* data) {

    CHECK_ERROR(pthread_mutex_lock(&critic_mutex), "locking critic mutex")

    while(num_writers < WRITERS_COUNT){
        CHECK_ERROR(pthread_cond_wait(&critic_cond, &critic_mutex), "waiting critic condition")
    }

    CHECK_ERROR(rk_sema_wait(&write_sem), "locking write semaphore")

    printf("(C) Critic started criticizing...\n");
    fflush(stdout);
    usleep(GetRandomNumber(800));
    printf("(C) Critic finished criticizing\n");
    fflush(stdout);

    CHECK_ERROR(rk_sema_post(&write_sem), "unlocking write semaphore")
    CHECK_ERROR(pthread_mutex_unlock(&critic_mutex), "unlocking critic mutex")

    return 0;
}

// Reader thread function
int Reader(void* data) {
    int i;
    int threadId = *(int*) data;

    for (i = 0; i < READER_TURNS; i++) {
        CHECK_ERROR(rk_sema_wait(&read_sem), "locking read semaphore")
        num_readers++;
        if(num_readers == 1){
            CHECK_ERROR(rk_sema_wait(&write_sem), "locking write semaphore")
        }
        CHECK_ERROR(rk_sema_post(&read_sem), "unlocking read semaphore")

        // Read
        printf("(R) Reader %d started reading...\n", threadId);
        fflush(stdout);
        // Read, read, read...
        usleep(GetRandomNumber(200));
        printf("(R) Reader %d finished reading\n", threadId);
        fflush(stdout);

        CHECK_ERROR(rk_sema_wait(&read_sem), "locking read semaphore")
        num_readers--;
        if(num_readers == 0){
            CHECK_ERROR(rk_sema_post(&write_sem), "unlocking write semaphore")
        }
        CHECK_ERROR(rk_sema_post(&read_sem), "unlocking read semaphore")

        usleep(GetRandomNumber(800));
    }

    free(data);

    return 0;
}

int main(int argc, char* argv[])
{
    srand(100005);


    // init semaphores
    rk_sema_init(&read_sem, 1);
    rk_sema_init(&write_sem, 1);

    pthread_t writerThreads[WRITERS_COUNT];
    pthread_t readerThreads[READERS_COUNT];
    pthread_t criticThread;

    int i,rc;

    rc = pthread_create(
            &criticThread,
            NULL,
            (void*) Critic,
            NULL);

    if (rc != 0)
    {
        fprintf(stderr,"Couldn't create the critic thread");
        exit (-1);
    }

    // Create the Writer threads
    for (i = 0; i < WRITERS_COUNT; i++) {
        // Reader initialization - takes random amount of time
        usleep(GetRandomNumber(100));
        int* threadId = malloc(sizeof(int));
        *threadId = i;
        rc = pthread_create(
                &writerThreads[i], // thread identifier
                NULL,              // thread attributes
                (void*) Writer,    // thread function
                (void*) threadId);     // thread function argument

        if (rc != 0)
        {
            fprintf(stderr,"Couldn't create the writer threads");
            exit (-1);
        }
    }


    // Create the Reader threads
    for (i = 0; i < READERS_COUNT; i++) {
        // Reader initialization - takes random amount of time
        usleep(GetRandomNumber(1000));
        int* threadId = malloc(sizeof(int));
        *threadId = i;
        rc = pthread_create(
                &readerThreads[i], // thread identifier
                NULL,              // thread attributes
                (void*) Reader,    // thread function
                (void*) threadId);     // thread function argument

        if (rc != 0)
        {
            fprintf(stderr,"Couldn't create the reader threads");
            exit (-1);
        }
    }

    // At this point, the readers and writers should perform their operations

    // Wait for the Readers
    for (i = 0; i < READERS_COUNT; i++)
        pthread_join(readerThreads[i],NULL);

    // Wait for the Writers
    for (i = 0; i < WRITERS_COUNT; i++)
        pthread_join(writerThreads[i],NULL);

    // Wait for the Critic
    pthread_join(criticThread,NULL);

    // destroy semaphores
    rk_sema_destroy(&read_sem);
    rk_sema_destroy(&write_sem);

    return 0;
}
