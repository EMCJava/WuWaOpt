add_library(Scan.lib INTERFACE)

add_subdirectory(Win)
add_subdirectory(Recognizer)
add_subdirectory(Thread)

add_library(BackpackEchoScanner BackpackEchoScanner.hpp BackpackEchoScanner.cpp)
target_link_libraries(BackpackEchoScanner FullStats.lib GameHandle.lib AdaptiveJobPool.lib EchoExtractor.lib spdlog::spdlog_header_only)
target_link_libraries(Scan.lib INTERFACE BackpackEchoScanner)
