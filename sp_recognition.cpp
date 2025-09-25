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

using edge_t = std::pair<int,int>;
struct graph;
void radix_sort(std::vector<int>& v);
bool trace_path(int end1, int end2, std::vector<edge_t> const& path, graph const& g, std::vector<bool>& seen);
int num_comps_after_removal(graph const& g, int v);
bool is_cut_vertex(graph const& g, int v);
int path_contains_edge(std::vector<edge_t> const& path, edge_t test);
// ==================== GRAPH ====================
struct graph {
    int n;
    int e;
    std::vector<std::vector<int>> adjLists;
    graph():n(0),e(0){}
    bool adjacent(int u,int v) const {
        if(u<0||v<0||u>=n||v>=n) return false;
        for(int x: adjLists[u]) if(x==v) return true;
        return false;
    }
    void add_edge(int u,int v){
        adjLists[u].push_back(v);
        adjLists[v].push_back(u);
        e++;
    }
    void reserve(graph const&other){
        n=other.n; e=0;
        adjLists.clear(); adjLists.resize(n);
        for(int i=0;i<n;i++) adjLists[i].reserve(other.adjLists[i].size());
    }
    void output_adj_list(int v,std::ostream&os) const{
        os<<"vertex "<<v<<" adjacencies: ";
        for(int w: adjLists[v]) os<<w<<" ";
        os<<"\n";
    }
};

std::istream& operator>>(std::istream&is, graph& g){
    g=graph{};
    is>>g.n>>g.e;
    g.adjLists.resize(g.n);
    int m=g.e; g.e=0;
    for(int i=0;i<m;i++){
        int u,v; is>>u>>v;
        g.add_edge(u,v);
    }
    for(auto& l:g.adjLists) l.shrink_to_fit();
    return is;
}

// ==================== SP TREE DEFINITIONS ====================
enum class c_type{edge,series,parallel,antiparallel,dangling};

struct sp_tree_node {
    int source; int sink; c_type comp;
    sp_tree_node* l; sp_tree_node* r;
    sp_tree_node(int s,int t,c_type c=c_type::edge)
        : source(s),sink(t),comp(c),l(nullptr),r(nullptr){}
    sp_tree_node(sp_tree_node const&o)
        : source(o.source),sink(o.sink),comp(o.comp),
          l(o.l?new sp_tree_node(*o.l):nullptr),
          r(o.r?new sp_tree_node(*o.r):nullptr){}
    ~sp_tree_node(){delete l;delete r;}
};

struct sp_tree {
    sp_tree_node* root;
    sp_tree():root(nullptr){}
    sp_tree(int s,int t):root(new sp_tree_node(s,t)){}
    sp_tree(sp_tree const&o):root(o.root?new sp_tree_node(*o.root):nullptr){}
    sp_tree(sp_tree&&o)noexcept:root(o.root){o.root=nullptr;}
    sp_tree& operator=(sp_tree&&o)noexcept{
        if(this!=&o){ delete root; root=o.root; o.root=nullptr;}
        return *this;
    }
    ~sp_tree(){delete root;}
    int source()const{ return root?root->source:-1;}
    int sink()const{ return root?root->sink:-1;}
    void compose(sp_tree&&other,c_type comp){
        if(!other.root) return;
        if(!root){ root=other.root; other.root=nullptr; return;}
        int ns=root->source, nt=root->sink;
        if(comp==c_type::series) nt=other.root->sink;
        if(comp==c_type::parallel||comp==c_type::antiparallel){
            ns=root->source; nt=root->sink;
        }
        sp_tree_node* newroot=new sp_tree_node(ns,nt,comp);
        newroot->l=root; newroot->r=other.root;
        root=newroot; other.root=nullptr;
    }
    void l_compose(sp_tree&&other,c_type comp){
        if(!other.root) return;
        if(!root){ root=other.root; other.root=nullptr; return;}
        sp_tree_node* newroot=new sp_tree_node(other.root->source,root->sink,comp);
        newroot->l=other.root; newroot->r=root;
        root=newroot; other.root=nullptr;
    }
    int underlying_tree_path_source() const {
        sp_tree_node* curr=root;
        while(curr&&curr->l) curr=curr->l;
        return curr?curr->source:-1;
    }
};

