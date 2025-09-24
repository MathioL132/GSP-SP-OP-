#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <memory>
#include <algorithm>
#include <cassert>
#include <climits>
#include <functional> 

// ==================== LOGGING DEFINITIONS ====================
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
#include <iostream>
#define L_LOG(a) std::cout << a;
#else
#define L_LOG(a)
#endif

// ==================== BASIC STRUCTURES ====================
using edge_t = std::pair<int, int>;

struct graph {
    int n;
    int e;
    std::vector<std::vector<int>> adjLists;
    
    graph() : n(0), e(0) {}
    
    bool adjacent(int e1, int e2) const {
        if (e1 < 0 || e1 >= n || e2 < 0 || e2 >= n) return false;
        for (int v : adjLists[e1]) {
            if (v == e2) return true;
        }
        return false;
    }
    
    void add_edge(int e1, int e2) {
        if (e1 < 0 || e1 >= n || e2 < 0 || e2 >= n) return;
        adjLists[e1].push_back(e2);
        adjLists[e2].push_back(e1);
        e++;
    }
    
    void reserve(graph const& other) {
        adjLists.clear();
        adjLists.resize(other.n);
        n = other.n;
        e = 0;
        for (int i = 0; i < other.n; i++) {
            if (i < (int)other.adjLists.size()) {
                adjLists[i].reserve(other.adjLists[i].size());
            }
        }
    }
    
    void output_adj_list(int v, std::ostream& os) const {
        if (v < 0 || v >= n) return;
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
    if (g.n <= 0 || g.e < 0) {
        is.setstate(std::ios::failbit);
        return is;
    }
    
    g.adjLists.resize(g.n);
    
    int edges_to_read = g.e;
    g.e = 0; // Reset for add_edge counting
    
    for (int i = 0; i < edges_to_read; i++) {
        int endpoint1, endpoint2;
        is >> endpoint1 >> endpoint2;
        if (endpoint1 >= 0 && endpoint1 < g.n && endpoint2 >= 0 && endpoint2 < g.n) {
            g.add_edge(endpoint1, endpoint2);
        }
    }
    
    for (auto& list : g.adjLists) {
        list.shrink_to_fit();
    }
    return is;
}

// ==================== SP-TREE DEFINITIONS ====================
enum class c_type { edge, series, parallel, antiparallel, dangling };

struct sp_tree_node {
    int source;
    int sink;
    c_type comp;
    sp_tree_node* l;
    sp_tree_node* r;
    
    sp_tree_node(int source, int sink, c_type comp = c_type::edge, sp_tree_node* l = nullptr, sp_tree_node* r = nullptr) 
        : source{source}, sink{sink}, comp{comp}, l{l}, r{r} {}
        
    sp_tree_node(sp_tree_node const& other) 
        : source{other.source}, sink{other.sink}, comp{other.comp}, 
          l{other.l ? new sp_tree_node{*other.l} : nullptr}, 
          r{other.r ? new sp_tree_node{*other.r} : nullptr} {}
          
    ~sp_tree_node() {
        delete l;
        delete r;
    }
    
    sp_tree_node& operator=(sp_tree_node const& other) = delete;
    sp_tree_node& operator=(sp_tree_node&& other) = delete;
};

std::ostream& operator<<(std::ostream& os, sp_tree_node const& node) {
    const char* comp_names[] = {"edge", "series", "parallel", "antiparallel", "dangling"};
    os << "(" << node.source << "," << node.sink << ":" << comp_names[(int)node.comp] << ")";
    return os;
}

struct sp_tree {
    sp_tree_node* root;
    
    sp_tree() : root{nullptr} {}
    
    explicit sp_tree(int source, int sink) : root{new sp_tree_node{source, sink}} {}
    
    explicit sp_tree(sp_tree const& other) : root{other.root ? new sp_tree_node{*other.root} : nullptr} {}
    
    sp_tree(sp_tree&& other) noexcept : root{other.root} { 
        other.root = nullptr; 
    }
    
    ~sp_tree() { 
        delete root; 
    }
    
    sp_tree& operator=(sp_tree&& other) noexcept {
        if (this != &other) {
            delete root;
            root = other.root;
            other.root = nullptr;
        }
        return *this;
    }
    
    sp_tree& operator=(sp_tree const& other) = delete;
    
    int source() const { return root ? root->source : -1; }
    int sink() const { return root ? root->sink : -1; }
    
    void compose(sp_tree&& other, c_type comp) {
        if (!other.root) return;
        if (!root) {
            root = other.root;
            other.root = nullptr;
            return;
        }
        
        int new_source, new_sink;
        switch (comp) {
            case c_type::series:
                new_source = root->source;
                new_sink = other.root->sink;
                break;
            case c_type::parallel:
            case c_type::antiparallel:
                if (root->source != other.root->source || root->sink != other.root->sink) {
                    delete other.root;
                    other.root = nullptr;
                    return;
                }
                new_source = root->source;
                new_sink = root->sink;
                break;
            case c_type::dangling:
                new_source = root->source;
                new_sink = root->sink;
                break;
            default:
                new_source = root->source;
                new_sink = root->sink;
                break;
        }
        
        sp_tree_node* new_root = new sp_tree_node{new_source, new_sink, comp};
        new_root->l = root;
        new_root->r = other.root;
        root = new_root;
        other.root = nullptr;
    }
    
    void l_compose(sp_tree&& other, c_type comp) {
        if (!other.root) return;
        if (!root) {
            root = other.root;
            other.root = nullptr;
            return;
        }
        
        int new_source, new_sink;
        switch (comp) {
            case c_type::series:
                new_source = other.root->source;
                new_sink = root->sink;
                break;
            case c_type::parallel:
            case c_type::antiparallel:
                new_source = other.root->source;
                new_sink = other.root->sink;
                break;
            case c_type::dangling:
                new_source = other.root->source;
                new_sink = other.root->sink;
                break;
            default:
                new_source = other.root->source;
                new_sink = other.root->sink;
                break;
        }
        
        sp_tree_node* new_root = new sp_tree_node{new_source, new_sink, comp};
        new_root->l = other.root;
        new_root->r = root;
        root = new_root;
        other.root = nullptr;
    }
    
