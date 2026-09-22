#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdint.h> 

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)
//wait function
void wait_ms(int msWait)
{
    struct timespec my_wait;

    my_wait.tv_sec = msWait / 1000;
    my_wait.tv_nsec = (msWait % 1000) * 1000000L;
    nanosleep(&my_wait, NULL);
}

void* threadfunc(void* thread_param)
{
    int mutex_status;
    struct thread_data* data = (struct thread_data *) thread_param;
    DEBUG_LOG("Hello this is worker thread!");
    data->thread = pthread_self();
    wait_ms(data->obtain_wait);

    mutex_status = pthread_mutex_lock(data->my_mutex); 
    
    DEBUG_LOG("Worker: mutex status is %d", mutex_status);
    DEBUG_LOG("Worker: Mutex is locked");
    
    wait_ms(data->release_wait);
    pthread_mutex_unlock(data->my_mutex);
    data->thread_complete_success = true;
    DEBUG_LOG("Worker: Mutex is unlocked");

    DEBUG_LOG("Worker: returning");
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data *worker_data = malloc(sizeof(struct thread_data)); //allocate memory
    DEBUG_LOG("Main: Is setting up the worker struct vars :)");
    worker_data->obtain_wait = wait_to_obtain_ms;
    worker_data->release_wait = wait_to_release_ms;
    worker_data->thread_complete_success = false;
    worker_data->my_mutex = mutex;

    DEBUG_LOG("Main: creating the worker thread now...");
    int status = pthread_create(thread, NULL, threadfunc, worker_data); //start worker
    //check for errors
    if (status != 0)
    {
        ERROR_LOG("thread create has failed with a error %d", status);
        return false;
    }
    else
    {
        status = true;
    }

    DEBUG_LOG("Main: Is leaving the building :D");
    return status;
}
