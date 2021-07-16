cmake_minimum_required(VERSION 3.5)

project(bin2obj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_PATH ${CMAKE_HOME_DIRECTORY}"/lib")

file(GLOB bin2obj_SRC
     "*.h"
     "*.cpp"

)

add_executable(bin2obj ${bin2obj_SRC})
target_link_libraries(bin2obj LINK_PUBLIC macro11lib)
