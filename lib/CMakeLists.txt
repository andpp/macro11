cmake_minimum_required(VERSION 3.5)

file(GLOB macro11lib_SRC
     ${CMAKE_HOME_DIRECTORY}/lib/*.h
     ${CMAKE_HOME_DIRECTORY}/lib/*.cpp
) 

add_library (macro11lib STATIC ${macro11lib_SRC})
target_include_directories (macro11lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
