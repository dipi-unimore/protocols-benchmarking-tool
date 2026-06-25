add_executable(pb-tool main.cpp)
target_include_directories(pb-tool PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(pb-tool PRIVATE
    pbt_compiler_options
    pbt_core
    pbt_payload
    pbt_serialization
    pbt_compression
    pbt_transport
    pbt_sync
    pbt_metrics
    pbt_data
    nlohmann_json::nlohmann_json
    spdlog::spdlog)
