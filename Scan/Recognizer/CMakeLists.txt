add_library(StatExtractor.lib EchoExtractor.hpp EchoExtractor.cpp)

target_link_libraries(StatExtractor.lib opencv_ml FullStats.lib)

target_link_libraries(Scan.lib INTERFACE StatExtractor.lib)
