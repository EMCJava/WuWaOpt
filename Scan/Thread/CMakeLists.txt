add_library(AdaptiveJobPool.lib AdaptiveJobPool.hpp AdaptiveJobPool.cpp)

target_link_libraries(Scan.lib INTERFACE AdaptiveJobPool.lib)
