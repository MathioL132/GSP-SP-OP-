#!/bin/bash

echo "Quick Manual Test"
echo "================="

# Test 1: Generates a small graph
echo "Test 1: Generating small graph (2 cycles of length 4, 1 complete graph of size 4)..."
./graph_generator 2 4 1 4 0 12345 > test1.txt
echo "Generated graph:"
cat test1.txt
echo ""

# Test 2: Recognizes if it's series-parallel
echo "Test 2: Testing if graph is series-parallel..."
./sp_recognizer test1.txt
echo ""

# Test 3: Generates a graph that should be non-series-parallel
echo "Test 3: Generating potentially non-SP graph (with 3-edge connections)..."
./graph_generator 1 5 2 5 1 12346 > test2.txt
echo "Generated graph:"
head -5 test2.txt
echo "... (showing first 5 lines)"
echo ""

echo "Test 4: Testing if second graph is series-parallel..."
./sp_recognizer test2.txt
echo ""

# Cleanup
rm -f test1.txt test2.txt

echo "Quick test complete!"