std::ostream& operator<<(std::ostream&os,sp_tree const&t){
    if(t.root) os<<"{"<<t.source()<<","<<t.sink()<<"}";
    else os<<"{}";
    return os;
}
struct certificate {
    bool verified = false;
    virtual bool authenticate(graph const& g)=0;
    virtual ~certificate(){}
};

// ===================== NEGATIVE CERT K4 ==========================
struct negative_cert_K4:certificate {
    int a,b,c,d;
    std::vector<edge_t> ab,ac,ad,bc,bd,cd;

    bool authenticate(graph const& g) override {
        if(verified) return true;
        if(a==b||a==c||a==d||b==c||b==d||c==d) return false;
        std::vector<bool> seen((size_t)g.n,false);
        if(!trace_path(a,b,ab,g,seen)) return false;
        if(!trace_path(a,c,ac,g,seen)) return false;
        if(!trace_path(a,d,ad,g,seen)) return false;
        if(!trace_path(b,c,bc,g,seen)) return false;
        if(!trace_path(b,d,bd,g,seen)) return false;
        if(!trace_path(c,d,cd,g,seen)) return false;
        verified=true;
        return true;
    }
};

// ===================== NEGATIVE CERT T4 ==========================
struct negative_cert_T4:certificate {
    int c1,c2,a,b;
    std::vector<edge_t> c1a,c1b,c2a,c2b,ab;

    bool authenticate(graph const& g) override {
        if(verified) return true;
        if(a==b||a==c1||a==c2||b==c1||b==c2||c1==c2) return false;
        if(!is_cut_vertex(g,c1)) return false;
        if(!is_cut_vertex(g,c2)) return false;
        std::vector<bool> seen((size_t)g.n,false);
        if(!trace_path(c1,a,c1a,g,seen)) return false;
        if(!trace_path(c2,a,c2a,g,seen)) return false;
        if(!trace_path(a,b,ab,g,seen)) return false;
        if(!trace_path(c1,b,c1b,g,seen)) return false;
        if(!trace_path(c2,b,c2b,g,seen)) return false;
        verified=true;
        return true;
    }
};

struct negative_cert_K23 : certificate {
    int a, b; 
    std::vector<edge_t> one, two, three;
    bool authenticate(graph const& g) override {
        // Ensure three internally disjoint paths of length>=2 exist
        if (verified) return true;
        if (a==b) return false;
        std::vector<bool> seen(g.n,false);
        if(!trace_path(a,b,one,g,seen)) return false;
        if(one.size()<2) return false;
        if(!trace_path(a,b,two,g,seen)) return false;
        if(two.size()<2) return false;
        if(!trace_path(a,b,three,g,seen)) return false;
        if(three.size()<2) return false;
        verified=true;
        return true;
    }
};

// ============== NEGATIVE CERT TRI-COMP-CUT =======================
struct negative_cert_tri_comp_cut:certificate {
    int v;
    bool authenticate(graph const& g) override {
        if(verified) return true;
        int comps=num_comps_after_removal(g,v);
        if(comps<3) return false;
        verified=true;
        return true;
    }
};

