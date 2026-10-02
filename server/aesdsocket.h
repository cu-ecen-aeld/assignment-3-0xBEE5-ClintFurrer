/**
 * By Clint Furrer
 */

#ifndef AESDSOCKET_H
#define AESDSOCKET_H

#include <stdbool.h>
#include <pthread.h>
#include "queue.h"

struct thread_data{
    pthread_mutex_t *my_mutex; //muxtex for file IO
    pthread_t thread; //track the tread id so it can be joined
    int sock_conn_id;
    int file_id;
    char *dataBuff;
    bool thread_complete_success; //track if the tread is done
};

typedef struct slist_data_s slist_data_t;
struct slist_data_s{
    struct thread_data *thread_d_ptr; 
    SLIST_ENTRY(slist_data_s) entries;
};

#endif /* MY_HEADER_H */
