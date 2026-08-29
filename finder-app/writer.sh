#!/bin/bash

writefile=$1
writestr=$2

#check that the file path is not empty
if [ -z "$writefile" ]
then
    echo Error file path is not given
    exit 1
fi

#check that the sreach string is not empty
if [ -z "$writestr" ]
then
    echo String arg is zero length
    exit 1
fi

#parse out file path
DIR_PATH="${writefile%/*}"
#parse out file name
FILE_NAME="${writefile##*/}"

#check for a file name
if [ -z "$FILE_NAME" ]
then
    echo Error no file name given
    exit 1
fi

#create directory if it does not exist
if [ ! -d "$DIR_PATH" ]
then
    if ! mkdir -p "$DIR_PATH" #added -p to add any part of path needed
    then
        echo Failed to create directory
        exit 1
    fi
fi

#create the file if it does not exist
if [ ! -e "$writefile" ]
then
    if ! touch "$writefile"
    then
        echo Failed to create file
        exit 1
    fi
fi

#write string into the created file
echo "$writestr" >>  "$writefile"