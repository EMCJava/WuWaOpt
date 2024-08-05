add_library(FullStats.lib FullStats.hpp FullStats.cpp)
add_library(EffectiveStats.lib EffectiveStats.hpp EffectiveStats.cpp)

target_link_libraries(FullStats.lib yaml-cpp::yaml-cpp magic_enum::magic_enum Loca.lib)