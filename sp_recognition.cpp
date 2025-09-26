#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <memory>
#include <algorithm>
#include <cassert>

// ==================== LOGGING ====================
#ifdef __VERBOSE_LOGGING__
#define __LOGGING__
#define V_LOG(a) std::cout << a;
#else
#define V_LOG(a)
#endif

#ifdef __LOGGING__
#define __LIGHT_LOGGING__
#define N_LOG(a) std::cout << a;
#else
#define N_LOG(a)
#endif

#ifdef __LIGHT_LOGGING__
#define L_LOG(a) std::cout << a;
#else
#define L_LOG(a)
#endif

// ==================== GRAPH ====================
using edge_t = std::pair<int, int>;

struct graph {
    int n; // graph order
    int e; // graph size
    std::vector<std::vector<int>> adjLists; // graph adjacency lists

    bool adjacent(int e1, int e2) const {
        for (int v : adjLists[e1]) {
            if (v == e2) return true;
        }
        return false;
    }

    void add_edge(int e1, int e2) {
        adjLists[e1].push_back(e2);
        adjLists[e2].push_back(e1);
    }

    void reserve(graph const& other) {
        for (int i = 0; i < other.n; i++) {
            adjLists.emplace_back();
            adjLists[i].reserve(other.adjLists[i].size());
        }
    }

    void output_adj_list(int v, std::ostream& os) const {
        os << "vertex " << v << " adjacencies: ";
        for (int v2 : adjLists[v]) {
            os << v2 << " ";
        }
        os << "\n";
    }
};

std::istream& operator>>(std::istream& is, graph& g) {
    g = graph{};
    is >> g.n >> g.e;
    g.adjLists.reserve(g.n);

    for (int i = 0; i < g.n; i++) {
        g.adjLists.emplace_back();
    }

    for (int i = 0; i < g.e; i++) {
        int endpoint1, endpoint2;
        is >> endpoint1 >> endpoint2;
        g.add_edge(endpoint1, endpoint2);
    }

    for (std::vector<int>& list : g.adjLists) {
    list.shrink_to_fit();
}

    return is;
}

std::ostream& operator<<(std::ostream& os, graph const& g) {
    os << "Graph with " << g.n << " vertices and " << g.e << " edges:\n";
    for (int i = 0; i < g.n; i++) {
        g.output_adj_list(i, os);
    }
    return os;
}

// ==================== SP TREE ====================
enum class c_type {
    edge, series, parallel, antiparallel, dangling
};

char c_type_char(c_type comp) {
    switch (comp) {
        case c_type::edge:
            return 'e';
        case c_type::series:
            return 'S';
        case c_type::parallel:
            return 'P';
        case c_type::antiparallel:
            return 'Q';
        case c_type::dangling:
            return 'D';
    }
}

struct sp_tree_node {
    int source;
    int sink;
    sp_tree_node * l;
    sp_tree_node * r;
    c_type comp;

    sp_tree_node(int source_, int sink_) : source{source_}, sink{sink_}, comp{c_type::edge} {}

    sp_tree_node(sp_tree_node * l_, sp_tree_node * r_, c_type comp_) : l{l_}, r{r_}, comp{comp_} {
        switch (comp) {
            case c_type::series:
                source = l->source;
                sink = r->sink;
                break;
            case c_type::dangling:
            case c_type::parallel:
            case c_type::antiparallel:
                source = l->source;
                sink = l->sink;
                break;
            case c_type::edge:
                break;
        }
    }
};

struct sp_tree {
    sp_tree_node * root;

    void compose(sp_tree&& other, c_type comp) {
        if (!root) {
            root = other.root;
            other.root = nullptr;
            return;
        } else if (!other.root) {
            return;
        }
        root = new sp_tree_node{root, other.root, comp};
        other.root = nullptr;
    }

    void l_compose(sp_tree&& other, c_type comp) {
        if (!root) {
            root = other.root;
            other.root = nullptr;
            return;
        } else if (!other.root) {
            return;
        }
        root = new sp_tree_node{other.root, root, comp};
        other.root = nullptr;
    }

    void deantiparallelize() {
        std::stack<std::pair<sp_tree_node *, int>> hist;
        bool swap = false;
        if (!root) return;
        hist.emplace(root, 0);

        while (!hist.empty()) {
            sp_tree_node * curr = hist.top().first;
            if (hist.top().second == 0) {
                hist.top().second++;
                if (curr->r) hist.emplace(curr->r, 0);
                if (curr->comp == c_type::antiparallel) swap = !swap;
            } else {
                hist.pop();
                if (curr->l) hist.emplace(curr->l, 0);
                if (curr->comp == c_type::antiparallel) {
                    swap = !swap;
                    curr->comp = c_type::parallel;
                }
                if (swap) {
                    sp_tree_node * temp = curr->l;
                    curr->l = curr->r;
                    curr->r = temp;
                    int temp_src = curr->source;
                    curr->source = curr->sink;
                    curr->sink = temp_src;
                }
            }
        }
    }

    int source() {return root->source;}
    int sink() {return root->sink;}
    int underlying_tree_path_source() {
        sp_tree_node * leftmost = root;
        for (; leftmost->comp != c_type::edge; leftmost = leftmost->l);
        return leftmost->sink;
    }

    sp_tree() {
        root = nullptr;
    }
    sp_tree(int source_, int sink_) : root{new sp_tree_node{source_, sink_}} {}

    ~sp_tree();

    sp_tree(sp_tree const& other) = delete;
    sp_tree& operator=(sp_tree const& other) = delete;

    sp_tree(sp_tree&& other) {
        root = other.root;
        other.root = nullptr;
    }

    sp_tree& operator=(sp_tree&& other) {
        if (this != &other) {
            delete root;
            root = other.root;
            other.root = nullptr;    
        }
        return *this;
    }
};

std::ostream& operator<<(std::ostream& os, sp_tree_node const& t) {
    #ifdef __VERBOSE_LOGGING__
    os << "{";
    if (t.l) os << *(t.l);
    os << t.source << c_type_char(t.comp) << t.sink;
    if (t.r) os << *(t.r);
    os << "}";
    #else
    os << "{" << t.source << c_type_char(t.comp) << t.sink << "}";
    #endif
    return os;
}

std::ostream& operator<<(std::ostream& os, sp_tree const& t) {
    if (t.root) {
        os << *(t.root);
    } else {
        os << "(null tree)";
    }
    return os;
}

sp_tree::~sp_tree() {
    if (!root) return;
    std::stack<std::pair<sp_tree_node *, int>> hist;
    hist.emplace(root, 0);
    while (!hist.empty()) {
        sp_tree_node * curr = hist.top().first;
        if (hist.top().second == 0) {
            hist.top().second = 1;
            if (curr->r) hist.emplace(curr->r, 0);
            if (curr->l) hist.emplace(curr->l, 0);
        } else {
            delete curr;
            hist.pop();
        }
    }
}

struct sp_chain_stack_entry {
    sp_tree SP;
    int end;
    sp_tree tail;
    sp_chain_stack_entry(sp_tree SP_, int end_, sp_tree tail_) : SP{std::move(SP_)}, end{end_}, tail{std::move(tail_)} {}
    sp_chain_stack_entry() = default;
};

