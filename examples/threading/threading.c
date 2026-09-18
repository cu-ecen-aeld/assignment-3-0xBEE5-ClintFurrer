#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void wait_ms(int msWait)
{
    struct timespec my_wait;

    my_wait.tv_sec = msWait / 1000;
    my_wait.tv_nsec = (msWait % 1000) * 1000000L;
    nanosleep(&my_wait, NULL);
}

void* threadfunc(void* thread_param)
{
    struct thread_data* data = (struct thread_data *) thread_param;


    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    void *return_Val;
    struct thread_data *worker_data = malloc(sizeof(struct thread_data));

    worker_data->obtain_wait = wait_to_obtain_ms;
    worker_data->release_wait = wait_to_release_ms;
    worker_data->thread_complete_success = false;
    worker_data->my_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    int status = pthread_create(thread, NULL, threadfunc, worker_data);

    if (status != 0)
    {
        ERROR_LOG("thread create has failed with a error %d", status);
        return false;
    }

    pthread_join(thread, &return_Val);

    if (return_Val != NULL)
    {

    }
    else
    {
        ERROR_LOG("worker thread did not return vaild data...Something has gone poorly :("); 
    }
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    free(worker_data);
    return false;
}

