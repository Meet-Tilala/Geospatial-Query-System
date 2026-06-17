#include <bits/stdc++.h>
#include <memory>
using namespace std;

class Point {
public:
    double x, y;

    Point() : x(0), y(0) {}
    Point(double x, double y) : x(x), y(y) {}

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

class Rectangle {
public:
    Point lower;
    Point upper;

    Rectangle() : lower(Point()), upper(Point()) {}
    Rectangle(const Point& l, const Point& u) : lower(l), upper(u) {}

    double area() const {
        return (upper.x - lower.x) * (upper.y - lower.y);
    }

    bool contains(const Point& p) const {
        return p.x >= lower.x && p.x <= upper.x &&
               p.y >= lower.y && p.y <= upper.y;
    }

    bool intersects(const Rectangle& other) const {
        return !(other.lower.x > upper.x ||
                 other.upper.x < lower.x ||
                 other.lower.y > upper.y ||
                 other.upper.y < lower.y);
    }

    static Rectangle bounding_box(const Rectangle& a, const Rectangle& b) {
        Point lower(min(a.lower.x, b.lower.x), min(a.lower.y, b.lower.y));
        Point upper(max(a.upper.x, b.upper.x), max(a.upper.y, b.upper.y));
        return Rectangle(lower, upper);
    }
};

const int MAX_ENTRIES = 4;
const int MIN_ENTRIES = MAX_ENTRIES / 2;

class Node {
public:
    bool is_leaf;
    Rectangle mbr;
    vector<unique_ptr<Node>> children;
    vector<Point> entries;

    Node(bool leaf) : is_leaf(leaf) {
        if (!is_leaf) {
            children.reserve(MAX_ENTRIES + 1);
        } else {
            entries.reserve(MAX_ENTRIES + 1);
        }
    }

    void adjust_mbr() {
        if (is_leaf) {
            if (entries.empty()) {
                mbr = Rectangle();
                return;
            }

            mbr.lower = entries[0];
            mbr.upper = entries[0];

            for (const auto& entry : entries) {
                mbr.lower.x = min(mbr.lower.x, entry.x);
                mbr.lower.y = min(mbr.lower.y, entry.y);
                mbr.upper.x = max(mbr.upper.x, entry.x);
                mbr.upper.y = max(mbr.upper.y, entry.y);
            }
        } else {
            if (children.empty()) {
                mbr = Rectangle();
                return;
            }

            for (size_t i = 0; i < children.size(); i++) {
                children[i]->adjust_mbr();
            }

            mbr = children[0]->mbr;

            for (const auto& child : children) {
                mbr = Rectangle::bounding_box(mbr, child->mbr);
            }
        }
    }

    bool is_full() const {
        return is_leaf ? entries.size() >= MAX_ENTRIES : children.size() >= MAX_ENTRIES;
    }

    bool is_underfull() const {
        return is_leaf ? entries.size() < MIN_ENTRIES : children.size() < MIN_ENTRIES;
    }
};

class RTree {
private:

    void delete_entry_from_leaf(Node* leaf, const Point& point) {
        auto it = find_if(leaf->entries.begin(), leaf->entries.end(),
            [&point](const Point& p) { return p == point; });

        if (it != leaf->entries.end()) {
            leaf->entries.erase(it);
        }
    }

    void merge_nodes(Node* node1, Node* node2) {
        if (node1->is_leaf) {
            node1->entries.insert(node1->entries.end(),
                make_move_iterator(node2->entries.begin()),
                make_move_iterator(node2->entries.end()));
        } else {
            node1->children.insert(node1->children.end(),
                make_move_iterator(node2->children.begin()),
                make_move_iterator(node2->children.end()));
        }

        node1->adjust_mbr();
    }

    void remove_child(Node* parent, Node* child) {
        auto it = find_if(parent->children.begin(), parent->children.end(),
            [child](const unique_ptr<Node>& p) { return p.get() == child; });

        if (it != parent->children.end()) {
            parent->children.erase(it);
        }
    }

