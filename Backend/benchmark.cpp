// Benchmark: R-Tree vs brute-force range search and k-NN timing,
// plus a correctness check verifying k-NN results match brute-force.
#include <bits/stdc++.h>
#include "RTrees.cpp"

using namespace std;
using namespace std::chrono;

// ── Brute-force helpers ──────────────────────────────────────────────────────

vector<Point> brute_force_range(const vector<Point>& pts, const Rectangle& rect) {
    vector<Point> result;
    for (const auto& p : pts)
        if (rect.contains(p)) result.push_back(p);
    return result;
}

vector<pair<double, Point>> brute_force_knn(const vector<Point>& pts,
                                             const Point& query, int k) {
    vector<pair<double, Point>> dists;
    dists.reserve(pts.size());
    for (const auto& p : pts) {
        double dx = p.x - query.x, dy = p.y - query.y;
        dists.push_back({sqrt(dx*dx + dy*dy), p});
    }
    sort(dists.begin(), dists.end());
    if ((int)dists.size() > k) dists.resize(k);
    return dists;
}

// ── Timing utility ───────────────────────────────────────────────────────────

template<typename F>
double time_ms(F&& fn) {
    auto t0 = high_resolution_clock::now();
    fn();
    auto t1 = high_resolution_clock::now();
    return duration<double, milli>(t1 - t0).count();
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main() {
    const int N = 50000;
    const int K = 10;
    const int RANGE_QUERIES = 100;
    const int KNN_QUERIES   = 100;

    mt19937 rng(42);
    uniform_real_distribution<double> dist_x(0.0, 1.0);
    uniform_real_distribution<double> dist_y(0.0, 1.0);

    // Generate random dataset
    vector<Point> points;
    points.reserve(N);
    for (int i = 0; i < N; ++i)
        points.push_back({dist_x(rng), dist_y(rng)});

    // Build R-Tree
    RTree tree;
    cout << "Inserting " << N << " points into R-Tree...\n";
    double build_ms = time_ms([&]{ for (const auto& p : points) tree.insert(p); });
    cout << "  Build time: " << fixed << setprecision(2) << build_ms << " ms\n\n";

    // Generate random query rectangles (5% of coordinate space)
    uniform_real_distribution<double> origin_x(0.0, 0.95);
    uniform_real_distribution<double> origin_y(0.0, 0.95);
    vector<Rectangle> rects;
    for (int i = 0; i < RANGE_QUERIES; ++i) {
        double lx = origin_x(rng), ly = origin_y(rng);
        rects.push_back({Point(lx, ly), Point(lx + 0.05, ly + 0.05)});
    }

    // Generate random query points for k-NN
    vector<Point> queries;
    for (int i = 0; i < KNN_QUERIES; ++i)
        queries.push_back({dist_x(rng), dist_y(rng)});

    // ── Range query benchmark ────────────────────────────────────────────────
    cout << "=== Range query benchmark (" << RANGE_QUERIES << " queries) ===\n";

    double rtree_range_ms = time_ms([&]{
        for (const auto& r : rects) tree.search(r);
    });

    double brute_range_ms = time_ms([&]{
        for (const auto& r : rects) brute_force_range(points, r);
    });

    cout << "  R-Tree:      " << setw(8) << rtree_range_ms << " ms\n";
    cout << "  Brute-force: " << setw(8) << brute_range_ms << " ms\n";
    cout << "  Speedup:     " << setw(8) << setprecision(2)
         << brute_range_ms / rtree_range_ms << "x\n\n";

    // ── k-NN benchmark ───────────────────────────────────────────────────────
    cout << "=== k-NN benchmark (k=" << K << ", " << KNN_QUERIES << " queries) ===\n";

    double rtree_knn_ms = time_ms([&]{
        for (const auto& q : queries) tree.k_nearest_neighbors(q, K);
    });

    double brute_knn_ms = time_ms([&]{
        for (const auto& q : queries) brute_force_knn(points, q, K);
    });

    cout << "  R-Tree:      " << setw(8) << rtree_knn_ms << " ms\n";
    cout << "  Brute-force: " << setw(8) << brute_knn_ms << " ms\n";
    cout << "  Speedup:     " << setw(8) << setprecision(2)
         << brute_knn_ms / rtree_knn_ms << "x\n\n";

    // ── k-NN correctness check ───────────────────────────────────────────────
    cout << "=== k-NN correctness check ===\n";
    int pass = 0, fail = 0;
    for (const auto& q : queries) {
        auto rtree_res = tree.k_nearest_neighbors(q, K);
        auto brute_res = brute_force_knn(points, q, K);

        // Compare distances (allow small floating-point tolerance)
        bool ok = (rtree_res.size() == brute_res.size());
        if (ok) {
            for (size_t i = 0; i < rtree_res.size(); ++i) {
                if (fabs(rtree_res[i].first - brute_res[i].first) > 1e-9) {
                    ok = false;
                    break;
                }
            }
        }
        ok ? ++pass : ++fail;
    }
    cout << "  Passed: " << pass << "/" << KNN_QUERIES << "\n";
    if (fail > 0)
        cout << "  FAILED: " << fail << " queries produced wrong results!\n";
    else
        cout << "  All k-NN results match brute-force.\n";

    return fail > 0 ? 1 : 0;
}
