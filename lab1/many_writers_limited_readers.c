//
// Created by @Zhylkaaa on 26/10/2020.
//

#include "many_writers_limited_readers.h"

#define READER_TURNS 15
#define READERS_COUNT 5
#define WRITER_TURNS 10
#define BUFFER_SIZE 3
#define WRITERS_COUNT 5

rk_sema read_sem[BUFFER_SIZE];
rk_sema write_sem[BUFFER_SIZE];
pthread_mutex_t acquisition_mutex[BUFFER_SIZE];
int readers_counter[BUFFER_SIZE];
int messages[BUFFER_SIZE];

//returns random amount of time from [1, max]
int GetRandomNumber(int max){
    return (rand() % max) + 1;
}

// Writer thread function
int Writer(void* data) {
    int i, buff_id, success;
    int threadId = *(int*) data;

    for (i = 0; i < WRITER_TURNS; i++) {
        success=0;
        // find unused buffer
        while(!success){
            for(buff_id = 0;buff_id<BUFFER_SIZE;buff_id++){
                if(rk_sema_trywait(&write_sem[buff_id]) == 0){
                    // Write
                    printf("(W) Writer %d started writing to buffer %d...\n", threadId, buff_id);
                    fflush(stdout);
                    messages[buff_id]++;
                    usleep(GetRandomNumber(800));
                    printf("(W) Writer %d finished writing to buffer %d msg %d\n", threadId, buff_id, messages[buff_id]);
                    fflush(stdout);
                    CHECK_ERROR(rk_sema_post(&write_sem[buff_id]), "unlocking mutex")
                    success = 1;
                    break;
                }
            }
        }

        // Think, think, think, think
        usleep(GetRandomNumber(1000));
    }

    return 0;
}

// Reader thread function
int Reader(void* data) {
    int i, buff_id, success;
    int threadId = *(int*) data;

    for (i = 0; i < READER_TURNS; i++) {
        success = 0;
        while(!success){
            for(buff_id=0;buff_id<BUFFER_SIZE;buff_id++){
                if(rk_sema_trywait(&read_sem[buff_id]) == 0){
                    CHECK_ERROR(pthread_mutex_lock(&acquisition_mutex[i]), "locking acquisition mutex")
                    readers_counter[buff_id]++;
                    if(readers_counter[buff_id] == 1){
                        if(rk_sema_trywait(&write_sem[buff_id]) != 0){
                            readers_counter[buff_id]--;
                            CHECK_ERROR(rk_sema_post(&read_sem[buff_id]), "posting read semaphore")
                            CHECK_ERROR(pthread_mutex_unlock(&acquisition_mutex[i]), "unlocking acquisition mutex")
                            continue;
                        }
                    }
                    CHECK_ERROR(pthread_mutex_unlock(&acquisition_mutex[i]), "unlocking acquisition mutex")

                    // Read
                    printf("(R) Reader %d started reading buffer %d...\n", threadId, buff_id);
                    fflush(stdout);
                    // Read, read, read...
                    usleep(GetRandomNumber(200));
                    printf("(R) Reader %d finished reading buffer %d: msg %d\n", threadId, buff_id, messages[buff_id]);
                    fflush(stdout);

                    CHECK_ERROR(pthread_mutex_lock(&acquisition_mutex[i]), "locking acquisition mutex")
                    readers_counter[buff_id]--;
                    if(readers_counter[buff_id] == 0){
                        CHECK_ERROR(rk_sema_post(&write_sem[buff_id]), "unlocking write semaphore")
                    }
                    CHECK_ERROR(pthread_mutex_unlock(&acquisition_mutex[i]), "unlocking acquisition mutex")
                    CHECK_ERROR(rk_sema_post(&read_sem[buff_id]), "posting read semaphore")
                    success = 1;
                    break;
                }
            }
        }

        usleep(GetRandomNumber(800));
    }

    free(data);

    return 0;
}

int main(int argc, char* argv[])
{
    srand(100005);

    pthread_t writerThreads[WRITERS_COUNT];
    pthread_t readerThreads[READERS_COUNT];

    int i, rc, readers_limit;

    // init semaphores and mutexes
    for(i = 0;i<BUFFER_SIZE;i++){
        readers_limit = GetRandomNumber(READERS_COUNT);
        rk_sema_init(&read_sem[i], readers_limit);
        rk_sema_init(&write_sem[i], 1);
        printf("(I) semaphore %d have readers limit %d\n", i, readers_limit);
        CHECK_ERROR(pthread_mutex_init(&acquisition_mutex[i], NULL), "initializing acquisition mutex")
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

    // releasing mutexes and semaphores
    for(i=0;i<BUFFER_SIZE;i++){
        CHECK_ERROR(pthread_mutex_destroy(&acquisition_mutex[i]), "destroying acquisition mutex")
        rk_sema_destroy(&read_sem[i]);
        rk_sema_destroy(&write_sem[i]);
    }

    return 0;
}
