cmake_minimum_required(VERSION 3.8)
project(GeospatialQuerySystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# Optional: OpenSSL (needed only if Crow is compiled with HTTPS support)
find_package(OpenSSL QUIET)

# ── Server ────────────────────────────────────────────────────────────────────
add_executable(server Backend/server.cpp)

target_include_directories(server PRIVATE
    ${Boost_INCLUDE_DIRS}
    Backend
)

target_link_libraries(server PRIVATE
    ${Boost_LIBRARIES}
    Threads::Threads
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::Crypto>
)

# ── Benchmark ─────────────────────────────────────────────────────────────────
add_executable(benchmark Backend/benchmark.cpp)

target_include_directories(benchmark PRIVATE Backend)

target_link_libraries(benchmark PRIVATE Threads::Threads)

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS server DESTINATION bin)
