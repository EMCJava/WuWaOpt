add_library(Loca.lib Loca.hpp Loca.cpp)
add_library(StringArrayObserver.lib StringArrayObserver.hpp StringArrayObserver.cpp)

target_link_libraries(Loca.lib nlohmann_json::nlohmann_json spdlog::spdlog_header_only yaml-cpp::yaml-cpp)
target_link_libraries(StringArrayObserver.lib Loca.lib)
