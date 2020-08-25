#!/bin/bash

printf "===TTGen - BUILD===\n\n"

##########
# CONFIG #
##########
printf "\n===LOADING CONFIG===\n\n"
typeset -A config
config=(
    [CLIENT_ID]=""
    [CLIENT_SECRET]=""
    [FACULTY]=""
    [SEMESTER]=""
    [COURSES]=""
    [IGNORE_LECT]=""
    [IGNORE_TUTS]=""
    [IGNORE_LABS]=""
    [NE_730]=""
    [NE_915]=""
    [BUILD]=""
    [PORT]=""
)

while read line
do
    if echo $line | grep -F = &>/dev/null
    then
        varname=$(echo "$line" | cut -d '=' -f 1)
        config[$varname]=$(echo "$line" | cut -d '=' -f 2-)
    fi
done < config.txt

DIR_BUILD=${config[BUILD]}

#########
# BUILD #
#########
HERE=$(pwd)
rm -rf "$DIR_BUILD"
mkdir "$DIR_BUILD"

# Build - LoadData
printf "\n===BUILDING - LoadData===\n\n"
DIR_LOADDATA="$DIR_BUILD/LoadData"
mkdir "$DIR_LOADDATA"
cp LoadData/main.py "$DIR_LOADDATA/main.py"
$ret = $?
if [ $ret -ne 0 ]; then
    printf "\n===ERROR: Building LoadData failed===\n\n"
    echo "$DATA_LOAD"
    exit 1
fi

# Build - ProcessData
printf "\n===BUILDING - ProcessData===\n\n"
DIR_PROCESSDATA="$DIR_BUILD/ProcessData"
mkdir "$DIR_PROCESSDATA"
cd "$DIR_PROCESSDATA"
cmake -DCMAKE_BUILD_TYPE=Release "$HERE/ProcessData"
$ret = $?
if [ $ret -ne 0 ]; then
    printf "\n===ERROR: Building ProcessData (CMake) failed===\n\n"
    echo "$DATA_LOAD"
    exit 1
fi
make
$ret = $?
if [ $ret -ne 0 ]; then
    printf "\n===ERROR: Building ProcessData (Make) failed===\n\n"
    echo "$DATA_LOAD"
    exit 1
fi
cd "$HERE"

# Build - Viewer
printf "\n===BUILDING - Viewer===\n\n"
DIR_VIEWER="$DIR_BUILD/Viewer"
mkdir "$DIR_VIEWER"
cp -r Viewer/* "$DIR_VIEWER"
cd "$DIR_VIEWER"
yarn install
$ret = $?
if [ $ret -ne 0 ]; then
    printf "\n===ERROR: Building Viewer (install) failed===\n\n"
    echo "$DATA_LOAD"
    exit 1
fi
yarn run build
$ret = $?
if [ $ret -ne 0 ]; then
    printf "\n===ERROR: Building Viewer (build) failed===\n\n"
    echo "$DATA_LOAD"
    exit 1
fi
cd "$HERE"
mkdir "$DIR_BUILD/run"
mv "$DIR_VIEWER/build"/* "$DIR_BUILD/run/"


