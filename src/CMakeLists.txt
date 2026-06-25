set(PBT_INCLUDE ${CMAKE_SOURCE_DIR}/include)

# pbt_core
add_library(pbt_core STATIC
    core/BenchmarkConfig.cpp
    core/RunResult.cpp)
target_include_directories(pbt_core PUBLIC ${PBT_INCLUDE})
target_link_libraries(pbt_core PUBLIC
    pbt_compiler_options
    nlohmann_json::nlohmann_json)

# pbt_payload
add_library(pbt_payload STATIC
    payload/PayloadSource.cpp
    payload/RandomSource.cpp
    payload/FileSource.cpp)
target_include_directories(pbt_payload PUBLIC ${PBT_INCLUDE})
target_link_libraries(pbt_payload PUBLIC
    pbt_compiler_options
    pbt_core
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp)

# pbt_serialization
add_library(pbt_serialization STATIC
    serialization/NoneSerializer.cpp
    serialization/CborSerializer.cpp
    serialization/ProtobufSerializer.cpp)
target_include_directories(pbt_serialization PUBLIC
    ${PBT_INCLUDE}
    ${CMAKE_BINARY_DIR})
target_link_libraries(pbt_serialization PUBLIC
    pbt_compiler_options
    pbt_core
    pbt_proto
    tinycbor
    nlohmann_json::nlohmann_json)

# pbt_compression
add_library(pbt_compression STATIC
    compression/NoopCompressor.cpp
    compression/ZstdCompressor.cpp)
target_include_directories(pbt_compression PUBLIC ${PBT_INCLUDE})
target_link_libraries(pbt_compression PUBLIC
    pbt_compiler_options
    pbt_core
    libzstd_static)

# pbt_transport
add_library(pbt_transport STATIC
    transport/Sender.cpp
    transport/Receiver.cpp
    transport/ReassemblyBuffer.cpp
    transport/TcpSender.cpp
    transport/TcpReceiver.cpp
    transport/UdpSender.cpp
    transport/UdpReceiver.cpp
    transport/ZmqTcpSender.cpp
    transport/ZmqTcpReceiver.cpp
    transport/MqttTcpSender.cpp
    transport/MqttTcpReceiver.cpp)
target_include_directories(pbt_transport PUBLIC ${PBT_INCLUDE})
target_link_libraries(pbt_transport PUBLIC
    pbt_compiler_options
    pbt_core
    pbt_payload
    pbt_serialization
    pbt_compression
    cppzmq-static
    paho-mqttpp3-static)

# pbt_sync
add_library(pbt_sync STATIC
    sync/NtpSync.cpp)
target_include_directories(pbt_sync PUBLIC ${PBT_INCLUDE})
target_link_libraries(pbt_sync PUBLIC
    pbt_compiler_options
    pbt_core)

# pbt_data
add_library(pbt_data STATIC
    data/DataManager.cpp
    data/CsvPacketWriter.cpp
    data/CsvSenderWriter.cpp)
target_include_directories(pbt_data PUBLIC ${PBT_INCLUDE})
target_link_libraries(pbt_data PUBLIC
    pbt_compiler_options
    pbt_core
    nlohmann_json::nlohmann_json)

# pbt_metrics
add_library(pbt_metrics STATIC
    metrics/Statistics.cpp
    metrics/PacketProcessor.cpp)
target_include_directories(pbt_metrics PUBLIC ${PBT_INCLUDE})
target_link_libraries(pbt_metrics PUBLIC
    pbt_compiler_options
    pbt_core
    pbt_serialization
    pbt_compression
    pbt_data)