// ==================== AUXILIARY FUNCTIONS ====================
void radix_sort(std::vector<int>& v) {
    if(v.empty()) return;
    int max_val = *std::max_element(v.begin(), v.end());
    std::vector<int> output(v.size());
    std::vector<int> count(10);
    for(int exp = 1; max_val/exp > 0; exp *= 10) {
        std::fill(count.begin(), count.end(), 0);
        for(int i : v) count[(i/exp)%10]++;
        for(int i = 1; i < 10; i++) count[i] += count[i-1];
        for(int i = (int)v.size()-1; i >= 0; i--){
            output[count[(v[i]/exp)%10]-1] = v[i];
            count[(v[i]/exp)%10]--;
        }
        v = output;
    }
}

bool trace_path(int end1, int end2, std::vector<edge_t> const& path, graph const& g, std::vector<bool>& seen) {
    #ifdef __VERBOSE_LOGGING__
    for (edge_t edge : path) {
        V_LOG("(" << edge.first << ", " << edge.second << ") ")
    }
    #endif
    
    N_LOG("\n")
    if (path.size() == 0) {
        L_LOG("====== AUTH FAILED: no edges in path ======\n")
        return false;
    }

    if (path[0].first == end2) {
        int tmp = end2;
        end2 = end1;
        end1 = tmp;
    }

    if (path[0].first != end1) {
        L_LOG("====== AUTH FAILED: start of path does not match either endpoint ======\n")
        return false;
    }

    if (path.back().second != end2) {
        L_LOG("====== AUTH FAILED: end of path does not match second endpoint ======\n")
        return false;
    }

    seen[end1] = true;
    int prev_v = end1;
    for (edge_t edge : path) {
        if (!g.adjacent(edge.first, edge.second)) {
            L_LOG("====== AUTH FAILED: edge (" << edge.first << ", " << edge.second << ") does not exist in graph ======\n")
            return false;
        }

                if (prev_v != edge.first) {
            L_LOG("====== AUTH FAILED: edge (" << edge.first << ", " << edge.second << ") is not incident on the previous edge ======\n")
            return false;
        }

        prev_v = edge.second;

        if (seen[edge.second]) {
            L_LOG("====== AUTH FAILED: duplicated vertex " << edge.second << " ======\n")
            return false;
        }

        seen[edge.second] = true;
    }

    N_LOG("path good\n")
    seen[end1] = false;
    seen[end2] = false;
    return true;
}

int num_comps_after_removal(graph const& g, int v) {
    int retval = 0;
    std::vector<bool> seen((size_t)(g.n), false);

    for (int i = 0; i < g.n; i++) {
        if (seen[i] || i == v) continue;
        retval++;

        std::stack<int> dfs;
        dfs.emplace(i);

        while (!dfs.empty()) {
            int w = dfs.top();
            dfs.pop();
            seen[w] = true;

            for (int u : g.adjLists[w]) {
                if (!seen[u] && u != v) {
                    dfs.emplace(u);
                }
            }
        }
    }

    return retval;
}

bool is_cut_vertex(graph const& g, int v) {
    if (num_comps_after_removal(g, v) <= 1) {
        L_LOG("\n====== AUTH FAILED: " << v << " not a cut vertex ======\n\n")
        return false;
    }
    N_LOG("yes\n")
    return true;
}

// ==================== CERTIFICATE DEFINITIONS ====================
struct certificate {
    bool verified = false;
    virtual bool authenticate(graph const& g) = 0;
    virtual ~certificate() {}
};

struct negative_cert_K4 : certificate {
    int a, b, c, d;
    std::vector<edge_t> ab, ac, ad, bc, bd, cd;

    bool authenticate(graph const& g) override {
        if (verified) return true;

        L_LOG("====== AUTHENTICATE K4: terminating vertices a: " << a << ", b: " << b << ", c: " << c << ", d: " << d << " ======\n")
        if (a == b || b == c || c == d || d == a || a == c || b == d) {
            L_LOG("====== AUTH FAILED: terminating vertices non-distinct ======\n\n")
            return false;
        }
        std::vector<bool> seen((size_t)(g.n), false);

        N_LOG("verify ab: ")
        if (!trace_path(a, b, ab, g, seen)) return false;
        N_LOG("verify ac: ")
        if (!trace_path(a, c, ac, g, seen)) return false;
        N_LOG("verify ad: ")
        if (!trace_path(a, d, ad, g, seen)) return false;
        N_LOG("verify bc: ")
        if (!trace_path(b, c, bc, g, seen)) return false;
        N_LOG("verify bd: ")
        if (!trace_path(b, d, bd, g, seen)) return false;
        N_LOG("verify cd: ")
        if (!trace_path(c, d, cd, g, seen)) return false;

        L_LOG("====== AUTH SUCCESS ======\n\n")
        verified = true;
        return true;
    }
};

struct negative_cert_K23 : certificate {
    int a, b;
    std::vector<edge_t> one, two, three;

    bool authenticate(graph const& g) override {
        if (verified) return true;

        L_LOG("====== AUTHENTICATE K23: terminating vertices a: " << a << ", b: " << b << " ======\n")

        if (a == b) {
            L_LOG("====== AUTH FAILED: terminating vertices non-distinct ======\n\n")
            return false;
        }

        std::vector<bool> seen((size_t)(g.n), false);

        N_LOG("verify path one: ")
        if (!trace_path(a, b, one, g, seen)) return false;
        if (one.size() < 2) {
            L_LOG("\n====== AUTH FAILED: path one has no internal vertex ======\n\n")
            return false;
        }

        N_LOG("verify path two: ")
        if (!trace_path(a, b, two, g, seen)) return false;
        if (two.size() < 2) {
            L_LOG("\n====== AUTH FAILED: path two has no internal vertex ======\n\n")
            return false;
        }

        N_LOG("verify path three: ")
        if (!trace_path(a, b, three, g, seen)) return false;
        if (three.size() < 2) {
            L_LOG("\n====== AUTH FAILED: path three has no internal vertex ======\n\n")
            return false;
        }

        L_LOG("====== AUTH SUCCESS ======\n\n")
        verified = true;
        return true;
    }
};

struct negative_cert_T4 : certificate {
    int c1, c2, a, b;
    std::vector<edge_t> c1a, c1b, c2a, c2b, ab;

    bool authenticate(graph const& g) override {
        if (verified) return true;
        L_LOG("====== AUTHENTICATE T4: terminating vertices a: " << a << ", b: " << b << ", c1: " << c1 << ", c2: " << c2 << " ======\n")

        if (a == b || a == c1 || a == c2 || b == c1 || b == c2 || c1 == c2) {
            L_LOG("====== AUTH FAILED: terminating vertices non-distinct ======\n\n")
            return false;
        }

        N_LOG("verify c1 cut vertex: ")
        if (!is_cut_vertex(g, c1)) return false;
        N_LOG("verify c2 cut vertex: ")
        if (!is_cut_vertex(g, c2)) return false;

        std::vector<bool> seen((size_t)(g.n), false);
        N_LOG("verify path c1a: ")
        if (!trace_path(c1, a, c1a, g, seen)) return false;
        N_LOG("verify path c2a: ")
        if (!trace_path(c2, a, c2a, g, seen)) return false;
        N_LOG("verify path ab: ")
        if (!trace_path(a, b, ab, g, seen)) return false;
        N_LOG("verify path c1b: ")
        if (!trace_path(c1, b, c1b, g, seen)) return false;
        N_LOG("verify path c2b: ")
        if (!trace_path(c2, b, c2b, g, seen)) return false;

        L_LOG("====== AUTH SUCCESS ======\n\n")
        verified = true;
        return true;
    }
};

