#!/bin/bash

echo "Building and Testing Graph Generator and Series-Parallel Recognizer"
echo "=================================================================="

# Cleasn up any existing files
rm -f graph_generator sp_recognizer tester test_graph.txt sp_result.txt

# Compiles graph generator
echo "Compiling graph generator..."
if clang++ -std=c++20 -Wall -Wextra graph_generator.cpp -o graph_generator; then
    echo "✓ Graph generator compiled successfully"
else
    echo "✗ Failed to compile graph generator"
    exit 1
fi

# Compiles series-parallel recognizer
echo "Compiling series-parallel recognizer..."
if clang++ -std=c++20 -Wall -Wextra sp_recognizer.cpp -o sp_recognizer; then
    echo "✓ Series-parallel recognizer compiled successfully"
else
    echo "✗ Failed to compile series-parallel recognizer"
    exit 1
fi

# Compiles tester
echo "Compiling tester..."
if clang++ -std=c++20 -Wall -Wextra tester.cpp -o tester; then
    echo "✓ Tester compiled successfully"
else
    echo "✗ Failed to compile tester"
    exit 1
fi

echo ""
echo "Running tests..."
echo ""

# Run the tests
./tester

echo ""
echo "Testing complete!"
