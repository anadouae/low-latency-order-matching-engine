add_executable(ome_tests
  unit/order_book_test.cpp
  unit/matching_engine_test.cpp
  unit/cancel_amend_test.cpp
  unit/queue_test.cpp
  integration/gateway_to_bus_test.cpp
)

target_link_libraries(ome_tests PRIVATE ome GTest::gtest_main)
target_include_directories(ome_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fixtures)
ome_set_project_warnings(ome_tests)
ome_enable_sanitizers(ome_tests)

include(GoogleTest)
gtest_discover_tests(ome_tests)