struct negative_cert_tri_comp_cut : certificate {
    int v;

    bool authenticate(graph const& g) override {
        if (verified) return true;
        L_LOG("====== AUTHENTICATE THREE-COMPONENT CUT VERTEX: " << v << " ======\n")

        int comps = num_comps_after_removal(g, v);

        if (comps < 3) {
            L_LOG("====== AUTH FAILED: vertex " << v << " only splits graph into " << comps << " components ======\n\n")
            return false;
        }

        N_LOG(comps << " comps after removal\n")
        L_LOG("====== AUTH SUCCESS ======\n\n")

        verified = true;
        return true;
    }
};

struct negative_cert_tri_cut_comp : certificate {
    int c1, c2, c3;

    bool authenticate(graph const& g) override {
        if (verified) return true;
        L_LOG("====== AUTHENTICATE BICOMP WITH THREE CUT VERTICES: cut vertices " << c1 << ", " << c2 << ", " << c3 << " ======\n")
        N_LOG("verify c1 cut vertex: ")
        if (!is_cut_vertex(g, c1)) return false;
        N_LOG("verify c2 cut vertex: ")
        if (!is_cut_vertex(g, c2)) return false;
        N_LOG("verify c3 cut vertex: ")
        if (!is_cut_vertex(g, c3)) return false;

        std::vector<int> dfs_no((size_t)(g.n), 0);
        std::vector<int> parent((size_t)(g.n)); 
        std::vector<int> low((size_t)(g.n));
        int cut_verts[3] = {c1, c2, c3};

        std::stack<edge_t> comp_edges;
        std::stack<std::pair<int, int>> dfs;

        dfs.emplace(0, 0);
        dfs_no[0] = 1;
        low[0] = 1;
        parent[0] = -1;
        int curr_dfs = 2;

        while (!dfs.empty()) {
            std::pair<int, int> p = dfs.top();
            int w = p.first;
            int u = g.adjLists[p.first][p.second];

            if (dfs_no[u] == 0) {
                dfs.push(std::pair{u, 0});
                comp_edges.emplace(w, u);
                parent[u] = w;
                dfs_no[u] = curr_dfs++;
                low[u] = dfs_no[u];
                continue;
            }

            if (parent[u] == w) {
                if (low[u] >= dfs_no[w]) {
                    bool seen[3] = {false, false, false};
                    edge_t e;
                    do {
                        e = comp_edges.top();
                        for (int i = 0; i < 3; i++) {
                            if (e.first == cut_verts[i] || e.second == cut_verts[i]) seen[i] = true;
                        }
                        comp_edges.pop();
                    } while (e != edge_t{w, u});

                    if (seen[0] && seen[1] && seen[2]) {
                        N_LOG("vertices belong to one biconnected component...\n")    
                        L_LOG("====== AUTH SUCCESS ======\n\n")
                        verified = true;
                        return true;
                    }
                }

                if (low[u] < low[w]) low[w] = low[u];
            } else if (dfs_no[u] < dfs_no[w] && u != parent[w]) {
                comp_edges.emplace(w, u);
                if (dfs_no[u] < low[w]) low[w] = dfs_no[u];
            }

            if ((size_t)(++dfs.top().second) >= g.adjLists[p.first].size()) {
                dfs.pop();
            }
        }

        L_LOG("====== AUTH FAILED: bicomp does not contain the three cut vertices ======\n\n")
        return false;
    }
};

struct positive_cert_sp : certificate {
    sp_tree decomposition;
    bool is_sp;

