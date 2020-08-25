#!/bin/bash

printf "===TTGen===\n\n"

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

#########
# BUILD #
#########
HERE=$(pwd)
DIR_BUILD=_build
rm -rf "$DIR_BUILD"
mkdir "$DIR_BUILD"

# Build - LoadData
printf "\n===BUILDING - LoadData===\n\n"
DIR_LOADDATA="$DIR_BUILD/LoadData"
mkdir "$DIR_LOADDATA"
cp LoadData/main.py "$DIR_LOADDATA/main.py"

# Build - ProcessData
printf "\n===BUILDING - ProcessData===\n\n"
DIR_PROCESSDATA="$DIR_BUILD/ProcessData"
mkdir "$DIR_PROCESSDATA"
cd "$DIR_PROCESSDATA"
cmake -DCMAKE_BUILD_TYPE=Release "$HERE/ProcessData"
make
cd "$HERE"

# Build - Viewer
printf "\n===BUILDING - Viewer===\n\n"
DIR_VIEWER="$DIR_BUILD/Viewer"
mkdir "$DIR_VIEWER"
cp -r Viewer/* "$DIR_VIEWER"
cd "$DIR_VIEWER"
yarn install
yarn run build
cd "$HERE"
mkdir "$DIR_BUILD/run"
mv "$DIR_VIEWER/build"/* "$DIR_BUILD/run/"

#######
# RUN #
#######

printf "\n===RUNNING - Loading data...===\n\n"
DATA_LOAD=$(python3.8 "$DIR_BUILD/LoadData/main.py" ${config[CLIENT_ID]} ${config[CLIENT_SECRET]} ${config[FACULTY]} ${config[SEMESTER]} "${config[COURSES]}")
ret=$?
if [ $ret -ne 0 ]; then
    printf "\n===ERROR: Loading data failed===\n\n"
    echo "$DATA_LOAD"
    exit 1
fi

printf "\n===RUNNING - Processing data...===\n\n"
DATA_PROCESS=$(echo "$DATA_LOAD" | "$DIR_BUILD/ProcessData/TTGen")
ret=$?
if [ $ret -ne 0 ]; then
    printf "\n===ERROR: Processing data failed===\n\n"
    echo "$DATA_PROCESS"
    exit 1
fi

printf "\n===RUNNING - Saving data...===\n\n"
echo "$DATA_PROCESS" > "$DIR_BUILD/run/timetables.json"

printf "\n===DONE - Starting web server===\n\n"
printf "\nOpen http://0.0.0.0:${config[PORT]} in a browser\n"
python3.8 -m http.server ${config[PORT]} -d "$DIR_BUILD/run" &> /dev/null

