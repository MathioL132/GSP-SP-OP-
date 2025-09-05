#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <cassert>

struct TestResult {
    bool generator_success;
    bool recognizer_success;
    bool is_series_parallel;
    std::string error_message;
    double generation_time;
    double recognition_time;
    int vertices;
    int edges;
};

class TestSuite {
private:
    int tests_run = 0;
    int tests_passed = 0;
    std::vector<TestResult> results;

public:
    void run_test(const std::string& name, long nC, long lC, long nK, long lK, long three_edges, long seed = -1) {
        tests_run++;
        std::cout << "Test " << tests_run << ": " << name << " ... " << std::flush;
        
        TestResult result = run_single_test(nC, lC, nK, lK, three_edges, seed);
        results.push_back(result);
        
        if (result.generator_success && result.recognizer_success) {
            tests_passed++;
            std::cout << "PASSED";
            if (result.is_series_parallel) {
                std::cout << " (SP)";
            } else {
                std::cout << " (Non-SP)";
            }
            std::cout << " [" << result.vertices << "v," << result.edges << "e]";
            std::cout << " Gen:" << std::fixed << std::setprecision(1) << result.generation_time << "ms";
            std::cout << " Rec:" << std::fixed << std::setprecision(1) << result.recognition_time << "ms";
            std::cout << std::endl;
        } else {
            std::cout << "FAILED - " << result.error_message << std::endl;
        }
    }
    
    TestResult run_single_test(long nC, long lC, long nK, long lK, long three_edges, long seed) {
        TestResult result = {};
        
    
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::string gen_cmd = "./graph_generator " + std::to_string(nC) + " " + 
                             std::to_string(lC) + " " + std::to_string(nK) + " " + 
                             std::to_string(lK) + " " + std::to_string(three_edges);
        if (seed != -1) {
            gen_cmd += " " + std::to_string(seed);
        }
        gen_cmd += " > test_graph.txt 2>&1";
        
        int gen_exit_code = system(gen_cmd.c_str());
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.generation_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        if (gen_exit_code != 0) {
            result.generator_success = false;
            result.error_message = "Graph generator failed";
            return result;
        }
        
  
        std::ifstream graph_file("test_graph.txt");
        if (!graph_file.is_open()) {
            result.generator_success = false;
            result.error_message = "Graph file not created";
            return result;
        }
        
        //  graph size info
        graph_file >> result.vertices >> result.edges;
        graph_file.close();
        
        result.generator_success = true;
        
        // Test series-parallel recognizer
        start_time = std::chrono::high_resolution_clock::now();
        
        std::string rec_cmd = "./sp_recognizer test_graph.txt > sp_result.txt 2>&1";
        int rec_exit_code = system(rec_cmd.c_str());
        
        end_time = std::chrono::high_resolution_clock::now();
        result.recognition_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        if (rec_exit_code != 0) {
            result.recognizer_success = false;
            result.error_message = "SP recognizer failed";
            return result;
        }
        
        // Read recognizer output
        std::ifstream result_file("sp_result.txt");
        if (!result_file.is_open()) {
            result.recognizer_success = false;
            result.error_message = "SP result file not created";
            return result;
        }
        
        std::string line;
        std::getline(result_file, line);
        result.is_series_parallel = (line.find("YES") != std::string::npos);
        result.recognizer_success = true;
        
        return result;
    }
    
