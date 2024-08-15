cmake_minimum_required(VERSION 3.28)

set(VCPKG_TARGET_TRIPLET x64-windows-static)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(WuWaOpt VERSION 1.2.4 LANGUAGES CXX)

if (NOT MSVC)
    message(FATAL_ERROR "MSVC should be used as compiler")
endif ()

set(CMAKE_CXX_STANDARD 23)

find_package(OpenCV CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(SFML COMPONENTS system window graphics CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)

add_compile_options(/bigobj /utf-8)
add_compile_definitions(UNICODE _UNICODE IMGUI_DEFINE_MATH_OPERATORS)
add_compile_definitions(WUWAOPT_VERSION="v${PROJECT_VERSION}")

include_directories(.)

add_subdirectory(Loca)
add_subdirectory(Common)
add_subdirectory(Scan)
add_subdirectory(Opt)

add_executable(WuWaScan WuWaScan.cpp)
add_executable(WuWaOptim WuWaOptim.cpp)

add_library(WuWaGA INTERFACE Opt/WuWaGa.hpp Opt/WuWaGa.inl)

target_link_libraries(WuWaGA INTERFACE EchoConstraint.lib CharacterConfig.lib)

add_library(WuWaGASwitcher Opt/OptimizerParmSwitcher.hpp Opt/OptimizerParmSwitcher.cpp)
target_link_libraries(WuWaGASwitcher spdlog::spdlog_header_only)
target_link_libraries(WuWaGA INTERFACE nlohmann_json::nlohmann_json WuWaGASwitcher)

target_link_libraries(WuWaScan PRIVATE Loca.lib opencv_highgui opencv_ml opencv_dnn opencv_core opencv_flann nlohmann_json::nlohmann_json Scan.lib)
target_link_libraries(WuWaOptim PRIVATE CharacterConfig.lib Pages.lib Backpack.lib EchoConstraint.lib OptimizerUIConfig.lib StringArrayObserver.lib OptimizerConfig.lib SubStatRollConfig.lib CombinationTweaker.lib CombinationMetaCache.lib WuWaGA httplib::httplib Loca.lib nlohmann_json::nlohmann_json implot::implot sfml-system sfml-network sfml-graphics sfml-window ImGui-SFML::ImGui-SFML spdlog::spdlog_header_only)

set_target_properties(WuWaScan
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/WuWaOpt"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/WuWaOpt"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/WuWaOpt"
        LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
)

set_target_properties(WuWaOptim
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/WuWaOpt"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/WuWaOpt"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/WuWaOpt"
)

add_custom_target(
        MakeReleaseFiles
        COMMAND
        ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/data/"
        "data/"
        COMMAND
        ${CMAKE_COMMAND} -E tar
        "cfv" "Release.zip" --format=zip
        "WuWaOptim.exe"
        "WuWaScan.exe"
        "data/"
        COMMAND
        ${CMAKE_COMMAND} -E copy
        "Release.zip"
        "Release_v${PROJECT_VERSION}.zip"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/WuWaOpt"
        DEPENDS
        WuWaScan WuWaOptim)