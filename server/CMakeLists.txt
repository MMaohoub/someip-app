cmake_minimum_required (VERSION 2.8.7)

project (VSomeIPClient)

add_subdirectory("./vsomeip")

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(${VSOMEIP_INCLUDE_DIRS})

if (NOT ${VSOMEIP_NAME}_FOUND)
    message("${VSOMEIP_NAME} was not found. Please specify vsomeip_DIR")
endif()

add_executable (serverApp server.cpp)
target_link_libraries(serverApp vsomeip3)
add_dependencies(serverApp vsomeip3-cfg vsomeip3-sd)

target_link_libraries(serverApp  ${VSOMEIP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
