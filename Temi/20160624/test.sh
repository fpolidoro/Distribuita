#!/bin/bash
SOURCE_DIRECTORY="source"
GCC_OUTPUT="gcc_output.txt"
TOOLS_DIR="tools"
PORTFINDER_DIR="portfinder"
PORTFINDER_EXEC="port_finder"
SERVER1="socket_server1"
SERVER2="socket_server2"
CLIENT="socket_client"
TESTCLIENT="testclient"
TESTSERVER="test_server"
TESTSERVER_DIR="testserver"
SMALL_TEST_FILE="small_file.txt"
BIG_TEST_FILE="big_file.txt"

# Command to avoid infinite loops
TIMEOUT="timeout"

# Maximum allowed running time of a standard client (to avoid infinite loops)
MAX_EXEC_TIME=30

#*******************************KILL PROCESSES**********************************
function killProcesses
{
	# Kill running servers
	for f in `ps -ef | grep $SERVER1 | awk '{print $2}'`; do kill -9 $f &> /dev/null; done
	for f in `ps -ef | grep $SERVER2 | awk '{print $2}'`; do kill -9 $f &> /dev/null; done
	for f in `ps -ef | grep $TESTSERVER | awk '{print $2}'`; do kill -9 $f &> /dev/null; done

	# Kill running clients
	for f in `ps -ef | grep $CLIENT | awk '{print $2}'`; do kill -9 $f 2>&1 &> /dev/null; done
	for f in `ps -ef | grep $TESTCLIENT | awk '{print $2}'`; do kill -9 $f 2>&1 &> /dev/null; done
}

#**********************************CLEANUP***************************************
function cleanup
{
	echo -n "Cleaning up..."
	killProcesses
	# Delete previously generated folders and files (if they exist)
	rm -r -f temp*  2>&1 &> /dev/null
	rm -r -f client_temp_dir* 2>&1 &> /dev/null
	rm -f $TOOLS_DIR/$PORTFINDER_EXEC 2>&1 &> /dev/null
	rm -f $SOURCE_DIRECTORY/$CLIENT 2>&1 &> /dev/null
	rm -f $SOURCE_DIRECTORY/$SERVER1 2>&1 &> /dev/null
	rm -f $SOURCE_DIRECTORY/$SERVER2 2>&1 &> /dev/null
	echo -e "\t\t\tOk!"
}