// ============== NEGATIVE CERT TRI-CUT-COMP =======================
struct negative_cert_tri_cut_comp:certificate {
    int c1,c2,c3;
    bool authenticate(graph const& g) override {
        if(verified) return true;
        if(!is_cut_vertex(g,c1)) return false;
        if(!is_cut_vertex(g,c2)) return false;
        if(!is_cut_vertex(g,c3)) return false;

        // Full bicomp check like Nathan’s code:
        std::vector<int> dfs_no(g.n,0), parent(g.n,-1), low(g.n,0);
        std::stack<edge_t> comp_edges;
        std::stack<std::pair<int,int>> dfs;

        dfs.emplace(0,0); dfs_no[0]=1; low[0]=1; int curr=2;
        while(!dfs.empty()){
            auto [w,it]=dfs.top();
            if(it<(int)g.adjLists[w].size()){
                dfs.top().second++;
                int u=g.adjLists[w][it];
                if(dfs_no[u]==0){
                    dfs.push({u,0});
                    comp_edges.emplace(w,u);
                    parent[u]=w; dfs_no[u]=curr++; low[u]=dfs_no[u];
                }
            } else {
                dfs.pop();
                if(parent[w]!=-1){
                    if(low[w]>=dfs_no[parent[w]]){
                        bool seen[3]={false,false,false};
                        edge_t e;
                        do{
                            e=comp_edges.top(); comp_edges.pop();
                            if(e.first==c1||e.second==c1) seen[0]=true;
                            if(e.first==c2||e.second==c2) seen[1]=true;
                            if(e.first==c3||e.second==c3) seen[2]=true;
                        } while(e!=edge_t{parent[w],w});
                        if(seen[0]&&seen[1]&&seen[2]){
                            verified=true;
                            return true;
                        }
                    }
                    if(low[w]<low[parent[w]]) low[parent[w]]=low[w];
                }
            }
        }
        return false;
    }
};

// ===================== POSITIVE CERT SP ==========================
struct positive_cert_sp:certificate {
    sp_tree decomposition;
    bool is_sp=true;

    bool authenticate(graph const& g) override {
        if(verified) return true;
        if(!decomposition.root) { verified=true; return true; }
        graph g2; g2.n=g.n; g2.adjLists.resize(g.n); g2.e=0;
        std::stack<std::pair<sp_tree_node*,bool>> st;
        st.push({decomposition.root,false});
        while(!st.empty()){
            auto [node,sw]=st.top(); st.pop();
            if(!node) continue;
            int s=(sw?node->sink:node->source);
            int t=(sw?node->source:node->sink);
            if(!node->l && !node->r){
                if(node->comp!=c_type::edge) return false;
                if(s!=t) g2.add_edge(s,t);
            } else {
                bool lsw=sw,rsw=sw;
                if(node->comp==c_type::antiparallel) rsw=!sw;
                st.push({node->r,rsw});
                st.push({node->l,lsw});
            }
        }
        for(int i=0;i<g.n;i++){
            auto o=g.adjLists[i], p=g2.adjLists[i];
            radix_sort(o); radix_sort(p);
            if(o!=p) return false;
        }
        verified=true;
        return true;
    }
};
struct sp_chain_stack_entry {
    sp_tree SP;
    int end;
    sp_tree tail;
    sp_chain_stack_entry(sp_tree&& S, int e, sp_tree&& T)
        : SP(std::move(S)), end(e), tail(std::move(T)) {}
};

// ===================== SP RESULT ==========================
struct sp_result {
    bool is_sp=false;
    std::shared_ptr<certificate> reason;
    bool authenticate(graph const& g){
        if(!reason) return false;
        return reason->authenticate(g);
    }
};
// ===================== UTILITIES ======================
void radix_sort(std::vector<int>& v) {
    if(v.empty()) return;
    int max_val=*std::max_element(v.begin(),v.end());
    std::vector<int> output(v.size());
    std::vector<int> count(10);
    for(int exp=1; max_val/exp>0; exp*=10) {
        std::fill(count.begin(),count.end(),0);
        for(int i:v) count[(i/exp)%10]++;
        for(int i=1;i<10;i++) count[i]+=count[i-1];
        for(int i=(int)v.size()-1;i>=0;i--){
            output[count[(v[i]/exp)%10]-1]=v[i];
            count[(v[i]/exp)%10]--;
        }
        v=output;
    }
}

bool trace_path(int end1,int end2,std::vector<edge_t> const&path,graph const& g,std::vector<bool>& seen){
    if(path.empty()) return false;
    int start=end1, finish=end2;
    if(path[0].first==end2){ start=end2; finish=end1; }
    if(path[0].first!=start) return false;
    if(path.back().second!=finish) return false;
    seen[start]=true; int prev=start;
    for(auto e: path) {
        if(!g.adjacent(e.first,e.second)) return false;
        if(prev!=e.first) return false;
        prev=e.second;
        if(seen[e.second]) return false;
        seen[e.second]=true;
    }
    seen[start]=false; seen[finish]=false;
    return true;
}

