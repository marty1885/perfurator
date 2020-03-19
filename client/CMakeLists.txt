project(client)
add_executable(client client.cpp)
target_link_libraries(client PRIVATE curl fmt)