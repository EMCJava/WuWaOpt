add_library(MouseControl.lib MouseControl.hpp MouseControl.cpp)
add_library(GameHandle.lib GameHandle.hpp GameHandle.cpp)

target_link_libraries(GameHandle.lib PRIVATE spdlog::spdlog)

add_library(Scan.lib INTERFACE)
target_link_libraries(Scan.lib INTERFACE MouseControl.lib GameHandle.lib)