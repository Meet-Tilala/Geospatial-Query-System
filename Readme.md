# Geospatial Query System

A high-performance spatial indexing and querying system built with a custom **R-tree** in C++, exposed via a REST API ([Crow](https://crowcpp.org/) framework). Supports concurrent access, range queries, nearest-neighbor search, k-nearest-neighbor search, and polygon intersection.

---

## Features

| Feature | Endpoint |
|---|---|
| Insert a point | `POST /api/point` |
| Nearest neighbor | `POST /api/nearest_neighbor` |
| **k-nearest neighbors** | **`POST /api/k_nearest`** |
| Range query | `POST /api/range_query` |
| Polygon intersection | `POST /api/intersection` |

### R-Tree implementation

- Maximum entries per node: 4 (configurable via `MAX_ENTRIES`)
- Minimum entries per node: 2
- Quadratic split heuristic
- Dynamic insertion and deletion with MBR adjustment
- 1-NN via recursive branch-and-bound
- **k-NN via priority-queue branch-and-bound** — visits subtrees in order of minimum MBR distance and prunes branches that cannot improve the current k-th best result

---

## Project structure

```
.
├── Backend/
│   ├── RTrees.cpp      # R-tree data structure + k-NN
│   ├── server.cpp      # Crow REST API
│   ├── benchmark.cpp   # R-Tree vs brute-force timing & k-NN correctness
│   └── json.hpp        # nlohmann/json (vendored)
├── Frontend/
│   ├── index.html
│   ├── script.js
│   └── style.css
├── CMakeLists.txt
└── Readme.md
```

---

## Prerequisites

| Tool | Minimum version |
|---|---|
| C++ compiler | C++17 (GCC 7+, Clang 5+, MSVC 2017+) |
| CMake | 3.8 |
| Boost (Asio) | 1.66 |
| [Crow](https://github.com/CrowCpp/Crow) | latest |
| OpenSSL | optional (HTTPS) |

### Install dependencies

**Ubuntu / Debian**
```bash
sudo apt update
sudo apt install build-essential cmake libboost-all-dev libssl-dev
```

**macOS (Homebrew)**
```bash
brew install cmake boost openssl
```

**Windows (vcpkg)**
```powershell
vcpkg install boost:x64-windows openssl:x64-windows
vcpkg integrate install
```

### Install Crow

```bash
git clone https://github.com/CrowCpp/Crow.git
cd Crow && mkdir build && cd build
cmake .. && make && sudo make install
```

---

## Build

```bash
git clone https://github.com/Meet-Tilala/Geospatial-Query-System.git
cd Geospatial-Query-System

mkdir build && cd build
cmake ..
make -j$(nproc)
```

This produces two binaries inside `build/`:

| Binary | Purpose |
|---|---|
| `server` | REST API server (port 3000) |
| `benchmark` | Timing & correctness suite |

---

## Run

**Server**
```bash
./build/server
```
The server starts on `http://localhost:3000`.

**Benchmark**
```bash
./build/benchmark
```
Inserts 50 000 random points, then runs 100 range queries and 100 k-NN queries (k = 10) comparing R-tree vs brute-force, followed by a correctness check.

---

## API reference

All requests and responses use JSON (`Content-Type: application/json`).

### `POST /api/point`
Insert a geographic point.
```json
{ "lat": 26.4751, "lng": 73.1170 }
```
Response: `200 OK`

---

### `POST /api/nearest_neighbor`
Find the single nearest stored point to a query location.
```json
{ "lat": 26.4765, "lng": 73.1132 }
```
```json
{ "lat": 26.4751, "lng": 73.1170 }
```

---

### `POST /api/k_nearest`
Find the **k nearest** stored points, returned sorted by ascending distance.
```json
{ "lat": 26.4765, "lng": 73.1132, "k": 5 }
```
```json
[
  { "lat": 26.4751, "lng": 73.1170, "distance": 0.0023 },
  { "lat": 26.4769, "lng": 73.1142, "distance": 0.0041 },
  ...
]
```

---

### `POST /api/range_query`
Return all points within an axis-aligned bounding box.
```json
{ "min_lat": 26.472, "min_lng": 73.110, "max_lat": 26.477, "max_lng": 73.118 }
```
```json
[{ "lat": 26.4751, "lng": 73.1170 }, ...]
```

---

### `POST /api/intersection`
Return all stored points that fall inside a polygon (ray-casting algorithm).
```json
{ "points": [[26.472, 73.110], [26.477, 73.110], [26.477, 73.118], [26.472, 73.118]] }
```
```json
[{ "lat": 26.4751, "lng": 73.1170 }, ...]
```

---

## HTTP status codes

| Code | Meaning |
|---|---|
| 200 | Success |
| 400 | Invalid or malformed input |
| 500 | Internal server error |

---

## Limitations

- 2D spatial data only
- In-memory storage (no persistence across restarts)
- Fixed node capacity (change `MAX_ENTRIES` in `RTrees.cpp` and recompile)

---

## Contact

Meet Tilala — meettilala2005@gmail.com
