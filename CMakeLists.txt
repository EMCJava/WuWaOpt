cmake_minimum_required(VERSION 3.28)
project(WuWaOpt)

set(CMAKE_CXX_STANDARD 23)

find_package(OpenCV CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(SFML COMPONENTS system window graphics CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

include_directories(.)
add_subdirectory(Scan)

add_executable(WuWaScan WuWaScan.cpp)
add_executable(WuWaOptim WuWaOptim.cpp)

add_library(WuWaGA INTERFACE Opt/WuWaGa.hpp Opt/WuWaGa.inl)
target_link_libraries(WuWaGA INTERFACE nlohmann_json::nlohmann_json)

target_link_libraries(WuWaScan PRIVATE opencv_highgui opencv_ml opencv_dnn opencv_core opencv_flann nlohmann_json::nlohmann_json Scan.lib)
target_link_libraries(WuWaOptim PRIVATE WuWaGA nlohmann_json::nlohmann_json implot::implot sfml-system sfml-network sfml-graphics sfml-window ImGui-SFML::ImGui-SFML spdlog::spdlog)
