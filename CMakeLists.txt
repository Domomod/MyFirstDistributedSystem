cmake_minimum_required(VERSION 3.15)
project(MyFirstDistributedSystem)

set(CMAKE_CXX_STANDARD 20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Adds Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_subdirectory(Source)
add_subdirectory(Build)