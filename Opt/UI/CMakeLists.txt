add_library(UIConfig.lib UIConfig.hpp UIConfig.cpp)
add_library(Backpack.lib Backpack.hpp Backpack.cpp)

target_link_libraries(UIConfig.lib ImGui-SFML::ImGui-SFML)
target_link_libraries(Backpack.lib FullStats.lib UIConfig.lib Loca.lib CharacterPage.lib magic_enum::magic_enum spdlog::spdlog_header_only)

add_subdirectory(Page)
