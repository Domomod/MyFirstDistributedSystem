set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CPP_COMPILER MPI_CXX_COMPILER)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE DistributedSystemAPI)

add_executable(invite_only invite_only.cpp)
target_link_libraries(invite_only PRIVATE DistributedSystemAPI)

add_executable(renewable_only renewable_only.cpp)
target_link_libraries(renewable_only PRIVATE DistributedSystemAPI)