#***************************COMPILING TESTING TOOLS******************************
function compileTools
{
	echo -n "Compiling testing tools..."

	gcc -g -DTRACE -Wno-format-security -o $TOOLS_DIR/$PORTFINDER_EXEC $TOOLS_DIR/$PORTFINDER_DIR/*.c $TOOLS_DIR/*.c 2> /dev/null
	if [ ! -e $TOOLS_DIR/$PORTFINDER_EXEC ] || [ ! -e $TOOLS_DIR/$TESTCLIENT ] || [ ! -e $TOOLS_DIR/$SMALL_TEST_FILE ] || [ ! -e $TOOLS_DIR/$BIG_TEST_FILE ] ; then
		echo -e "\tFail!. \n[ERROR] Unable to compile testing tools or missing test files. Test aborted!"
		exit -1
	else
		echo -e "\tOk!"
	fi
}

#********************************COMPILING SOURCES******************************
function compileSource
{
	cd $SOURCE_DIRECTORY
	rm -f $GCC_OUTPUT

	echo -n "Test 0.1 (compiling server1)...";
	gcc -g -o $SERVER1 server1/*.c *.c -Iserver1 -lpthread -lm >> $GCC_OUTPUT 2>&1 
	if [ ! -e $SERVER1 ] ; then
		TEST_01_PASSED=false
		echo -e "\tFail! \n\t[ERROR] Unable to compile source code for server1."
		echo -e "\tGCC log is available in $SOURCE_DIRECTORY/$GCC_OUTPUT"
	else
		TEST_01_PASSED=true
		echo -e "\tOk!"
	fi

	echo -n "Test 0.2 (compiling client)...";
	gcc -g -o $CLIENT client/*.c *.c -Iclient -lpthread -lm >> $GCC_OUTPUT 2>&1 
	if [ ! -e $CLIENT ] ; then
		TEST_02_PASSED=false
		echo -e "\tFail! \n\t[ERROR] Unable to compile source code for the client."
		echo -e "\tGCC log is available in $SOURCE_DIRECTORY/$GCC_OUTPUT"
	else
		TEST_02_PASSED=true
		echo -e "\tOk!"
	fi

	echo -n "Test 0.3 (compiling server2)...";
	gcc -g -o $SERVER2 server2/*.c *.c -Iserver2 -lpthread -lm >> $GCC_OUTPUT 2>&1 
	if [ ! -e $SERVER2 ] ; then
		TEST_03_PASSED=false
		echo -e "\tFail! \n\t[ERROR] Unable to compile source code for server2."
		echo -e "\tGCC log is available in $SOURCE_DIRECTORY/$GCC_OUTPUT"
	else
		TEST_03_PASSED=true
		echo -e "\tOk!"
	fi

	cd ..
}

#************************************RUN SERVER*************************************
# First argument: the server to be run
# Second argument: the file for recording the server standard output
# Return value: the listening port
function runServer
{
	local FREE_PORT=`./$PORTFINDER_EXEC`				# This will find a free port
	$1 $FREE_PORT &> $2 &						# Launch the server
	ensureServerStarted $FREE_PORT
	echo $FREE_PORT	# Returning to the caller the port on which the server is listening
}

#************************************RUN CLIENT*************************************
# First argument: the client to be run
# Second argument: the address or name of the server
# Third argument: the port number of the server
# Fourth argument: the file to be transferred
# Fifth argument: the part id
# Sixth argument: the run id
# Seventh argument: -t3 (optional)
function runClient
{
	echo -n "Running client $1 (server $2, port $3 , file $4) ..."
	if [[ $7 == "-t3" ]] ; then
		output=$($TIMEOUT $MAX_EXEC_TIME ./$1 "$2" "$3" "$4" "$7")
		rc=$?
	else
		output=$($TIMEOUT $MAX_EXEC_TIME ./$1 "$2" "$3" "$4")
		rc=$?
	fi
	outputname="testclientoutput$5$6"
	eval ${outputname}="'$output'"
	returnname="testclientreturn$5$6"
	eval ${returnname}="'$rc'"
	if [[ $testStudentClient == false ]] && (( $rc == 0 )) ; then
		echo -e "\t$1 could not connect to $2 at port $3"
	elif [[ $testStudentClient == false ]] && (( $rc == 1 )) ; then
		echo -e "\tBad response received from $2"
	elif [[ $testStudentClient == false ]] && (( $rc == 2 )) ; then
		echo -e "\tTest client run terminated successfully"
		mv port.txt "testclientport$5$6" 2>&1 &> /dev/null
	else
		echo -e "\tClient run terminated"
	fi
}


#************************************ENSURE SERVER STARTED*************************************
# Check a server is listening on the specified port. Wait for at most WAITSEC seconds
# first parameter -> the port to be checked
#
function ensureServerStarted
{
	local WAITSEC=5				# Maximum waiting time in seconds
	# Ensure the server has started
	for (( i=1; i<=$WAITSEC; i++ ))	# Maximum WAITSEC tries
	do
		#fuser $1/tcp &> /dev/null
		rm -f temp
		netstat -ln | grep "tcp" | grep ":$1 " > temp
		if [[ -s temp ]] ; then
			# Server started
			break
		else
			# Server non started
			sleep 1
		fi
	done
	rm -f temp
}

#************************************TEST CLIENT_SERVER INTERACTION*************************************
# first parameter -> server to be used 
# second parameter -> client to be used
# third parameter -> start test number
#
function testClientServerInteraction
{
	# Creating temp directories
	mkdir temp_dir 2>&1 &> /dev/null
	mkdir client_temp_dir 2>&1 &> /dev/null
	
	# Preparing the server directory
	# Copying server source files to temporary directory
	cp -f $SOURCE_DIRECTORY/$1 temp_dir 2>&1 &> /dev/null
		
	# Copying test-related files to temporary directory
	cp -f $TOOLS_DIR/$PORTFINDER_EXEC temp_dir 2>&1 &> /dev/null
	cp -f $TOOLS_DIR/$SMALL_TEST_FILE temp_dir 2>&1 &> /dev/null
	cp -f $TOOLS_DIR/$BIG_TEST_FILE temp_dir 2>&1 &> /dev/null
	
	# Preparing the client directory
	testStudentClient=false		# true if testing with the student client	
	if [[ $2 == $CLIENT ]] ; then
		cp -f $SOURCE_DIRECTORY/$2 client_temp_dir 2>&1 &> /dev/null
		testStudentClient=true
	else
		cp -f $TOOLS_DIR/$2 client_temp_dir 2>&1 &> /dev/null
	fi

	test_number=$3
	
	# Run the server
	echo -n "Running server $1..."
	pushd temp_dir >> /dev/null
		local suffix="output1.txt"
		server_port=$(runServer "./$1" "$1$suffix")
		echo -e "\t[PORT $server_port] Ok!"
	popd >> /dev/null
	
	# Run client with small file
	local fname=$SMALL_TEST_FILE
	pushd client_temp_dir >> /dev/null
		runClient $2 "127.0.0.1" "$server_port" "$fname" "$test_suite" "1"
	popd >> /dev/null

	# TEST return value of test client
	retvalname="testclientreturn$test_suite"
	retvalname+="1"
	rc="${!retvalname}"
	if [[ $testStudentClient == false ]] ; then
		echo -e -n "[TEST $test_suite.$test_number] Checking result of interaction of test client with server $1..."
		local tname="TEST_$test_suite$test_number"
		local tname+="_PASSED"
	fi		
	if [[ $testStudentClient == false ]] && [[ $rc -lt 2 ]] ; then
		# We are using testclient in this test and the return code is less than 2 -> skip following tests
		echo -e "\n\t[--TEST $test_suite.$test_number FAILED--] No correct response from $1. Skipping next tests"
		eval ${tname}=false
		return
	elif [[ $testStudentClient == false ]] && (( $rc == 2 )) ; then
		echo -e "\n\t[++TEST $test_suite.$test_number PASSED++] "
		eval ${tname}=true
		test_number=$(($test_number + 1))
	fi

	# TEST received file
	echo -e -n "[TEST $test_suite.$test_number] Checking that the small file exists in client dir and is the same..."
	local tname="TEST_$test_suite$test_number"
	local tname+="_PASSED"
	if [ -e client_temp_dir/$fname ] ; then
		diff "client_temp_dir/$fname" "$TOOLS_DIR/$fname" 2>&1 &> /dev/null
		rc=$?
		if [ $rc -eq 0 ] ; then
			echo -e "\n\t[++TEST $test_suite.$test_number PASSED++] Ok!"
			eval ${tname}=true
		else
			echo -e "\n\t[--TEST $test_suite.$test_number FAILED--] Files are not equal!"
			eval ${tname}=false
		fi
	else
		echo -e "\n\t[--TEST $test_suite.$test_number FAILED--] File is not present in the client directory."
		eval ${tname}=false
	fi
	test_number=$(($test_number + 1))

	# Run client 1 2 (with big file)
	local fname=$BIG_TEST_FILE
	pushd client_temp_dir >> /dev/null
		if [[ $testStudentClient == false  && $test_suite == "2" ]] ; then
			runClient $2 "localhost" "$server_port" "$fname" "$test_suite" "2"
		elif [[ $testStudentClient == false  && $test_suite == "3" ]] ; then
			runClient $2 "127.0.0.1" "$server_port" "$fname" "$test_suite" "2" -t3
		else
			runClient $2 "127.0.0.1" "$server_port" "$fname" "$test_suite" "2"
		fi
	popd >> /dev/null
	
	# TEST received file
	echo -e -n "[TEST $test_suite.$test_number] Checking that the big file exists in client dir and is the same..."
	local tname="TEST_$test_suite$test_number"
	local tname+="_PASSED"
	if [ -e client_temp_dir/$fname ] ; then
		diff "client_temp_dir/$fname" "$TOOLS_DIR/$fname" 2>&1 &> /dev/null
		rc=$?
		if [ $rc -eq 0 ] ; then
			echo -e "\n\t[++TEST $test_suite.$test_number PASSED++] Ok!"
			eval ${tname}=true
		else
			echo -e "\n\t[--TEST $test_suite.$test_number FAILED--] Files are not equal!"
			eval ${tname}=false
		fi
	else
		echo -e "\n\t[--TEST $test_suite.$test_number FAILED--] File is not present in the client directory."
		eval ${tname}=false
	fi
	test_number=$(($test_number + 1))

	#TEST server output for small file (IP address and port)
	serveroutputfname="$1"
	serveroutputfname+="output1.txt"
	outp=$(grep $SMALL_TEST_FILE temp_dir/$serveroutputfname)
	outp_arr=( $outp )
	echo -e -n "[TEST $test_suite.$test_number] Checking the server output (IP and port) for the small file..."
	local tname="TEST_$test_suite$test_number"
	local tname+="_PASSED"
	if [[ ${outp_arr[1]} != "127.0.0.1" ]] ; then
		echo -e "\n\t[--TEST $test_suite.$test_number FAILED--] client IP address not found in server output!"
		eval ${tname}=false
	elif [[ $testStudentClient == false ]] ; then
		testclientportfname="testclientport$test_suite"
		testclientportfname+="1"
		local cportref=$(cat client_temp_dir/$testclientportfname)
		if [[ ${outp_arr[2]} != $cportref ]] ; then
			echo -e "\n\t[--TEST $test_suite.$test_number FAILED--] client port not found in server output!"
			eval ${tname}=false
		else
			echo -e "\n\t[++TEST $test_suite.$test_number PASSED++]"
			eval ${tname}=true		
		fi
	else
		echo -e "\n\t[++TEST $test_suite.$test_number PASSED++]"
		eval ${tname}=true
	fi
	test_number=$(($test_number + 1))

	clientoutputname="testclientoutput$test_suite"
	clientoutputname+="1"
	clientoutput=${!clientoutputname}
	clientoutput_arr=( $clientoutput )
	echo -e -n "[TEST $test_suite.$test_number] Checking the client output (time stamp) for the small file..."
	local tname="TEST_$test_suite$test_number"
	local tname+="_PASSED"
	if [[ ${clientoutput_arr[1]} != ${outp_arr[4]} ]] ; then
		echo -e "\n\t[--TEST $test_suite.$test_number FAILED--] client timestamp different from server timestamp!"
		eval ${tname}=false
	else
		echo -e "\n\t[++TEST $test_suite.$test_number PASSED++]"
		eval ${tname}=true
	fi
	test_number=$(($test_number + 1))


	#TEST server output for big file (IP address and port)
	outp=$(grep $BIG_TEST_FILE temp_dir/$serveroutputfname)
	outp_arr=( $outp )
	echo -e -n "[TEST $test_suite.$test_number] Checking the server output (IP and port) for the big file..."
	local tname="TEST_$test_suite$test_number"
	local tname+="_PASSED"
	if [[ ${outp_arr[1]} != "127.0.0.1" ]] ; then
		echo -e "\n\t[--TEST $test_suite.$test_number FAILED--] client IP address not found in server output!"
		eval ${tname}=false
	elif [[ $testStudentClient == false ]] ; then
		testclientportfname="testclientport$test_suite"
		testclientportfname+="2"
		local cportref=$(cat client_temp_dir/$testclientportfname)
		if [[ ${outp_arr[2]} != $cportref ]] ; then
			echo -e "\n\t[--TEST $test_suite.$test_number FAILED--] client port not found in server output!"
			eval ${tname}=false
		else
			echo -e "\n\t[++TEST $test_suite.$test_number PASSED++]"
			eval ${tname}=true		
		fi
	else
		echo -e "\n\t[++TEST $test_suite.$test_number PASSED++]"
		eval ${tname}=true
	fi
	test_number=$(($test_number + 1))

	#TEST client output for big file (time stamp)
	clientoutputname="testclientoutput$test_suite"
	clientoutputname+="2"
	clientoutput=${!clientoutputname}
	clientoutput_arr=( $clientoutput )
	echo -e -n "[TEST $test_suite.$test_number] Checking the client output (time stamp) for the big file..."
	local tname="TEST_$test_suite$test_number"
	local tname+="_PASSED"
	if [[ ${clientoutput_arr[1]} != ${outp_arr[4]} ]] ; then
		echo -e "\n\t[--TEST $test_suite.$test_number FAILED--] client timestamp different from server timestamp!"
		eval ${tname}=false
	else
		echo -e "\n\t[++TEST $test_suite.$test_number PASSED++]"
		eval ${tname}=true
	fi
	test_number=$(($test_number + 1))

	#CLEANUP
	rm -r -f temp_dir 2>&1 &> /dev/null
	rm -r -f client_temp_dir 2>&1 &> /dev/null
	killProcesses
}

#********************************** TEST INITIALIZATION ****************************************

cleanup
compileTools
compileSource


#********************************** TEST SUITE 1 ****************************************
echo -e "\n************* TESTING PART 1 *************"
test_suite=1

if [[ $TEST_01_PASSED == true ]] ; then	
	testClientServerInteraction "$SERVER1" "$TESTCLIENT" "1"
else
	echo -e "---Skipping test of Part 1 because your server1 didn't compile---"
fi
echo -e "************* END OF TESTING PART 1 *************"

#********************************** TEST SUITE 2 ****************************************
echo -e "\n\n************* TESTING PART 2 *************"
test_suite=2

if [[ $TEST_02_PASSED == true ]] && [[ $TEST_01_PASSED == true ]] ; then
	testClientServerInteraction "$SERVER1" "$CLIENT" "1"
else
	echo -e "---Skipping test of Part 2 because your client OR your server1 didn't compile---"
fi
echo -e "************* END OF TESTING PART 2 *************"

#********************************** TEST SUITE 3 ****************************************
echo -e "\n\n************* TESTING PART 3 *************"
test_suite=3

if [[ $TEST_03_PASSED == true ]] ; then
	testClientServerInteraction "$SERVER2" "$TESTCLIENT" "1"
else
	echo -e "---Skipping test of Part 3 because your server2 didn't compile---"
fi
echo "************* END OF TESTING PART 3 *************"


#**************************************** FINAL STEPS ************************************

echo -e "\n\n************* FINAL STEPS *************"
cleanup

# Checking minimum requirements 
#((testclientreturn11 == 2)) || ((testclientreturn11 >= 1) && (test 2.1, test 2.2, and test 2.3 passed))
messages=0
#if [[ "$testclientreturn11" -eq 2 ]] || [[ "$testclientreturn11" -ge 1 ]] ; then
if [[ "$testclientreturn11" -eq 2 ]] ; then
	echo -e "\n+++++ OK: You have met the minimum requirements to pass the exam!!! +++++\n"
	exit
else
	((messages++))
	echo -e "\n \t ${messages})  Your server1 didn't respond with the correct message in test 1.1\n "
fi

if [[ "$testclientreturn11" -gt 1 ]] && [[ "$TEST_21_PASSED" == true ]] && [[ "$TEST_23_PASSED" == true ]] ; then
	echo -e "\n+++++ OK: You have met the minimum requirements to pass the exam!!! +++++\n"
	exit
else
	((messages++))
	echo -e "\t ${messages})  Your client and  your server1 were not able to complete a file transfer \n "
fi

echo -e "\n----- FAIL: You DO NOT have met the minimum requirements to pass the exam!!! -----\n"
if [[ "$messages" == 2 ]] ; then
	echo -e "\n### Fix at least one of the two items to meet the minimum requirements ###\n "	
fi
echo -e "\n************* END OF test.sh *************\n"
