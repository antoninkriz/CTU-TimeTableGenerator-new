#!/bin/bash

printf "===TTGen - START===\n\n"

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

#######
# RUN #
#######

printf "\n===RUNNING - Loading data...===\n\n"
DATA_LOAD=$(python3.8 "$DIR_BUILD/LoadData/main.py" "${config[CLIENT_ID]}" "${config[CLIENT_SECRET]}" "${config[FACULTY]}" "${config[SEMESTER]}" "${config[COURSES]}" "${config[IGNORE_LECT]}" "${config[IGNORE_TUTS]}" "${config[IGNORE_LABS]}")
ret=$?
if [ $ret -ne 0 ]; then
    printf "\n===ERROR: Loading data failed===\n\n"
    echo "$DATA_LOAD"
    exit 1
fi

printf "\n===RUNNING - Processing data...===\n\n"
DATA_PROCESS=$(echo "$DATA_LOAD" | "$DIR_BUILD/ProcessData/TTGen" $([ "${config[NE_730]}" == "true" ] && echo "ne730" || echo "") $([ "${config[NE_915]}" == "true" ] && echo "ne915" || echo ""))
ret=$?
if [ $ret -ne 0 ]; then
    printf "\n===ERROR: Processing data failed===\n\n"
    echo "$DATA_PROCESS"
    exit 1
fi

printf "\n===RUNNING - Saving data...===\n\n"
mkdir -p "$DIR_BUILD/run/"
echo "$DATA_PROCESS" > "$DIR_BUILD/run/timetables.json"

printf "\n===DONE - Starting web server===\n\n"
printf "\nOpen http://0.0.0.0:${config[PORT]} in a browser\n"
python3.8 -m http.server ${config[PORT]} -d "$DIR_BUILD/run" &> /dev/null


