add_library(FullStats.lib INTERFACE FullStats.hpp)
target_link_libraries(FullStats.lib INTERFACE nlohmann_json::nlohmann_json)

add_subdirectory(UI)
add_subdirectory(SubStatRolling)
add_subdirectory(Tweak)
add_subdirectory(Config)
