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

#define PORT_num "9000"

volatile sig_atomic_t signal_received = 0;

void handle_signal(int sig) 
{
    signal_received = 1; 
    if (sig == SIGINT)
    {
        syslog(LOG_DEBUG, "Caugth SIGINT Signal\n");
    }
    if (sig == SIGTERM)
    {
        syslog(LOG_DEBUG, "Caugth SIGTERM Signal\n");
    }
}

int main(int argc, char *argv[])
{
    struct sigaction mysig;
    struct addrinfo hints, *servinfo, *ptr;
    struct sockaddr_storage client_addr;
    socklen_t s_in_size;
    pid_t pid;
    int addrinfo_status;
    int socket_fd;
    int conn_fd;
    int bind_status;
    int listen_status;
    int daemonFLG = 0;
    int sock_bind_err_cnt = 0;
    int sigStatus;
    int yes=1;
    char ipaddr_buf[64];
    char inDataBuff[1024];
    
    //open logging 
    openlog(NULL, 0, LOG_USER); // open the logging
    printf("hello server\n");
    
    const char *prams = argv[1]; //copy input prams
    printf("input prams %s\n", prams);
    if (prams != NULL)
    {
        int pramStatus = strcmp(prams, "-d");
        if (pramStatus == 0)
        {
            daemonFLG = 1;
        }
    }
    printf("daemon FLAG status %d\n",daemonFLG);
    memset(&mysig, 0, sizeof(struct sigaction));
    memset(&hints, 0, sizeof hints); // zero out struct
    mysig.sa_handler = handle_signal;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // use my IP

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
    addrinfo_status = getaddrinfo(NULL, PORT_num, &hints, &servinfo);
    if (addrinfo_status != 0)
    {
        syslog(LOG_ERR, "get addr info function failed :( %d\n", errno);
        return -1;
    }

    for(ptr = servinfo; ptr != NULL; ptr = ptr->ai_next)
    {
        void *addr;
        struct sockaddr_in *ipv4;
        ipv4 = (struct sockaddr_in *)ptr->ai_addr;
        addr = &(ipv4->sin_addr);
        inet_ntop(ptr->ai_family, addr, ipaddr_buf, sizeof ipaddr_buf);
        syslog(LOG_DEBUG,"My address :P %s\n", ipaddr_buf);

        socket_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (socket_fd == -1)
        {
            int saved_errno = errno;
            sock_bind_err_cnt++;
            syslog(LOG_ERR, "socket function call failed :( %d\n", saved_errno);
        }

        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        bind_status = bind(socket_fd, ptr->ai_addr, ptr->ai_addrlen);
        if (bind_status != 0)
        {
            sock_bind_err_cnt++;
            close(socket_fd);
            syslog(LOG_ERR, "could not bind to socket :( %d\n", errno);
        }
        break;
    }

    if (ptr == NULL)
    {
        syslog(LOG_ERR, "Error count in socket and or bind :( %d\n", sock_bind_err_cnt);
        syslog(LOG_ERR, "Errors in socket or in bind :( %d\n", errno);
        return -1;
    }
    freeaddrinfo(servinfo); // all done with this structure

    if (daemonFLG == 1)
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
        pid = fork();
        if (pid < 0)
        {
            syslog(LOG_ERR, "Error in fork could not make second process :( %d\n", errno);
            exit(1);
        }
        if (pid > 0) //exit first child process
        {
            exit(0);
        }
        int chgdirStatus = chdir("/");
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
    listen_status = listen(socket_fd, 10);
    if (listen_status != 0)
    {
        syslog(LOG_ERR, "listen failed :( %d\n", errno);
        close(fd);
        close(socket_fd);
        return -1;
    }

    while(signal_received == 0)
    {
        s_in_size = sizeof client_addr;
        conn_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &s_in_size);
        if (conn_fd == -1)
        {
            if (signal_received || errno == EINTR)
            {
                break;
            }
            syslog(LOG_ERR, "accept has failed :( %d\n", errno);
        }
        struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
        inet_ntop(AF_INET, &(s->sin_addr), ipaddr_buf, sizeof ipaddr_buf);
        syslog(LOG_DEBUG,"ip address connected %s\n", ipaddr_buf);

        int TX_RX_FLG = 0;
        int in_cnt;
        //int data_wr_cnt = 0;
        ssize_t ret;
        ssize_t bytes_read;
        do
        {
            in_cnt = recv(conn_fd, inDataBuff, sizeof(inDataBuff) - 1, 0);
            if (in_cnt <= 0)
            {
                break; //client disconnectd or socket error
            }
            ret = write(fd, inDataBuff, in_cnt); //start writing the data stream 
            //data_wr_cnt++;
            if (ret == -1)
            {
                syslog(LOG_ERR, "File write failed :( %d\n", errno);
            }
            
            char *first_n = memchr(inDataBuff, '\n', in_cnt); //look for the new line packet terminator 
            if (first_n != NULL)
            {
                syslog(LOG_DEBUG,"new line dectected\n");
                //printf("made %d writes\n", data_wr_cnt);
                TX_RX_FLG = 1;
            }            
        } while (TX_RX_FLG != 1);
        fsync(fd); //force data to file
        lseek(fd, 0, SEEK_SET); //set to the beginning of file
        TX_RX_FLG = 0; //reset FLG
        do
        {
            bytes_read = read(fd, inDataBuff, sizeof(inDataBuff));
            if (bytes_read == -1)
            {
                syslog(LOG_ERR, "Error reading file back to client: %d\n", errno);
                TX_RX_FLG = 1;
                break;
            }
            if (bytes_read == 0)
            {
                syslog(LOG_DEBUG,"hit read 0 confirm %ld\n", bytes_read);
                TX_RX_FLG = 1;
                break;
            }
            syslog(LOG_DEBUG,"Sending! data back\n");
            send(conn_fd, inDataBuff, bytes_read, 0);
        } while (TX_RX_FLG != 1);
        
        syslog(LOG_DEBUG,"out of do while\n");
        syslog(LOG_DEBUG,"sig FLAG status %d\n", signal_received);
        close(conn_fd);
        syslog(LOG_DEBUG, "Closed connection from %s\n", ipaddr_buf);
    }

    while(signal_received == 0); //waiting for signal to shutdown
    close(socket_fd);    
    close(fd);
    int del_status = remove("/var/tmp/aesdsocketdata");
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
