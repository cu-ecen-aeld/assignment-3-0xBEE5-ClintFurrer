/*
* Name: Clint Furrer
* Assignment 2
* Writer program
*/
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    char *fileName;
    char *fileData_str;
    char *og_fileData_str;
    //open logging 
    openlog(NULL, 0, LOG_USER); //open the logging
    //check if the file string data is not NULL
    if (argv[2] == NULL)
    {
        syslog(LOG_ERR, "Empty File String :( \n\r");
        return 1;
    }
    else
    {
        fileData_str = strdup(argv[2]); //copy arg in to local program memory
        og_fileData_str = fileData_str;
        if (fileData_str == NULL)
        {
            syslog(LOG_ERR, "String memory allocation failed :( \n\r");
            return 1;
        }
    }
    //check if the file name arg is not empty
    if (argv[1] == NULL)
    {
        syslog(LOG_ERR, "Empty File Name :( \n\r");
        return 1;        
    }
    else
    {
        fileName  = strdup(argv[1]); //copy arg in to local program memory
        if (fileName == NULL)
        {
            syslog(LOG_ERR, "String file name memory allocation failed :( \n\r");
            return 1;
        }        
    }

    int fd;
    //open the file if it does not exist create it
    fd = creat(fileName, 0644);
    if (fd == -1)
    {
        syslog(LOG_ERR, "File failed to open :( %d\n\r", errno);
        free(fileName);
        free(og_fileData_str);
        return 1;
    }

    ssize_t ret;
    int len;
    len = strlen(fileData_str);  
    syslog(LOG_DEBUG, "Writing %s to %s file\n\r", fileData_str, fileName);
    syslog(LOG_DEBUG, "File length: %d\n\r", len);
    //Idea for write from the Linux System Programming Book Chapter 2 Pg 37 - 38
    do
    {
        ret = write(fd, fileData_str, len);
        syslog(LOG_DEBUG, "ret value: %d\n\r", (int)ret);
        if (ret == -1)
        {
            switch (errno) //if error read error type
            {
            case EINTR:
                break; //contiune
            
            default:
                syslog(LOG_ERR, "File write failed :( %d\n\r", errno);
                close(fd);
                free(fileName);
                free(og_fileData_str);
                return -1; 
            }
        }
        len -= ret;
        fileData_str += ret;
        syslog(LOG_DEBUG, "len value post: %d\n\r", len);
    } while (len != 0 && ret != 0);
    
    //free and close files and memory
    free(fileName);
    free(og_fileData_str);
    int status = close(fd);
    if (status == -1)
    {
        syslog(LOG_ERR, "File failed to close :( %d\n\r", errno);
        return 1;
    }

    syslog(LOG_DEBUG, "Finished! :P\n\r");
    return 0;
}