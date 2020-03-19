project(gen_data)
add_executable(gen_data gen_data.cpp)
target_link_libraries(gen_data PRIVATE ssl crypto)