cmake_minimum_required(VERSION 3.5)

project(macro11 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_PATH ${CMAKE_HOME_DIRECTORY}"/lib")

file(GLOB macro11_SRC
     "*.h"
     "*.cpp"

)

add_executable(macro11 ${macro11_SRC})
target_link_libraries(macro11 LINK_PUBLIC macro11lib)

