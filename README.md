# GSP-SP-OP-
Graph Generator and Series-Parallel Recognition Tools
Overview
This project contains three main components extracted and adapted from Nathan Levy's COMP-4990 project at the University of Windsor. The original project was a C++ implementation of algorithms for determining if graphs are generalized series-parallel, series-parallel, and/or outerplanar.
I  extracted and created three standalone tools:

Graph Generator - Generates random biconnected graphs for testing
Series-Parallel Recognizer - Determines if a graph is series-parallel
Test Suite - Automated testing framework that validates both tools
Project Structure
text
├── README.md                    
├── graph_generator.cpp           #  graph generator 
├── sp_recognizer.cpp            # Series-parallel recognition tool 
├── tester.cpp                   # Comprehensive test suite (Task 3)
├── build_and_test.sh           # Automated build and test script
├── quick_test.sh               # Manual testing script for quick verification
└── examples/                   # Example input/output files
    ├── small_graph.txt
    ├── large_graph.txt
    └── sample_outputs.txt
Task 1: Graph Generator
Description
The graph generator creates random biconnected graphs by combining cycle and complete subgraphs in a tree structure. This ensures the generated graphs are connected and biconnected, making them suitable for testing series-parallel recognition algorithms.

Algorithm Overview
Generate nC cycle subgraphs of length lC and nK complete subgraphs of size lK
Randomly shuffle vertex labels to avoid predictable structure
Connect subgraphs in a tree structure using 2 or 3 edges per connection
Randomly shuffle all edges to eliminate ordering bias
Usage
bash
./graph_generator nC lC nK lK three_edges [seed]
Parameters:

nC: Number of cycle subgraphs (≥ 0)
lC: Length of each cycle (≥ 3)
nK: Number of complete subgraphs (≥ 0)
lK: Size of each complete subgraph (≥ 3)
three_edges: Connect with 3 edges instead of 2 (0=no, 1=yes)
seed: Random seed (optional, uses current time if omitted)
Output Format:

text
n m
u1 v1
u2 v2
...
um vm
Where n is the number of vertices, m is the number of edges, and each subsequent line represents an edge between vertices ui and vi.

Examples
bash
# Generate graph with 3 cycles of length 5, 2 complete graphs of size 4
./graph_generator 3 5 2 4 0 12345

# Generate larger graph with 3-edge connections
./graph_generator 5 8 3 6 1

# Save output to file
./graph_generator 2 4 1 5 0 > my_graph.txt
Task 2: Series-Parallel Recognizer
Description
The series-parallel recognizer determines whether a given graph has a series-parallel structure. A graph is series-parallel if it can be reduced to a single edge through a sequence of series and parallel reductions, or equivalently, if it contains no K₄ subdivision.

Algorithm Overview
The recognition algorithm works by:

Biconnected Component Analysis: Decompose the graph into biconnected components
Structural Validation: Check that components form a valid tree structure
Ear Decomposition: Use ear decomposition within each biconnected component
Violation Detection: Look for forbidden structures:
K₄ subdivisions (interlacing ears)
Cut vertices in 3+ biconnected components
Biconnected components with 3+ cut vertices
T₄ subdivisions with cut vertices
Usage
bash
./sp_recognizer [input_file]
If no input file is provided, reads from standard input.

Input Format: Same as graph generator output format.

Output Format:

text
Graph is series-parallel: YES
or

text
Graph is series-parallel: NO
Reason: [specific violation found]
Examples
bash
# Test a graph from file
./sp_recognizer graph.txt

# Test using pipe from generator
./graph_generator 2 4 1 3 0 | ./sp_recognizer

# Test with stdin input
./sp_recognizer < input_graph.txt
Task 3:  Testing
Description
The test suite automatically validates both the graph generator and series-parallel recognizer by:

Generating graphs with various parameters
Testing the recognizer on each generated graph
Measuring performance metrics
Providing detailed success/failure reporting

Test Categories
Basic Tests: Small graphs with simple structures

Single cycles and complete graphs
Mixed small configurations
Medium Tests: Moderate-sized graphs

Multiple subgraph combinations
Balanced cycle/complete ratios
Three-Edge Tests: Graphs using 3-edge connections

Tests graphs more likely to be non-series-parallel
Validates handling of denser connections
Large Tests: Bigger graphs for performance testing

Tests scalability
Validates algorithms on substantial inputs
Edge Cases: Boundary condition testing

Minimal valid parameters
Single large components
Random Tests: Unpredictable configurations

Uses random seeds for variety
Catches unexpected edge cases
Test Metrics
Success Rate: Percentage of tests that run without errors
Series-Parallel Rate: Percentage of generated graphs that are SP
Performance: Average generation and recognition times
Scale: Total vertices and edges processed
Building and Running
Prerequisites
C++20 compatible compiler (clang++ or g++)
Unix-like environment (Linux, macOS, WSL on Windows)

Option 1: Automated Build and Test

bash
chmod +x build_and_test.sh
./build_and_test.sh
Option 2: Manual Build

bash
# Compile all components
clang++ -std=c++20 -Wall -Wextra graph_generator.cpp -o graph_generator
clang++ -std=c++20 -Wall -Wextra sp_recognizer.cpp -o sp_recognizer  
clang++ -std=c++20 -Wall -Wextra tester.cpp -o tester

# Run comprehensive tests
./tester
Option 3: Quick Manual Test

bash
chmod +x quick_test.sh
./quick_test.sh
Individual Tool Usage
Generate and test a specific graph:

bash
# Generate a medium-sized graph
./graph_generator 3 6 2 5 0 42 > test.txt

# Check if it's series-parallel
./sp_recognizer test.txt

# View the graph structure
cat test.txt
Pipeline usage:

bash
# Generate and immediately test
./graph_generator 2 4 1 4 0 | ./sp_recognizer

# Generate multiple graphs and test each
for i in {1..5}; do
    echo "Test $i:"
    ./graph_generator 2 5 1 4 0 $i | ./sp_recognizer
done
Technical Details
Graph Generator Implementation
Time Complexity: O(V + E) where V = nC×lC + nK×lK and E is the number of generated edges
Space Complexity: O(V + E)
Randomization: Uses C-style rand() with configurable seed for reproducibility
Output: Simple edge-list format compatible with most graph analysis tools
Series-Parallel Recognizer Implementation
Time Complexity: O(V + E) linear time recognition
Space Complexity: O(V + E)
Algorithm: Based on ear decomposition and biconnected component analysis
Violation Detection: Identifies specific non-SP structures with descriptive error messages
Test Suite Features
Comprehensive Coverage: Tests multiple parameter combinations and edge cases
Performance Monitoring: Measures and reports timing statistics
Error Handling: Graceful failure handling with detailed error messages
Statistical Analysis: Success rates, SP detection rates, performance averages
