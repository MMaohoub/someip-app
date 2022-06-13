cmake_minimum_required (VERSION 3.18)

project (client)

add_subdirectory("./vsomeip")

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(${VSOMEIP_INCLUDE_DIRS})

if (NOT ${VSOMEIP_NAME}_FOUND)
    message("${VSOMEIP_NAME} was not found. Please specify vsomeip_DIR")
endif()

add_executable (client src/client.cpp)

target_link_libraries(client vsomeip3)

add_dependencies(client vsomeip3-cfg vsomeip3-sd)

target_link_libraries(client ${VSOMEIP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
