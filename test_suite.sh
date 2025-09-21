#!/bin/bash

# Comprehensive test suite for Graph Generator and SP Recognition
# Tests the graph generator (part 1) and SP recognition (part 2) programs

set -e  # Exit on any error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
GRAPH_GEN="./graph_generator"
SP_RECOG="./sp_recognition"
TEST_DIR="test_results"
TEMP_GRAPH="temp_graph.txt"

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    case $status in
        "PASS")
            echo -e "${GREEN}[PASS]${NC} $message"
            ;;
        "FAIL")
            echo -e "${RED}[FAIL]${NC} $message"
            ;;
        "INFO")
            echo -e "${BLUE}[INFO]${NC} $message"
            ;;
        "WARN")
            echo -e "${YELLOW}[WARN]${NC} $message"
            ;;
    esac
}

# Function to run a single test
run_test() {
    local test_name="$1"
    local nC="$2"
    local lC="$3"
    local nK="$4"
    local lK="$5"
    local three_edges="$6"
    local seed="$7"
    local expected_sp="$8"  # "SP", "NON-SP", or "UNKNOWN"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    print_status "INFO" "Running test: $test_name"
    print_status "INFO" "Parameters: nC=$nC lC=$lC nK=$nK lK=$lK three_edges=$three_edges seed=$seed"
    
    # Generate graph
    local gen_output
    if ! gen_output=$("$GRAPH_GEN" "$nC" "$lC" "$nK" "$lK" "$three_edges" "$seed" 2>&1); then
        print_status "FAIL" "$test_name: Graph generation failed"
        echo "Generation error: $gen_output"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
    
    # Save graph to file
    echo "$gen_output" > "$TEMP_GRAPH"
    
    # Validate graph format
    local first_line=$(head -n1 "$TEMP_GRAPH")
    local n_vertices=$(echo "$first_line" | cut -d' ' -f1)
    local n_edges=$(echo "$first_line" | cut -d' ' -f2)
    
    if ! [[ "$n_vertices" =~ ^[0-9]+$ ]] || ! [[ "$n_edges" =~ ^[0-9]+$ ]]; then
        print_status "FAIL" "$test_name: Invalid graph format"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
    
    local actual_edge_lines=$(($(wc -l < "$TEMP_GRAPH") - 1))
    if [ "$actual_edge_lines" -ne "$n_edges" ]; then
        print_status "FAIL" "$test_name: Edge count mismatch (expected $n_edges, got $actual_edge_lines)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
    
    print_status "INFO" "Generated graph: $n_vertices vertices, $n_edges edges"
    
    # Run SP recognition
    local sp_output
    local sp_exit_code=0
    sp_output=$("$SP_RECOG" "$TEMP_GRAPH" 2>&1) || sp_exit_code=$?
    
    if [ $sp_exit_code -ne 0 ]; then
        print_status "FAIL" "$test_name: SP recognition failed (exit code $sp_exit_code)"
        echo "SP recognition error: $sp_output"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
    
    # Parse SP recognition result
    local is_sp="UNKNOWN"
    if echo "$sp_output" | grep -q "The graph IS series-parallel"; then
        is_sp="SP"
    elif echo "$sp_output" | grep -q "The graph is NOT series-parallel"; then
        is_sp="NON-SP"
    fi
    
    # Check if certificate was authenticated
    local auth_success=false
    if echo "$sp_output" | grep -q "Certificate authenticated successfully"; then
        auth_success=true
    fi
    
    print_status "INFO" "SP result: $is_sp, Authentication: $auth_success"
    
    # Validate result against expectation
    local test_passed=true
    if [ "$expected_sp" != "UNKNOWN" ] && [ "$is_sp" != "$expected_sp" ]; then
        print_status "FAIL" "$test_name: Expected $expected_sp but got $is_sp"
        test_passed=false
    fi
    
    if [ "$auth_success" != true ]; then
        print_status "FAIL" "$test_name: Certificate authentication failed"
        test_passed=false
    fi
    
    # Additional validation based on graph structure
    case "$test_name" in
        *"Single_Edge"*)
            if [ "$is_sp" != "SP" ]; then
                print_status "FAIL" "$test_name: Single edge should be SP"
                test_passed=false
            fi
            ;;
        *"Single_Cycle"*)
            if [ "$is_sp" != "SP" ]; then
                print_status "FAIL" "$test_name: Single cycle should be SP"
                test_passed=false
            fi
            ;;
        *"K4"*)
            if [ "$is_sp" != "NON-SP" ]; then
                print_status "FAIL" "$test_name: K4 should be non-SP"
                test_passed=false
            fi
            if ! echo "$sp_output" | grep -q "K4 subdivision"; then
                print_status "WARN" "$test_name: Expected K4 subdivision but got different reason"
            fi
            ;;
        *"K5"*)
            if [ "$is_sp" != "NON-SP" ]; then
                print_status "FAIL" "$test_name: K5 should be non-SP"
                test_passed=false
            fi
            ;;
    esac
    
    if [ "$test_passed" = true ]; then
        print_status "PASS" "$test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        
        # Save successful test results
        local result_file="$TEST_DIR/${test_name}.result"
        {
            echo "=== Test: $test_name ==="
            echo "Parameters: nC=$nC lC=$lC nK=$nK lK=$lK three_edges=$three_edges seed=$seed"
            echo "Graph: $n_vertices vertices, $n_edges edges"
            echo "Result: $is_sp"
            echo ""
            echo "=== Graph ==="
            cat "$TEMP_GRAPH"
            echo ""
            echo "=== SP Recognition Output ==="
            echo "$sp_output"
            echo ""
        } > "$result_file"
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
        
        # Save failed test for debugging
        local fail_file="$TEST_DIR/${test_name}.FAILED"
        {
            echo "=== FAILED Test: $test_name ==="
            echo "Parameters: nC=$nC lC=$lC nK=$nK lK=$lK three_edges=$three_edges seed=$seed"
            echo "Expected: $expected_sp, Got: $is_sp"
            echo ""
            echo "=== Graph ==="
            cat "$TEMP_GRAPH"
            echo ""
            echo "=== SP Recognition Output ==="
            echo "$sp_output"
            echo ""
        } > "$fail_file"
    fi
    
    echo ""
    return $test_passed
}