    Node* find_sibling(Node* parent, Node* node) {
        if (parent->children.size() < MIN_ENTRIES) return nullptr;

        for (size_t i = 0; i < parent->children.size(); i++) {
            if (parent->children[i].get() == node) {
                if (i > 0) return parent->children[i-1].get();
                return parent->children[i+1].get();
            }
        }
        return nullptr;
    }

    double calculate_distance(const Point& p1, const Point& p2) {
        double dx = p1.x - p2.x;
        double dy = p1.y - p2.y;
        return sqrt(dx * dx + dy * dy);
    }

    double min_dist_to_rect(const Point& p, const Rectangle& rect) {
        double dx = max({0.0, rect.lower.x - p.x, p.x - rect.upper.x});
        double dy = max({0.0, rect.lower.y - p.y, p.y - rect.upper.y});
        return sqrt(dx * dx + dy * dy);
    }

    void nearest_neighbor_recursive(Node* node, const Point& query_point,
                                    Point& nearest_point, double& min_dist) {
        if (min_dist_to_rect(query_point, node->mbr) >= min_dist) {
            return;
        }

        if (node->is_leaf) {
            for (const auto& entry : node->entries) {
                double dist = calculate_distance(query_point, entry);
                if (dist < min_dist) {
                    min_dist = dist;
                    nearest_point = entry;
                }
            }
        } else {
            vector<pair<double, Node*>> node_distances;
            for (const auto& child : node->children) {
                double dist = min_dist_to_rect(query_point, child->mbr);
                node_distances.push_back({dist, child.get()});
            }

            sort(node_distances.begin(), node_distances.end());

            for (const auto& nd : node_distances) {
                if (nd.first >= min_dist) break;
                nearest_neighbor_recursive(nd.second, query_point, nearest_point, min_dist);
            }
        }
    }

    unique_ptr<Node> root;

    Node* choose_leaf(const Point& point) {
        Node* current = root.get();

        while (!current->is_leaf) {
            double min_enlargement = numeric_limits<double>::max();
            Node* best_child = nullptr;

            for (const auto& child : current->children) {
                double enlargement = calculate_enlargement(child->mbr, point);
                if (enlargement < min_enlargement) {
                    min_enlargement = enlargement;
                    best_child = child.get();
                }
            }

            current = best_child;
        }

        return current;
    }

