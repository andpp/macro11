cmake_minimum_required(VERSION 3.5)

project(dumpobj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB dumpobj_SRC
     "*.h"
     "*.cpp"

)

add_executable(dumpobj ${dumpobj_SRC})
