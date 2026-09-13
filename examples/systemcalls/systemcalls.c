#include "systemcalls.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h> 
#include <stdio.h>
/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    int status = system(cmd);
    if (status != 0)
    {
        status = false; //cmd failed
    }
    else
    {
        status = true; //system() call completed with success
    }
    return status;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/
static void print_helper(char **arr)
{
    int nullCnt = 0;
    for(int idx = 0; arr[idx] != NULL; idx++)
    {
        printf("Element [%d]: %s \n\r", idx, arr[idx]);
        nullCnt++;
    }
    if (arr[nullCnt] == NULL)
    {
        printf("NULL is in element %d\n\r", nullCnt);
    }
}


bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    va_end(args);

    printf("recieved command string is: \r\n");
    print_helper(command);
    fflush(stdout);
    pid_t pid;
    int exeStat = true;
    pid = fork();
    if (pid == -1)
    {
        return false;
    }
    printf("got pid number %d\r\n", pid);
    if (pid == 0) //child process 
    {
        const char *path = command[0];
        printf("calling execv...\r\n");
        int status = execv(path, command);
        printf("execv has failed status is :( %d\r\n", status);
        exit(EXIT_FAILURE);
    }
    else if(pid > 0) //parent
    {
        int pid_status;
        pid_t wait_status;
        wait_status = waitpid(pid, &pid_status, 0);
        printf("Wait status is: %d\r\n", pid_status);
        if (wait_status == -1)
        {
            exeStat = false;
            printf("waitpid failed with -1\r\n");
        }
        int exited_status;
        exited_status = WIFEXITED (pid_status);
        printf("exited code %d\r\n", exited_status);
        if (exited_status)
        {
            printf("checking the child process...\r\n");   
            int kid_exit_code = WEXITSTATUS(pid_status);
            printf("pid_exit code %d\r\n", kid_exit_code);
            if (kid_exit_code != 0) 
            {
                exeStat = false;
            }
        }
        if (pid_status != 0)
        {
            exeStat = false;
        }
    }
    printf("exit status is: %d\r\n", exeStat);
    return exeStat;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    va_end(args);

    printf("Starting my redirect\r\n");
    printf("File path is %s\r\n", outputfile);
    if (outputfile == NULL)
    {
        return false;
    }
    int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        printf("File did not open\r\n");
        return false;
    }
    int pid;
    int exeStat = true;
    pid = fork(); //create the kid process
    switch(pid)
    {
        case -1: //kid failed to be created 
            close(fd);
            return false;
        case 0:
            int status;
            status = dup2(fd, 1); //redirect the output to stdout printf will not print to terminal after this
            close(fd);
            if (status == -1)
            {
                printf("dup2 failed...");
                exit(EXIT_FAILURE);
            }
            const char *path = command[0];
            status = execv(path, command);
            exit(EXIT_FAILURE);
        default:
            close(fd);
    }

    if(pid > 0) //parent
    {
        int pid_status;
        pid_t wait_status;
        wait_status = waitpid(pid, &pid_status, 0); //wait on the kid process
        printf("Wait status is: %d\r\n", pid_status);
        if (wait_status == -1)
        {
            exeStat = false;
            printf("waitpid failed with -1\r\n");
        }
        int exited_status;
        exited_status = WIFEXITED (pid_status);
        printf("exited code %d\r\n", exited_status);
        if (exited_status)
        {
            printf("checking the child process...\r\n");   
            int kid_exit_code = WEXITSTATUS(pid_status);
            printf("pid_exit code %d\r\n", kid_exit_code);
            if (kid_exit_code != 0) 
            {
                exeStat = false; //kid process failed
            }
        }
        if (pid_status != 0)
        {
            exeStat = false;
        }
    }
    printf("exit status is: %d\r\n", exeStat);
    return exeStat;  
}
