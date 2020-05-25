#Source folder

file(GLOB_RECURSE HEADER_LIST CONFIGURE_DEPENDS "${MyFirstDistributedSystem_SOURCE_DIR}/Source/*.h")
file(GLOB_RECURSE SOURCE_LIST CONFIGURE_DEPENDS "${MyFirstDistributedSystem_SOURCE_DIR}/Source/*.cpp")

message(STATUS "HEADER_LIST: ${HEADER_LIST}")
message(STATUS "SOURCE_LIST: ${SOURCE_LIST}")

# Make an automatic library - will be static or dynamic based on user setting
add_library(DistributedSystemAPI ${SOURCE_LIST} ${HEADER_LIST})

find_package(MPI REQUIRED)
set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})

target_include_directories(DistributedSystemAPI PUBLIC ${MyFirstDistributedSystem_SOURCE_DIR}/Source)

target_include_directories(DistributedSystemAPI PUBLIC ${MPI_INCLUDE_PATH})
target_link_libraries(DistributedSystemAPI PUBLIC ${MPI_LIBRARIES})

target_link_libraries(DistributedSystemAPI PRIVATE Threads::Threads)


target_compile_features(DistributedSystemAPI PUBLIC cxx_std_11)
