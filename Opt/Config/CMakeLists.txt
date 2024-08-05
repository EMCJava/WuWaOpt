add_library(OptimizerConfig.lib OptimizerConfig.hpp OptimizerConfig.cpp)
add_library(EchoConstraint.lib EchoConstraint.hpp EchoConstraint.cpp)
add_library(CharacterConfig.lib CharacterConfig.hpp CharacterConfig.cpp)

target_link_libraries(OptimizerConfig.lib spdlog::spdlog_header_only FullStats.lib)
target_link_libraries(EchoConstraint.lib FullStats.lib)
target_link_libraries(CharacterConfig.lib PRIVATE yaml-cpp::yaml-cpp FullStats.lib)
