set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CPP_COMPILER MPI_CXX_COMPILER)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE DistributedSystemAPI)