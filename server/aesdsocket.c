/**
 * Clint Furrer
 * Created on: 9/25/26
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>   // For write() and close()
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    printf("hello server\n");

    //open the file if it does not exist create it
    int fd = open("/var/tmp/aesdsocketdata", O_WRONLY | O_CREAT | O_APPEND, 0644); 
    if (fd == -1)
    {
        return -1;
    }



}