int num_comps_after_removal(graph const& g,int v){
    int comps=0; std::vector<bool> seen(g.n,false);
    for(int i=0;i<g.n;i++){
        if(i==v||seen[i]) continue;
        comps++;
        std::stack<int> st; st.push(i);
        while(!st.empty()){
            int u=st.top(); st.pop();
            if(seen[u]) continue;
            seen[u]=true;
            for(int w:g.adjLists[u]) if(w!=v && !seen[w]) st.push(w);
        }
    }
    return comps;
}

bool is_cut_vertex(graph const& g,int v){ return num_comps_after_removal(g,v)>1; }

int path_contains_edge(std::vector<edge_t> const& path,edge_t test){
    for(size_t i=0;i<path.size();i++){
        auto e=path[i];
        if(e==test || (e.first==test.second && e.second==test.first))
            return (int)i;
    }
    return -1;
}

// ===================== FULL GET_BICOMPS_SP ======================
std::vector<edge_t> get_bicomps_sp(graph const& g, std::vector<int>& cut_verts, sp_result& cert_out, int root=0) {
    std::vector<int> dfs_no(g.n,0), parent(g.n,-1), low(g.n,0);
    std::vector<edge_t> retval;
    std::stack<std::pair<int,int>> dfs;

    dfs.emplace(root,0);
    dfs_no[root]=1; low[root]=1; parent[root]=-1;
    int curr_dfs=2;
    bool root_cut=false;

    while(!dfs.empty()){
        auto &p = dfs.top(); int w=p.first, i=p.second;
        if(i < (int)g.adjLists[w].size()){
            int u = g.adjLists[w][i];
            p.second++;
            if(!dfs_no[u]){
                dfs.push({u,0});
                parent[u]=w;
                dfs_no[u]=curr_dfs++; low[u]=dfs_no[u];
            } else if(u!=parent[w]){
                low[w] = std::min(low[w], dfs_no[u]);
            }
        } else {
            dfs.pop();
            if(parent[w]!=-1){
                if(low[w]>=dfs_no[parent[w]]){
                    if(cut_verts[w]!=-1){
                        if(w!=root || root_cut){
                            auto cut = std::make_shared<negative_cert_tri_comp_cut>();
                            cut->v = w;
                            cert_out.reason = cut;
                            cert_out.is_sp = false;
                            return retval;
                        } else root_cut=true;
                    } else cut_verts[w]=retval.size();
                    retval.emplace_back(w,parent[w]);
                }
                if(low[w]<low[parent[w]]) low[parent[w]]=low[w];
            }
        }
    }

    if(!root_cut) cut_verts[root]=-1;
    if(cert_out.reason) return retval;

    // Tri-cut-comp detection
    std::vector<int> prev_cut(retval.size(),-1);
    int root_one=-1, root_two=-1;
    for(int i=0;i<(int)retval.size()-1;i++){
        int w=retval[i].first, u=-1, start=w;
        while(w!=root){
            u=w; w=parent[w];
            if(cut_verts[w]!=-1 && u==retval[cut_verts[w]].second){
                if(prev_cut[cut_verts[w]]==-1) prev_cut[cut_verts[w]]=start;
                else {
                    auto cut = std::make_shared<negative_cert_tri_cut_comp>();
                    cut->c1=w; cut->c2=start; cut->c3=prev_cut[cut_verts[w]];
                    cert_out.reason=cut;
                    cert_out.is_sp=false;
                    return retval;
                }
                break;
            }
        }
        if(w==root && (u==retval.back().second||u==-1)){
            if(root_one==-1) root_one=start;
            else if(root_two==-1) root_two=start;
            else {
                auto cut = std::make_shared<negative_cert_tri_cut_comp>();
                cut->c1=root_one; cut->c2=root_two; cut->c3=start;
                cert_out.reason=cut;
                cert_out.is_sp=false;
                return retval;
            }
        }
    }

    // Chain reordering
    int n_bicomps=(int)retval.size();
    if(n_bicomps>1){
        int second_endpoint = n_bicomps-1;
        for(int i=1;i<n_bicomps-1;i++){
            if(prev_cut[i]==-1){ second_endpoint=i; break; }
        }
        std::reverse(retval.begin()+second_endpoint, retval.end()-1);
        if(second_endpoint!=n_bicomps-1){
            retval.back().second=retval[n_bicomps-2].first;
            retval.back().first=retval[n_bicomps-2].second;
        } else {
            retval.back().first=retval.back().second;
            retval.back().second=retval[n_bicomps-2].first;
        }
        for(int i=second_endpoint;i<n_bicomps-1;i++){
            retval[i].second=parent[retval[i].first];
        }
    }
    return retval;
}
// ===================== FULL REPORT_K4_SP ======================
void report_K4_sp(sp_result& cert_out,
                  std::vector<int> const& parent,
                  std::vector<std::stack<sp_chain_stack_entry>>& vertex_stacks,
                  int a,int b,int d,int elose,int ewin_src,int ewin_sink){
    auto k4=std::make_shared<negative_cert_K4>();
    k4->a=a; k4->b=b; k4->d=d; k4->c=-1;
    sp_tree earliest;
    for(int bw=parent[b];bw!=d;bw=parent[bw]){
        while(!vertex_stacks[bw].empty()){
            if(vertex_stacks[bw].top().end==k4->a){
                earliest=std::move(vertex_stacks[bw].top().SP);
                k4->c=bw; break;
            } else vertex_stacks[bw].pop();
        }
        if(k4->c!=-1) break;
    }
    for(int x=k4->a;x!=k4->b;x=parent[x]) k4->ab.emplace_back(x,parent[x]);
    for(int x=k4->b;x!=k4->c;x=parent[x]) k4->bc.emplace_back(x,parent[x]);
    for(int x=k4->c;x!=k4->d;x=parent[x]) k4->cd.emplace_back(x,parent[x]);
    k4->ad.emplace_back(k4->d,elose);
    for(int x=elose;x!=k4->a;x=parent[x]) k4->ad.emplace_back(x,parent[x]);
    for(int x=k4->d;x!=ewin_src;x=parent[x]) k4->bd.emplace_back(x,parent[x]);
    k4->bd.emplace_back(ewin_src,ewin_sink);
    for(int x=ewin_sink;x!=k4->b;x=parent[x]) k4->bd.emplace_back(x,parent[x]);
    if(earliest.root){
        int ep=earliest.underlying_tree_path_source();
        k4->ac.emplace_back(k4->c,ep);
        for(int x=ep;x!=k4->a;x=parent[x]) k4->ac.emplace_back(x,parent[x]);
    }
    cert_out.reason=k4;
    cert_out.is_sp = false;


}
sp_result SP_RECOGNITION(graph const& g) {
    sp_result retval;
    std::vector<int> cut_verts(g.n,-1);
    std::vector<edge_t> bicomps = get_bicomps_sp(g, cut_verts, retval);
    if (retval.reason) return retval;

    int n_bicomps = (int)bicomps.size();
    std::vector<sp_tree> cut_vertex_attached_tree(n_bicomps);
    std::vector<int> comp(g.n,-1);
    std::vector<std::stack<sp_chain_stack_entry>> vertex_stacks(g.n);
    std::vector<int> dfs_no(g.n+1), parent(g.n);
    std::vector<edge_t> ear(g.n, edge_t{g.n,g.n});
    std::vector<sp_tree> seq(g.n);
    std::vector<int> earliest_outgoing(g.n,g.n);
    std::vector<char> num_children(g.n);
    std::vector<int> alert(g.n,-1);
    std::stack<std::pair<int,int>> dfs;

    dfs_no[g.n]=g.n;
    bool do_k23_edge_replacement = true;

    for(int bicomp=0; bicomp<n_bicomps; bicomp++) {
        // reset per bicomp
        for (int i=0;i<g.n;i++){
            seq[i]=sp_tree{};
            ear[i]={g.n,g.n};
            earliest_outgoing[i]=g.n;
            num_children[i]=0;
            alert[i]=-1;
        }

        int root=bicomps[bicomp].first;
        int next=bicomps[bicomp].second;
        dfs.emplace(root,-1);
        dfs.emplace(next,0);

        bool fake_edge=true;
        for(int u1: g.adjLists[next]) if(u1==root){ fake_edge=false; break; }

        dfs_no[root]=1; parent[root]=-1;
        dfs_no[next]=2; parent[next]=root; comp[next]=bicomp;
        int curr_dfs=3;

        while(!dfs.empty()){
            auto &p=dfs.top(); int w=p.first; int v=parent[w];
            if((size_t)p.second>=g.adjLists[w].size()){
                if(w!=root){
                    if(earliest_outgoing[w]!=g.n){
                        if(!vertex_stacks[earliest_outgoing[w]].empty())
                            vertex_stacks[earliest_outgoing[w]].top().tail=std::move(seq[w]);
                    }
                    if(v==root){
                        seq[w].compose((fake_edge? sp_tree{}: sp_tree{w,v}), c_type::parallel);
                        if(cut_verts[w]!=-1) seq[w].compose(std::move(cut_vertex_attached_tree[cut_verts[w]]),c_type::series);
                        seq[next]=std::move(seq[w]);
                        break;
                    } else {
                        if(cut_verts[w]!=-1){
                            cut_vertex_attached_tree[cut_verts[w]].l_compose(sp_tree{w,v},c_type::dangling);
                            seq[w].compose(std::move(cut_vertex_attached_tree[cut_verts[w]]),c_type::series);
                        } else {
                            seq[w].compose(sp_tree{w,v},c_type::series);
                        }
                    }
                }
                dfs.pop();
                continue;
            }
            int u=g.adjLists[w][p.second++];
            if(comp[u]!=-1 && comp[u]!=bicomp) continue;
            if(dfs_no[u]==0){
                dfs.push({u,0});
                parent[u]=w; dfs_no[u]=curr_dfs++; comp[u]=bicomp; num_children[w]++;
                continue;
            }
            bool child_back=(dfs_no[u]<dfs_no[w] && u!=v);
            if(parent[u]==w){
                for(;!vertex_stacks[w].empty();vertex_stacks[w].pop()){
                    if(seq[u].source()!=vertex_stacks[w].top().end){
                        report_K4_sp(retval,parent,vertex_stacks,seq[u].source(),w,
                                     vertex_stacks[w].top().end,ear[u].first,ear[u].second,ear[w].first);
                        retval.is_sp=false; return retval;
                    }
                    seq[u].compose(std::move(vertex_stacks[w].top().SP),c_type::antiparallel);
                    seq[u].l_compose(std::move(vertex_stacks[w].top().tail),c_type::series);
                }
                if(retval.reason) return retval;
            }
            if(parent[u]==w||child_back){
                edge_t ear_f=(child_back? edge_t{w,u}:ear[u]);
                sp_tree seq_u=(child_back? sp_tree{u,w}:std::move(seq[u]));
                if(dfs_no[ear_f.second]<dfs_no[ear[w].second]){
                    if(ear[w].first!=g.n){
                        if(seq[w].source()!=ear[w].second){
                            report_K4_sp(retval,parent,vertex_stacks,seq[w].source(),w,
                                         ear[w].second,ear[w].first,ear_f.second,ear_f.first);
                            retval.is_sp=false; return retval;
                        }
                        vertex_stacks[ear[w].second].emplace(std::move(seq[w]),w,sp_tree{});
                        earliest_outgoing[w]=ear[w].second;
                    }
                    ear[w]=ear_f; seq[w]=std::move(seq_u);
                } else {
                    if(seq_u.source()!=ear_f.second){
                        report_K4_sp(retval,parent,vertex_stacks,seq_u.source(),w,
                                     ear_f.second,ear_f.first,ear[w].second,ear[w].first);
                        retval.is_sp=false; return retval;
                    }
                    if(dfs_no[ear_f.second]==dfs_no[ear[w].second]){
                        if(seq[w].source()!=ear[w].second){
                            report_K4_sp(retval,parent,vertex_stacks,seq[w].source(),w,
                                         ear[w].second,ear[w].first,ear_f.second,ear_f.first);
                            retval.is_sp=false; return retval;
                        }
                        seq[w].compose(std::move(seq_u),c_type::parallel);
                        if((ear[w].first==w||dfs_no[ear_f.first]<dfs_no[ear[w].first])&&ear_f.first!=w) ear[w]=ear_f;
                    } else {
                        if(!vertex_stacks[ear_f.second].empty() && vertex_stacks[ear_f.second].top().end==w){
                            vertex_stacks[ear_f.second].top().SP.compose(std::move(seq_u),c_type::parallel);
                        } else {
                            vertex_stacks[ear_f.second].emplace(std::move(seq_u),w,sp_tree{});
                            if(dfs_no[ear_f.second]<dfs_no[earliest_outgoing[w]]) earliest_outgoing[w]=ear_f.second;
                        }
                    }
                }
            }
        }

        dfs_no[root]=0;

        // only mark positive cert if no negative detected
        if(!retval.reason){
            auto sp = std::make_shared<positive_cert_sp>();
            sp->decomposition=std::move(seq[next]);
            sp->is_sp=true;
            retval.reason=sp;
            retval.is_sp=true;
        }
    }
    return retval;
}
int main(int argc, char* argv[]) {
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <graph_input_file>\n";
        return 1;
    }

    std::ifstream infile(argv[1]);
    if(!infile) {
        std::cerr << "Error: could not open file " << argv[1] << "\n";
        return 1;
    }

    graph g;
    infile >> g;
    if(g.n <= 0) {
        std::cerr << "Error: Graph must have at least one vertex\n";
        return 1;
    }

    std::cout << "Read graph with " << g.n << " vertices and " << g.e << " edges\n\n";

    sp_result result = SP_RECOGNITION(g);

    std::cout << "=== Series-Parallel Recognition Results ===\n";
    if(result.is_sp) {
        std::cout << "The graph IS Series-Parallel.\n";
        auto sp = std::dynamic_pointer_cast<positive_cert_sp>(result.reason);
        if(sp && sp->decomposition.root) {
            std::cout << "SP decomposition tree root: {"
                      << sp->decomposition.source() << ","
                      << sp->decomposition.sink() << "}\n";
        } else {
            std::cout << "Empty SP decomposition (trivial).\n";
        }
    } else {
        std::cout << "The graph is NOT Series-Parallel.\n";
        if(auto k4 = std::dynamic_pointer_cast<negative_cert_K4>(result.reason)) {
            std::cout << "Reason: K4 subdivision on vertices {"
                      << k4->a << "," << k4->b << "," << k4->c << "," << k4->d << "}\n";
        } else if(auto t4 = std::dynamic_pointer_cast<negative_cert_T4>(result.reason)) {
            std::cout << "Reason: T4 (theta-4) subdivision with cut vertices "
                      << t4->c1 << "," << t4->c2
                      << " and others " << t4->a << "," << t4->b << "\n";
        } else if(auto tri = std::dynamic_pointer_cast<negative_cert_tri_comp_cut>(result.reason)) {
            std::cout << "Reason: cut vertex " << tri->v << " splits into >=3 components\n";
        } else if(auto tric = std::dynamic_pointer_cast<negative_cert_tri_cut_comp>(result.reason)) {
            std::cout << "Reason: bicomp with 3 cut vertices {"
                      << tric->c1 << "," << tric->c2 << "," << tric->c3 << "}\n";
        } else {
            std::cout << "Reason: unknown (unhandled cert type)\n";
        }
    }

    std::cout << "\n=== Certificate Authentication ===\n";
    if(!result.reason) {
        std::cerr << "ERROR: No certificate generated\n";
        return 1;
    }

    bool auth_ok = false;
    try { auth_ok = result.authenticate(g); }
    catch(...) { auth_ok=false; }

    if(!auth_ok) {
        std::cerr << "ERROR: Certificate authentication failed!\n";
        return 1;
    }
    std::cout << "Certificate authenticated successfully.\n";
    return 0;
}
