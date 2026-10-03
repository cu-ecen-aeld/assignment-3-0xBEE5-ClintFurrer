/**
 * Clint Furrer
 * Created on: 9/25/26
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>   // For write() and close()
#include <sys/stat.h>
#include <sys/types.h>
#include <netdb.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>


#include "aesdsocket.h"
#include "queue.h"

#define PORT_num "9000"

volatile sig_atomic_t signal_received = 0;
volatile sig_atomic_t alarm_sig_received = 0;
volatile sig_atomic_t sigtype = 0;
void handle_signal(int sig) 
{
    signal_received = 1; //set signal flag
    sigtype = sig;
    /*
    if (sig == SIGINT)
    {
        syslog(LOG_DEBUG, "Caugth SIGINT Signal\n");
    }
    if (sig == SIGTERM)
    {
        syslog(LOG_DEBUG, "Caugth SIGTERM Signal\n");
    } */
}

void alarm_handler(int sig)
{
    alarm_sig_received = 1;
    sigtype = sig;
    //if (sig == SIGALRM)
    //{
    //    syslog(LOG_DEBUG, "Caugth SIGALRM Signal\n");
    //}
}

void* socket_WrkBee(void* thread_param)
{
    struct thread_data* data = (struct thread_data *) thread_param;
    int TX_RX_FLG = 0;
    int in_cnt;
    int mutex_status;
    ssize_t ret;
    ssize_t bytes_read;

    //data->thread = pthread_self();

    do //read client data stream
    {   //read the incoming data stream
        in_cnt = recv(data->sock_conn_id, data->dataBuff, data->buffLen - 1, 0);
        if (in_cnt <= 0)
        {
            break; //client disconnectd or socket error
        }
        mutex_status = pthread_mutex_lock(data->my_mutex); 
        if (mutex_status != 0)
        {

        }
        ret = write(data->file_id, data->dataBuff, in_cnt); //start writing the data stream to file
        pthread_mutex_unlock(data->my_mutex);
        if (ret == -1)
        {
            syslog(LOG_ERR, "File write failed :( %d\n", errno);
        }      
        char *first_n = memchr(data->dataBuff, '\n', in_cnt); //look for the new line packet terminator 
        if (first_n != NULL)
        {
            syslog(LOG_DEBUG,"new line dectected\n");
            TX_RX_FLG = 1; //found termination of data stream
        }            
    } while (TX_RX_FLG != 1);
    mutex_status = pthread_mutex_lock(data->my_mutex); 
    fsync(data->file_id); //force data to file
    lseek(data->file_id, 0, SEEK_SET); //set to the beginning of file
    
    TX_RX_FLG = 0; //reset FLG
    do //send loop of receieved client data
    {   //read data back out of the file
        //mutex_status = pthread_mutex_lock(data->my_mutex); 
        bytes_read = read(data->file_id, data->dataBuff, data->buffLen);
        //pthread_mutex_unlock(data->my_mutex);
        if (bytes_read == -1)
        {
            syslog(LOG_ERR, "Error reading file back to client: %d\n", errno);
            TX_RX_FLG = 1;
            break;
        }
        if (bytes_read == 0) //end of file
        {
            syslog(LOG_DEBUG,"hit read 0 confirm %ld\n", bytes_read);
            TX_RX_FLG = 1; //exit
            break;
        }
        syslog(LOG_DEBUG,"Sending! data back\n");
        send(data->sock_conn_id, data->dataBuff, bytes_read, 0);
    } while (TX_RX_FLG != 1);
    pthread_mutex_unlock(data->my_mutex);
    //log debug messages
    syslog(LOG_DEBUG,"out of do while\n");
    syslog(LOG_DEBUG,"sig FLAG status %d\n", signal_received);
    close(data->sock_conn_id); //almost done here close socket connection
    
    //free(data->dataBuff); //free the data buffer
    data->thread_complete_success = 1;
    return NULL;
}

