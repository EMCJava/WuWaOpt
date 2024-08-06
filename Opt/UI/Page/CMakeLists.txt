add_library(CharacterPage.lib CharacterPage.hpp CharacterPage.cpp)

target_link_libraries(CharacterPage.lib PRIVATE yaml-cpp::yaml-cpp CharacterConfig.lib)

add_library(Pages.lib INTERFACE)
target_link_libraries(Pages.lib INTERFACE CharacterPage.lib)