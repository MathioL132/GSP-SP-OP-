#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <set>

void print_usage(const char* program_name) {
    printf("Usage: %s nC lC nK lK three_edges [seed]\n", program_name);
    printf("  nC: number of cycle subgraphs\n");
    printf("  lC: length of cycles (must be at least 3)\n");
    printf("  nK: number of complete subgraphs\n");
    printf("  lK: size of complete subgraphs (must be at least 3)\n");
    printf("  three_edges: connect with 3 edges instead of 2 (0=no, 1=yes)\n");
    printf("  seed: random seed (optional, uses current time if not provided)\n");
}

int main(int argc, char* argv[]) {
    if (argc < 6 || argc > 7) {
        print_usage(argv[0]);
        return 1;
    }

    long nC = atol(argv[1]);
    long lC = atol(argv[2]);
    long nK = atol(argv[3]);
    long lK = atol(argv[4]);
    long three_edges = atol(argv[5]);
    long seed = (argc == 7) ? atol(argv[6]) : time(0);

    // Validating parameterss
    if (lC < 3) {
        fprintf(stderr, "Error: lC must be at least 3\n");
        return 1;
    }
    if (lK < 3) {
        fprintf(stderr, "Error: lK must be at least 3\n");
        return 1;
    }
    if (nC < 0 || nK < 0) {
        fprintf(stderr, "Error: nC and nK must be non-negative\n");
        return 1;
    }
    if (nC + nK == 0) {
        fprintf(stderr, "Error: Must have at least one subgraph (nC + nK > 0)\n");
        return 1;
    }

    srand(seed);

    long n = nC * lC + nK * lK;
    long m = nC * lC + nK * (lK * (lK - 1)) / 2 + (2 + three_edges) * (nC + nK - 1);

    // Use vectors to store edges for easier management
    std::vector<std::pair<long, long>> edges;
    edges.reserve(m);

    // Shuffle nodes
    std::vector<long> nodes(n);
    for (long i = 0; i < n; i++) {
        nodes[i] = i;
    }
    for (long i = 0; i < n; i++) {
        long j = i + rand() % (n - i);
        std::swap(nodes[i], nodes[j]);
    }

    // Shuffle types
    std::vector<char> graph_type;
    for (long i = 0; i < nC; i++) {
        graph_type.push_back(0); // cycle
    }
    for (long i = 0; i < nK; i++) {
        graph_type.push_back(1); // complete
    }
    for (long i = 0; i < nC + nK; i++) {
        long j = i + rand() % (nC + nK - i);
        std::swap(graph_type[i], graph_type[j]);
    }

    // Create subgraph edges
    std::vector<long> startNode(nC + nK);
    long currentNode = 0;
    for (long i = 0; i < nC + nK; i++) {
        startNode[i] = currentNode;
        if (graph_type[i] == 0) { // cycle
            for (long j = 0; j < lC; j++) {
                edges.push_back({nodes[currentNode + j], nodes[currentNode + (j + 1) % lC]});
            }
            currentNode += lC;
        } else { // complete
            for (long j = 0; j < lK; j++) {
                for (long k = j + 1; k < lK; k++) {
                    edges.push_back({nodes[currentNode + j], nodes[currentNode + k]});
                }
            }
            currentNode += lK;
        }
    }

    // Connect the subgraphs in a tree structure
    for (long i = 1; i < nC + nK; i++) {
        long j = rand() % i;
        long mod1 = (graph_type[i] == 1) ? lK : lC;
        long mod2 = (graph_type[j] == 1) ? lK : lC;

        if (!three_edges) {
            long x1, y1, x2, y2;
            x1 = rand() % mod1;
            x2 = (x1 + (1 + rand() % (mod1 - 2))) % mod1;
            y1 = rand() % mod2;
            y2 = (y1 + (1 + rand() % (mod2 - 2))) % mod2;
            edges.push_back({nodes[startNode[i] + x1], nodes[startNode[j] + y1]});
            edges.push_back({nodes[startNode[i] + x2], nodes[startNode[j] + y2]});
        } else {
            long x1, y1, x2, y2, x3, y3;
            if (mod1 == 3) {
                x1 = 0; x2 = 1; x3 = 2;
            } else {
                x1 = rand() % mod1;
                x2 = (x1 + (2 + rand() % (mod1 - 3))) % mod1;
                x3 = (x1 + (1 + rand() % ((mod1 + x2 - x1 - 1) % mod1))) % mod1;
            }
            if (mod2 == 3) {
                y1 = 0; y2 = 1; y3 = 2;
            } else {
                y1 = rand() % mod2;
                y2 = (y1 + (2 + rand() % (mod2 - 3))) % mod2;
                y3 = (y1 + (1 + rand() % ((mod2 + y2 - y1 - 1) % mod2))) % mod2;
            }
            edges.push_back({nodes[startNode[i] + x1], nodes[startNode[j] + y1]});
            edges.push_back({nodes[startNode[i] + x2], nodes[startNode[j] + y2]});
            edges.push_back({nodes[startNode[i] + x3], nodes[startNode[j] + y3]});
        }
    }

    // Shuffle edges
    for (long i = 0; i < m; i++) {
        long j = i + rand() % (m - i);
        std::swap(edges[i], edges[j]);
        if (rand() % 2 == 0) {
            std::swap(edges[i].first, edges[i].second);
        }
    }

    // Remove duplicate edges, just in case shoudn't have any duplicates.
    std::set<std::pair<long, long>> unique_edges;
    for (const auto& edge : edges) {
        long u = edge.first, v = edge.second;
        if (u > v) std::swap(u, v); // normalize edge direction
        unique_edges.insert({u, v});
    }

   
    std::cout << n << " " << unique_edges.size() << "\n";
    for (const auto& edge : unique_edges) {
        std::cout << edge.first << " " << edge.second << "\n";
    }

    return 0;
}
