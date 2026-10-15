cmake_minimum_required(VERSION 3.20)
project(order-matching-engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(OME_BUILD_TESTS "Build Google Test suite" ON)
option(OME_BUILD_BENCHMARKS "Build Google Benchmark targets" ON)
option(OME_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(OME_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(OME_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(OME_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Sanitizers)

add_library(ome
  src/order.cpp
  src/price_level.cpp
  src/order_book.cpp
  src/matching_strategy.cpp
  src/matching_engine.cpp
  src/order_gateway.cpp
  src/protocol/binary_codec.cpp
)

add_library(ome::ome ALIAS ome)

target_include_directories(ome
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

ome_set_project_warnings(ome)
ome_enable_sanitizers(ome)

add_executable(ome_cli src/main.cpp)
target_link_libraries(ome_cli PRIVATE ome)
ome_set_project_warnings(ome_cli)
ome_enable_sanitizers(ome_cli)

if(OME_BUILD_TESTS OR OME_BUILD_BENCHMARKS)
  include(Dependencies)
endif()

if(OME_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(OME_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

install(TARGETS ome ome_cli
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ome DESTINATION include)