    void split_node(Node* node) {
        if (node->is_leaf) {
            if (node == root.get()) {
                vector<Point> all_entries = std::move(node->entries);
                node->entries.clear();

                sort(all_entries.begin(), all_entries.end(),
                     [](const Point& a, const Point& b) { return a.x < b.x; });

                size_t mid = all_entries.size() / 2;

                auto new_node = make_unique<Node>(true);
                for (size_t i = 0; i < mid; ++i) {
                    new_node->entries.push_back(all_entries[i]);
                }

                auto new_node1 = make_unique<Node>(true);
                for (size_t i = mid; i < all_entries.size(); ++i) {
                    new_node1->entries.push_back(all_entries[i]);
                }

                node->is_leaf = false;
                node->children.push_back(std::move(new_node));
                node->children.push_back(std::move(new_node1));
            } else {
                vector<Point> all_entries = std::move(node->entries);
                node->entries.clear();

                sort(all_entries.begin(), all_entries.end(),
                     [](const Point& a, const Point& b) { return a.x < b.x; });

                size_t mid = all_entries.size() / 2;

                for (size_t i = 0; i < mid; ++i) {
                    node->entries.push_back(all_entries[i]);
                }

                auto new_node = make_unique<Node>(true);
                for (size_t i = mid; i < all_entries.size(); ++i) {
                    new_node->entries.push_back(all_entries[i]);
                }

                Node* parent = find_parent(root.get(), node);
                if (parent == root.get() && parent->children.empty()) {
                    auto new_root = make_unique<Node>(false);
                    new_root->children.push_back(std::move(root));
                    new_root->children.push_back(std::move(new_node));
                    root = std::move(new_root);
                } else {
                    parent->children.push_back(std::move(new_node));
                }
            }
        } else {
            if (node == root.get()) {
                vector<unique_ptr<Node>> all_children = std::move(node->children);
                node->children.clear();

                sort(all_children.begin(), all_children.end(),
                     [](const unique_ptr<Node>& a, const unique_ptr<Node>& b) {
                         return a->mbr.lower.x < b->mbr.lower.x; });

                size_t mid = all_children.size() / 2;

                auto new_node = make_unique<Node>(false);
                for (size_t i = 0; i < mid; ++i) {
                    new_node->children.push_back(std::move(all_children[i]));
                }

                auto new_node1 = make_unique<Node>(false);
                for (size_t i = mid; i < all_children.size(); ++i) {
                    new_node1->children.push_back(std::move(all_children[i]));
                }

                node->children.push_back(std::move(new_node));
                node->children.push_back(std::move(new_node1));
            } else {
                vector<unique_ptr<Node>> all_children = std::move(node->children);
                node->children.clear();

                sort(all_children.begin(), all_children.end(),
                     [](const unique_ptr<Node>& a, const unique_ptr<Node>& b) {
                     return a->mbr.lower.x < b->mbr.lower.x; });

                size_t mid = all_children.size() / 2;

                for (size_t i = 0; i < mid; ++i) {
                    node->children.push_back(std::move(all_children[i]));
                }

                auto new_node = make_unique<Node>(false);
                for (size_t i = mid; i < all_children.size(); ++i) {
                    new_node->children.push_back(std::move(all_children[i]));
                }

                Node* parent = find_parent(root.get(), node);
                if (parent == root.get() && parent->children.empty()) {
                    auto new_root = make_unique<Node>(false);
                    new_root->children.push_back(std::move(root));
                    new_root->children.push_back(std::move(new_node));
                    root = std::move(new_root);
                } else {
                    parent->children.push_back(std::move(new_node));
                }
            }
        }
    }

    Node* find_parent(Node* current, Node* child) {
        if (current == nullptr || current->is_leaf || current->children.empty()) {
            return nullptr;
        }

        for (const auto& child_node : current->children) {
            if (child_node.get() == child) {
                return current;
            }

            Node* parent = find_parent(child_node.get(), child);
            if (parent) {
                return parent;
            }
        }

        return nullptr;
    }

    void adjust_tree(Node* leaf) {
        Node* current = leaf;
        while (current != root.get()) {
            current->adjust_mbr();
            current = find_parent(root.get(), current);
        }
        root->adjust_mbr();
    }

    double calculate_enlargement(const Rectangle& mbr, const Point& point) {
        Rectangle enlarged = mbr;
        enlarged.lower.x = min(enlarged.lower.x, point.x);
        enlarged.lower.y = min(enlarged.lower.y, point.y);
        enlarged.upper.x = max(enlarged.upper.x, point.x);
        enlarged.upper.y = max(enlarged.upper.y, point.y);

        return enlarged.area() - mbr.area();
    }

public:
    RTree() : root(make_unique<Node>(true)) {}

    void insert(const Point& point) {
        if (root->is_full()) {
            auto new_root = make_unique<Node>(false);
            new_root->children.push_back(std::move(root));
            root = std::move(new_root);
            split_node(root->children[0].get());
        }

        Node* leaf = choose_leaf(point);
        leaf->entries.push_back(point);

        if (leaf->is_full()) {
            split_node(leaf);
        }

        adjust_tree(leaf);
    }

    vector<Point> search(const Rectangle& query_rect) const {
        vector<Point> result;
        search_recursive(root.get(), query_rect, result);
        return result;
    }

    void print() const {
        cout << "R-Tree Structure:\n";
        print_recursive(root.get(), 0);
    }

