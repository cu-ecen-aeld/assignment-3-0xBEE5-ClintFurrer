/**
 * By Clint Furrer
 */

#ifndef AESDSOCKET_H
#define AESDSOCKET_H

#include <stdbool.h>
#include <pthread.h>

struct thread_data{
    pthread_mutex_t *my_mutex; //muxtex for file IO
    pthread_t thread; //track the tread id so it can be joined
    int sock_conn_id;
    int file_id;
    char *dataBuff;
    bool thread_complete_success; //track if the tread is done
};

#endif /* MY_HEADER_H */