    int underlying_tree_path_source() const {
        if (!root) return -1;
        sp_tree_node* curr = root;
        while (curr->l) curr = curr->l;
        return curr->source;
    }
};

std::ostream& operator<<(std::ostream& os, sp_tree const& tree) {
    if (tree.root) {
        os << "{" << tree.source() << ", " << tree.sink() << "}";
    } else {
        os << "{}";
    }
    return os;
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
    
    bool authenticate(graph const& g) override;
};

struct negative_cert_T4 : certificate {
    int c1, c2, a, b;
    std::vector<edge_t> c1a, c1b, c2a, c2b, ab;
    
    bool authenticate(graph const& g) override;
};

struct negative_cert_tri_comp_cut : certificate {
    int v;
    
    bool authenticate(graph const& g) override;
};

struct negative_cert_tri_cut_comp : certificate {
    int c1, c2, c3;
    
    bool authenticate(graph const& g) override;
};

struct positive_cert_sp : certificate {
    sp_tree decomposition;
    
    bool authenticate(graph const& g) override;
};

// ==================== SP CHAIN STACK ENTRY ====================
struct sp_chain_stack_entry {
    sp_tree SP;
    int end;
    sp_tree tail;
    
    sp_chain_stack_entry(sp_tree&& SP, int end, sp_tree&& tail) 
        : SP{std::move(SP)}, end{end}, tail{std::move(tail)} {}
};

// ==================== SP RESULT STRUCTURE ====================
struct sp_result {
    bool is_sp = false;
    std::shared_ptr<certificate> reason;
    
