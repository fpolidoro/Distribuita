#!/bin/bash
### This script does tests xa-xd where x can be 1/2/3/4/5/7/10 ###
### First argument: the client (executable) to be tested ###

# Command to avoid infinite loops
TIMEOUT="timeout"

# Maximum allowed running time of a standard client (to avoid infinite loops)
MAX_EXEC_TIME=30

# Error allowed in file comparison
ERROR=0.2

#*********************************test a******************************************
TEST_1A_PASSED=true
rm -f $OUTPUT_FILE
final_score=0
echo -n "Launching client on port $port, inFile $HOME_DIR/$TOOLS_DIR/$DATA_FILE...";
# Launch client
$TIMEOUT $MAX_EXEC_TIME $1 127.0.0.1 $port ENCODE $HOME_DIR/$TOOLS_DIR/$DATA_FILE #&> /dev/null
echo -e "\tOk!"
echo -n "[TEST $test_suite a] Checking existence of output file..";
if [ ! -e $OUTPUT_FILE ] ; then
	echo -e "\t[--TEST FAILED--] File $OUTPUT_FILE not generated."
	TEST_1A_PASSED=false
else
	((final_score++))	# Add one point to the final score
	echo -e "\t\t [++TEST $test_suite a passed++] Ok!"
fi

#*********************************test b******************************************
TEST_1B_PASSED=true
if [ "$TEST_1A_PASSED" = true ] ; then
	echo -n "[TEST $test_suite b] Checking correctness of output file...";
	$HOME_DIR/$TOOLS_DIR/$COMPARATOR_EXEC $OUTPUT_FILE $HOME_DIR/$TOOLS_DIR/$EXPECTED_OUTPUT_FILE $ERROR
	rc=$?
	if [[ $rc != 0 ]] ; then
		echo -e "\t[--TEST FAILED--] $OUTPUT_FILE contains wrong data. Test aborted!"
		TEST_1B_PASSED=false
	else
		((final_score++))
		echo -e "\t [++TEST $test_suite b passed++] Ok!"
	fi
else
	echo -e "[--TEST $test_suite b skipped--]";
fi

#*********************************test c******************************************
TEST_1C_PASSED=true
echo "Copying file $OUTPUT_FILE..."
cp $OUTPUT_FILE temp_file
rm -f $OUTPUT_FILE	# We must check that this file will be recreated
if [ "$TEST_1A_PASSED" = true ] ; then
	echo -n "[TEST $test_suite c] Testing DECODE operation...";
	$TIMEOUT $MAX_EXEC_TIME $1 127.0.0.1 $port DECODE temp_file &> /dev/null
	if [ ! -e $OUTPUT_FILE ] ; then
		echo -e "\t\t\t [--TEST FAILED--] File $OUTPUT_FILE not generated."
		TEST_1C_PASSED=false
	else
		((final_score++))
		echo -e "\t\t\t [++TEST $test_suite c passed++] Ok!"
	fi
else
	echo -e "[--TEST $test_suite c skipped--]";
fi

#*********************************test d******************************************
TEST_1D_PASSED=true
if [ "$TEST_1C_PASSED" = true ] ; then
	echo -n "[TEST $test_suite d] Checking correctness of output file...";
	$HOME_DIR/$TOOLS_DIR/$COMPARATOR_EXEC $OUTPUT_FILE $HOME_DIR/$TOOLS_DIR/$DATA_FILE $ERROR
	rc=$?
	if [[ $rc != 0 ]] ; then
		echo -e "\t [--TEST FAILED--] $OUTPUT_FILE contains wrong data. Test aborted!"
		TEST_1D_PASSED=false
	else
		((final_score++))
		echo -e "\t [++TEST $test_suite d passed++] Ok!"
	fi
else
	echo -e "[--TEST $test_suite d skipped--]";
fi

if [[ $final_score == 4 ]] ; then
	echo "Error: 0"	# Everything ok!
else
	echo "Error: 1"	# At least one test failed!
fi

# Return the score ranging from 0 to 4
exit $final_score
