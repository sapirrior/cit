#!/bin/bash
# Comprehensive Test Script for Cit Workflow

CIT_BIN="../../build/cit"
TEST_DIR="tests/test_1"

# Utility to print status
function check_status {
    if [ $1 -eq 0 ]; then
        echo "[PASS] $2"
    else
        echo "[FAIL] $2"
        exit 1
    fi
}

echo "Starting Comprehensive Test 1..."
mkdir -p $TEST_DIR
cd $TEST_DIR
rm -rf .cit
rm -f file1.txt file2.txt

# 1. Init
$CIT_BIN init
check_status $? "Repository initialization"

# 2. Config
$CIT_BIN config -u "Test User"
$CIT_BIN config -e "test@example.com"
check_status $? "User configuration"

# 3. Add
echo "Content for file 1" > file1.txt
$CIT_BIN add file1.txt
check_status $? "Add file1.txt"

# 4. Status (Check if file1 is staged)
$CIT_BIN status | grep "file1.txt" > /dev/null
check_status $? "Status reports file1.txt"

# 5. Commit
$CIT_BIN commit "First commit with file1"
check_status $? "First commit"

# 6. Check Object Storage (CIT-LZ Verification)
OBJ_COUNT=$(find .cit/objects -type f | wc -l)
if [ "$OBJ_COUNT" -gt 0 ]; then
    echo "[PASS] Objects created in store"
else
    echo "[FAIL] No objects created"
    exit 1
fi

# 7. Add second file
echo "Content for file 2" > file2.txt
$CIT_BIN add file2.txt
check_status $? "Add file2.txt"

# 8. Second Commit
$CIT_BIN commit "Second commit with file2"
check_status $? "Second commit"

# 9. Log
$CIT_BIN log | grep "First commit with file1" > /dev/null
check_status $? "Log contains first commit"
$CIT_BIN log | grep "Second commit with file2" > /dev/null
check_status $? "Log contains second commit"

# 10. Branch
$CIT_BIN branch feature-1
check_status $? "Create branch feature-1"
$CIT_BIN branch | grep "feature-1" > /dev/null
check_status $? "Branch list contains feature-1"

echo "---------------------------------------"
echo "COMPREHENSIVE TEST 1 COMPLETED SUCCESSFULLY"
echo "---------------------------------------"