void* timestamper(void* thread_param)
{
    struct thread_data* data = (struct thread_data *) thread_param;
    int mutex_status;
    time_t t;
    struct tm my_time;
    char outstr[64];
    ssize_t ret;

    struct timespec ten_sec_delay;
    ten_sec_delay.tv_sec = 10;
    ten_sec_delay.tv_nsec = 0;

    while (signal_received == 0)
    {    
        nanosleep(&ten_sec_delay, NULL);

        if (signal_received != 0) //double check
        {
            break;
        }
        //pause();

        //if (alarm_sig_received == 1)
        //{
            //alarm_sig_received = 0;
            t = time(NULL);
            localtime_r(&t, &my_time);

            int status = strftime(outstr, sizeof(outstr), "timestamp:%Y-%m-%d %H:%M:%S\n", &my_time);
            if (status != 0)
            {
                syslog(LOG_ERR, "Error with timestamp..:(%d\n", errno);
            }

            if (sigtype == SIGALRM)
            {
                syslog(LOG_DEBUG, "Caugth SIGALRM Signal\n");
            }
            mutex_status = pthread_mutex_lock(data->my_mutex); 
            if (mutex_status != 0)
            {
                syslog(LOG_DEBUG, "Time stamp muxtex error %d\n", errno);
            }
            ret = write(data->file_id, outstr, strlen(outstr));
            if (ret == -1)
            {
                syslog(LOG_ERR, "File write failed :( %d\n", errno);
            } 
            pthread_mutex_unlock(data->my_mutex);

            printf("%s\n", outstr);
        //}
    }
    data->thread_complete_success = 1;
    return NULL;
}

