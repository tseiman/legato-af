#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

OnFail() {
    echo "App Info Test Failed!"
}

function CheckAppIsRunning
{
    appName=$1

    numMatches=$(ssh root@$targetAddr "/usr/local/bin/app status $appName | grep -c \"running\"")

    if [ $numMatches -eq 0 ]
    then
        echo -e $COLOR_ERROR "App $appName is not running." $COLOR_RESET
        exit 1
    fi
}

if [ "$LEGATO_ROOT" == "" ]
then
    if [ "$WORKSPACE" == "" ]
    then
        echo "Neither LEGATO_ROOT nor WORKSPACE are defined." >&2
        exit 1
    else
        LEGATO_ROOT="$WORKSPACE"
    fi
fi

echo "******** App Info Test Starting ***********"

echo "Make sure Legato is running."
ssh root@$targetAddr "/usr/local/bin/legato start"
CheckRet

appDir="$LEGATO_ROOT/build/$targetType/bin/tests"
cd "$appDir"
CheckRet

echo "Stop all other apps."
ssh root@$targetAddr "/usr/local/bin/app stop \"*\""
sleep 1

echo "Install the testAppInfo app."
instapp testAppInfo.$targetType $targetAddr
CheckRet

ssh root@$targetAddr  "/usr/local/bin/app start testAppInfo"
CheckRet

sleep 1

CheckAppIsRunning testAppInfo

ssh root@$targetAddr  "/usr/local/bin/app remove testAppInfo"
CheckRet

echo "App Info Test Passed!"
exit 0