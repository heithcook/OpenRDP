add_executable(openrdp_tests TestCore.cpp)
target_link_libraries(openrdp_tests PRIVATE openrdp_core Qt6::Test)
target_compile_options(openrdp_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME openrdp_tests COMMAND openrdp_tests)