int main(int argc, char *argv[])
{
    struct sigaction mysig;
    //struct sigaction alarmsig;
    struct addrinfo hints, *servinfo, *ptr;
    struct sockaddr_storage client_addr;
    //struct itimerval delay;
    socklen_t s_in_size;
    pid_t pid;
    slist_data_t *next_node = NULL;
    slist_data_t *prev = NULL;
    slist_data_t *datap = NULL;
    int addrinfo_status;
    int socket_fd;
    int conn_fd;
    int bind_status;
    int listen_status;
    int daemonFLG = 0;
    int timestampFLG = 0;
    int sock_bind_err_cnt = 0;
    int sigStatus;
    int yes=1;
    char ipaddr_buf[64];
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    SLIST_HEAD(slisthead, slist_data_s) head;
    SLIST_INIT(&head);

    //setup timer struct
    //delay.it_value.tv_sec = 10; //
    //delay.it_value.tv_usec = 0; 
    //delay.it_interval.tv_sec = 10; //10 seconds interval alarm
    //delay.it_interval.tv_usec = 0;
    //open logging 
    openlog(NULL, 0, LOG_USER); // open the logging
    printf("hello server\n");
    
    const char *prams = argv[1]; //copy input prams
    printf("input prams %s\n", prams); //debug
    if (prams != NULL)
    {
        int pramStatus = strcmp(prams, "-d"); //look for the param -d option
        if (pramStatus == 0)
        {
            daemonFLG = 1;
        }
    }
    printf("daemon FLAG status %d\n",daemonFLG); //debug
    memset(&mysig, 0, sizeof(struct sigaction)); //zero out
    memset(&hints, 0, sizeof hints); // zero out struct
    mysig.sa_handler = handle_signal;
    //alarmsig.sa_handler = alarm_handler;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // use my IP
    //set up the signals to catch sigterm and sinint
    sigStatus = sigaction(SIGTERM, &mysig, NULL);
    if (sigStatus != 0)
    {
        syslog(LOG_ERR, "signal SIGTERM failed to register :( %d\n", errno);
        return -1;        
    }
    sigStatus = sigaction(SIGINT, &mysig, NULL);
    if (sigStatus != 0)
    {
        syslog(LOG_ERR, "signal SIGINT failed to register :( %d\n", errno);
        return -1;        
    }    

    //sigemptyset(&alarmsig.sa_mask);
    //alarmsig.sa_flags = SA_RESTART; 
    //sigStatus = sigaction(SIGALRM, &alarmsig, NULL);    
    //if (sigStatus != 0)
    //{
    //    syslog(LOG_ERR, "signal SIGALRM failed to register :( %d\n", errno);
    //    return -1;        
    //} 

    addrinfo_status = getaddrinfo(NULL, PORT_num, &hints, &servinfo); //network helper function
    if (addrinfo_status != 0)
    {
        syslog(LOG_ERR, "get addr info function failed :( %d\n", errno);
        return -1;
    }
    //get a socket then bind to it. Also sets socket option to have kernel drop the connection and not hold it
    for(ptr = servinfo; ptr != NULL; ptr = ptr->ai_next)
    {
        void *addr;
        struct sockaddr_in *ipv4;
        ipv4 = (struct sockaddr_in *)ptr->ai_addr;
        addr = &(ipv4->sin_addr);
        inet_ntop(ptr->ai_family, addr, ipaddr_buf, sizeof ipaddr_buf);
        syslog(LOG_DEBUG,"My address :P %s\n", ipaddr_buf);
        //get the socket
        socket_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (socket_fd == -1)
        {
            int saved_errno = errno; //debug
            sock_bind_err_cnt++;
            syslog(LOG_ERR, "socket function call failed :( %d\n", saved_errno);
        }

        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        //bind to socket
        bind_status = bind(socket_fd, ptr->ai_addr, ptr->ai_addrlen);
        if (bind_status != 0)
        {
            sock_bind_err_cnt++;
            close(socket_fd);
            syslog(LOG_ERR, "could not bind to socket :( %d\n", errno);
        }
        break;
    }
    //check for any errors that happen with socket and bind
    if (ptr == NULL)
    {
        syslog(LOG_ERR, "Error count in socket and or bind :( %d\n", sock_bind_err_cnt);
        syslog(LOG_ERR, "Errors in socket or in bind :( %d\n", errno);
        return -1;
    }
    freeaddrinfo(servinfo); // all done with this structure

    if (daemonFLG == 1) //if daemon option set then execute this
    {
        pid = fork(); //make new process
        if (pid == -1)
        {
            syslog(LOG_ERR, "Error in fork could not make new process :( %d\n", errno);
            return -1;
        }
        //exit parent
        if (pid > 0)
        {
            exit(0);
        }
        int procGrpStat = setsid(); //make new process group and session
        if (procGrpStat == -1)
        {
            syslog(LOG_ERR, "Error setsid failed :( %d\n", errno);
            return -1;
        }
        pid = fork(); //second fork to ensure terminal disconnect and true back ground process
        if (pid < 0)
        {
            syslog(LOG_ERR, "Error in fork could not make second process :( %d\n", errno);
            exit(1);
        }
        if (pid > 0) //exit first child process
        {
            exit(0);
        }
        int chgdirStatus = chdir("/"); //change to root dir
        if (chgdirStatus == -1)
        {
            syslog(LOG_ERR, "Error failed change to root dir :( %d\n", errno);
            exit(1);
        }
        //close out the standard files
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        //redirect to null
        int fdclose = open("/dev/null", O_RDWR);
        if (fdclose != -1)
        {
            dup2(fdclose, STDIN_FILENO);
            dup2(fdclose, STDOUT_FILENO);
            dup2(fdclose, STDERR_FILENO);
            close(fdclose);
        }
    }

    // open the file if it does not exist create it
    int fd = open("/var/tmp/aesdsocketdata", O_RDWR | O_CREAT | O_APPEND, 0644); 
    if (fd == -1)
    {
        syslog(LOG_ERR, "File failed to open :( %d\n", errno);
        close(socket_fd);
        return -1;
    }

    printf("All set to get connections....waiting\n");
    syslog(LOG_DEBUG,"Starting and waiting for connections\n");
    //int timeStatus = setitimer(ITIMER_REAL, &delay, NULL);
    //if (timeStatus != 0)
    //{
    //    syslog(LOG_ERR, "timer failed to start :( %d\n", errno);
    //    return -1;
    //}
    //timestamper(fd, &file_mutex); //write inital timestamp to file

    listen_status = listen(socket_fd, 10);
    if (listen_status != 0)
    {
        syslog(LOG_ERR, "listen failed :( %d\n", errno);
        close(fd);
        close(socket_fd);
        return -1;
    }
    //run main loop to read and write
    while(signal_received == 0)
    {
        s_in_size = sizeof client_addr;
        conn_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &s_in_size); //accept an incoming client connection
        if (conn_fd == -1)
        {
            if (signal_received || errno == EINTR)
            {
                break;
            }
            syslog(LOG_ERR, "accept has failed :( %d\n", errno);
        }

        //get the client ip address and log it
        struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
        inet_ntop(AF_INET, &(s->sin_addr), ipaddr_buf, sizeof ipaddr_buf);
        syslog(LOG_DEBUG,"ip address connected %s\n", ipaddr_buf);

        //slist_data_t *tvar = NULL;
        struct thread_data *worker_data = malloc(sizeof(struct thread_data)); //allocate thread work bee memory
        /*slist_data_t * */datap = malloc(sizeof(slist_data_t)); //allocate list node
        char *buffer = malloc(512 * sizeof(char)); //allocate data buffer for socket data stream freed by worker bee
        worker_data->my_mutex = &file_mutex;
        worker_data->file_id = fd;
        worker_data->sock_conn_id = conn_fd;
        worker_data->dataBuff = buffer;
        worker_data->thread_complete_success = 0;
        worker_data->buffLen = 512;
        datap->thread_d_ptr = worker_data; //put data into node
        
        SLIST_INSERT_HEAD(&head, datap, entries);

        pthread_create(&worker_data->thread, NULL, socket_WrkBee, worker_data); //fly my worker bee!
        
        if (timestampFLG == 0)
        {
            timestampFLG = 1; //only needs to run once!
                    //slist_data_t *tvar = NULL;
            struct thread_data *worker_data = malloc(sizeof(struct thread_data)); //allocate thread work bee memory
            /*slist_data_t * */datap = malloc(sizeof(slist_data_t)); //allocate list node
            char *buffer = malloc(512 * sizeof(char)); //allocate data buffer for socket data stream freed by worker bee
            worker_data->my_mutex = &file_mutex;
            worker_data->file_id = fd;
            worker_data->sock_conn_id = conn_fd;
            worker_data->dataBuff = buffer;
            worker_data->thread_complete_success = 0;
            worker_data->buffLen = 512;
            datap->thread_d_ptr = worker_data; 

            SLIST_INSERT_HEAD(&head, datap, entries);

            pthread_create(&worker_data->thread, NULL, timestamper, worker_data); //off to timestamp the file
        }

        //SLIST_FOREACH_SAFE(datap, &head, entries, tvar)
        while(datap != NULL)
        {
            next_node = SLIST_NEXT(datap, entries);
            if (datap->thread_d_ptr->thread_complete_success == 1)
            {
                syslog(LOG_DEBUG, "Closed connection from %s\n", ipaddr_buf);
                if (datap == SLIST_FIRST(&head))
                {
                    SLIST_REMOVE_HEAD(&head, entries);
                }
                else
                {
                    // Bypass the deleted node completely
                    SLIST_NEXT(prev, entries) = next_node;                    
                }
                pthread_join(datap->thread_d_ptr->thread, NULL);
                free(datap->thread_d_ptr->dataBuff); //free the data buffer
                free(datap->thread_d_ptr);
                free(datap);
            }
            else
            {
                prev = datap; //only if current node not deleted
            }
            datap = next_node;
        }

    }   
    while(signal_received == 0); //waiting for signal to shutdown
        
    if (sigtype == SIGINT)
    {
        syslog(LOG_DEBUG, "Caugth SIGINT Signal\n");
    }
    if (sigtype == SIGTERM)
    {
        syslog(LOG_DEBUG, "Caugth SIGTERM Signal\n");
    } 
    //delay.it_value.tv_sec = 0;
    //delay.it_interval.tv_sec = 0;
    //timeStatus = setitimer(ITIMER_REAL, &delay, NULL);
    //if (timeStatus != 0)
    //{
    //    syslog(LOG_ERR, "timer failed to start :( %d\n", errno);
    //    return -1;
    //}

    while(datap != NULL)
    {
        next_node = SLIST_NEXT(datap, entries);
        if (datap->thread_d_ptr->thread_complete_success == 1)
        {
            pthread_join(datap->thread_d_ptr->thread, NULL);
            free(datap->thread_d_ptr->dataBuff); //free the data buffer
            free(datap->thread_d_ptr);
            free(datap);
        }
        datap = next_node;
    }

    close(socket_fd);    //close out handlers
    close(fd);    
    // Clear the list head macro tracking reference
    SLIST_INIT(&head); 

    syslog(LOG_INFO, "All thread memory cleaned up gracefully \n");

    

    int del_status = remove("/var/tmp/aesdsocketdata"); //delete file
    if (del_status !=0)
    {
        syslog(LOG_ERR,"file did not delete...\n");
    }
    else
    {
        syslog(LOG_DEBUG,"file is gone!\n");
    }
    syslog(LOG_DEBUG,"GOOD Bye!\n");
    return 0;
}