# Function to run stress test
run_stress_test() {
    local test_name="$1"
    local count="$2"
    
    print_status "INFO" "Running stress test: $test_name ($count iterations)"
    
    local stress_passed=0
    local stress_failed=0
    
    for i in $(seq 1 "$count"); do
        local seed=$((RANDOM + i * 1000))
        
        # Generate random parameters
        local nC=$((RANDOM % 5 + 1))
        local lC=$((RANDOM % 8 + 3))
        local nK=$((RANDOM % 3))
        local lK=$((RANDOM % 4 + 3))
        local three_edges=$((RANDOM % 2))
        
        local sub_test_name="${test_name}_${i}"
        
        if run_test "$sub_test_name" "$nC" "$lC" "$nK" "$lK" "$three_edges" "$seed" "UNKNOWN"; then
            stress_passed=$((stress_passed + 1))
        else
            stress_failed=$((stress_failed + 1))
            # Don't fail immediately in stress test, just continue
        fi
        
        # Show progress
        if [ $((i % 10)) -eq 0 ]; then
            print_status "INFO" "Stress test progress: $i/$count completed"
        fi
    done
    
    print_status "INFO" "Stress test $test_name completed: $stress_passed passed, $stress_failed failed"
}

# Main testing function
main() {
    print_status "INFO" "Starting comprehensive test suite for Graph Generator and SP Recognition"
    
    # Check if programs exist
    if [ ! -x "$GRAPH_GEN" ]; then
        print_status "FAIL" "Graph generator not found or not executable: $GRAPH_GEN"
        exit 1
    fi
    
    if [ ! -x "$SP_RECOG" ]; then
        print_status "FAIL" "SP recognition program not found or not executable: $SP_RECOG"
        exit 1
    fi
    
    # Create test directory
    mkdir -p "$TEST_DIR"
    
    echo "=== BASIC FUNCTIONALITY TESTS ==="
    
    # Test 1: Simple cycle (should be SP)
    run_test "Simple_C4_Cycle" 1 4 0 0 0 12345 "SP"
    
    # Test 2: Simple triangle cycle (should be SP)
    run_test "Simple_C3_Triangle" 1 3 0 0 0 12346 "SP"
    
    # Test 3: Single K4 (should be non-SP)
    run_test "Single_K4" 0 0 1 4 0 12347 "NON-SP"
    
    # Test 4: Single K5 (should be non-SP)
    run_test "Single_K5" 0 0 1 5 0 12348 "NON-SP"
    
    # Test 5: Two cycles connected (should be SP)
    run_test "Two_Cycles_Connected" 2 4 0 0 0 12349 "SP"
    
    # Test 6: Mixed cycle and complete graph (should be non-SP due to K4)
    run_test "Cycle_Plus_K4" 1 3 1 4 0 12350 "NON-SP"
    
    # Test 7: Three edges connection
    run_test "Two_Cycles_Three_Edges" 2 3 0 0 1 12351 "NON-SP"
    
    echo "=== EDGE CASES ==="
    
    # Test 8: Minimum cycle size
    run_test "Min_Cycle_C3" 1 3 0 0 0 12352 "SP"
    
    # Test 9: Minimum complete graph size
    run_test "Min_Complete_K3" 0 0 1 3 0 12353 "SP"
    
    # Test 10: Large cycle
    run_test "Large_Cycle_C10" 1 10 0 0 0 12354 "SP"
    
    # Test 11: Multiple small components
    run_test "Multiple_Small_Cycles" 5 3 0 0 0 12355 "SP"
    
    echo "=== COMPLEX STRUCTURES ==="
    
    # Test 12: Mix of cycles and complete graphs
    run_test "Complex_Mix" 2 5 2 3 0 12356 "NON-SP"
    
    # Test 13: Many K3s (should remain SP)
    run_test "Many_Triangles" 0 0 5 3 0 12357 "SP"
    
    # Test 14: Chain of cycles
    run_test "Chain_Of_Cycles" 4 4 0 0 0 12358 "SP"
    
    echo "=== STRESS TESTING ==="
    
    # Run stress tests with random parameters
    run_stress_test "Random_Graphs_Stress" 50
    
    echo "=== VALIDATION TESTS ==="
    
    # Test parameter validation
    print_status "INFO" "Testing parameter validation..."
    
    # Test invalid lC
    if "$GRAPH_GEN" 1 2 0 0 0 12345 2>/dev/null; then
        print_status "FAIL" "Should reject lC < 3"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    else
        print_status "PASS" "Correctly rejected lC < 3"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # Test invalid lK
    if "$GRAPH_GEN" 0 0 1 2 0 12345 2>/dev/null; then
        print_status "FAIL" "Should reject lK < 3"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    else
        print_status "PASS" "Correctly rejected lK < 3"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # Test no subgraphs
    if "$GRAPH_GEN" 0 3 0 3 0 12345 2>/dev/null; then
        print_status "FAIL" "Should reject nC + nK = 0"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    else
        print_status "PASS" "Correctly rejected nC + nK = 0"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # Clean up
    rm -f "$TEMP_GRAPH"
    
    # Final report
    echo ""
    echo "=== FINAL REPORT ==="
    print_status "INFO" "Total tests run: $TOTAL_TESTS"
    print_status "INFO" "Passed: $PASSED_TESTS"
    print_status "INFO" "Failed: $FAILED_TESTS"
    
    local success_rate=$(( (PASSED_TESTS * 100) / TOTAL_TESTS ))
    print_status "INFO" "Success rate: ${success_rate}%"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        print_status "PASS" "ALL TESTS PASSED!"
        echo ""
        print_status "INFO" "Both programs appear to be working correctly."
        print_status "INFO" "Test results saved in: $TEST_DIR/"
        exit 0
    else
        print_status "FAIL" "Some tests failed. Check the detailed results in: $TEST_DIR/"
        echo ""
        print_status "INFO" "Failed test files are saved with .FAILED extension for debugging."
        exit 1
    fi
}

# Handle script arguments
if [ $# -eq 0 ]; then
    main
else
    case $1 in
        --help|-h)
            echo "Graph Generator and SP Recognition Test Suite"
            echo ""
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --help, -h     Show this help message"
            echo "  --quick, -q    Run only basic tests (faster)"
            echo "  --stress, -s   Run only stress tests"
            echo ""
            echo "The script expects the following executables in the current directory:"
            echo "  ./graph_generator  - The graph generator program"
            echo "  ./sp_recognition   - The SP recognition program"
            exit 0
            ;;
        --quick|-q)
            print_status "INFO" "Running quick test suite..."
            # Set stress test to smaller number
            run_stress_test() { 
                print_status "INFO" "Skipping stress test in quick mode"
            }
            main
            ;;
        --stress|-s)
            print_status "INFO" "Running stress tests only..."
            # Create test directory
            mkdir -p "$TEST_DIR"
            run_stress_test "Stress_Test_Only" 100
            ;;
        *)
            print_status "FAIL" "Unknown option: $1"
            print_status "INFO" "Use --help for usage information"
            exit 1
            ;;
    esac
fi
