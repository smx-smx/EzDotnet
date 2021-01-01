add_library(coreclrhost SHARED dotnetcore.cpp)
target_link_libraries(coreclrhost common)