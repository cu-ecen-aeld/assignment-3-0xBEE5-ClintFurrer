#!/bin/bash

filesdir=$1
searchstr=$2

#check if the directory exists
if [ ! -d "$filesdir" ]
then
    echo Directory does not exist
    exit 1
fi    

#check that the sreach string is not empty
if [ -z "$searchstr" ]
then
    echo String arg is zero length
    exit 1
fi
#use the -r recursive read all files option  and -o option to only send matching parts 
lineCnt=$(grep -r -o "$searchstr" "$filesdir" | wc -w)
#use the -l to count files with matches 
fileCnt=$(grep -r -l "$searchstr" "$filesdir" | wc -l)
echo The number of files are "$fileCnt" and the number of matching lines are "$lineCnt"
