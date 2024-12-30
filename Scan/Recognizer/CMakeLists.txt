add_library(EchoExtractor.lib EchoExtractor.hpp EchoExtractor.cpp)

target_link_libraries(EchoExtractor.lib opencv_ml FullStats.lib)

target_link_libraries(Scan.lib INTERFACE EchoExtractor.lib)
