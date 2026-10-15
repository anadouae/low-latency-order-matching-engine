add_executable(lob_hotpath_bench lob_hotpath_bench.cpp)
target_link_libraries(lob_hotpath_bench PRIVATE ome benchmark::benchmark)
ome_set_project_warnings(lob_hotpath_bench)

add_executable(queue_bench queue_bench.cpp)
target_link_libraries(queue_bench PRIVATE ome benchmark::benchmark)
ome_set_project_warnings(queue_bench)
