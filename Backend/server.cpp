#include "crow.h"
#include "json.hpp"
#include "crow/middlewares/cors.h"
#include <mutex>
#include <memory>
#include "RTrees.cpp"

using json = nlohmann::json;

// Forward declaration of isPointInPolygon
bool isPointInPolygon(const Point& point, const std::vector<Point>& polygon);

// Utility functions for coordinate conversion
Point latLngToPoint(double lat, double lng) {
    return Point(lat, lng);
}

json pointToLatLng(const Point& point) {
    json result;
    result["lat"] = point.x;
    result["lng"] = point.y;
    return result;
}

// Main application
int main() {
    crow::App<crow::CORSHandler> app;
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .headers("Content-Type", "Authorization", "Accept")
        .methods("POST"_method, "GET"_method, "OPTIONS"_method)
        .origin("*")
        .allow_credentials();

    static RTree rtree;
    static std::mutex tree_mutex;

    // Handle CORS OPTIONS requests
    CROW_ROUTE(app, "/api/<path>").methods("OPTIONS"_method)
    ([](const crow::request&, const std::string&) {
        return crow::response(200);
    });

    // POST endpoint for adding points
    CROW_ROUTE(app, "/api/point").methods("POST"_method)
    ([](const crow::request& req) {
        auto x = json::parse(req.body, nullptr, false);
        if (x.is_discarded()) return crow::response(400, "Invalid JSON");

        try {
            double lat = x["lat"];
            double lng = x["lng"];
            {
                std::lock_guard<std::mutex> lock(tree_mutex);
                rtree.insert(latLngToPoint(lat, lng));
            }
            return crow::response(200);
        } catch (const std::exception& e) {
            return crow::response(400, "Invalid point data");
        }
    });

    // POST endpoint for nearest neighbor
    CROW_ROUTE(app, "/api/nearest_neighbor").methods("POST"_method)
    ([](const crow::request& req) {
        auto x = json::parse(req.body, nullptr, false);
        if (x.is_discarded()) return crow::response(400, "Invalid JSON");

        try {
            double lat = x["lat"];
            double lng = x["lng"];
            Point query = latLngToPoint(lat, lng);
            Point nearest;

            {
                std::lock_guard<std::mutex> lock(tree_mutex);
                nearest = rtree.nearest_neighbor(query);
            }
            return crow::response(pointToLatLng(nearest).dump());
        } catch (const std::exception& e) {
            return crow::response(400, "Invalid query point");
        }
    });

    // POST endpoint for range queries
    CROW_ROUTE(app, "/api/range_query").methods("POST"_method)
    ([](const crow::request& req) {
        auto x = json::parse(req.body, nullptr, false);
        if (x.is_discarded()) return crow::response(400, "Invalid JSON");

        try {
            double min_lat = x["min_lat"];
            double min_lng = x["min_lng"];
            double max_lat = x["max_lat"];
            double max_lng = x["max_lng"];

            Rectangle query_rect(latLngToPoint(min_lat, min_lng), latLngToPoint(max_lat, max_lng));
            std::vector<Point> results;
            
            {
                std::lock_guard<std::mutex> lock(tree_mutex);
                results = rtree.search(query_rect);
            }

            json response = json::array();
            for (const auto& point : results) {
                response.push_back(pointToLatLng(point));
            }
            return crow::response(response.dump());
        } catch (const std::exception& e) {
            return crow::response(400, "Invalid rectangle coordinates");
        }
    });

    // POST endpoint for intersection detection
    CROW_ROUTE(app, "/api/intersection").methods("POST"_method)
    ([](const crow::request& req) {
        auto x = json::parse(req.body, nullptr, false);
        if (x.is_discarded()) return crow::response(400, "Invalid JSON");

        try {
            std::vector<Point> polygon_points;
            for (const auto& point : x["points"]) {
                double lat = point[0];
                double lng = point[1];
                polygon_points.push_back(latLngToPoint(lat, lng));
            }

            // Calculate bounding box
            double min_lat = std::numeric_limits<double>::max();
            double min_lng = std::numeric_limits<double>::max();
            double max_lat = std::numeric_limits<double>::lowest();
            double max_lng = std::numeric_limits<double>::lowest();

            for (const auto& point : polygon_points) {
                min_lat = std::min(min_lat, point.x);
                min_lng = std::min(min_lng, point.y);
                max_lat = std::max(max_lat, point.x);
                max_lng = std::max(max_lng, point.y);
            }

            Rectangle query_rect(Point(min_lat, min_lng), Point(max_lat, max_lng));
            std::vector<Point> results;

            {
                std::lock_guard<std::mutex> lock(tree_mutex);
                results = rtree.search(query_rect);
            }

            json intersecting_points = json::array();
            for (const auto& point : results) {
                if (isPointInPolygon(point, polygon_points)) {
                    intersecting_points.push_back(pointToLatLng(point));
                }
            }
            return crow::response(intersecting_points.dump());
        } catch (const std::exception& e) {
            return crow::response(400, "Invalid polygon coordinates");
        }
    });

    // POST endpoint for k-nearest neighbors
    CROW_ROUTE(app, "/api/k_nearest").methods("POST"_method)
    ([](const crow::request& req) {
        auto x = json::parse(req.body, nullptr, false);
        if (x.is_discarded()) return crow::response(400, "Invalid JSON");

        try {
            double lat = x["lat"];
            double lng = x["lng"];
            int k = x.value("k", 1);
            if (k <= 0) return crow::response(400, "k must be positive");

            Point query = latLngToPoint(lat, lng);
            vector<pair<double, Point>> results;

            {
                std::lock_guard<std::mutex> lock(tree_mutex);
                results = rtree.k_nearest_neighbors(query, k);
            }

            json response = json::array();
            for (const auto& [dist, pt] : results) {
                json entry = pointToLatLng(pt);
                entry["distance"] = dist;
                response.push_back(entry);
            }
            return crow::response(response.dump());
        } catch (const std::exception& e) {
            return crow::response(400, "Invalid request data");
        }
    });

    app.port(3000).multithreaded().run();
    return 0;
}

// Function to check if a point is within a polygon
bool isPointInPolygon(const Point& point, const std::vector<Point>& polygon) {
    bool inside = false;
    size_t j = polygon.size() - 1;
    
    for (size_t i = 0; i < polygon.size(); i++) {
        if ((polygon[i].y > point.y) != (polygon[j].y > point.y) &&
            point.x < (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) /
                      (polygon[j].y - polygon[i].y) + polygon[i].x) {
            inside = !inside;
        }
        j = i;
    }
    return inside;
} 
