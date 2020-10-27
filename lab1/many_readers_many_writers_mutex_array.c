//
// Created by @Zhylkaaa on 26/10/2020.
//

#include "many_readers_many_writers_mutex_array.h"

#define READER_TURNS 10
#define READERS_COUNT 5
#define WRITER_TURNS 10
#define BUFFER_SIZE 5
#define WRITER_TURNS 10
#define WRITERS_COUNT 3

pthread_mutex_t mutex_array[BUFFER_SIZE];
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
                if(pthread_mutex_trylock(&mutex_array[buff_id]) == 0){
                    // Write
                    printf("(W) Writer %d started writing to buffer %d...\n", threadId, buff_id);
                    fflush(stdout);
                    messages[buff_id]++;
                    usleep(GetRandomNumber(800));
                    printf("(W) Writer %d finished writing to buffer %d msg %d\n", threadId, buff_id, messages[buff_id]);
                    fflush(stdout);
                    success = 1;
                    CHECK_ERROR(pthread_mutex_unlock(&mutex_array[buff_id]), "unlocking mutex")
                    break;
                }
            }
        }

        // Think, think, think, think
        usleep(GetRandomNumber(1000));
    }

    free(data);

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
                if(pthread_mutex_trylock(&mutex_array[buff_id]) == 0){
                    // Read
                    printf("(R) Reader %d started reading buffer %d...\n", threadId, buff_id);
                    fflush(stdout);
                    // Read, read, read...
                    usleep(GetRandomNumber(200));
                    printf("(R) Reader %d finished reading buffer %d: msg %d\n", threadId, buff_id, messages[buff_id]);
                    fflush(stdout);
                    success = 1;
                    CHECK_ERROR(pthread_mutex_unlock(&mutex_array[buff_id]), "unlocking mutex")
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

    int i,rc;

    for(i=0;i<BUFFER_SIZE;i++){
        CHECK_ERROR(pthread_mutex_init(&mutex_array[i], NULL), "initializing mutex")
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

    for(i=0;i<BUFFER_SIZE;i++){
        CHECK_ERROR(pthread_mutex_destroy(&mutex_array[i]), "destroying mutex")
    }

    return 0;
}