    void print_summary() {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "Test Summary:" << std::endl;
        std::cout << "Total tests: " << tests_run << std::endl;
        std::cout << "Passed: " << tests_passed << std::endl;
        std::cout << "Failed: " << (tests_run - tests_passed) << std::endl;
        std::cout << "Success rate: " << std::fixed << std::setprecision(1) 
                  << (100.0 * tests_passed / tests_run) << "%" << std::endl;
        
        // Stats
        if (!results.empty()) {
            double avg_gen_time = 0, avg_rec_time = 0;
            int sp_count = 0, total_vertices = 0, total_edges = 0;
            
            for (const auto& r : results) {
                if (r.generator_success && r.recognizer_success) {
                    avg_gen_time += r.generation_time;
                    avg_rec_time += r.recognition_time;
                    total_vertices += r.vertices;
                    total_edges += r.edges;
                    if (r.is_series_parallel) sp_count++;
                }
            }
            
            int successful_tests = tests_passed;
            if (successful_tests > 0) {
                avg_gen_time /= successful_tests;
                avg_rec_time /= successful_tests;
                
                std::cout << "\nStatistics:" << std::endl;
                std::cout << "Series-parallel graphs: " << sp_count << "/" << successful_tests 
                          << " (" << std::fixed << std::setprecision(1) 
                          << (100.0 * sp_count / successful_tests) << "%)" << std::endl;
                std::cout << "Average generation time: " << std::fixed << std::setprecision(2) 
                          << avg_gen_time << "ms" << std::endl;
                std::cout << "Average recognition time: " << std::fixed << std::setprecision(2) 
                          << avg_rec_time << "ms" << std::endl;
                std::cout << "Total vertices processed: " << total_vertices << std::endl;
                std::cout << "Total edges processed: " << total_edges << std::endl;
            }
        }
        
        std::cout << std::string(70, '=') << std::endl;
    }
};

int main() {
    std::cout << "Testing Graph Generator and Series-Parallel Recognizer" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // Check if executables exist
    if (system("test -f ./graph_generator") != 0) {
        std::cerr << "Error: graph_generator executable not found!" << std::endl;
        std::cerr << "Please compile it first with: clang++ -std=c++20 -Wall -Wextra graph_generator.cpp -o graph_generator" << std::endl;
        return 1;
    }
    
    if (system("test -f ./sp_recognizer") != 0) {
        std::cerr << "Error: sp_recognizer executable not found!" << std::endl;
        std::cerr << "Please compile it first with: clang++ -std=c++20 -Wall -Wextra sp_recognizer.cpp -o sp_recognizer" << std::endl;
        return 1;
    }
    
    TestSuite suite;
    
    // Basic tests
    std::cout << "\n--- Basic Tests ---" << std::endl;
    suite.run_test("Small cycle", 1, 3, 0, 3, 0, 12345);
    suite.run_test("Small complete graph", 0, 3, 1, 4, 0, 12346);
    suite.run_test("Mixed small", 1, 4, 1, 3, 0, 12347);
    
    // Medium tests
    std::cout << "\n--- Medium Tests ---" << std::endl;
    suite.run_test("Multiple cycles", 3, 5, 0, 3, 0, 12348);
    suite.run_test("Multiple complete", 0, 3, 3, 4, 0, 12349);
    suite.run_test("Mixed medium", 2, 6, 2, 5, 0, 12350);
    
    // Three-edge connection tests
    std::cout << "\n--- Three-Edge Connection Tests ---" << std::endl;
    suite.run_test("Cycles with 3-edges", 2, 4, 0, 3, 1, 12351);
    suite.run_test("Complete with 3-edges", 0, 3, 2, 4, 1, 12352);
    suite.run_test("Mixed with 3-edges", 1, 5, 1, 4, 1, 12353);
    
    // Larger tests
    std::cout << "\n--- Larger Tests ---" << std::endl;
    suite.run_test("Large cycles", 5, 8, 0, 3, 0, 12354);
    suite.run_test("Large complete", 0, 3, 4, 6, 0, 12355);
    suite.run_test("Large mixed", 3, 10, 3, 7, 0, 12356);
    
    // Edge cases
    std::cout << "\n--- Edge Cases ---" << std::endl;
    suite.run_test("Minimal cycle", 1, 3, 0, 3, 0, 12357);
    suite.run_test("Minimal complete", 0, 3, 1, 3, 0, 12358);
    suite.run_test("Single large cycle", 1, 20, 0, 3, 0, 12359);
    suite.run_test("Single large complete", 0, 3, 1, 10, 0, 12360);
    
    // Random seed tests
    std::cout << "\n--- Random Seed Tests ---" << std::endl;
    for (int i = 0; i < 5; i++) {
        suite.run_test("Random test " + std::to_string(i+1), 2, 6, 2, 5, 0);
    }
    
    suite.print_summary();
    
    // Cleanup
    system("rm -f test_graph.txt sp_result.txt");
    
    return 0;
}