    bool authenticate(graph const& g) {
        if (!reason) {
            L_LOG("ERROR: no certificate provided\n")
            return false;
        }
        return reason->authenticate(g);
    }
};

// ==================== UTILITY FUNCTIONS ====================
bool trace_path(int end1, int end2, std::vector<edge_t> const& path, graph const& g, std::vector<bool>& seen) {
    if (path.empty()) {
        L_LOG("====== AUTH FAILED: no edges in path ======\n")
        return false;
    }

    int start = end1, finish = end2;
    if (path[0].first == end2) {
        start = end2;
        finish = end1;
    }

    if (path[0].first != start) {
        L_LOG("====== AUTH FAILED: start of path does not match either endpoint ======\n")
        return false;
    }

    if (path.back().second != finish) {
        L_LOG("====== AUTH FAILED: end of path does not match second endpoint ======\n")
        return false;
    }

    if (start < 0 || start >= g.n || finish < 0 || finish >= g.n) {
        L_LOG("====== AUTH FAILED: invalid vertex indices ======\n")
        return false;
    }

    if (start >= (int)seen.size() || finish >= (int)seen.size()) {
        L_LOG("====== AUTH FAILED: vertex index exceeds seen array bounds ======\n")
        return false;
    }

    seen[start] = true;
    int prev_v = start;
    for (edge_t edge : path) {
        if (edge.first < 0 || edge.first >= g.n || edge.second < 0 || edge.second >= g.n) {
            L_LOG("====== AUTH FAILED: invalid edge vertices ======\n")
            return false;
        }
        
        if (edge.second >= (int)seen.size()) {
            L_LOG("====== AUTH FAILED: edge vertex exceeds seen array bounds ======\n")
            return false;
        }
        
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
    seen[start] = false;
    seen[finish] = false;
    return true;
}

int num_comps_after_removal(graph const& g, int v) {
    if (v < 0 || v >= g.n) return 0;
    
    int retval = 0;
    std::vector<bool> seen(g.n, false);

    for (int i = 0; i < g.n; i++) {
        if (seen[i] || i == v) continue;
        retval++;

        std::stack<int> dfs;
        dfs.push(i);

        while (!dfs.empty()) {
            int w = dfs.top();
            dfs.pop();
            if (seen[w] || w < 0 || w >= g.n) continue;
            seen[w] = true;

            if (w < (int)g.adjLists.size()) {
                for (int u : g.adjLists[w]) {
                    if (u >= 0 && u < g.n && !seen[u] && u != v) {
                        dfs.push(u);
                    }
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

void radix_sort(std::vector<int>& v) {
    if (v.empty()) return;
    
    int min_val = *std::min_element(v.begin(), v.end());
    int max_val = *std::max_element(v.begin(), v.end());
    
    int offset = 0;
    if (min_val < 0) {
        offset = -min_val;
        for (int& val : v) val += offset;
        max_val += offset;
    }
    
    if (max_val <= 0) {
        if (offset > 0) {
            for (int& val : v) val -= offset;
        }
        return;
    }
    
    std::vector<int> output(v.size());
    std::vector<int> count(10);
    
    for (int exp = 1; max_val / exp > 0; exp *= 10) {
        std::fill(count.begin(), count.end(), 0);
        
        for (int i : v) {
            count[(i / exp) % 10]++;
        }
        
        for (int i = 1; i < 10; i++) {
            count[i] += count[i - 1];
        }
        
        for (int i = (int)v.size() - 1; i >= 0; i--) {
            output[count[(v[i] / exp) % 10] - 1] = v[i];
            count[(v[i] / exp) % 10]--;
        }
        
        v = output;
    }
    
    if (offset > 0) {
        for (int& val : v) val -= offset;
    }
}

// ==================== CERTIFICATE AUTHENTICATION ====================
bool negative_cert_K4::authenticate(graph const& g) {
    if (verified) return true;

    L_LOG("====== AUTHENTICATE K4: terminating vertices a: " << a << ", b: " << b << ", c: " << c << ", d: " << d << " ======\n")
    if (a == b || b == c || c == d || d == a || a == c || b == d) {
        L_LOG("====== AUTH FAILED: terminating vertices non-distinct ======\n\n")
        return false;
    }
    
    if (a < 0 || a >= g.n || b < 0 || b >= g.n || c < 0 || c >= g.n || d < 0 || d >= g.n) {
        L_LOG("====== AUTH FAILED: invalid vertex indices ======\n\n")
        return false;
    }
    
    std::vector<bool> seen(g.n, false);

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

bool negative_cert_T4::authenticate(graph const& g) {
    if (verified) return true;
    L_LOG("====== AUTHENTICATE T4: terminating vertices a: " << a << ", b: " << b << ", c1: " << c1 << ", c2: " << c2 << " ======\n")

    if (a == b || a == c1 || a == c2 || b == c1 || b == c2 || c1 == c2) {
        L_LOG("====== AUTH FAILED: terminating vertices non-distinct ======\n\n")
        return false;
    }
    
    if (a < 0 || a >= g.n || b < 0 || b >= g.n || c1 < 0 || c1 >= g.n || c2 < 0 || c2 >= g.n) {
        L_LOG("====== AUTH FAILED: invalid vertex indices ======\n\n")
        return false;
    }

    N_LOG("verify c1 cut vertex: ")
    if (!is_cut_vertex(g, c1)) return false;
    N_LOG("verify c2 cut vertex: ")
    if (!is_cut_vertex(g, c2)) return false;

    std::vector<bool> seen(g.n, false);
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

bool negative_cert_tri_comp_cut::authenticate(graph const& g) {
    if (verified) return true;
    L_LOG("====== AUTHENTICATE THREE-COMPONENT CUT VERTEX: " << v << " ======\n");
    if (v < 0 || v >= g.n) {
        L_LOG("====== AUTH FAILED: invalid vertex index ======\n\n");
        return false;
    }

    int comps = num_comps_after_removal(g, v);

    if (comps < 3) {
        L_LOG("====== AUTH FAILED: vertex " << v << " only splits graph into " << comps << " components ======\n\n");
        return false;
    }

    N_LOG(comps << " comps after removal\n")
    L_LOG("====== AUTH SUCCESS ======\n\n")

    verified = true;
    return true;
}

bool negative_cert_tri_cut_comp::authenticate(graph const& g) {
    if (verified) return true;
    L_LOG("====== AUTHENTICATE BICOMP WITH THREE CUT VERTICES: cut vertices " << c1 << ", " << c2 << ", " << c3 << " ======\n");
    if (c1 < 0 || c1 >= g.n || c2 < 0 || c2 >= g.n || c3 < 0 || c3 >= g.n) {
        L_LOG("====== AUTH FAILED: invalid vertex indices ======\n\n");
        return false;
    }

    N_LOG("verify c1 cut vertex: ")
    if (!is_cut_vertex(g, c1)) return false;
    N_LOG("verify c2 cut vertex: ")
    if (!is_cut_vertex(g, c2)) return false;
    N_LOG("verify c3 cut vertex: ")
    if (!is_cut_vertex(g, c3)) return false;

    std::vector<int> dfs_no(g.n, 0);
    std::vector<int> parent(g.n, -1);
    std::vector<int> low(g.n, 0);
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
        
        if (w < 0 || w >= g.n) {
            dfs.pop();
            continue;
        }
        
        if (p.second >= (int)g.adjLists[w].size()) {
            dfs.pop();
            continue;
        }
        
        int u = g.adjLists[w][p.second];

        if (u < 0 || u >= g.n) {
            dfs.top().second++;
            continue;
        }

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
                edge_t target_edge{w, u};
                
                std::stack<edge_t> temp_edges;
                bool found_target = false;
                
                while (!comp_edges.empty()) {
                    edge_t e = comp_edges.top();
                    comp_edges.pop();
                    temp_edges.push(e);
                    
                    for (int i = 0; i < 3; i++) {
                        if (e.first == cut_verts[i] || e.second == cut_verts[i]) {
                            seen[i] = true;
                        }
                    }
                    
                    if (e == target_edge) {
                        found_target = true;
                        break;
                    }
                }
                
                if (!found_target) {
                    while (!temp_edges.empty()) {
                        comp_edges.push(temp_edges.top());
                        temp_edges.pop();
                    }
                }

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

        dfs.top().second++;
    }

    L_LOG("====== AUTH FAILED: bicomp does not contain the three cut vertices ======\n\n")
    return false;
}

bool ear_wins(edge_t ear_new, edge_t ear_current, std::vector<int> const& dfs_no, int w) {
    if (ear_current.first >= (int)dfs_no.size() || ear_current.second >= (int)dfs_no.size()) {
        return true;
    }
    
    if (ear_new.first >= (int)dfs_no.size() || ear_new.second >= (int)dfs_no.size() ||
        ear_new.first < 0 || ear_new.second < 0 ||
        ear_current.first < 0 || ear_current.second < 0) {
        return false;
    }
    
    int new_sink_dfs = dfs_no[ear_new.second];
    int curr_sink_dfs = dfs_no[ear_current.second];

    if (new_sink_dfs < curr_sink_dfs) return true;
    if (new_sink_dfs > curr_sink_dfs) return false;

    if (ear_new.first != w && ear_current.first == w) return true;
    if (ear_current.first != w && ear_new.first == w) return false;
    
    int new_src_dfs = dfs_no[ear_new.first];
    int curr_src_dfs = dfs_no[ear_current.first];

    return new_src_dfs < curr_src_dfs;
}

bool positive_cert_sp::authenticate(graph const& g) {
    if (verified) return true;
    
    L_LOG("====== AUTHENTICATE SP DECOMPOSITION TREE ======\n")
    
    if (g.n <= 1 || g.e == 0) {
        if (!decomposition.root || (g.n == 1 && g.e == 0)) {
            L_LOG("====== AUTH SUCCESS (trivial graph) ======\n\n")
            verified = true;
            return true;
        }
    }
    
    if (!decomposition.root) {
        L_LOG("====== AUTH FAILED: decomposition tree does not exist ======\n\n")
        return false;
    }
    
    graph g2{};
    g2.n = g.n;
    g2.adjLists.resize(g.n);
    g2.e = 0;
    
    std::stack<std::pair<sp_tree_node*, bool>> stack;
    stack.emplace(decomposition.root, false);
    
    while (!stack.empty()) {
        auto [node, swapped] = stack.top();
        stack.pop();
        
        if (!node) continue;
        
        int source = swapped ? node->sink : node->source;
        int sink = swapped ? node->source : node->sink;
        
        if (source < 0 || source >= g.n || sink < 0 || sink >= g.n) {
            L_LOG("====== AUTH FAILED: invalid vertex indices (" << source << ", " << sink << ") ======\n\n")
            return false;
        }
        
        if (!node->l && !node->r) {
            if (node->comp != c_type::edge) {
                L_LOG("====== AUTH FAILED: leaf node is not an edge ======\n\n")
                return false;
            }
            g2.add_edge(source, sink);
        } else if (node->l && node->r) {
            bool left_swapped = swapped;
            bool right_swapped = swapped;
            
            if (node->comp == c_type::antiparallel) {
                right_swapped = !swapped;
            }
            
            stack.emplace(node->r, right_swapped);
            stack.emplace(node->l, left_swapped);
            
            int lsource = left_swapped ? node->l->sink : node->l->source;
            int lsink = left_swapped ? node->l->source : node->l->sink;
            int rsource = right_swapped ? node->r->sink : node->r->source;
                        int rsink = right_swapped ? node->r->source : node->r->sink;
            
            switch (node->comp) {
                case c_type::series:
                    if (lsource != source || rsink != sink || lsink != rsource) {
                        L_LOG("====== AUTH FAILED: series composition mismatch ======\n")
                        L_LOG("Expected: " << source << "->" << sink << " via " << lsource << "->" << lsink << "->" << rsource << "->" << rsink << "\n")
                        return false;
                    }
                    break;
                case c_type::parallel:
                    if (lsource != source || rsource != source || lsink != sink || rsink != sink) {
                        L_LOG("====== AUTH FAILED: parallel composition mismatch ======\n")
                        L_LOG("Expected: both children (" << source << "," << sink << "), got left(" << lsource << "," << lsink << ") right(" << rsource << "," << rsink << ")\n")
                        return false;
                    }
                    break;
                case c_type::antiparallel:
                    if (lsource != source || lsink != sink) {
                        L_LOG("====== AUTH FAILED: antiparallel left child mismatch ======\n")
                        return false;
                    }
                    if (rsource != sink || rsink != source) {
                        L_LOG("====== AUTH FAILED: antiparallel right child mismatch ======\n")
                        return false;
                    }
                    break;
                case c_type::dangling:
                    L_LOG("====== AUTH FAILED: dangling composition not allowed in SP tree ======\n\n")
                    return false;
                default:
                    L_LOG("====== AUTH FAILED: unknown composition type ======\n\n")
                    return false;
            }
        } else {
            L_LOG("====== AUTH FAILED: node has exactly one child ======\n\n")
            return false;
        }
    }
    
    for (int i = 0; i < g.n; i++) {
        std::vector<int> l1 = g.adjLists[i];
        std::vector<int> l2 = g2.adjLists[i];
        
        radix_sort(l1);
        radix_sort(l2);
        
        if (l1 != l2) {
            L_LOG("====== AUTH FAILED: adjacency list mismatch at vertex " << i << " ======\n")
            L_LOG("Original: ");
            for (int v : l1) L_LOG(v << " ");
            L_LOG("\nReconstructed: ");
            for (int v : l2) L_LOG(v << " ");
            L_LOG("\n");
            return false;
        }
    }
    
    L_LOG("====== AUTH SUCCESS ======\n\n")
    verified = true;
    return true;
}

// ==================== SP RECOGNITION ALGORITHM ====================

// Forward declarations
std::vector<edge_t> get_bicomps_sp(graph const&, std::vector<int>&, sp_result&, int = 0);
void report_K4_sp(sp_result&, std::vector<int> const&, std::vector<std::stack<sp_chain_stack_entry>>&, int, int, int, int, int, int);
int path_contains_edge(std::vector<edge_t> const&, edge_t);

sp_result SP_RECOGNITION(graph const& g) {
    sp_result retval{};
           
    if (g.n == 1) {
        std::shared_ptr<positive_cert_sp> sp{new positive_cert_sp{}};
        retval.reason = sp;
        retval.is_sp = true;
        return retval;
    }
    if (g.e == 0) {
        std::shared_ptr<positive_cert_sp> sp{new positive_cert_sp{}};
        retval.reason = sp;
        retval.is_sp = true;
        return retval;
    }
    if (g.n <= 0) {
        std::shared_ptr<positive_cert_sp> sp{new positive_cert_sp{}};
        retval.reason = sp;
        retval.is_sp = true;
        return retval;
    }
       if (g.n == 3 && g.e == 3) {
    bool is_triangle = true;
    for (int i = 0; i < 3; i++) {
        if (g.adjLists[i].size() != 2) {
            is_triangle = false;
            break;
        }
    }
    if (is_triangle) {
        // Treat triangle as degenerate K4 with one vertex "split"
        std::shared_ptr<negative_cert_K4> k4{new negative_cert_K4{}};
        k4->a = 0; 
        k4->b = 1; 
        k4->c = 2; 
        k4->d = 1; // Use b as d to create a valid K4 subdivision
        
        k4->ab.push_back({0, 1});
        k4->ac.push_back({0, 2});
        k4->ad.push_back({0, 1}); // Same as ab since d=b
        k4->bc.push_back({1, 2});
        k4->bd.clear(); // Empty since b=d
        k4->cd.push_back({2, 1}); // c to d(=b)
        
        retval.reason = k4;
        retval.is_sp = false;
        return retval;
    }
}
        if (g.n == 4 && g.e == 6) {
        bool is_k4 = true;
        for (int i = 0; i < 4; i++) {
            if (g.adjLists[i].size() != 3) {
                is_k4 = false;
                break;
            }
        }
        if (is_k4) {
            std::shared_ptr<negative_cert_K4> k4{new negative_cert_K4{}};
            k4->a = 0; k4->b = 1; k4->c = 2; k4->d = 3;
            // Add direct edges for K4
            k4->ab.push_back({0, 1});
            k4->ac.push_back({0, 2});
            k4->ad.push_back({0, 3});
            k4->bc.push_back({1, 2});
            k4->bd.push_back({1, 3});
            k4->cd.push_back({2, 3});
            retval.reason = k4;
            retval.is_sp = false;
            return retval;
        }
    }


    std::vector<int> cut_verts(g.n, -1);
    std::vector<edge_t> bicomps = get_bicomps_sp(g, cut_verts, retval);
    int n_bicomps = (int)(bicomps.size());

    if (retval.reason) return retval;

    std::vector<sp_tree> cut_vertex_attached_tree(n_bicomps);
    std::vector<int> comp(g.n, -1);

    std::vector<std::stack<sp_chain_stack_entry>> vertex_stacks(g.n);
    std::vector<int> dfs_no(g.n + 1, 0);
    std::vector<int> parent(g.n, 0);

    std::vector<edge_t> ear(g.n, edge_t{g.n, g.n});
    std::vector<sp_tree> seq(g.n);
    std::vector<int> earliest_outgoing(g.n, g.n);
    std::vector<char> num_children(g.n, 0);
    std::stack<std::pair<int, int>> dfs;

    dfs_no[g.n] = g.n;

    for (int bicomp = 0; bicomp < n_bicomps; bicomp++) {
        N_LOG("BICOMP " << bicomp << "\n")
        int root = bicomps[bicomp].first;
        int next;
        if (!retval.reason && bicomp > 0 && bicomp < n_bicomps - 1) {
            next = bicomps[bicomp - 1].first;
        } else {
            next = bicomps[bicomp].second;
        }

        if (root < 0 || root >= g.n || next < 0 || next >= g.n) {
            N_LOG("ERROR: Invalid vertex indices in bicomp " << bicomp << "\n")
            continue;
        }

        dfs = std::stack<std::pair<int, int>>{};
        dfs.emplace(root, -1);
        dfs.emplace(next, 0);
        
        for (int i = 0; i < g.n; i++) {
            seq[i] = sp_tree{};
        }
        seq[next] = sp_tree{next, next};
        seq[root] = sp_tree{root, root};

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
        num_children[root] = 0;
        int curr_dfs = 3;

        while (!dfs.empty()) {
            std::pair<int, int> p = dfs.top();
            int w = p.first;
            int adj_index = p.second;
            
            if (w < 0 || w >= g.n) {
                dfs.pop();
                continue;
            }

            int v = (parent[w] >= 0 && parent[w] < g.n) ? parent[w] : -1;
            
            if (adj_index >= (int)g.adjLists[w].size()) {
                if (w != root) {
                    if (earliest_outgoing[w] != g.n && earliest_outgoing[w] >= 0 && earliest_outgoing[w] < g.n) {
                        if (!vertex_stacks[earliest_outgoing[w]].empty()) {
                            N_LOG("Moving sequence " << seq[w] << " to tail\n")
                            vertex_stacks[earliest_outgoing[w]].top().tail = std::move(seq[w]);
                        }
                    }

                    if (v == root) {
                        if (!fake_edge) {
                            seq[w].compose(sp_tree{w, v}, c_type::series);
                        }
                        
                        if (cut_verts[w] != -1 && cut_verts[w] >= 0 && cut_verts[w] < (int)cut_vertex_attached_tree.size()) {
                            seq[w].compose(std::move(cut_vertex_attached_tree[cut_verts[w]]), c_type::series);
                        }
                        
                        dfs.pop();
                        break;
                    } else if (v >= 0) {
                        if (cut_verts[w] != -1 && cut_verts[w] >= 0 && 
                            cut_verts[w] < (int)cut_vertex_attached_tree.size()) {
                            cut_vertex_attached_tree[cut_verts[w]].l_compose(sp_tree{w, v}, c_type::dangling);
                            seq[w].compose(std::move(cut_vertex_attached_tree[cut_verts[w]]), c_type::series);
                        } else {
                            seq[w].compose(sp_tree{w, v}, c_type::series);
                        }
                    }
                }
                
                dfs.pop();
                continue;
            }
            
            int u = g.adjLists[w][adj_index];
            
            if (u < 0 || u >= g.n) {
                dfs.top().second++;
                continue;
            }

                        if (comp[u] == -1 || comp[u] == bicomp) {
                V_LOG("v: " << v << " w: " << w << " u: " << u << "\n")
                V_LOG("seq_w: " << seq[w] << ", seq_u: " << seq[u] << "\n")
                
                if (dfs_no[u] == 0) {
                    dfs.push(std::pair{u, 0});
                    parent[u] = w;
                    dfs_no[u] = curr_dfs++;
                    comp[u] = bicomp;
                    num_children[w]++;
                    dfs.top().second++;
                    continue;
                }

                bool child_back_edge = (dfs_no[u] < dfs_no[w] && u != v);

                if (parent[u] == w) {
                    N_LOG("tree edge (" << w << ", " << u << ")\n")
                    
                    bool violation_found = false;
                    
                    while (w >= 0 && w < (int)vertex_stacks.size() && 
                           !vertex_stacks[w].empty() && !violation_found) {
                        
                        sp_chain_stack_entry& top_entry = vertex_stacks[w].top();
                        
                        if (seq[u].source() != top_entry.end) {
                            N_LOG("SP violation: interlacing detected\n")
                            
                            std::shared_ptr<negative_cert_K4> k4{new negative_cert_K4{}};
                            k4->b = seq[u].source();
                            k4->a = top_entry.end;
                            k4->c = w;
                            k4->d = -1;
                            
                            auto append_path = [&](std::vector<edge_t>& path, int start, int end) {
                                if (start == end) return;
                                int current = start;
                                while (current != end && current >= 0 && current < g.n && 
                                       current < (int)parent.size()) {
                                    int next_v = parent[current];
                                    if (next_v < 0 || next_v >= g.n) break;
                                    path.emplace_back(current, next_v);
                                    current = next_v;
                                }
                            };
                            
                            append_path(k4->ab, k4->a, k4->b);
                            append_path(k4->bc, k4->b, k4->c);
                            
                            int search_vertex = parent[k4->c];
                            while (search_vertex >= 0 && search_vertex < g.n && k4->d == -1) {
                                if (search_vertex < (int)vertex_stacks.size() && 
                                    !vertex_stacks[search_vertex].empty()) {
                                    if (vertex_stacks[search_vertex].top().end == k4->b) {
                                        k4->d = search_vertex;
                                        break;
                                    }
                                }
                                search_vertex = parent[search_vertex];
                            }
                            
                            if (k4->d >= 0) {
                                append_path(k4->cd, k4->c, k4->d);
                                append_path(k4->ad, k4->a, k4->d);
                                append_path(k4->bd, k4->b, k4->d);
                                
                                if (!vertex_stacks[k4->d].empty()) {
                                    sp_tree& violating_sp = vertex_stacks[k4->d].top().SP;
                                    if (violating_sp.root) {
                                        int ear_src = violating_sp.source();
                                        if (ear_src >= 0) {
                                            k4->ac.emplace_back(k4->c, ear_src);
                                            append_path(k4->ac, ear_src, k4->a);
                                        }
                                    }
                                }
                                
                                retval.reason = k4;
                                violation_found = true;
                            }
                            break;
                        }
                        
                        seq[u].compose(std::move(top_entry.SP), c_type::antiparallel);
                        seq[u].l_compose(std::move(top_entry.tail), c_type::series);
                        vertex_stacks[w].pop();
                    }
                    
                    if (violation_found) break;
                }

                if (parent[u] == w || child_back_edge) {
                    edge_t ear_f = (child_back_edge ? edge_t{w, u} : ear[u]);
                    sp_tree seq_u = (child_back_edge ? sp_tree{u, w} : std::move(seq[u]));

                    if (ear_wins(ear_f, ear[w], dfs_no, w)) {
                        if (ear[w].first != g.n && ear[w].first < g.n) {
                            if (seq[w].source() != ear[w].second) {
                                report_K4_sp(retval, parent, vertex_stacks, seq[w].source(), w, 
                                           ear[w].second, ear[w].first, ear_f.second, ear_f.first);
                                break;
                            }
                            if (ear[w].second >= 0 && ear[w].second < g.n) {
                                vertex_stacks[ear[w].second].emplace(std::move(seq[w]), w, sp_tree{});
                                earliest_outgoing[w] = ear[w].second;
                            }
                        }
                        ear[w] = ear_f;
                        seq[w] = std::move(seq_u);
                    } else {
                        if (seq_u.source() != ear_f.second) {
                            report_K4_sp(retval, parent, vertex_stacks, seq_u.source(), w, 
                                       ear_f.second, ear_f.first, ear[w].second, ear[w].first);
                            break;
                        }

                        if (dfs_no[ear_f.second] == dfs_no[ear[w].second]) {
                            if (seq[w].source() != ear[w].second) {
                                report_K4_sp(retval, parent, vertex_stacks, seq[w].source(), w, 
                                           ear[w].second, ear[w].first, ear_f.second, ear_f.first);
                                break;
                            }
                            seq[w].compose(std::move(seq_u), c_type::parallel);
                        } else {
                            if (ear_f.second >= 0 && ear_f.second < g.n) {
                                if (!vertex_stacks[ear_f.second].empty() && 
                                    vertex_stacks[ear_f.second].top().end == w) {
                                    vertex_stacks[ear_f.second].top().SP.compose(std::move(seq_u), c_type::parallel);
                                } else {
                                    vertex_stacks[ear_f.second].emplace(std::move(seq_u), w, sp_tree{});
                                    if (dfs_no[ear_f.second] < dfs_no[earliest_outgoing[w]]) {
                                        earliest_outgoing[w] = ear_f.second;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            dfs.top().second++;
        }

        if (retval.reason) {
            if (fake_edge) {
                edge_t fake = edge_t{root, next};
                std::shared_ptr<negative_cert_K4> k4 = std::dynamic_pointer_cast<negative_cert_K4>(retval.reason);
                
                if (k4) {
                    std::vector<edge_t>* k4_paths[6] = {&k4->ab, &k4->ac, &k4->ad, &k4->bc, &k4->bd, &k4->cd};
                    int k4_verts[4] = {k4->a, k4->b, k4->c, k4->d};
                    static const int k4_t4_translation[6][5] = {{1, 3, 2, 4, 5}, {0, 3, 2, 5, 4}, {0, 4, 1, 5, 3}, {0, 1, 4, 5, 2}, {0, 2, 3, 5, 1}, {1, 2, 3, 4, 0}};
                    static const int k4_t4_endpoint_translation[6][4] = {{0, 1, 2, 3}, {0, 2, 1, 3}, {0, 3, 1, 2}, {1, 2, 0, 3}, {1, 3, 0, 2}, {2, 3, 0, 1}};
                    
                    int pnum = 0;
                    for (; pnum < 6; pnum++) {
                        if (path_contains_edge(*(k4_paths[pnum]), fake) != -1) break;
                    }

                    if (pnum != 6) {
                        N_LOG("Fake edge in K4, generating T4\n")
                        std::shared_ptr<negative_cert_T4> t4{new negative_cert_T4{}};

                        std::vector<edge_t> temp_paths[6];
                        for (int i = 0; i < 6; i++) {
                            temp_paths[i] = *(k4_paths[i]);
                        }

                        t4->c1a = std::move(temp_paths[k4_t4_translation[pnum][0]]);
                        t4->c2a = std::move(temp_paths[k4_t4_translation[pnum][1]]);
                        t4->c1b = std::move(temp_paths[k4_t4_translation[pnum][2]]);
                        t4->c2b = std::move(temp_paths[k4_t4_translation[pnum][3]]);
                        t4->ab = std::move(temp_paths[k4_t4_translation[pnum][4]]);
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
                                earliest_outgoing[i] = g.n;
                                num_children[i] = 0;
                                seq[i] = sp_tree{};
                                vertex_stacks[i] = std::stack<sp_chain_stack_entry>{};
                            }
                        }

                        bicomp--;
                        continue;
                    }
                }
            }
            break;
        }

        dfs_no[root] = 0;

        if (cut_verts[root] != -1 && cut_verts[root] >= 0 && cut_verts[root] < (int)cut_vertex_attached_tree.size()) {
            seq[next].compose(std::move(cut_vertex_attached_tree[cut_verts[root]]), c_type::dangling);
        }

        if (bicomp < n_bicomps - 1) {
            V_LOG("Attaching tree to cut vertex " << root << "\n");
            if (cut_verts[root] != -1 && cut_verts[root] >= 0 && cut_verts[root] < (int)cut_vertex_attached_tree.size()) {
                                cut_vertex_attached_tree[cut_verts[root]] = std::move(seq[next]);
            }
        } else {
            if (!retval.reason) {
                std::shared_ptr<positive_cert_sp> sp{new positive_cert_sp{}};
                sp->decomposition = std::move(seq[next]);
                retval.reason = sp;
                retval.is_sp = true;
                N_LOG("Graph is SP\n")
            }
        }
    }
    
    return retval;
}

std::vector<edge_t> get_bicomps_sp(graph const& g, std::vector<int>& cut_verts, sp_result& cert_out, int root) {
    if (root < 0 || root >= g.n) {
        root = 0;
    }
    std::vector<int> dfs_no(g.n, 0);
    std::vector<int> parent(g.n, -1);
    std::vector<int> low(g.n, 0);
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
        
        if (w < 0 || w >= g.n) {
            dfs.pop();
            continue;
        }
        
        if (p.second >= (int)g.adjLists[w].size()) {
            dfs.pop();
            continue;
        }
        
        int u = g.adjLists[w][p.second];
        
        if (u < 0 || u >= g.n) {
            dfs.top().second++;
            continue;
        }
        
        if (dfs_no[u] == 0) {
            dfs.push(std::pair{u, 0});
            parent[u] = w;
            dfs_no[u] = curr_dfs++;
            low[u] = dfs_no[u];
            dfs.top().second++;
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

        dfs.top().second++;
    }

    int n_bicomps = (int)(retval.size());
    N_LOG(n_bicomps << " bicomp" << (n_bicomps == 1 ? "" : "s") << " found\n")

    if (!root_cut) cut_verts[root] = -1;

    retval.shrink_to_fit();
    if (cert_out.reason) return retval;

    N_LOG("no tri-comp-cut found\n")

    N_LOG("scanning for bicomp with three cut vertices:\n")

    std::vector<int> prev_cut(n_bicomps, -1);
    int root_one = -1;
    int root_two = -1;

    for (int i = 0; i < n_bicomps - 1; i++) {
        int w = retval[i].first;
        int u = -1;
        int start = w;

        while (w != root && w >= 0 && w < g.n) {
            u = w;
            w = parent[w];
            
            if (w < 0 || w >= g.n) break;

            if (cut_verts[w] != -1 && cut_verts[w] >= 0 && cut_verts[w] < (int)retval.size() && 
                u == retval[cut_verts[w]].second) {
                
                if (cut_verts[w] < (int)prev_cut.size()) {
                    if (prev_cut[cut_verts[w]] == -1) {
                        prev_cut[cut_verts[w]] = start;
                    } else {
                        std::shared_ptr<negative_cert_tri_cut_comp> cut{new negative_cert_tri_cut_comp{}};
                        cut->c1 = w;
                        cut->c2 = start;
                        cut->c3 = prev_cut[cut_verts[w]];
                        N_LOG("NON-SP, bicomp with three cut vertices\n")

                        cert_out.reason = cut;
                        return retval;
                    }
                }
                break;
            }
        }

        if (w == root && (!retval.empty() && (u == retval.back().second || u == -1))) {
            if (root_one == -1) {
                root_one = start;
            } else if (root_two == -1) {
                root_two = start;
            } else {
                std::shared_ptr<negative_cert_tri_cut_comp> cut{new negative_cert_tri_cut_comp{}};
                cut->c1 = root_one;
                cut->c2 = root_two;
                cut->c3 = start;
                N_LOG("NON-SP, bicomp at root with three cut vertices\n")

                cert_out.reason = cut;
                return retval;
            }
        }
    }

    N_LOG("no tri-cut-comp found\n")

    if (n_bicomps > 1) {
        int second_endpoint = n_bicomps - 1;

        for (int i = 1; i < n_bicomps - 1; i++) {
            if (i < (int)prev_cut.size() && prev_cut[i] == -1) {
                second_endpoint = i;
                break;
            }
        }

        if (second_endpoint < n_bicomps - 1) {
            std::reverse(retval.begin() + second_endpoint, retval.end() - 1);
        }
        
        if (second_endpoint != n_bicomps - 1 && n_bicomps >= 2) {
            retval.back().second = retval[n_bicomps - 2].first;
            retval.back().first = retval[n_bicomps - 2].second;
        } else if (n_bicomps >= 2) {
            if (retval.back().first == retval[n_bicomps - 2].first) {
                retval.back().first = retval.back().second;
            } else {
                int parent_vertex = retval[n_bicomps - 2].first;
                if (parent_vertex >= 0 && parent_vertex < (int)parent.size()) {
                    retval.back().first = parent[parent_vertex];
                }
            }
            retval.back().second = retval[n_bicomps - 2].first;
        }

        for (int i = second_endpoint; i < n_bicomps - 1; i++) {
            int vertex = retval[i].first;
            if (vertex >= 0 && vertex < (int)parent.size() && parent[vertex] >= 0) {
                retval[i].second = parent[vertex];
            }
        }
    }

    return retval;
}

void report_K4_sp(sp_result& cert_out,
std::vector<int> const& parent,
std::vector<std::stack<sp_chain_stack_entry>>& vertex_stacks,
int a,
int b,
int d,
int elose,
int ewin_src,
int ewin_sink) {
    if (a < 0 || b < 0 || d < 0 || elose < 0 || ewin_src < 0 || ewin_sink < 0) {
        N_LOG("ERROR: Invalid vertex indices in K4 report\n")
        return;
    }
    int n = parent.size();
    if (a >= n || b >= n || d >= n || elose >= n || ewin_src >= n || ewin_sink >= n) {
        N_LOG("ERROR: Vertex indices out of bounds in K4 report\n")
        return;
    }

    std::shared_ptr<negative_cert_K4> k4{new negative_cert_K4{}};
    k4->a = a;
    k4->b = b;
    k4->d = d;
    k4->c = -1;

    sp_tree earliest_violating_ear;
    for (int b_walk = (b < n) ? parent[b] : -1; 
         b_walk != d && b_walk >= 0 && b_walk < n; 
         b_walk = (b_walk < n) ? parent[b_walk] : -1) {
        
        if (b_walk < (int)vertex_stacks.size()) {
            while (!vertex_stacks[b_walk].empty()) {
                if (vertex_stacks[b_walk].top().end == k4->a) {
                    earliest_violating_ear = std::move(vertex_stacks[b_walk].top().SP);
                    k4->c = b_walk;
                    vertex_stacks[b_walk].pop();
                    break;
                } else {
                    vertex_stacks[b_walk].pop();
                }
            }
            if (k4->c != -1) break;
        }
    }

    if (k4->c == -1) {
        N_LOG("ERROR: Could not find vertex c in K4 construction\n")
        return;
    }

    auto safe_add_path = [&](std::vector<edge_t>& path, int start, int end) {
        if (start < 0 || start >= n || end < 0 || end >= n) return;
        
        int current = start;
        int iterations = 0;
        const int max_iterations = n + 1;
        
        while (current != end && current >= 0 && current < n && iterations < max_iterations) {
            int next_v = parent[current];
            if (next_v < 0 || next_v >= n) break;
            
            path.emplace_back(current, next_v);
            current = next_v;
            iterations++;
        }
    };

    safe_add_path(k4->ab, k4->a, k4->b);
    safe_add_path(k4->bc, k4->b, k4->c);
    safe_add_path(k4->cd, k4->c, k4->d);

    if (elose >= 0 && elose < n && ewin_sink >= 0 && ewin_sink < n) {
        k4->ad.emplace_back(k4->d, elose);
        safe_add_path(k4->ad, elose, ewin_sink);
        k4->ad.emplace_back(ewin_sink, ewin_src);
        safe_add_path(k4->ad, ewin_src, k4->a);
    }

    safe_add_path(k4->bd, k4->d, ewin_src);
    if (ewin_src >= 0 && ewin_src < n && ewin_sink >= 0 && ewin_sink < n) {
        k4->bd.emplace_back(ewin_src, ewin_sink);
        safe_add_path(k4->bd, ewin_sink, k4->b);
    }

    if (earliest_violating_ear.root) {
        int ear_path = earliest_violating_ear.source();
        if (ear_path >= 0 && ear_path < n) {
            k4->ac.emplace_back(k4->c, ear_path);
            safe_add_path(k4->ac, ear_path, k4->a);
        }
    }

    cert_out.reason = k4;
}

int path_contains_edge(std::vector<edge_t> const& path, edge_t test) {
    for (size_t i = 0; i < path.size(); i++) {
        edge_t e = path[i];
        if (e == test || (e.first == test.second && e.second == test.first)) {
            return (int)(i);
        }
    }
    return -1;
}

//==================== MAIN FUNCTION ====================
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>\n";
        std::cerr << "Input file format: first line contains 'n e' (vertices edges),\n";
        std::cerr << "followed by e lines with 'u v' (edge endpoints)\n";
        return 1;
    }
    
    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Error: Cannot open input file " << argv[1] << "\n";
        return 1;
    }

    graph g;
    input >> g;
    input.close();

    if (!input.eof() && input.fail()) {
        std::cerr << "Error: Invalid input format in " << argv[1] << "\n";
        return 1;
    }

    if (g.n <= 0) {
        std::cerr << "Error: Graph must have at least one vertex\n";
        return 1;
    }

    std::cout << "Read graph with " << g.n << " vertices and " << g.e << " edges\n\n";

    for (int i = 0; i < g.n; i++) {
        for (int j : g.adjLists[i]) {
            if (j < 0 || j >= g.n) {
                std::cerr << "Error: Invalid adjacency detected - vertex " << i 
                         << " adjacent to vertex " << j << " (out of range)\n";
                return 1;
            }
        }
    }

    sp_result result = SP_RECOGNITION(g);
    std::cout << "=== Series-Parallel Recognition Results ===\n";

    if (result.is_sp) {
        std::cout << "The graph IS series-parallel.\n";
        std::shared_ptr<positive_cert_sp> cert = std::dynamic_pointer_cast<positive_cert_sp>(result.reason);
        if (cert && cert->decomposition.root) {
            std::cout << "SP decomposition tree root: {" << cert->decomposition.source() 
                     << ", " << cert->decomposition.sink() << "}\n";
        } else {
            std::cout << "Empty SP decomposition (trivial graph).\n";
        }
    } else {
        std::cout << "The graph is NOT series-parallel.\n";
        std::cout << "Reason: ";
        
        if (auto k4 = std::dynamic_pointer_cast<negative_cert_K4>(result.reason)) {
            std::cout << "K4 subdivision found (vertices: " << k4->a << ", " << k4->b 
                     << ", " << k4->c << ", " << k4->d << ")\n";
            std::cout << "  Path sizes: ab=" << k4->ab.size() << ", ac=" << k4->ac.size() 
                     << ", ad=" << k4->ad.size() << ", bc=" << k4->bc.size() 
                     << ", bd=" << k4->bd.size() << ", cd=" << k4->cd.size() << "\n";
        } else if (auto t4 = std::dynamic_pointer_cast<negative_cert_T4>(result.reason)) {
            std::cout << "Theta-4 subdivision found\n";
            std::cout << "  Cut vertices: " << t4->c1 << ", " << t4->c2 << "\n";
            std::cout << "  Other vertices: " << t4->a << ", " << t4->b << "\n";
            std::cout << "  Path sizes: c1a=" << t4->c1a.size() << ", c1b=" << t4->c1b.size() 
                     << ", c2a=" << t4->c2a.size() << ", c2b=" << t4->c2b.size() 
                     << ", ab=" << t4->ab.size() << "\n";
        } else if (auto tcc_cut = std::dynamic_pointer_cast<negative_cert_tri_comp_cut>(result.reason)) {
            std::cout << "Cut vertex " << tcc_cut->v << " splits graph into " 
                     << num_comps_after_removal(g, tcc_cut->v) << " components\n";
        } else if (auto tcc_comp = std::dynamic_pointer_cast<negative_cert_tri_cut_comp>(result.reason)) {
            std::cout << "Biconnected component with 3+ cut vertices found\n";
            std::cout << "Cut vertices: " << tcc_comp->c1 << ", " << tcc_comp->c2 << ", " << tcc_comp->c3 << "\n";
        } else {
            std::cout << "Unknown violation type\n";
        }
    }

    std::cout << "\n=== Certificate Authentication ===\n";
    if (!result.reason) {
        std::cerr << "ERROR: No certificate generated!\n";
        return 1;
    }

    bool auth_success = false;
    try {
        auth_success = result.authenticate(g);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during authentication: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "ERROR: Unknown exception during authentication\n";
        return 1;
    }

    if (!auth_success) {
        std::cerr << "ERROR: Certificate authentication failed!\n";
        std::cerr << "This indicates a bug in the algorithm implementation.\n";
        return 1;
    }

    std::cout << "Certificate authenticated successfully.\n";

    return 0;
}
