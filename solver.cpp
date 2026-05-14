#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include <chrono>
#include <random>
#include <numeric>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <cassert>
#include <functional>
#include <cstdlib>
#ifndef _WIN32
#include <unistd.h>
#endif
using namespace std;
//edho 
#ifdef _WIN32
  #define gc() getchar()
#else
  #define gc() getchar_unlocked()
#endif

inline int readint() {
    int x = 0; int c = gc();
    while (c != EOF && (c < '0' || c > '9')) c = gc();
    while (c >= '0' && c <= '9') { x = x * 10 + (c - '0'); c = gc(); }
    return x;
}

inline long long readll() {
    long long x = 0; int c = gc();
    while (c != EOF && (c < '0' || c > '9')) c = gc();
    while (c >= '0' && c <= '9') { x = x * 10 + (c - '0'); c = gc(); }
    return x;
}

static char obuf[1 << 24];
static int opos = 0;

void writechar(char c) {
    obuf[opos++] = c;
}

void writell(long long x) {
    if (x == 0) { writechar('0'); return; }
    char tmp[20]; int len = 0;
    while (x > 0) { tmp[len++] = '0' + (int)(x % 10); x /= 10; }
    for (int i = len - 1; i >= 0; --i) writechar(tmp[i]);
}

void flushout() {
    fwrite(obuf, 1, opos, stdout);
    opos = 0;
}

pair<long long, vector<int>> greedy_mwis(int N, vector<long long>& w, vector<vector<int>>& adj) {
    vector<pair<double, int>> score_idx(N);
    for (int i = 0; i < N; ++i) {
        int deg = (int)adj[i].size();
        score_idx[i] = {deg == 0 ? 1e18 : w[i] / (double)deg, i};
    }

    sort(score_idx.begin(), score_idx.end(), [](const pair<double, int>& a, const pair<double, int>& b) {
        return a.first > b.first;
    });

    vector<bool> selected(N, false);
    vector<bool> blocked(N, false);
    long long total_weight = 0;
    vector<int> selected_nodes;

    for (int i = 0; i < N; ++i) {
        int u = score_idx[i].second;
        if (!blocked[u]) {
            selected[u] = true;
            blocked[u] = true;
            total_weight += w[u];
            selected_nodes.push_back(u);
            for (int v : adj[u]) {
                blocked[v] = true;
            }
        }
    }

    return {total_weight, selected_nodes};
}

pair<long long, vector<int>> local_search(
    int N, vector<long long>& w,
    vector<vector<int>>& adj,
    vector<int> init_set,
    double time_limit_sec,
    chrono::steady_clock::time_point start_time)
{
    mt19937 rng(42);

    vector<bool> in_set(N, false);
    vector<int> neighbor_count(N, 0);
    long long cur_weight = 0;

    for (int u : init_set) {
        in_set[u] = true;
        cur_weight += w[u];
        for (int v : adj[u]) {
            neighbor_count[v]++;
        }
    }

    long long best_weight = cur_weight;
    vector<int> best_set = init_set;

    auto snapshot_best = [&]() {
        if (cur_weight > best_weight) {
            best_weight = cur_weight;
            best_set.clear();
            for (int i = 0; i < N; ++i) {
                if (in_set[i]) best_set.push_back(i);
            }
        }
    };

    auto add_node = [&](int v) {
        in_set[v] = true;
        cur_weight += w[v];
        for (int nb : adj[v]) {
            neighbor_count[nb]++;
        }
    };

    auto remove_node = [&](int v) {
        in_set[v] = false;
        cur_weight -= w[v];
        for (int nb : adj[v]) {
            neighbor_count[nb]--;
        }
    };

    vector<int> indices(N);
    iota(indices.begin(), indices.end(), 0);

    int iter_count = 0;

    while (true) {
        bool improved = false;
        shuffle(indices.begin(), indices.end(), rng);

        for (int idx = 0; idx < N; ++idx) {
            int v = indices[idx];

            if (!in_set[v] && neighbor_count[v] == 0) {
                add_node(v);
                improved = true;
                snapshot_best();
                continue;
            }

            if (in_set[v]) {
                int u = v;
                for (int ni = 0; ni < (int)adj[u].size(); ++ni) {
                    int cand = adj[u][ni];
                    if (in_set[cand]) continue;
                    if (w[cand] <= w[u]) continue;

                    remove_node(u);

                    if (neighbor_count[cand] == 0) {
                        add_node(cand);
                        improved = true;
                        snapshot_best();
                        goto next_index;
                    } else {
                        add_node(u);
                    }
                }
            }

            next_index:;

            if (++iter_count % 500 == 0) {
                auto now = chrono::steady_clock::now();
                double elapsed = chrono::duration<double>(now - start_time).count();
                if (elapsed >= time_limit_sec) return {best_weight, best_set};
            }
        }

        if (!improved) break;

        {
            auto now = chrono::steady_clock::now();
            double elapsed = chrono::duration<double>(now - start_time).count();
            if (elapsed >= time_limit_sec) break;
        }
    }

    return {best_weight, best_set};
}

pair<long long, vector<int>> restart_search(
    int N, vector<long long>& w,
    vector<vector<int>>& adj,
    vector<int> init_set,
    double total_limit_sec)
{
    random_device rd;
    mt19937 rng(rd());
    auto start = chrono::steady_clock::now();

    auto elapsed_sec = [&]() -> double {
        return chrono::duration<double>(chrono::steady_clock::now() - start).count();
    };

    auto global_best = local_search(N, w, adj, init_set, total_limit_sec, start);

    while (true) {
        double now_t = elapsed_sec();
        if (now_t > total_limit_sec - 5.0) break;
        double per_restart = min(40.0, total_limit_sec - now_t - 5.0);
        auto restart_start = chrono::steady_clock::now();
        vector<int> perturbed = global_best.second;
        int drop_count = max(1, (int)perturbed.size() / 4);
        shuffle(perturbed.begin(), perturbed.end(), rng);
        perturbed.resize(perturbed.size() - drop_count);
        auto result = local_search(N, w, adj, perturbed, per_restart, restart_start);
        if (result.first > global_best.first) global_best = result;
    }

    return global_best;
}

void signal_handler(int) {
    flushout();
    exit(0);
}

int main() {
    signal(SIGTERM, signal_handler);
#ifndef _WIN32
    signal(SIGALRM, signal_handler);
    alarm(295);
#endif

    int N = readint(), M = readint();

    vector<long long> w(N);
    for (int i = 0; i < N; ++i) w[i] = readll();

    vector<vector<int>> adj(N);
    for (int i = 0; i < M; ++i) {
        int u = readint() - 1, v = readint() - 1;
        adj[u].push_back(v);
        adj[v].push_back(u);
    }

    pair<long long, vector<int>> gres = greedy_mwis(N, w, adj);
    vector<int>& gi = gres.second;

    pair<long long, vector<int>> bres = restart_search(N, w, adj, gi, 290.0);
    long long bs = bres.first;
    vector<int>& bi = bres.second;

    sort(bi.begin(), bi.end());

    writell(bs); writechar('\n');
    for (int idx : bi) { writell(idx + 1); writechar(' '); }
    writechar('\n');
    flushout();

    return 0;
}
