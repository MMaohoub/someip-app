project (server)

add_subdirectory("./vsomeip")

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(${VSOMEIP_INCLUDE_DIRS})

if (NOT ${VSOMEIP_NAME}_FOUND)
    message("${VSOMEIP_NAME} was not found. Please specify vsomeip_DIR")
endif()

add_executable (server src/server.cpp)

target_link_libraries(server vsomeip3)

add_dependencies(server vsomeip3-cfg vsomeip3-sd)

target_link_libraries(server ${VSOMEIP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})