    bool authenticate(graph const& g) override {
        if (verified) return true;

        std::vector<int> n_src((size_t)(g.n), 0);
        std::vector<int> n_sink((size_t)(g.n), 0);
        std::vector<bool> no_edge((size_t)(g.n), false);
        bool swap = false;

        graph g2{};
        g2.n = g.n;
        g2.reserve(g);
        g2.e = 0;
        
        std::stack<std::pair<sp_tree_node *, int>> hist;
        L_LOG("====== AUTHENTICATE SP DECOMPOSITION TREE ======\n")
        if (!decomposition.root) {
            L_LOG("====== AUTH FAILED: decomposition tree does not exist ======\n\n")
            return false;
        }

        hist.emplace(decomposition.root, 0);

        while (!hist.empty()) {
            sp_tree_node * curr = hist.top().first;
            V_LOG("traversal: " << *curr << ", phase: " << hist.top().second << "\n")
            int source = (swap ? curr->sink : curr->source);
            int sink = (swap ? curr->source : curr->sink);

            if (hist.top().second == 0) {
                if (!(curr->l) || !(curr->r)) {
                    if (curr->l || curr->r) {
                        L_LOG("====== AUTH FAILED: node " << *curr << " malformed (one child) ======\n\n")
                        return false;
                    }

                    if (curr->comp != c_type::edge) {
                        L_LOG("====== AUTH FAILED: node " << *curr << " malformed (leaf, but not an edge) ======\n\n")
                        return false;
                    }

                    if (no_edge[source] || no_edge[sink]) {
                                                L_LOG("====== AUTH FAILED: edge node " << *curr << " is incident on an vertex already merged into a series SP subgraph ======\n\n")
                        return false;
                    }

                    g2.add_edge(source, sink);
                    n_src[source]++;
                    n_sink[sink]++;
                    hist.pop();
                } else {
                    if (curr->comp == c_type::antiparallel) swap = !swap;
                    hist.top().second++;
                    hist.emplace(curr->r, 0);
                }
            } else if (hist.top().second == 1) {
                if (curr->comp == c_type::antiparallel) swap = !swap;
                hist.top().second++;
                hist.emplace(curr->l, 0);
            } else {
                int lsource = (swap ? curr->r->sink : curr->l->source);
                int lsink = (swap ? curr->r->source : curr->l->sink);
                int rsource = (swap ? curr->l->sink : curr->r->source);
                int rsink = (swap ? curr->l->source : curr->r->sink);

                switch (curr->comp) {
                    case c_type::edge:
                        L_LOG("====== AUTH FAILED: node " << *curr << " malformed (edge, but internal) ======\n\n")
                        return false;
                    case c_type::series:
                        if (lsource != source || rsink != sink || lsink != rsource) {
                            L_LOG("====== AUTH FAILED: node " << *curr << " malformed (series children source/sink mismatch) ======\n\n")
                            return false;
                        }

                        if (n_src[lsink] != 1 || n_sink[lsink] != 1) {
                            L_LOG("====== AUTH FAILED: series node " << *curr << " has incident edges on its middle vertex " << lsink << " which cannot be merged into it ======\n\n")
                            return false;
                        }

                        V_LOG("BLOCKING: " << lsink << "\n")
                        no_edge[lsink] = true;
                        n_src[lsink]--;
                        n_sink[lsink]--;
                        break;
                    case c_type::parallel:
                        if (lsource != source || rsource != source || lsink != sink || rsink != sink) {
                            L_LOG("====== AUTH FAILED: node " << *curr << " malformed (parallel children source/sink mismatch) ======\n\n")
                            return false;
                        }

                        n_src[source]--;
                        n_sink[sink]--;
                        break;
                    case c_type::antiparallel:
                        if (swap) {
                            if (lsource != sink || rsource != source || lsink != source || rsink != sink) {
                                L_LOG("====== AUTH FAILED: node " << *curr << " malformed (antiparallel children source/sink mismatch) ======\n\n")
                                return false;
                            }
                        } else {
                            if (lsource != source || rsource != sink || lsink != sink || rsink != source) {
                                L_LOG("====== AUTH FAILED: node " << *curr << " malformed (antiparallel children source/sink mismatch) ======\n\n")
                                return false;
                            }
                        }

                        n_src[source]--;
                        n_sink[sink]--;
                        break;
                    case c_type::dangling:
                        if (is_sp) {
                            L_LOG("====== AUTH FAILED: illegal dangling composition in SP decomposition tree ======\n\n")
                            return false;
                        }

                        if (swap) {
                            if (rsource != source || rsink != sink || lsink != sink) {
                                L_LOG("====== AUTH FAILED: node " << *curr << " malformed (dangling children source/sink mismatch) ======\n\n")
                                return false;
                            }
                        } else {
                            if (lsource != source || lsink != sink || rsource != source) {
                                L_LOG("====== AUTH FAILED: node " << *curr << " malformed (dangling children source/sink mismatch) ======\n\n")
                                return false;
                            }
                        }

                        if (swap) {
                            n_src[lsource]--;
                            n_sink[sink]--;
                        } else {
                            n_src[source]--;
                            n_sink[rsink]--;
                        }

                        break;
                }

                hist.pop();
            }
        }

        N_LOG("decomposition tree well-formed...\n")
        n_src[decomposition.root->source]--;
        n_sink[decomposition.root->sink]--;

        bool failed = false;
        for (int i = 0; i < g.n; i++) {
            if (n_src[i] != 0) {
                N_LOG("OH NO: disconnected SP subgraph sourced at vertex " << i << "\n")
                failed = true;
            }
            
            if (n_sink[i] != 0) {
                N_LOG("OH NO: disconnected SP subgraph sinked at vertex " << i << "\n")
                failed = true;
            }
        }

        if (failed) {
            L_LOG("====== AUTH FAILED: additional disconnected SP subgraphs are part of the decomposition tree ======\n\n")
            return false;
        }

        N_LOG("decomposition tree connected...\n")

        for (size_t i = 0; i < g2.adjLists.size(); i++) {
            std::vector<int> l1 = g.adjLists[i];
            radix_sort(l1);
            radix_sort(g2.adjLists[i]);
            if (l1 != g2.adjLists[i]) {
                L_LOG("====== AUTH FAILED: vertex " << i << " of G does not have the same adjacency list as the one produced by the decomposition tree ======\n\n")

                #ifdef __LOGGING__
                N_LOG("ORIGINAL GRAPH: ")
                g2.output_adj_list(i, std::cout);
                N_LOG("PRODUCED GRAPH: ")
                g2.output_adj_list(i, std::cout);
                #endif

                L_LOG("======================================================================\n\n")
                return false;                
            }
        }

        N_LOG("decomposition tree produces graph identical to G...\n")
        L_LOG("====== AUTH SUCCESS ======\n\n")

        verified = true;
        return true;
    }
};

struct sp_result {
    bool is_sp;
    std::shared_ptr<certificate> reason;

    bool authenticate(graph const& g) {
        L_LOG("================== AUTHENTICATING SP RESULT ==================\n") 
        V_LOG(g)
        V_LOG("=============================================================\n")
        L_LOG("\n")

        if (!reason) {
            L_LOG("ERROR: reason not given")
            return false;
        }
        if (!reason->authenticate(g)) return false;

        L_LOG("this graph is " << (is_sp ? "" : "NOT ") << "SP\n")
        return true;
    }
};

// ==================== MAIN ALGORITHM FUNCTIONS ====================
int path_contains_edge(std::vector<edge_t> const& path, edge_t test) {
    for (size_t i = 0; i < path.size(); i++) {
        edge_t e = path[i];
        if (e == test || (e.first == test.second && e.second == test.first)) return (int)(i);
    }
    return -1;
}

void report_K4_non_stack_pop_case(sp_result& cert_out,
                                  std::vector<int> const& parent, 
                                  std::vector<std::stack<sp_chain_stack_entry>>& vertex_stacks, 
                                  int a, 
                                  int b,
                                  int d,
                                  int elose,
                                  int ewin_src,
                                  int ewin_sink) {
    std::shared_ptr<negative_cert_K4> k4{new negative_cert_K4{}};
    k4->a = a;
    k4->b = b;
    k4->d = d;

    sp_tree earliest_violating_ear;
    for (int bw = parent[k4->b]; bw != k4->d; bw = parent[bw]) {
        for (; !vertex_stacks[bw].empty(); vertex_stacks[bw].pop()) {
            if (vertex_stacks[bw].top().end == k4->a) {
                earliest_violating_ear = std::move(vertex_stacks[bw].top().SP);
                k4->c = bw;
            }
        }
    }

    for (int a = k4->a; a != k4->b; a = parent[a]) k4->ab.emplace_back(a, parent[a]);
    for (int b = k4->b; b != k4->c; b = parent[b]) k4->bc.emplace_back(b, parent[b]);
    for (int c = k4->c; c != k4->d; c = parent[c]) k4->cd.emplace_back(c, parent[c]);

    k4->ad.emplace_back(k4->d, elose);
    for (int d = elose; d != k4->a; d = parent[d]) k4->ad.emplace_back(d, parent[d]);
    for (int e = k4->d; e != ewin_src; e = parent[e]) {
        k4->bd.emplace_back(e, parent[e]);
    }
    k4->bd.emplace_back(ewin_src, ewin_sink);
    for (int e = ewin_sink; e != k4->b; e = parent[e]) k4->bd.emplace_back(e, parent[e]);
    int ear_path = earliest_violating_ear.underlying_tree_path_source();
    k4->ac.emplace_back(k4->c, ear_path);
    for (; ear_path != k4->a; ear_path = parent[ear_path]) k4->ac.emplace_back(ear_path, parent[ear_path]);

    cert_out.reason = k4;
}

