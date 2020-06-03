
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/console.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mqtt_client.cpp)

add_executable(sample-master ${TEST_SOURCES})
set_property(TARGET sample-master PROPERTY CXX_STANDARD 11)
target_compile_options(sample-master PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-exit-time-destructors -Wno-disabled-macro-expansion -Wno-global-constructors -Wno-weak-vtables>)
target_link_libraries(sample-master PRIVATE homie-cpp gtest gtest_main pthread)
