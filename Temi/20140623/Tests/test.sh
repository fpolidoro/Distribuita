#!/bin/bash
# !!!!!
# 	This script expects the file under test to be named socket.zip and to be placed
# 	into the directory of the script itself.
# !!!!!

export SOCKET_SERVER="socket_server"
export SOCKET_CLIENT="socket_client"
export TEMP_DIRECTORY="solution"
export ZIP_FILE="socket.zip"
export GCC_OUTPUT="gcc_output.txt"
export TOOLS_DIR="tools"
export PORTFINDER_DIR="portfinder"
export PORTFINDER_EXEC="port_finder"
export COMPARATOR_EXEC="comparator"
export COMPARATOR_DIR="file_comparator"
export KAPPA="10.5"
export DATA_FILE="data.txt"
export OUTPUT_FILE="output.txt"
export EXPECTED_OUTPUT_FILE="output_expected.txt"
export HOME_DIR="."

#*******************************KILL PROCESSES**********************************
function killProcesses
{
	# Kill running servers
	ps -ef | grep $SOCKET_SERVER | awk '{print $2}' | xargs kill 2> /dev/null

	# Kill running clients
	ps -ef | grep $SOCKET_CLIENT | awk '{print $2}' | xargs kill 2> /dev/null
}

#**********************************CLEANUP***************************************
function cleanup
{
	echo -n "Cleaning up..."
	killProcesses
	# Delete previously generated folders and files (if they exist)
	rm -r -f $TEMP_DIRECTORY
	rm -r -f $TOOLS_DIR/$PORTFINDER_EXEC
	rm -r -f $TOOLS_DIR/$COMPARATOR_EXEC
	rm -r -f $TOOLS_DIR/types.c
	rm -r -f $TOOLS_DIR/types.h
	echo -e "\t\t\tOk!"
}

#***************************COMPILING TESTING TOOLS******************************
function compileTools
{
	echo -n "Compiling testing tools..."
	cd $TOOLS_DIR
	rpcgen -h types.x -o types.h
	rpcgen -c types.x -o types.c
	echo -e "\tOk!"
	cd ..
	gcc -g -DTRACE -Wno-format-security -o $TOOLS_DIR/$PORTFINDER_EXEC $TOOLS_DIR/$PORTFINDER_DIR/*.c 2> /dev/null
	gcc -o $TOOLS_DIR/$COMPARATOR_EXEC $TOOLS_DIR/$COMPARATOR_DIR/*.c 2> /dev/null
	if [ ! -e $TOOLS_DIR/$PORTFINDER_EXEC ] || [ ! -e $TOOLS_DIR/$COMPARATOR_EXEC ] ; then
		echo -e "\tFail!. \n[ERROR] Unable to compile testing tools. Test aborted!"
		exit -1
	fi

	echo -n "Checking testing tools..."
	if [ ! -e $TOOLS_DIR/$EXPECTED_OUTPUT_FILE ] ; then
		echo -e "\tFail!. \n[ERROR] Testing tools missing. Test aborted!"
		exit -2
	fi
	echo -e "\tOk!"
}

#***************************CHECK .ZIP FILE EXISTENCE****************************
function checkZip
{
	echo -n "Checking $ZIP_FILE file...";
	if [[ ! -f $ZIP_FILE ]]; then
		echo -e "\tFail!. \n[ERROR] File $ZIP_FILE is not present. Test aborted!"
		exit -3
	fi
	echo -e "\tOk!";
}

#*****************************EXTRACTING ZIP FILE********************************
function extractZip
{
	echo -n "Extracting $ZIP_FILE file...";
	unzip $ZIP_FILE -d $TEMP_DIRECTORY > /dev/null
	rc=$?
	if [[ $rc != 0 ]] ; then
		echo -e "\tFail!. \n[ERROR] Unable to extrack $ZIP_FILE. Test aborted!"
		exit -4
	fi
	echo -e "\tOk!"
}

#********************************COMPILING SOURCES******************************
function compileSource
{
	cd $TEMP_DIRECTORY
	if [[ -e types.x ]] ; then
		echo -e "[WARNING] types.x found in your .zip! It will be ignored."
	fi
	cp -rf ../$TOOLS_DIR/types.x .
	rm -r -f $GCC_OUTPUT
	echo -n "Invoking rcpgen...";
	rpcgen -h types.x -o types.h
	rc=$?
	if [[ $rc != 0 ]] ; then
		echo -e "\tFail!. \n[ERROR] rpcgen failed to generate types.h. Test aborted!"
		exit -5
	fi
	rpcgen -c types.x -o types.c
	rc=$?
	if [[ $rc != 0 ]] ; then
		echo -e "\tFail!. \n[ERROR] rpcgen failed to generate types.c. Test aborted!"
		exit -5
	fi
	echo -e "\t\tOk!"

	echo -n "Compiling...";
	gcc -g -o $SOCKET_CLIENT client/*.c *.c -I client -lpthread -lm >> $GCC_OUTPUT 2>&1 
	gcc -g -o $SOCKET_SERVER server/*.c *.c -I server -lpthread -lm >> $GCC_OUTPUT 2>&1 
	if [ ! -e $SOCKET_SERVER ] || [ ! -e $SOCKET_CLIENT ] ; then
		echo -e "\tFail!. \n[ERROR] Unable to compile source code. Test aborted!"
		echo "GCC log is available in $TEMP_DIRECTORY/$GCC_OUTPUT"
		exit -6
	fi
	echo -e "\t\t\tOk!"
	cd ..
}

#************************************RUN SERVER*************************************
# First argument: the server to be run
# Return value: the listening port
function runServer
{
	local FREE_PORT=`$TOOLS_DIR/$PORTFINDER_EXEC 2000`	# This will find a free port
	$1 $FREE_PORT $KAPPA &> /dev/null &					# Launch the server
	echo $FREE_PORT	# Returning the port on which the server is listening to the caller
}

# Setup stage
cleanup
compileTools
checkZip
extractZip
compileSource

# Initialize the student's score
final_score=0

# Run the student's server
echo -n "Running student's server...";
export port=$(runServer "./solution/$SOCKET_SERVER")
echo -e "\tOk!"

#********************************** TEST SUITE 1 ****************************************
export test_suite=1
export DATA_FILE="data.txt"
export EXPECTED_OUTPUT_FILE="output_expected.txt"
client=$TEMP_DIRECTORY/$SOCKET_CLIENT
./$TOOLS_DIR/runTests.sh $client #&> /dev/null
rc=$?
if [[ $rc == 4 ]] ; then
	((final_score+=$rc))	# One point for each passed test
fi

#********************************** TEST SUITE 2 ****************************************
export test_suite=2
export DATA_FILE="data_big.txt"
export EXPECTED_OUTPUT_FILE="output_expected_big.txt"
client=$TEMP_DIRECTORY/$SOCKET_CLIENT
./$TOOLS_DIR/runTests.sh $client #&> /dev/null
rc=$?
if [[ $rc == 4 ]] ; then
	((final_score+=$rc))	# One point for each passed test
fi

#**************************************** FINAL STEPS ************************************
cleanup
echo -e "\n\n************Your score is: $final_score/8************\n\n"
echo -e "+++++  Note that the score is not the final mark of your socket exam.  +++++\n\n"

rm -f temp
rm -f temp_file
rm $OUTPUT_FILE
exit $final_score	# Return the final score obtained
