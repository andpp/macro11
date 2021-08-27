cmake_minimum_required(VERSION 3.5)

project(link11 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB link11_SRC
     "*.h"
     "*.cpp"

)

add_executable(link11 ${link11_SRC})