void K23_test(std::shared_ptr<certificate>& cert_ptr, std::vector<int>& alert, std::vector<int> const& parent, edge_t ear_found, edge_t ear_winning, int w) {
    V_LOG("testing K23: found ear (" << ear_found.first << ", " << ear_found.second << "), winning ear (" << ear_winning.first << ", " << ear_winning.second << ")\n")
    if (ear_found.second != parent[w]) {
        N_LOG("OOPS, 3.5(a) violation, nonouterplanar\n")
        std::shared_ptr<negative_cert_K23> k23{new negative_cert_K23{}};
        k23->a = w;
        k23->b = ear_found.second;

        k23->one.emplace_back(k23->b, ear_found.first);
        for (int i = ear_found.first; i != k23->a; i = parent[i]) k23->one.emplace_back(i, parent[i]);

        for (int i = k23->a; i != k23->b; i = parent[i]) k23->two.emplace_back(i, parent[i]);

        for (int i = k23->b; i != ear_winning.second; i = parent[i]) k23->three.emplace_back(i, parent[i]);
                k23->three.emplace_back(ear_winning.second, ear_winning.first);
        for (int i = ear_winning.first; i != k23->a; i = parent[i]) k23->three.emplace_back(i, parent[i]);

        cert_ptr = k23;
        return;
    }

    if (alert[w] != -1) {
        N_LOG("OOPS, 3.5(b) violation, nonouterplanar\n")
        std::shared_ptr<negative_cert_K23> k23{new negative_cert_K23{}};
        k23->a = w;
        k23->b = ear_found.second;

        k23->one.emplace_back(k23->b, ear_found.first);
        for (int i = ear_found.first; i != k23->a; i = parent[i]) k23->one.emplace_back(i, parent[i]);

        k23->two.emplace_back(k23->b, alert[w]);
        for (int i = alert[w]; i != k23->a; i = parent[i]) k23->two.emplace_back(i, parent[i]);

        for (int i = k23->b; i != ear_winning.second; i = parent[i]) k23->three.emplace_back(i, parent[i]);
        k23->three.emplace_back(ear_winning.second, ear_winning.first);
        for (int i = ear_winning.first; i != k23->a; i = parent[i]) k23->three.emplace_back(i, parent[i]);

        cert_ptr = k23;
        return;
    } else {
        alert[w] = ear_found.first;
    }
}

std::vector<edge_t> get_bicomps(graph const& g, std::vector<int>& cut_verts, sp_result& cert_out, int root = 0) {
    std::vector<int> dfs_no((size_t)(g.n), 0);
    std::vector<int> parent((size_t)(g.n), 0);
    std::vector<int> low((size_t)(g.n), 0);

    std::vector<edge_t> retval;
    std::stack<std::pair<int, int>> dfs;

    dfs.emplace(root, 0);
    dfs_no[root] = 1;
    low[root] = 1;
    parent[root] = -1;
    int curr_dfs = 2;
    bool root_cut = false;

    while (!dfs.empty()) {
        std::pair<int, int> p = dfs.top();
        int w = p.first;
        int u = g.adjLists[p.first][p.second];
        if (dfs_no[u] == 0) {
            dfs.push(std::pair{u, 0});
            parent[u] = w;
            dfs_no[u] = curr_dfs++;
            low[u] = dfs_no[u];
            continue;
        }

        if (parent[u] == w) {
            if (low[u] >= dfs_no[w]) {
                if (cut_verts[w] != -1) {
                    if (w != root || root_cut) {
                        if (!cert_out.reason) {
                            N_LOG("NON-SP, three component cut vertex at " << w << "\n")
                            std::shared_ptr<negative_cert_tri_comp_cut> cut{new negative_cert_tri_comp_cut{}};
                            cut->v = w;
                            cert_out.reason = cut;
                            cert_out.is_sp = false;
                        }
                    } else {
                        root_cut = true;
                    }
                } else {
                    cut_verts[w] = retval.size();
                }
                retval.emplace_back(w, u);
            }

            if (low[u] < low[w]) low[w] = low[u];
        } else if (dfs_no[u] < dfs_no[w] && u != parent[w]) {
            if (dfs_no[u] < low[w]) low[w] = dfs_no[u];
        }

        if ((size_t)(++dfs.top().second) >= g.adjLists[p.first].size()) {
            dfs.pop();
        }
    }

    int n_bicomps = (int)(retval.size());
    N_LOG(n_bicomps << " bicomp" << (n_bicomps == 1 ? "" : "s") << " found\n")
    for (int i = 0; i < n_bicomps; i++) {
        V_LOG("bicomp " << i << ": root " << retval[i].first << ", edge " << retval[i].second << "\n")
    }

    if (!root_cut) cut_verts[root] = -1;

    retval.shrink_to_fit();
    if (cert_out.reason) return retval;

    N_LOG("no tri-comp-cut found\n")

    std::vector<int> prev_cut((size_t)(n_bicomps), -1);
    int root_one = -1;
    int root_two = -1;

    for (int i = 0; i < n_bicomps - 1; i++) {
        int w = retval[i].first;
        int u = -1;
        int start = w;

        while (w != root) {
            u = w;
            w = parent[w];
            V_LOG("walking up tree for bicomp " << i << ", w: " << w << ", u: " << u <<"\n")

            if (cut_verts[w] != -1 && u == retval[cut_verts[w]].second) {
                V_LOG("found child bicomp: vertex " << start << " (bicomp " << i << ") child of vertex " << w << " (bicomp " << cut_verts[w] << ")\n")
                if (prev_cut[cut_verts[w]] == -1) {
                    prev_cut[cut_verts[w]] = start;
                } else {
                    std::shared_ptr<negative_cert_tri_cut_comp> cut{new negative_cert_tri_cut_comp{}};
                    cut->c1 = w;
                    cut->c2 = start;
                    cut->c3 = prev_cut[cut_verts[w]];
                    N_LOG("NON-SP, bicomp (not at root) with three cut vertices: " << cut->c1 << ", " << cut->c2 << ", " << cut->c3 << "\n")
                    cert_out.reason = cut;
                    cert_out.is_sp = false;
                    return retval;
                }
                break;
            }
        }

        if (w == root && (u == retval.back().second || u == -1)) {
            V_LOG("found child bicomp of root: vertex " << start << " (bicomp " << i << ") child of vertex " << w << " (bicomp " << n_bicomps - 1 << ")\n")
            if (root_one == -1) {
                root_one = start;
            } else if (root_two == -1) {
                root_two = start;
            } else {
                std::shared_ptr<negative_cert_tri_cut_comp> cut{new negative_cert_tri_cut_comp{}};
                cut->c1 = root_one;
                cut->c2 = root_two;
                cut->c3 = start;
                N_LOG("NON-SP, bicomp (at root) with three cut vertices: " << cut->c1 << ", " << cut->c2 << ", " << cut->c3 << "\n")
                cert_out.reason = cut;
                cert_out.is_sp = false;
                return retval;
            }
        }
    }

    N_LOG("no tri-cut-comp found\n")

    if (n_bicomps > 1) {
        N_LOG("ordering bicomps as chain: ")
        int second_endpoint = n_bicomps - 1;

        for (int i = 1; i < n_bicomps - 1; i++) {
            if (prev_cut[i] == -1) {
                second_endpoint = i;
                break;
            }
        }

        N_LOG("bicomp " << second_endpoint << " is the other bicomp with no child\n")

        std::reverse(retval.begin() + second_endpoint, retval.end() - 1);
        if (second_endpoint != n_bicomps - 1) {
            retval.back().second = retval[n_bicomps - 2].first;
            retval.back().first = retval[n_bicomps - 2].second;
        } else {
            if (retval.back().first == retval[n_bicomps - 2].first) {
                retval.back().first = retval.back().second;
            } else {
                retval.back().first = parent[retval[n_bicomps - 2].first];
            }
            retval.back().second = retval[n_bicomps - 2].first;
        }

        for (int i = second_endpoint; i < n_bicomps - 1; i++) {
            retval[i].second = parent[retval[i].first];
        }

        #ifdef __VERBOSE_LOGGING__
            for (int i = 0; i < n_bicomps; i++) {
                V_LOG("bicomp " << i << " after reordering: root " << retval[i].first << ", edge " << retval[i].second << "\n")
            }
        #endif
    }

    return retval;
}