    void delete_point(const Point& point) {
        Node* leaf = choose_leaf(point);
        delete_entry_from_leaf(leaf, point);

        if (leaf->is_underfull() && leaf != root.get()) {
            Node* parent = find_parent(root.get(), leaf);
            Node* sibling = find_sibling(parent, leaf);

            if (sibling && !sibling->is_full()) {
                merge_nodes(sibling, leaf);
                remove_child(parent, leaf);

                if (parent->is_underfull() && parent != root.get()) {
                    Node* grandparent = find_parent(root.get(), parent);
                    Node* parent_sibling = find_sibling(grandparent, parent);

                    if (parent_sibling && !parent_sibling->is_full()) {
                        merge_nodes(parent_sibling, parent);
                        remove_child(grandparent, parent);
                    }
                }
            } else {
                if (root->children.size() == 1 && root->is_underfull()) {
                    root = std::move(root->children[0]);
                }
            }
        }

        adjust_tree(leaf);
    }

    Point nearest_neighbor(const Point& query_point) {
        Point nearest_point;
        double min_dist = numeric_limits<double>::max();

        nearest_neighbor_recursive(root.get(), query_point, nearest_point, min_dist);

        return nearest_point;
    }

    // Branch-and-bound k-nearest neighbors.
    // Uses a min-heap ordered by MBR minimum distance to visit the most-promising
    // subtrees first, and a max-heap of size k to prune branches that cannot
    // improve the current best results.
    vector<pair<double, Point>> k_nearest_neighbors(const Point& query_point, int k) {
        using DistPoint = pair<double, Point>;
        using DistNode  = pair<double, Node*>;

        // max-heap: top element is the farthest of the k nearest found so far
        priority_queue<DistPoint> result_heap;
        // min-heap: visit subtrees closest to query_point first
        priority_queue<DistNode, vector<DistNode>, greater<DistNode>> node_queue;

        node_queue.push({0.0, root.get()});

        while (!node_queue.empty()) {
            auto [node_min_dist, node] = node_queue.top();
            node_queue.pop();

            // Prune: once we have k results, any subtree farther than the
            // current kth-closest point cannot improve the answer
            if ((int)result_heap.size() == k && node_min_dist >= result_heap.top().first)
                break;

            if (node->is_leaf) {
                for (const auto& entry : node->entries) {
                    double d = calculate_distance(query_point, entry);
                    if ((int)result_heap.size() < k) {
                        result_heap.push({d, entry});
                    } else if (d < result_heap.top().first) {
                        result_heap.pop();
                        result_heap.push({d, entry});
                    }
                }
            } else {
                for (const auto& child : node->children) {
                    double d = min_dist_to_rect(query_point, child->mbr);
                    if ((int)result_heap.size() < k || d < result_heap.top().first)
                        node_queue.push({d, child.get()});
                }
            }
        }

        vector<DistPoint> results;
        while (!result_heap.empty()) {
            results.push_back(result_heap.top());
            result_heap.pop();
        }
        // Return sorted by ascending distance
        sort(results.begin(), results.end());
        return results;
    }

private:
    void search_recursive(Node* node, const Rectangle& query_rect, vector<Point>& result) const {
        if (!node->mbr.intersects(query_rect)) {
            return;
        }

        if (node->is_leaf) {
            for (const auto& entry : node->entries) {
                if (query_rect.contains(entry)) {
                    result.push_back(entry);
                }
            }
        } else {
            for (const auto& child : node->children) {
                search_recursive(child.get(), query_rect, result);
            }
        }
    }

    void print_recursive(Node* node, int level) const {
        string indent(level * 2, ' ');

        cout << indent << "Node MBR: ("
             << node->mbr.lower.x << "," << node->mbr.lower.y << ") - ("
             << node->mbr.upper.x << "," << node->mbr.upper.y << ")\n";

        if (node->is_leaf) {
            for (const auto& entry : node->entries) {
                cout << indent << " Entry: (" << entry.x << "," << entry.y << ")\n";
            }
        } else {
            for (const auto& child : node->children) {
                print_recursive(child.get(), level + 1);
            }
        }
    }
};
