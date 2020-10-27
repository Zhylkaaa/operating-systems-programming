//
// Created by @Zhylkaaa on 25/10/2020.
//

#include "many_readers_many_writers_buffer.h"

#define READER_TURNS 15
#define READERS_COUNT 5
#define WRITER_TURNS 10
#define WRITERS_COUNT 3
#define BUFFER_MAX_SIZE 5

typedef struct message {
    int i;
    struct message* next_message;
} message;

typedef struct messages_buffer {
    int max_size;
    int current_size;
    message* head;
    message* tail;
} messages_buffer;

messages_buffer buffer;

rk_sema head_modify_sem;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t read_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t null_head_cond = PTHREAD_COND_INITIALIZER;

int num_readers = 0;
int msg_id = 0;

//returns random amount of time from [1, max]
int GetRandomNumber(int max){
    return (rand() % max) + 1;
}

void write_to_buffer(int writer_id){
    message* msg = (message*) calloc(1, sizeof(message));
    msg->i = msg_id;
    msg->next_message = NULL;
    msg_id++;

    printf("(W) Writer %d started writing message %d...\n", writer_id, msg->i);
    fflush(stdout);
    usleep(GetRandomNumber(800));

    if(buffer.head == NULL){
        CHECK_ERROR(rk_sema_wait(&head_modify_sem), "locking write semaphore")
        // Write
        buffer.head = buffer.tail = msg;
        CHECK_ERROR(rk_sema_post(&head_modify_sem), "unlocking write semaphore")
        pthread_cond_signal(&null_head_cond);
    } else {
        buffer.tail->next_message = msg;
        buffer.tail = msg;
    }
    buffer.current_size += 1;
    printf("(W) Writer %d finished writing message %d (buffer status %d/%d)\n", writer_id, msg->i, buffer.current_size, buffer.max_size);
    fflush(stdout);
}

// Writer thread function
int Writer(void* data) {
    int threadId = *(int*) data;
    int i;

    for (i = 0; i < WRITER_TURNS; i++) {
        CHECK_ERROR(pthread_mutex_lock(&buffer_mutex), "locking buffer mutex")

        while(buffer.current_size == buffer.max_size){
            pthread_cond_wait(&full_cond, &buffer_mutex);
        }

        write_to_buffer(threadId);

        CHECK_ERROR(pthread_mutex_unlock(&buffer_mutex), "unlocking buffer mutex")

        // Think, think, think, think
        usleep(GetRandomNumber(1000));
    }

    return 0;
}

// Reader thread function
int Reader(void* data) {
    int i;
    int threadId = *(int*) data;

    for (i = 0; i < READER_TURNS;) {
        CHECK_ERROR(pthread_mutex_lock(&read_mutex), "locking read mutex")
        num_readers++;
        if(num_readers == 1){
            CHECK_ERROR(rk_sema_wait(&head_modify_sem), "locking write semaphore")
        }
        CHECK_ERROR(pthread_mutex_unlock(&read_mutex), "unlocking read mutex")

        if(buffer.head != NULL){
            // Read
            printf("(R) Reader %d started reading message %d...\n", threadId, buffer.head->i);
            fflush(stdout);
            // Read, read, read...
            usleep(GetRandomNumber(100));
            printf("(R) Reader %d finished reading message %d\n", threadId, buffer.head->i);
            i++;
        }

        CHECK_ERROR(pthread_mutex_lock(&read_mutex), "locking read mutex")
        num_readers--;
        if(num_readers == 0){
            CHECK_ERROR(rk_sema_post(&head_modify_sem), "unlocking write semaphore")
        }
        CHECK_ERROR(pthread_mutex_unlock(&read_mutex), "unlocking read semaphore")

        usleep(GetRandomNumber(1500));
    }

    free(data);

    return 0;
}


// Buffer thread function
int Buffer(void* data){
    usleep(GetRandomNumber(500));
    for(int i = 0;i<WRITER_TURNS*WRITERS_COUNT;){
        // wait before removing element from messages_buffer
        usleep(GetRandomNumber(500));
        CHECK_ERROR(pthread_mutex_lock(&buffer_mutex), "locking buffer mutex")
        CHECK_ERROR(rk_sema_wait(&head_modify_sem), "locking write semaphore")

        if(buffer.head != NULL) {
            i++;
            printf("(B) Removing message %d\n", buffer.head->i);
            fflush(stdout);

            message *prev_head = buffer.head;
            buffer.head = buffer.head->next_message;
            if (buffer.head == NULL)buffer.tail = NULL;

            free(prev_head);

            buffer.current_size -= 1;
            CHECK_ERROR(pthread_mutex_unlock(&buffer_mutex), "unlocking buffer mutex")
            CHECK_ERROR(rk_sema_post(&head_modify_sem), "unlocking write semaphore")
            pthread_cond_signal(&full_cond);
        } else {
            CHECK_ERROR(pthread_mutex_unlock(&buffer_mutex), "unlocking buffer mutex")
            CHECK_ERROR(rk_sema_post(&head_modify_sem), "unlocking write semaphore")
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    srand(100005);


    // semaphores initialization
    rk_sema_init(&head_modify_sem, 1);

    pthread_t writerThreads[WRITERS_COUNT];
    pthread_t readerThreads[READERS_COUNT];
    pthread_t bufferThread;

    int i,rc;

    // Initialize buffer
    buffer.current_size = 0;
    buffer.max_size = BUFFER_MAX_SIZE;
    buffer.head = NULL;
    buffer.tail = NULL;

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
            fprintf(stderr,"Couldn't create the reader threads");
            exit (-1);
        }
    }

    rc = pthread_create(
            &bufferThread, // thread identifier
            NULL,              // thread attributes
            (void*) Buffer,    // thread function
            NULL);     // thread function argument

    if (rc != 0)
    {
        fprintf(stderr,"Couldn't create the reader threads");
        exit (-1);
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

    pthread_join(bufferThread,NULL);
    /// Destroy semaphores
    rk_sema_destroy(&head_modify_sem);
    ////

    return 0;
}