sp_result SP_RECOGNITION(graph const& g) {
    sp_result retval{};

    std::vector<int> cut_verts(g.n, -1);
    std::vector<edge_t> bicomps = get_bicomps(g, cut_verts, retval);
    int n_bicomps = (int)(bicomps.size());
    std::vector<sp_tree> cut_vertex_attached_tree((size_t)(n_bicomps));
    std::vector<int> comp(g.n, -1);

    std::vector<std::stack<sp_chain_stack_entry>> vertex_stacks((size_t)(g.n));
    std::vector<int> dfs_no((size_t)(g.n + 1), 0);
    std::vector<int> parent((size_t)(g.n), 0);

    std::vector<edge_t> ear((size_t)(g.n), edge_t{g.n, g.n});
    std::vector<sp_tree> seq((size_t)(g.n));
    std::vector<int> earliest_outgoing((size_t)(g.n), g.n);

    std::vector<char> num_children((size_t)(g.n), 0);
    std::vector<int> alert((size_t)(g.n), -1);

    std::stack<std::pair<int, int>> dfs;

    dfs_no[g.n] = g.n;

    bool do_k23_edge_replacement = true;

    for (int bicomp = 0; bicomp < n_bicomps; bicomp++) {
        N_LOG("BICOMP " << bicomp << "\n")

        int root = bicomps[bicomp].first;
        int next;
        if (!retval.reason && bicomp > 0 && bicomp < n_bicomps - 1) {
            next = bicomps[bicomp - 1].first;
        } else {
            next = bicomps[bicomp].second;
        }

        dfs.emplace(root, -1);
        dfs.emplace(next, 0);

        bool fake_edge = false;
        if (!retval.reason) {
            fake_edge = true;
            for (int u1 : g.adjLists[next]) {
                if (u1 == root) {
                    fake_edge = false;
                    break;
                }
            }
        }
                dfs_no[root] = 1;
        parent[root] = -1;
        dfs_no[next] = 2;
        parent[next] = root;
        comp[next] = bicomp;
        int curr_dfs = 3;

        while (!dfs.empty()) {
            std::pair<int, int> p = dfs.top();
            int v = parent[p.first];
            int w = p.first;
            int u = g.adjLists[p.first][p.second];

            if (comp[u] == -1 || comp[u] == bicomp) {
                V_LOG("v: " << v << " w: " << w << " u: " << u << "\n")
                V_LOG("seq_w: " << seq[w] << ", seq_u: " << seq[u] << "\n")
                if (dfs_no[u] == 0) {
                    dfs.push(std::pair{u, 0});
                    parent[u] = w;
                    dfs_no[u] = curr_dfs++;
                    comp[u] = bicomp;
                    num_children[w]++;
                    continue;
                }

                bool child_back_edge = (dfs_no[u] < dfs_no[w] && u != v);
                #ifdef __LOGGING__
                    if (child_back_edge) N_LOG("BACK EDGE (" << w << ", " << u << ")\n")
                #endif

                if (parent[u] == w) {
                    N_LOG("tree edge (" << w << ", " << u << ")\n")
                    // --- update-seq in the paper begins here ---
                    for (; !vertex_stacks[w].empty(); vertex_stacks[w].pop()) {
                        if (seq[u].source() != vertex_stacks[w].top().end) {
                            N_LOG("OOPS, 3.4b due to POPPING STACK child seq " << seq[u] << " parent seq " << seq[w] << "\n")
                            std::shared_ptr<negative_cert_K4> k4{new negative_cert_K4{}};

                            k4->b = seq[u].source();
                            k4->a = vertex_stacks[w].top().end;
                            k4->c = w;
                            edge_t holding_ear = ear[u];

                            for (int a = k4->a; a != k4->b; a = parent[a]) k4->ab.emplace_back(a, parent[a]);
                            for (int b = k4->b; b != k4->c; b = parent[b]) k4->bc.emplace_back(b, parent[b]);

                            k4->d = -1;
                            int c = k4->c;
                            while (k4->d == -1) {
                                k4->cd.emplace_back(c, parent[c]);
                                c = parent[c];

                                for (; !vertex_stacks[c].empty(); vertex_stacks[c].pop()) {
                                    if (vertex_stacks[c].top().end == k4->b) {
                                        k4->d = c;
                                        break;
                                    }
                                }
                            }

                            for (int d = k4->d; d != holding_ear.second; d = parent[d]) k4->ad.emplace_back(d, parent[d]);
                            k4->ad.emplace_back(holding_ear.second, holding_ear.first);
                            for (int d = holding_ear.first; d != k4->a; d = parent[d]) k4->ad.emplace_back(d, parent[d]);

                            int ear1 = vertex_stacks[k4->d].top().SP.underlying_tree_path_source();
                            k4->bd.emplace_back(k4->d, ear1);
                            for (; ear1 != k4->b; ear1 = parent[ear1]) k4->bd.emplace_back(ear1, parent[ear1]);
                            int ear2 = vertex_stacks[k4->c].top().SP.underlying_tree_path_source();
                            k4->ac.emplace_back(k4->c, ear2);
                            for (; ear2 != k4->a; ear2 = parent[ear2]) k4->ac.emplace_back(ear2, parent[ear2]);

                            retval.reason = k4;
                            break;
                        }

                        seq[u].compose(std::move(vertex_stacks[w].top().SP), c_type::antiparallel);
                        seq[u].l_compose(std::move(vertex_stacks[w].top().tail), c_type::series);
                    }
                    // ---- update-seq in the paper ends here ----

                    if (retval.reason) break;
                }

                if (parent[u] == w || child_back_edge) {
                    // ---- update-ear-of-parent in the paper begins here ----
                    edge_t ear_f = (child_back_edge ? edge_t{w, u} : ear[u]);
                    sp_tree seq_u = (child_back_edge ? sp_tree{u, w} : std::move(seq[u]));

                    if (dfs_no[ear_f.second] < dfs_no[ear[w].second]) {
                        if (ear[w].first != g.n) {
                            if (!retval.reason && ear[w].first != w) K23_test(retval.reason, alert, parent, ear[w], ear_f, w);
                            if (seq[w].source() != ear[w].second) {
                                N_LOG("OOPS, 3.4a due to CASE B prev winner " << seq[w] << " prev winner ear (" << ear[w].first << ", " << ear[w].second << ")\n")
                                report_K4_non_stack_pop_case(retval, parent, vertex_stacks, seq[w].source(), w, ear[w].second, ear[w].first, ear_f.second, ear_f.first);
                                break;
                            }

                            N_LOG("CASE B (ear exists): placed " << seq[w] << " onto stk " << ear[w].second << "\n")
                            vertex_stacks[ear[w].second].emplace(std::move(seq[w]), w, sp_tree{});
                            earliest_outgoing[w] = ear[w].second;
                        }
                        ear[w] = ear_f;
                        seq[w] = std::move(seq_u);
                        N_LOG("CASE B (replace seq): current winning seq " << seq[w] << "\n")
                    } else {
                        if (seq_u.source() != ear_f.second) {
                            N_LOG("OOPS, 3.4a/b due to CASE A/C child seq " << seq_u << " child ear (" << ear_f.first << ", " << ear_f.second << ")\n")
                            report_K4_non_stack_pop_case(retval, parent, vertex_stacks, seq_u.source(), w, ear_f.second, ear_f.first, ear[w].second, ear[w].first);
                            break;
                        }

                        if (dfs_no[ear_f.second] == dfs_no[ear[w].second]) {
                            if (!retval.reason && !child_back_edge && ear[w].first != w) K23_test(retval.reason, alert, parent, ear_f, ear[w], w);

                            if (seq[w].source() != ear[w].second) {
                                N_LOG("OOPS, 3.4a/b due to CASE C parent seq " << seq[w] << " parent ear (" << ear[w].first << ", " << ear[w].second << ")\n")
                                report_K4_non_stack_pop_case(retval, parent, vertex_stacks, seq[w].source(), w, ear[w].second, ear[w].first, ear_f.second, ear_f.first);
                                break;
                            }
                            seq[w].compose(std::move(seq_u), c_type::parallel);
                            N_LOG("CASE C: current winning seq after merge " << seq[w] << "\n")

                            if ((ear[w].first == w || dfs_no[ear_f.first] < dfs_no[ear[w].first]) && ear_f.first != w) {
                                ear[w] = ear_f;
                            }
                        } else {
                            if (!retval.reason && !child_back_edge) K23_test(retval.reason, alert, parent, ear_f, ear[w], w);

                            if (!vertex_stacks[ear_f.second].empty() && vertex_stacks[ear_f.second].top().end == w) {
                                N_LOG("CASE A (merge onto existing stack entry for stk " << ear_f.second << "): current child seq before merge " << seq_u << "\n")
                                vertex_stacks[ear_f.second].top().SP.compose(std::move(seq_u), c_type::parallel);
                            } else {
                                N_LOG("CASE A (new stack entry): placed " << seq_u << " onto stk " << ear_f.second << " (earliest outgoing " << earliest_outgoing[w] << ")\n")
                                vertex_stacks[ear_f.second].emplace(std::move(seq_u), w, sp_tree{});
                                if (dfs_no[ear_f.second] < dfs_no[earliest_outgoing[w]]) {
                                    earliest_outgoing[w] = ear_f.second;
                                }
                            }
                        }
                    }
                    // ----- update-ear-of-parent in the paper ends here -----
                }
            }

            if ((size_t)(++dfs.top().second) >= g.adjLists[p.first].size()) {
                if (w != root) {
                    if (earliest_outgoing[w] != g.n) {
                        N_LOG("EARLIEST OUTGOING " << earliest_outgoing[w] << ": moved current winning seq " << seq[w] << " to vertex stack entry tail with SP " << vertex_stacks[earliest_outgoing[w]].top().SP << "\n")
                        vertex_stacks[earliest_outgoing[w]].top().tail = std::move(seq[w]);
                    }

                    if (v == root) {
                        seq[w].compose((fake_edge ? sp_tree{} : sp_tree{v, w}), c_type::parallel);

                        if (cut_verts[w] != -1) {
                            seq[w].compose(std::move(cut_vertex_attached_tree[cut_verts[w]]), c_type::series);
                        }
                        break;

                    } else {
                        if (cut_verts[w] != -1) {
                            cut_vertex_attached_tree[cut_verts[w]].l_compose(sp_tree{w, v}, c_type::dangling);
                                                        seq[w].compose(std::move(cut_vertex_attached_tree[cut_verts[w]]), c_type::series);
                        } else {
                            seq[w].compose(sp_tree{w, v}, c_type::series);
                        }
                    }
                }

                dfs.pop();
            }
        }

        dfs_no[root] = 0;

        if (!retval.reason) {
            N_LOG("no K23 found\n")
        }

        if (fake_edge) {
            edge_t fake = edge_t{root, next};

            if (retval.reason) {
                std::shared_ptr<negative_cert_K4> k4 = std::dynamic_pointer_cast<negative_cert_K4>(retval.reason);
                if (k4) {
                    std::vector<edge_t> * k4_paths[6] = {&k4->ab, &k4->ac, &k4->ad, &k4->bc, &k4->bd, &k4->cd};
                    int k4_verts[4] = {k4->a, k4->b, k4->c, k4->d};
                    static const int k4_t4_translation[6][5] = {{1, 3, 2, 4, 5}, {0, 3, 2, 5, 4}, {0, 4, 1, 5, 3}, {0, 1, 4, 5, 2}, {0, 2, 3, 5, 1}, {1, 2, 3, 4, 0}};
                    static const int k4_t4_endpoint_translation[6][4] = {{0, 1, 2, 3}, {0, 2, 1, 3}, {0, 3, 1, 2}, {1, 2, 0, 3}, {1, 3, 0, 2}, {2, 3, 0, 1}};

                    int pnum = 0;
                    for (; pnum < 6; pnum++) {
                        if (path_contains_edge(*(k4_paths[pnum]), fake) != -1) break;
                    }

                    if (pnum != 6) {
                        N_LOG("FAKE EDGE IN K4 (pnum " << pnum << "), GENERATE T4\n")
                        std::shared_ptr<negative_cert_T4> t4{new negative_cert_T4{}};

                        t4->c1a = std::move(*(k4_paths[k4_t4_translation[pnum][0]]));
                        t4->c2a = std::move(*(k4_paths[k4_t4_translation[pnum][1]]));
                        t4->c1b = std::move(*(k4_paths[k4_t4_translation[pnum][2]]));
                        t4->c2b = std::move(*(k4_paths[k4_t4_translation[pnum][3]]));
                        t4->ab = std::move(*(k4_paths[k4_t4_translation[pnum][4]]));
                        t4->c1 = k4_verts[k4_t4_endpoint_translation[pnum][0]];
                        t4->c2 = k4_verts[k4_t4_endpoint_translation[pnum][1]];
                        t4->a = k4_verts[k4_t4_endpoint_translation[pnum][2]];
                        t4->b = k4_verts[k4_t4_endpoint_translation[pnum][3]];

                        retval.reason = t4;

                        for (int i = 0; i < g.n; i++) {
                            if (comp[i] == bicomp) {
                                dfs_no[i] = 0;
                                parent[i] = 0;
                                ear[i] = edge_t{g.n, g.n};
                                num_children[i] = 0;
                                alert[i] = -1;
                                earliest_outgoing[i] = g.n;
                                seq[i] = sp_tree{};
                                vertex_stacks[i] = std::stack<sp_chain_stack_entry>{};
                            }
                        }

                        bicomp--;
                    }
                }
            }

            if (retval.reason && do_k23_edge_replacement) {
                std::shared_ptr<negative_cert_K23> k23 = std::dynamic_pointer_cast<negative_cert_K23>(retval.reason);
                if (k23) {
                    std::vector<edge_t> * k23_paths[3] = {&k23->one, &k23->two, &k23->three};

                    int pnum = 0;
                    int path_ind;
                    for (; pnum < 3; pnum++) {
                        path_ind = path_contains_edge(*(k23_paths[pnum]), fake);
                        if (path_ind != -1) break;
                    }

                    if (pnum != 3) {
                        std::vector<edge_t>& violating_path = *(k23_paths[pnum]);
                        N_LOG("FAKE EDGE IN K23 (" << violating_path[path_ind].first << ", " << violating_path[path_ind].second << "), REPLACE WITH PATH\n")

                        std::vector<edge_t> splice_path;
                        std::vector<bool> in_k23(g.n, false);

                        for (std::vector<edge_t> * path : k23_paths) {
                            for (edge_t e : *path) {
                                in_k23[e.first] = true;
                                in_k23[e.second] = true;
                                V_LOG("(" << e.first << ", " << e.second << ") in K23\n")
                            }
                        }

                        for (int u2 : g.adjLists[next]) {
                            if (comp[u2] == bicomp && parent[u2] == next && !in_k23[u2]) {
                                V_LOG("FOUND TREE CHILD OF NEXT " << next << " NOT IN K23: " << u2 << ", ear (" << ear[u2].first << ", " << ear[u2].second << ")\n")
                                splice_path.emplace_back(ear[u2].first, root);
                                for (int i = ear[u2].first; i != next; i = parent[i]) splice_path.emplace_back(parent[i], i);
                                break;
                            }
                        }

                        std::reverse(splice_path.begin(), splice_path.end());
                        violating_path.erase(violating_path.begin() + path_ind);
                        violating_path.insert(violating_path.begin() + path_ind, splice_path.begin(), splice_path.end());
                    }
                }
            }
        }

        if (retval.reason) do_k23_edge_replacement = false;

        if (retval.reason) {
            retval.is_sp = false;
            break;
        }

        if (cut_verts[root] != -1) {
            #ifdef __VERBOSE_LOGGING__
            if (cut_vertex_attached_tree[cut_verts[root]].root) {
                V_LOG("combine tree " << cut_vertex_attached_tree[cut_verts[root]] << " with " << seq[next] << " (bicomp " << bicomp << ")\n");
            }
            #endif

            seq[next].compose(std::move(cut_vertex_attached_tree[cut_verts[root]]), c_type::dangling);
        }

        if (bicomp < n_bicomps - 1) {
            V_LOG("ATTACH " << seq[next] << " to cut vertex " << root << " (bicomp " << bicomp << ")\n");
            cut_vertex_attached_tree[cut_verts[root]] = std::move(seq[next]);
        } else {
            if (!retval.reason) {
                std::shared_ptr<positive_cert_sp> sp{new positive_cert_sp{}};

                sp->decomposition = std::move(seq[next]);
                sp->is_sp = true;
                retval.reason = sp;
                retval.is_sp = true;
                N_LOG("graph is SP\n")
            }
        }
    }

    #ifdef __VERBOSE_LOGGING__
        for (int i = 0; i < g.n; i++) {
            V_LOG("vertex " << i << " ear: (" << ear[i].first << ", " << ear[i].second << ")\n")
            V_LOG("vertex " << i << " parent: " << parent[i] << "\n")
            V_LOG("vertex " << i << " dfs_no: " << dfs_no[i] << "\n")
        }
    #endif

    return retval;
}

