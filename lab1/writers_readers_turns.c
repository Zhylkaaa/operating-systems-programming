//
// Created by @Zhylkaaa on 27/10/2020.
//

#include "writers_readers_turns.h"

#define READER_TURNS 12
#define READERS_COUNT 5
#define WRITER_TURNS 12
#define WRITERS_COUNT 5
#define BUFFER_SIZE 3

pthread_mutex_t buffer_modification_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full_buffer_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty_buffer_cond = PTHREAD_COND_INITIALIZER;

typedef struct message {
    int i;
    struct message* next_message;
} message;

typedef struct message_buffer {
    int current_size;
    int max_size;
    message* head;
} message_buffer;

message_buffer buffer;

int message_id = 0;
int read_state = 0;

//returns random amount of time from [1, max]
int GetRandomNumber(int max){
    return (rand() % max) + 1;
}

int Writer(void* data){
    int i;
    int threadId = *(int*) data;
    message* msg;

    for (i = 0; i < WRITER_TURNS; i++) {
        CHECK_ERROR(pthread_mutex_lock(&buffer_modification_mutex), "locking mutex")

        if(!read_state && buffer.current_size == buffer.max_size){
            read_state = 1;
            CHECK_ERROR(pthread_cond_broadcast(&full_buffer_cond), "broadcasting full condition")
        }

        while(read_state){
            CHECK_ERROR(pthread_cond_wait(&empty_buffer_cond, &buffer_modification_mutex), "waiting for empty condition")
        }

        if(!read_state && buffer.current_size == buffer.max_size - 1){
            read_state = 1;
            CHECK_ERROR(pthread_cond_broadcast(&full_buffer_cond), "broadcasting full condition")
        }

        printf("(W%d) Writer %d started writing message...\n", i, threadId);
        fflush(stdout);
        msg = (message*) calloc(1, sizeof(message));
        msg->i = message_id++;
        msg->next_message = buffer.head;
        buffer.head = msg;
        buffer.current_size++;

        usleep(GetRandomNumber(800));
        printf("(W%d) Writer %d finished writing message %d\n", i, threadId, msg->i);
        fflush(stdout);

        CHECK_ERROR(pthread_mutex_unlock(&buffer_modification_mutex), "unlocking mutex")

        // Think, think, think, think
        usleep(GetRandomNumber(1000));
    }

    free(data);
    return 0;
}

int Reader(void* data){
    int i;
    int threadId = *(int*) data;
    message* msg;

    for (i = 0; i < READER_TURNS; i++) {
        CHECK_ERROR(pthread_mutex_lock(&buffer_modification_mutex), "locking mutex")
        if(read_state && buffer.current_size == 0){
            read_state = 0;
            CHECK_ERROR(pthread_cond_broadcast(&empty_buffer_cond), "broadcasting empty condition")
        }

        while(!read_state){
            CHECK_ERROR(pthread_cond_wait(&full_buffer_cond, &buffer_modification_mutex), "waiting for full condition")
        }

        if(read_state && buffer.current_size == 1){
            read_state = 0;
            CHECK_ERROR(pthread_cond_broadcast(&empty_buffer_cond), "broadcasting empty condition")
        }

        msg = buffer.head;
        buffer.head = buffer.head->next_message;
        buffer.current_size--;

        // Read
        printf("(R%d) Reader %d started reading...\n", i, threadId);
        fflush(stdout);
        // Read, read, read...
        usleep(GetRandomNumber(200));
        printf("(R%d) Reader %d finished reading: message %d...\n", i, threadId, msg->i);
        fflush(stdout);

        CHECK_ERROR(pthread_mutex_unlock(&buffer_modification_mutex), "unlocking mutex")

        free(msg);

        usleep(GetRandomNumber(800));
    }

    free(data);
    return 0;
}

int main(int argc, char* argv[]){
    srand(100005);

    int i, rc;

    pthread_t writerThreads[WRITERS_COUNT];
    pthread_t readerThreads[READERS_COUNT];

    buffer.current_size = 0;
    buffer.max_size = BUFFER_SIZE;
    buffer.head = NULL;

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
        usleep(GetRandomNumber(200));
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

    // Wait for the Readers
    for (i = 0; i < READERS_COUNT; i++)
        pthread_join(readerThreads[i],NULL);

    // Wait for the Writers
    for (i = 0; i < WRITERS_COUNT; i++)
        pthread_join(writerThreads[i],NULL);

    return 0;
}