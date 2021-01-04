cmake_minimum_required(VERSION 3.1)

project(
  bcenabler
  VERSION 1.0
  LANGUAGES CXX)

add_library(bcenabler bcenabler.cpp bcenabler.h)

include_directories(".")
target_link_libraries(bcenabler vulkan android vulkan)
target_compile_options(bcenabler PRIVATE -Wall)