// ==================== MAIN FUNCTION ====================
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <graph_input_file>\n";
        return 1;
    }

    std::ifstream infile(argv[1]);
    if (!infile) {
        std::cerr << "Error: could not open file " << argv[1] << "\n";
        return 1;
    }

    graph g;
    infile >> g;
    if (g.n <= 0) {
        std::cerr << "Error: Graph must have at least one vertex\n";
        return 1;
    }

    std::cout << "Read graph with " << g.n << " vertices and " << g.e << " edges\n\n";

    sp_result result = SP_RECOGNITION(g);

    std::cout << "=== Series-Parallel Recognition Results ===\n";
    if (result.is_sp) {
        std::cout << "The graph IS Series-Parallel.\n";
        auto sp = std::dynamic_pointer_cast<positive_cert_sp>(result.reason);
        if (sp && sp->decomposition.root) {
            std::cout << "SP decomposition tree root: {"
                      << sp->decomposition.source() << ","
                      << sp->decomposition.sink() << "}\n";
        } else {
            std::cout << "Empty SP decomposition (trivial).\n";
        }
    } else {
        std::cout << "The graph is NOT Series-Parallel.\n";
        if (auto k4 = std::dynamic_pointer_cast<negative_cert_K4>(result.reason)) {
            std::cout << "Reason: K4 subdivision on vertices {"
                      << k4->a << "," << k4->b << "," << k4->c << "," << k4->d << "}\n";
        } else if (auto k23 = std::dynamic_pointer_cast<negative_cert_K23>(result.reason)) {
            std::cout << "Reason: K23 subdivision between vertices {" 
                      << k23->a << "," << k23->b << "}\n";
        } else if (auto t4 = std::dynamic_pointer_cast<negative_cert_T4>(result.reason)) {
            std::cout << "Reason: T4 (theta-4) subdivision with cut vertices "
                      << t4->c1 << "," << t4->c2
                      << " and others " << t4->a << "," << t4->b << "\n";
        } else if (auto tri = std::dynamic_pointer_cast<negative_cert_tri_comp_cut>(result.reason)) {
            std::cout << "Reason: cut vertex " << tri->v << " splits into >=3 components\n";
               } else if (auto tric = std::dynamic_pointer_cast<negative_cert_tri_cut_comp>(result.reason)) {
            std::cout << "Reason: bicomp with 3 cut vertices {"
                      << tric->c1 << "," << tric->c2 << "," << tric->c3 << "}\n";
        } else {
            std::cout << "Reason: unknown (unhandled cert type)\n";
        }
    }

    std::cout << "\n=== Certificate Authentication ===\n";
    if (!result.reason) {
        std::cerr << "ERROR: No certificate generated\n";
        return 1;
    }

    bool auth_ok = false;
    try { 
        auth_ok = result.authenticate(g); 
    } catch(...) { 
        auth_ok = false; 
    }

    if (!auth_ok) {
        std::cerr << "ERROR: Certificate authentication failed!\n";
        return 1;
    }
    
    std::cout << "Certificate authenticated successfully.\n";
    return 0;
}
                       
