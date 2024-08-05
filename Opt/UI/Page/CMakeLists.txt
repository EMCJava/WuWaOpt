add_library(CharacterPage.lib CharacterPage.hpp CharacterPage.cpp)

add_library(Pages.lib INTERFACE)
target_link_libraries(Pages.lib INTERFACE CharacterPage.lib)