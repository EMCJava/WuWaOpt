add_library(MouseControl.lib MouseControl.hpp MouseControl.cpp)
add_library(GameHandle.lib GameHandle.hpp GameHandle.cpp)

target_link_libraries(GameHandle.lib PRIVATE spdlog::spdlog_header_only)

target_link_libraries(Scan.lib INTERFACE MouseControl.lib GameHandle.lib)