add_library(OptimizerUIConfig.lib OptimizerUIConfig.hpp OptimizerUIConfig.cpp)
add_library(Backpack.lib Backpack.hpp Backpack.cpp)

target_link_libraries(OptimizerUIConfig.lib ImGui-SFML::ImGui-SFML)
target_link_libraries(Backpack.lib FullStats.lib OptimizerUIConfig.lib Loca.lib)
