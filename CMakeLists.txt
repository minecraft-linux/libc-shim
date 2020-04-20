cmake_minimum_required (VERSION 2.6)

project(libc-shim LANGUAGES CXX)

add_library(libc-shim src/common.cpp src/pthreads.cpp src/pthreads.h src/meta.h src/common.h src/semaphore.cpp src/semaphore.h src/network.cpp src/network.h)
target_include_directories(libc-shim PUBLIC include/)