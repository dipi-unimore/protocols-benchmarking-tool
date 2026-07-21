include(GoogleTest)

set(PBT_INCLUDE ${CMAKE_SOURCE_DIR}/include)

# Unit tests
add_executable(test_wire_header   unit/test_wire_header.cpp)
add_executable(test_bounded_queue unit/test_bounded_queue.cpp)
add_executable(test_statistics    unit/test_statistics.cpp)
add_executable(test_serializers   unit/test_serializers.cpp)
add_executable(test_compressors   unit/test_compressors.cpp)
add_executable(test_benchmark_config unit/test_benchmark_config.cpp)
add_executable(test_udp_frag      unit/test_udp_fragmentation.cpp)
add_executable(test_packet_metrics unit/test_packet_metrics.cpp)

foreach(t
    test_wire_header
    test_bounded_queue
    test_statistics
    test_serializers
    test_compressors
    test_benchmark_config
    test_udp_frag
    test_packet_metrics)
  target_include_directories(${t} PRIVATE ${PBT_INCLUDE} ${CMAKE_BINARY_DIR})
  target_link_libraries(${t} PRIVATE
      pbt_compiler_options
      pbt_core
      pbt_payload
      pbt_serialization
      pbt_compression
      pbt_transport
      pbt_metrics
      pbt_data
      GTest::gtest_main)
  gtest_discover_tests(${t} DISCOVERY_MODE PRE_TEST)
endforeach()

# Integration tests
add_executable(test_tcp_loopback  integration/test_tcp_loopback.cpp)
add_executable(test_udp_loopback  integration/test_udp_loopback.cpp)
add_executable(test_zmq_tcp_loopback integration/test_zmq_tcp_loopback.cpp)
add_executable(test_mqtt_tcp_loopback integration/test_mqtt_tcp_loopback.cpp)

foreach(t
    test_tcp_loopback
    test_udp_loopback
    test_zmq_tcp_loopback
    test_mqtt_tcp_loopback)
  target_include_directories(${t} PRIVATE ${PBT_INCLUDE} ${CMAKE_BINARY_DIR})
  target_link_libraries(${t} PRIVATE
      pbt_compiler_options
      pbt_core
      pbt_payload
      pbt_serialization
      pbt_compression
      pbt_transport
      pbt_metrics
      pbt_data
      pbt_sync
      GTest::gtest_main)
  gtest_discover_tests(${t} DISCOVERY_MODE PRE_TEST)
endforeach()
