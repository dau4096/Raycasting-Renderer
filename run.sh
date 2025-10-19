#!/bin/bash

cd src || exit 1

#Delete all .o files
find . -name '*.o' -delete

#Go back to root directory and delete main.o
cd .. || exit 1
rm -f main.o

#Compile
make

#Check success
if [[ $? -ne 0 ]]; then
    echo "Build failed."
    exit 1
fi

#Run
cd "$(dirname "$0")"
./app

#Check success
if [[ $? -ne 0 ]]; then
    echo "An error occurred during execution. Exit code: $?"
    exit 1
fi

