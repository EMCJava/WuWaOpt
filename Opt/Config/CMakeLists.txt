add_library(OptimizerConfig.lib OptimizerConfig.hpp OptimizerConfig.cpp)

target_link_libraries(OptimizerConfig.lib spdlog::spdlog_header_only FullStats.lib)