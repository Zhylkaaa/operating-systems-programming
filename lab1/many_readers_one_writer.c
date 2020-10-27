#include "many_readers_one_writer.h"

#define READER_TURNS 10
#define READERS_COUNT 5
#define WRITER_TURNS 10

rk_sema read_sem;
rk_sema write_sem;
int num_readers = 0;

//returns random amount of time from [1, max]
int GetRandomNumber(int max){
    return (rand() % max) + 1;
}

// Writer thread function
int Writer(void* data) {
    int i;

    for (i = 0; i < WRITER_TURNS; i++) {
        CHECK_ERROR(rk_sema_wait(&write_sem), "locking write")

        // Write
        printf("(W) Writer started writing...\n");
        fflush(stdout);
        usleep(GetRandomNumber(800));
        printf("(W) finished\n");

        CHECK_ERROR(rk_sema_post(&write_sem), "unlocking write")

        // Think, think, think, think
        usleep(GetRandomNumber(1000));
    }

    return 0;
}

// Reader thread function
int Reader(void* data) {
    int i;
    int threadId = *(int*) data;

    for (i = 0; i < READER_TURNS; i++) {
        CHECK_ERROR(rk_sema_wait(&read_sem), "locking read")
        num_readers++;
        if(num_readers == 1){
            CHECK_ERROR(rk_sema_wait(&write_sem), "locking write")
        }
        CHECK_ERROR(rk_sema_post(&read_sem), "unlocking read")

        // Read
        printf("(R) Reader %d started reading...\n", threadId);
        fflush(stdout);
        // Read, read, read...
        usleep(GetRandomNumber(200));
        printf("Reader %d finished reading\n", threadId);

        CHECK_ERROR(rk_sema_wait(&read_sem), "locking read")
        num_readers--;
        if(num_readers == 0){
            CHECK_ERROR(rk_sema_post(&write_sem), "unlocking write")
        }
        CHECK_ERROR(rk_sema_post(&read_sem), "unlocking read")

        usleep(GetRandomNumber(800));
    }

    free(data);

    return 0;
}

int main(int argc, char* argv[])
{
    srand(100005);


    /// INITIALIZE SEMAPHORES

    rk_sema_init(&read_sem, 1);
    rk_sema_init(&write_sem, 1);

    pthread_t writerThread;
    pthread_t readerThreads[READERS_COUNT];
    ///

    int i,rc;

    // Create the Writer thread
    rc = pthread_create(
            &writerThread,  // thread identifier
            NULL,           // thread attributes
            (void*) Writer, // thread function
            (void*) NULL);  // thread function argument

    if (rc != 0)
    {
        fprintf(stderr,"Couldn't create the writer thread");
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

    // Wait for the Writer
    pthread_join(writerThread, NULL);

    /// Destroy semaphores
    rk_sema_destroy(&read_sem);
    rk_sema_destroy(&write_sem);
    ////

    return 0;
}
