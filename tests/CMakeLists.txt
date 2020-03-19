project(test)
add_executable(test test_main.cpp test_crypto.cpp)
target_link_libraries(test PRIVATE ssl crypto)