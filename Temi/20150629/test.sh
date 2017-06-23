#!/bin/bash
SOURCE_DIRECTORY="source"
GCC_OUTPUT="gcc_output.txt"
TOOLS_DIR="tools"
PORTFINDER_DIR="portfinder"
PORTFINDER_EXEC="port_finder"
TIMESTAMP_CHECKER_EXEC="timestamp_checker"
TIMESTAMP_CHECKER_DIR="checktimestamp"
SERVER1="socket_server1"
SERVER2="socket_server2"
CLIENT="socket_client"
TESTCLIENT="test_client"
TESTCLIENT_DIR="testclient"
TESTSERVER="test_server"
TESTSERVER_DIR="testserver"
SMALL_TEST_FILE="small_file.txt"
SMALL_TEST_FILE2="small_file2.txt"
BIG_TEST_FILE="big_file.txt"
BIG_TEST_FILE2="big_file2.txt"
N_SECONDS="1"

# Command to avoid infinite loops
TIMEOUT="timeout"

# Maximum allowed running time of a standard client (to avoid infinite loops)
MAX_EXEC_TIME=30

#*******************************KILL PROCESSES**********************************
function killProcesses
{
	# Kill running servers
	for f in `ps -ef | grep $SERVER1 | awk '{print $2}'`; do kill -9 $f &> /dev/null; done
	for f in `ps -ef | grep $SERVER2 | awk '{print $2}'`;	do kill -9 $f &> /dev/null; done

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
	rm -r -f temp* 2>&1 &> /dev/null
	rm -r -f client_temp_dir 2>&1 &> /dev/null
	rm -f $TOOLS_DIR/$PORTFINDER_EXEC 2>&1 &> /dev/null
	rm -f $TOOLS_DIR/$TIMESTAMP_CHECKER_EXEC 2>&1 &> /dev/null
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
	gcc -g -DTRACE -Wno-format-security -o $TOOLS_DIR/$TIMESTAMP_CHECKER_EXEC $TOOLS_DIR/$TIMESTAMP_CHECKER_DIR/*.c 2> /dev/null

	if [ ! -e $TOOLS_DIR/$PORTFINDER_EXEC ] || [ ! -e $TOOLS_DIR/$TESTCLIENT ] || [ ! -e $TOOLS_DIR/$SMALL_TEST_FILE ] || [ ! -e $TOOLS_DIR/$BIG_TEST_FILE ] ; then
		echo -e "\tFail!. \n[ERROR] Unable to compile testing tools. Test aborted!"
		exit -1
	fi
	
	if [ ! -e $TOOLS_DIR/$TIMESTAMP_CHECKER_EXEC ] || [ ! -e $TOOLS_DIR/$TESTSERVER ] ; then
		echo -e "\tFail!. \n[ERROR] Unable to compile testing tools. Test aborted!"
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
# Second argument: number of seconds for updating process
# Return value: the listening port
function runServer
{
	local FREE_PORT=`./$PORTFINDER_EXEC`				# This will find a free port
	$1 $FREE_PORT $2 &> /dev/null &						# Launch the server
	ensureServerStarted $FREE_PORT
	echo $FREE_PORT	# Returning to the caller the port on which the server is listening
}

#
# first parameter -> port to be checked
#
function ensureServerStarted
{
	# Ensure the server has started
	for (( i=1; i<=5; i++ ))	# Maximum 5 tries
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

cleanup
compileTools
compileSource

return1=0
return2=0
TEST22PASSED=false
TEST23PASSED=false

#********************************** TEST SUITE 1 ****************************************
echo -e "\n************* TESTING PART 1 *************"
test_suite=1

if [[ $TEST_01_PASSED == true ]] ; then
	# Creating temp directories
	mkdir temp_dir 2>&1 &> /dev/null
	mkdir client_temp_dir 2>&1 &> /dev/null
	
	# Preparing the server
	cp -f $SOURCE_DIRECTORY/$SERVER1 temp_dir 
	cp -f $TOOLS_DIR/$PORTFINDER_EXEC temp_dir
	cp -f $TOOLS_DIR/$SMALL_TEST_FILE temp_dir
	cp -f $TOOLS_DIR/$BIG_TEST_FILE temp_dir

	# Needed to test the timestamp is properly set by the client
	sleep 1
	
	# Preparing the testclient
	cp -f $TOOLS_DIR/$TESTCLIENT client_temp_dir
	
	# Run the student's server 1
	echo -n "Running student's server 1..."
	pushd temp_dir >> /dev/null
		student_server1_port=$(runServer "./$SERVER1" "$N_SECONDS")
		echo -e "\t[PORT $student_server1_port] Ok!"
	popd >> /dev/null
	
	echo -n "[TEST $test_suite.1] Launching test client on port $student_server1_port..."
	
	# Run testclient
	pushd client_temp_dir >> /dev/null
		$TIMEOUT $MAX_EXEC_TIME ./$TESTCLIENT "127.0.0.1" "$student_server1_port" "$SMALL_TEST_FILE" "0" 2>&1 &> /dev/null
		return1=$?
		echo -e -n "\tReturn code = $return1"
	popd >> /dev/null
	
	if (( $return1 == 4 )) ; then
		# Check the file existence
		if [ -e client_temp_dir/$SMALL_TEST_FILE ] ; then
			echo -e "\t\t\t\t[++TEST $test_suite.1 passed++] Ok!"
			
			# Check received file is the same as the sample file
			echo -n "[TEST $test_suite.2] Checking received file is the same as the sample file..."
			diff "client_temp_dir/$SMALL_TEST_FILE" "$TOOLS_DIR/$SMALL_TEST_FILE" 2>&1 &> /dev/null
			rc=$?
			if [ $rc -eq 0 ] ; then
				echo -e "\t\t\t\t[++TEST $test_suite.2 passed++] Ok!"
			else
				echo -e "\t\t\t\t[--TEST $test_suite.2 FAILED--] Files are not equal!"
			fi
			
			# Check the "last modified" timestamp is equal for both files
			echo -n "[TEST $test_suite.3] Checking received file has the same last modified timestamp as the sample file..."
			$TOOLS_DIR/$TIMESTAMP_CHECKER_EXEC "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE" 2>&1 &> /dev/null
			rc=$?
			if [ $rc -eq 0 ] ; then
				echo -e "\t[++TEST $test_suite.3 passed++] Ok!"
			else
				echo -e "\t[--TEST $test_suite.3 FAILED--] Files have different last modified timestamps! (rc=$rc)"
			fi
		else
			echo -e "\t\t[--TEST $test_suite.1 FAILED--] File is not present in the client directory."  
		fi
	else
		echo -e "\t\t\t\t[--TEST $test_suite.1 FAILED--] Your server1 didn't respond correctly. Skipping test 1.2/1.3."  
	fi

	rm -r -f temp_dir 2>&1 &> /dev/null
	rm -r -f client_temp_dir 2>&1 &> /dev/null
	killProcesses
	
	if [ $return1 -gt 3 ] ; then
		# Creating temp directories
		mkdir temp_dir 2>&1 &> /dev/null
		mkdir client_temp_dir 2>&1 &> /dev/null
		
		# Preparing the server
		cp -f $SOURCE_DIRECTORY/$SERVER1 temp_dir
		cp -f $TOOLS_DIR/$PORTFINDER_EXEC temp_dir
		cp -f $TOOLS_DIR/$SMALL_TEST_FILE temp_dir
		cp -f $TOOLS_DIR/$SMALL_TEST_FILE2 temp_dir
		cp -f $TOOLS_DIR/$BIG_TEST_FILE temp_dir

		sleep 1
		
		# Preparing the testclient
		cp -f $TOOLS_DIR/$TESTCLIENT client_temp_dir
		
		# Run the student's server 1
		echo -n "Running student's server 1..."
		pushd temp_dir >> /dev/null
			student_server1_port=$(runServer "./$SERVER1" "$N_SECONDS")
			echo -e "\t[PORT $student_server1_port] Ok!"
		popd >> /dev/null
		
		# Run testclient
		pushd client_temp_dir >> /dev/null
			$TIMEOUT $MAX_EXEC_TIME ./$TESTCLIENT "127.0.0.1" "$student_server1_port" "$SMALL_TEST_FILE" "1" 2>&1 &> /dev/null &
			pid=$!
			sleep 3	# In the meanwhile testclient transfers $SMALL_TEST_FILE
		popd >> /dev/null
		
		# Update $SMALL_TEST_FILE with the content of $SMALL_TEST_FILE2
		cp -f temp_dir/$SMALL_TEST_FILE2 temp_dir/$SMALL_TEST_FILE
		
		# Wait for testclient to terminate
		wait $pid
		returnc=$?
		
		# Check testclient return code
		echo -n "[TEST $test_suite.4] Checking return code of testclient..."
		do_test_1_9=true
		if [ $returnc -eq 5 ] ; then
			echo -e "\t\t\t\t\t\t[++TEST $test_suite.4 passed++] Ok!"
			
			# Check received file is the same as the sample file
			echo -n "[TEST $test_suite.5] Checking received file is the same as the _updated_ sample file..."
			diff "client_temp_dir/$SMALL_TEST_FILE" "$TOOLS_DIR/$SMALL_TEST_FILE2" 2>&1 &> /dev/null &
			rc=$?
			if [ $rc -eq 0 ] ; then
				echo -e "\t\t\t[++TEST $test_suite.5 passed++] Ok!"
			else
				echo -e "\t\t\t[--TEST $test_suite.5 FAILED--] Files are not equal!"
				do_test_1_9=false
			fi
			
			# Check the "last modified" timestamp is equal for both files
			echo -n "[TEST $test_suite.6] Checking received file has the same timestamp as the _updated_ sample file..."
			$TOOLS_DIR/$TIMESTAMP_CHECKER_EXEC "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE2" 2>&1 &> /dev/null &
			rc=$?
			if [ $rc -eq 0 ] ; then
				echo -e "\t[++TEST $test_suite.6 passed++] Ok!"
			else
				echo -e "\t[--TEST $test_suite.6 FAILED--] Files have different last modified timestamps!"
				do_test_1_9=false
			fi
		else
			echo -e "\t\t\t\t\t\t[--TEST $test_suite.4 FAILED--] Testclient code $returnc -> wrong response or no response"
			echo -e "Skipped test 1.5/1.6"
		fi
		
		rm -r -f temp_dir 2>&1 &> /dev/null
		rm -r -f client_temp_dir 2>&1 &> /dev/null
		killProcesses
		
		if [ "$do_test_1_9" = true ] ; then
			# Creating temp directories
			mkdir temp_dir 2>&1 &> /dev/null
			mkdir client_temp_dir 2>&1 &> /dev/null
			
			# Preparing the server
			cp -f $SOURCE_DIRECTORY/$SERVER1 temp_dir
			cp -f $TOOLS_DIR/$PORTFINDER_EXEC temp_dir
			cp -f $TOOLS_DIR/$BIG_TEST_FILE temp_dir
			cp -f $TOOLS_DIR/$BIG_TEST_FILE2 temp_dir

			sleep 1
			
			# Preparing the testclient
			cp -f $TOOLS_DIR/$TESTCLIENT client_temp_dir
			
			# Run the student's server 1
			echo -n "Running student's server 1..."
			pushd temp_dir >> /dev/null
				student_server1_port=$(runServer "./$SERVER1" "$N_SECONDS")
				echo -e "\t[PORT $student_server1_port] Ok!"
			popd >> /dev/null
			
			# Run testclient
			pushd client_temp_dir >> /dev/null
				$TIMEOUT $MAX_EXEC_TIME ./$TESTCLIENT "127.0.0.1" "$student_server1_port" "$BIG_TEST_FILE" "1" 2>&1 &> /dev/null &
				pid=$!
				sleep 10
			popd >> /dev/null
			
			# Update $BIG_TEST_FILE with the content of $BIG_TEST_FILE2
			cp -f temp_dir/$BIG_TEST_FILE2 temp_dir/$BIG_TEST_FILE
			
			# Wait for testclient to terminate
			wait $pid
			returnCode=$?
			
			# Check testclient return code
			echo -n "[TEST $test_suite.9] Checking return code of testclient (Return code = $returnCode)..."
			sleep 5 # The file is big... We sleep some time to be sure the write is completed
			if [ $returnCode -eq 5 ] ; then
				# Check received file is the same as the sample file
				diff "client_temp_dir/$BIG_TEST_FILE" "$TOOLS_DIR/$BIG_TEST_FILE2" 2>&1 &> /dev/null
				rc=$?
				if [ $rc -eq 0 ] ; then
					# Check the "last modified" timestamp is equal for both files
					echo -n -e "\n[TEST $test_suite.9] Checking received file has the same timestamp as the _updated_ sample file..."
					$TOOLS_DIR/$TIMESTAMP_CHECKER_EXEC "client_temp_dir/$BIG_TEST_FILE" "temp_dir/$BIG_TEST_FILE" 2>&1 &> /dev/null
					rc=$?
					if [ $rc -eq 0 ] ; then
						echo -e "\t[++TEST $test_suite.9 passed++] Ok!"
					else
						echo -e "\t[--TEST $test_suite.9 FAILED--] Files have different last modified timestamps!"
					fi
				else
					echo -e "\t[--TEST $test_suite.9 FAILED--] Files are not equal!"
				fi
			else
				echo -e "\t\t\t\t[--TEST $test_suite.9 FAILED--] Testclient code $returnCode -> wrong response or no response"
			fi
			
			rm -r -f temp_dir 2>&1 &> /dev/null
			rm -r -f client_temp_dir 2>&1 &> /dev/null
			killProcesses
		else
			echo -e "---Skipping test 1.9 because test 1.5 and/or 1.6 failed ---"
		fi
	else
		echo -e "---Skipping part 1.4/1.5/1.6 because testclient received bad responses from the server (file size and/or timestamp and/or file content) ---"	
	fi
	
else
	echo -e "---Skipping part 1 because your server1 didn't compile :( ---"
fi
echo "************* END PART 1 *************"

#********************************** TEST SUITE 2 ****************************************
echo -e "\n\n************* TESTING PART 2 *************"
test_suite=2
TEST21PASSED=false
if [[ $TEST_02_PASSED == true ]] ; then
	if [[ $TEST_01_PASSED == true ]] ; then
		# Creating temp directories
		mkdir temp_dir 2>&1 &> /dev/null
		mkdir client_temp_dir 2>&1 &> /dev/null
		
		# Preparing the server
		cp -f $SOURCE_DIRECTORY/$SERVER1 temp_dir
		cp -f $TOOLS_DIR/$PORTFINDER_EXEC temp_dir
		cp -f $TOOLS_DIR/$SMALL_TEST_FILE temp_dir
		cp -f $TOOLS_DIR/$SMALL_TEST_FILE2 temp_dir

		sleep 1
		
		# Preparing the testclient
		cp -f $SOURCE_DIRECTORY/$CLIENT client_temp_dir
		
		# Run the student's server 1
		echo -n "Running student's server 1..."
		pushd temp_dir >> /dev/null
			student_server1_port=$(runServer "./$SERVER1" "$N_SECONDS")
			echo -e "\t[PORT $student_server1_port] Ok!"
		popd >> /dev/null
		
		echo -n "[TEST $test_suite.1] Launching student client on port $student_server1_port..."
		pushd client_temp_dir >> /dev/null
			$TIMEOUT $MAX_EXEC_TIME ./$CLIENT "127.0.0.1" "$student_server1_port" "$SMALL_TEST_FILE" 2>&1 &> /dev/null &
			rc=$?
			echo -e "\tReturn code = $rc"
		popd >> /dev/null
		
		sleep 5
		
		if [[ -s client_temp_dir/$SMALL_TEST_FILE ]] ; then
			echo -e "[TEST $test_suite.1] File transferred. Test passed!\t\t\t\t\t\t\t[++TEST $test_suite.1 passed++] Ok!"
			# Check received file is the same as the sample file
			echo -n "[TEST $test_suite.2] Checking received file is the same as the sample file..."
			diff "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE" 2>&1 &> /dev/null
			rc=$?
			if [ $rc -eq 0 ] ; then
				echo -e "\t\t\t\t[++TEST $test_suite.2 passed++] Ok!"
				TEST22PASSED=true
			else
				echo -e "\t\t\t\t[--TEST $test_suite.2 FAILED--] Files are not equal!"
			fi
			
			# Check the "last modified" timestamp is equal for both files
			echo -n -e "[TEST $test_suite.3] Checking received file has the same timestamp as the sample file..."
			$TOOLS_DIR/$TIMESTAMP_CHECKER_EXEC "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE" 2>&1 &> /dev/null
			rc=$?
			if [ $rc -eq 0 ] ; then
				echo -e "\t\t\t[++TEST $test_suite.3 passed++] Ok!"
				TEST23PASSED=true
			else
				echo -e "\t\t\t[--TEST $test_suite.3 FAILED--] Files have different last modified timestamps!"
			fi
		else
			echo -e "[Test $test_suite.1] File is not present.\t\t[--TEST $test_suite.1 FAILED--]"
			echo -e "Skipping test 2.2/2.3"
		fi
		
		# Update $SMALL_TEST_FILE with the content of $SMALL_TEST_FILE2
		cp -f temp_dir/$SMALL_TEST_FILE2 temp_dir/$SMALL_TEST_FILE
		
		sleep 5
		
		echo -n "[TEST $test_suite.4] Checking updated file is the same as the sample file..."
		diff "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE" 2>&1 &> /dev/null
		rc=$?
		if [ $rc -eq 0 ] ; then
			echo -e "\t\t\t\t[++TEST $test_suite.4 passed++] Ok!"
		else
			echo -e "\t\t\t\t[--TEST $test_suite.4 FAILED--] Files are not equal!"			
		fi
		
		# Check the "last modified" timestamp is equal for both files
		echo -n -e "[TEST $test_suite.5] Checking updated file has the same timestamp as the sample file..."
		$TOOLS_DIR/$TIMESTAMP_CHECKER_EXEC "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE" 2>&1 &> /dev/null
		rc=$?
		if [ $rc -eq 0 ] ; then
			echo -e "\t\t\t[++TEST $test_suite.5 passed++] Ok!"
		else
			echo -e "\t\t\t[--TEST $test_suite.5 FAILED--] Files have different last modified timestamps!"
		fi

		rm -r -f temp_dir 2>&1 &> /dev/null
		rm -r -f client_temp_dir 2>&1 &> /dev/null
		killProcesses
	else
		echo -e "---Skipping part 2.1/2.2/2.3/2.4/2.5 (because server1 didn't compile)---"
	fi
	
	# Creating temp directories
	mkdir temp_dir 2>&1 &> /dev/null
	mkdir client_temp_dir 2>&1 &> /dev/null
	
	# Preparing the server
	cp -f $TOOLS_DIR/$TESTSERVER temp_dir
	cp -f $TOOLS_DIR/$PORTFINDER_EXEC temp_dir
	cp -f $TOOLS_DIR/$SMALL_TEST_FILE temp_dir
	cp -f $TOOLS_DIR/$SMALL_TEST_FILE2 temp_dir
	
	# Preparing the testclient
	cp -f $SOURCE_DIRECTORY/$CLIENT client_temp_dir

	sleep 1
	
	# Run testserver
	echo -n "Running the testserver..."
	pushd temp_dir >> /dev/null
		testserver_port=$(runServer "./$TESTSERVER" "$N_SECONDS")
		echo -e "\t[PORT $testserver_port] Ok!"
	popd >> /dev/null
	
	echo -n "[TEST $test_suite.6] Launching student client on port $testserver_port..."
	pushd client_temp_dir >> /dev/null
		null=`$TIMEOUT $MAX_EXEC_TIME ./$CLIENT "127.0.0.1" "$testserver_port" "$SMALL_TEST_FILE" 2>&1 &> /dev/null &`
		rc=$?
		echo -e "\tReturn code = $rc"
	popd >> /dev/null
	
	sleep 5
	
	if [[ -s client_temp_dir/$SMALL_TEST_FILE ]] ; then
		echo -e "[TEST $test_suite.6] File transferred. Test passed!\t\t\t\t\t\t\t[++TEST $test_suite.6 passed++] Ok!"
		# Check received file is the same as the sample file
		echo -n "[TEST $test_suite.7] Checking received file is the same as the sample file..."
		diff "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE" 2>&1 &> /dev/null
		rc=$?
		if [ $rc -eq 0 ] ; then
			echo -e "\t\t\t\t[++TEST $test_suite.7 passed++] Ok!"
		else
			echo -e "\t\t\t\t[--TEST $test_suite.7 FAILED--] Files are not equal!"			
		fi
		
		# Check the "last modified" timestamp is equal for both files
		echo -n -e "[TEST $test_suite.8] Checking received file has the same timestamp as the sample file..."
		$TOOLS_DIR/$TIMESTAMP_CHECKER_EXEC "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE" 2>&1 &> /dev/null
		rc=$?
		if [ $rc -eq 0 ] ; then
			echo -e "\t\t\t[++TEST $test_suite.8 passed++] Ok!"
		else
			echo -e "\t\t\t[--TEST $test_suite.8 FAILED--] Files have different last modified timestamps!"
		fi
	else
		echo -e "[Test $test_suite.6] File is not present.\t\t[--TEST $test_suite.6 FAILED--]"
	fi
	
	# Update $SMALL_TEST_FILE with the content of $SMALL_TEST_FILE2
	cp -f temp_dir/$SMALL_TEST_FILE2 temp_dir/$SMALL_TEST_FILE
	
	sleep 5
	
	echo -n "[TEST $test_suite.9] Checking updated file is the same as the sample file..."
	diff "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE" 2>&1 &> /dev/null
	rc=$?
	if [ $rc -eq 0 ] ; then
		echo -e "\t\t\t\t[++TEST $test_suite.9 passed++] Ok!"
	else
		echo -e "\t\t\t\t[--TEST $test_suite.9 FAILED--] Files are not equal!"			
	fi
	
	# Check the "last modified" timestamp is equal for both files
	echo -n -e "[TEST $test_suite.10] Checking updated file has the same timestamp as the sample file..."
	$TOOLS_DIR/$TIMESTAMP_CHECKER_EXEC "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE" 2>&1 &> /dev/null
	rc=$?
	if [ $rc -eq 0 ] ; then
		echo -e "\t\t\t[++TEST $test_suite.10 passed++] Ok!"
	else
		echo -e "\t\t\t[--TEST $test_suite.10 FAILED--] Files have different last modified timestamps!"
	fi
	
	rm -r -f temp_dir 2>&1 &> /dev/null
	rm -r -f client_temp_dir 2>&1 &> /dev/null
	killProcesses
	
else
	echo -e "---Skipping part 2 because your client didn't compile---"
fi
echo "************* END PART 2 *************"

#********************************** TEST SUITE 3 ****************************************
echo -e "\n\n************* TESTING PART 3 *************"
test_suite=3
if [[ $TEST_03_PASSED == true ]] ; then
	# Creating temp directories
	mkdir temp_dir 2>&1 &> /dev/null
	mkdir client_temp_dir 2>&1 &> /dev/null
	
	# Preparing the server
	cp -f $SOURCE_DIRECTORY/$SERVER2 temp_dir
	cp -f $TOOLS_DIR/$PORTFINDER_EXEC temp_dir
	cp -f $TOOLS_DIR/$SMALL_TEST_FILE temp_dir
	cp -f $TOOLS_DIR/$SMALL_TEST_FILE2 temp_dir
	cp -f $TOOLS_DIR/$BIG_TEST_FILE temp_dir

	# Needed to test the timestamp is properly set
	sleep 1
	
	# Preparing the testclient
	cp -f $TOOLS_DIR/$TESTCLIENT client_temp_dir
	
	# Run the student's server 2
	echo -n "Running student's server 2..."
	pushd temp_dir >> /dev/null
		student_server2_port=$(runServer "./$SERVER2" "$N_SECONDS")
		echo -e "\t[PORT $student_server2_port] Ok!"
	popd >> /dev/null	
	
	echo -n "[TEST $test_suite.1] Launching testclient on port $student_server2_port..."
	pushd client_temp_dir >> /dev/null
		$TIMEOUT $MAX_EXEC_TIME ./$TESTCLIENT "127.0.0.1" "$student_server2_port" "$SMALL_TEST_FILE" "1" 2>&1 &> /dev/null &
		pid=$!
	popd >> /dev/null
	
	sleep 3
	
	if [[ -s client_temp_dir/$SMALL_TEST_FILE ]] ; then
		echo -e "[TEST $test_suite.1] File transferred. Test passed!\t[++TEST $test_suite.1 passed++] Ok!"
		# Check received file is the same as the sample file
		echo -n "[TEST $test_suite.2] Checking received file is the same as the sample file..."
		diff "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE" 2>&1 &> /dev/null
		rc=$?
		if [ $rc -eq 0 ] ; then
			echo -e "\t\t\t\t[++TEST $test_suite.2 passed++] Ok!"
		else
			echo -e "\t\t\t\t[--TEST $test_suite.2 FAILED--] Files are not equal!"			
		fi
		
		# Check the "last modified" timestamp is equal for both files
		echo -n -e "[TEST $test_suite.3] Checking received file has the same timestamp as the sample file..."
		$TOOLS_DIR/$TIMESTAMP_CHECKER_EXEC "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE" 2>&1 &> /dev/null
		rc=$?
		if [ $rc -eq 0 ] ; then
			echo -e "\t\t\t[++TEST $test_suite.3 passed++] Ok!"
		else
			echo -e "\t\t\t[--TEST $test_suite.3 FAILED--] Files have different last modified timestamps!"
		fi
	else
		echo -e "\t\t\t\t\t\t[--TEST $test_suite.1 FAILED--]"
	fi
	
	# Update $SMALL_TEST_FILE with the content of $SMALL_TEST_FILE2
	cp -f temp_dir/$SMALL_TEST_FILE2 temp_dir/$SMALL_TEST_FILE
	
	wait $pid
	return2=$?
	
	echo -n "[TEST $test_suite.4] Checking updated file is the same as the sample file..."
	diff "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE" 2>&1 &> /dev/null
	rc=$?
	if [ $rc -eq 0 ] ; then
		echo -e "\t\t\t\t[++TEST $test_suite.4 passed++] Ok!"
	else
		echo -e "\t\t\t\t[--TEST $test_suite.4 FAILED--] Files are not equal!"			
	fi
	
	# Check the "last modified" timestamp is equal for both files
	echo -n -e "[TEST $test_suite.5] Checking updated file has the same timestamp as the sample file..."
	$TOOLS_DIR/$TIMESTAMP_CHECKER_EXEC "client_temp_dir/$SMALL_TEST_FILE" "temp_dir/$SMALL_TEST_FILE" 2>&1 &> /dev/null
	rc=$?
	if [ $rc -eq 0 ] ; then
		echo -e "\t\t\t[++TEST $test_suite.5 passed++] Ok!"
	else
		echo -e "\t\t\t[--TEST $test_suite.5 FAILED--] Files have different last modified timestamps!"
	fi
	
	echo -n -e "[TEST $test_suite.6] Checking return code of testclient (5 means everything OK)..."
	if (( $return2 == 5 )) ; then
		echo -e "\t\t\t[++TEST $test_suite.6 passed++] Ok!"	
	else
		echo -e "\t\t\t[--TEST $test_suite.6 FAILED--] Testclient returned a not-OK code!"  
	fi
	
	rm -r -f temp_dir 2>&1 &> /dev/null
	rm -r -f client_temp_dir 2>&1 &> /dev/null
	killProcesses

else
	echo -e "---Skipping part 3---"
fi
echo "************* END PART 3 *************"


#**************************************** FINAL STEPS ************************************
echo -e "\n\n************* FINAL STEPS *************"
cleanup
rm -f temp
if [ "$return1" -eq 4 ] ; then
	echo -e "\n+++++ OK: You have met the minimum requirements to pass the exam!!! +++++\n"
	exit
fi

if [ "$return1" -gt 0 ] && [[ "$TEST22PASSED" == true ]] && [[ "$TEST23PASSED" == true ]]  ; then
	echo -e "\n+++++ OK: You have met the minimum requirements to pass the exam!!! +++++\n"
	exit
fi

messages=0
echo -e "\n----- FAIL: You DO NOT have met the minimum requirements to pass the exam!!! -----\n"
if [ ! "$return1" -eq 4 ] ; then
	((messages++))
	echo -e "\n \t ${messages})  Your server1 didn't respond with the correct message in test 1.1\n "
fi
if [ ! "$return1" -gt 0 ] || [[ "$TEST22PASSED" == false ]] || [[ "$TEST23PASSED" == false ]] ; then
	((messages++))
	echo -e "\t ${messages})  Your client and server1 are not able to complete a file transfer (with correct content and timestamp)\n "
fi
if [[ "$messages" == 2 ]] ; then
	echo -e "\n### Fix at least one of the two items to meet the minimum requirements ###\n "	
fi
echo -e "\n************* END test.sh *************\n"
