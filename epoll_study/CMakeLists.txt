cmake_minimum_required(VERSION 2.8)
project(httpd)
set(CMAKE_CXX_FLAGS "-g")
aux_source_directory(. SRC_FILES)
add_executable(httpd ${SRC_FILES})
