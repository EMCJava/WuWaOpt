add_library(SubStatRollConfig.lib SubStatRollConfig.hpp SubStatRollConfig.cpp)

target_link_libraries(SubStatRollConfig.lib FullStats.lib)