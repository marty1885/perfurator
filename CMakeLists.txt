cmake_minimum_required(VERSION 3.0)
project(perfurator)

set(CMAKE_CXX_STANDARD 17)

include_directories(libs)

add_subdirectory(gen_data)
add_subdirectory(api_server)
add_subdirectory(validation_server)
add_subdirectory(client)
add_subdirectory(tests)