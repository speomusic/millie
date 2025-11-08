find_package(GTest)

add_executable(tests
    util.h
    config.cpp
    merc-test.cpp
    module.cpp
    space.cpp)
target_link_libraries(tests PRIVATE merc GTest::GTest GTest::Main)

if(APPLE)
    set_target_properties(tests PROPERTIES
        XCODE_GENERATE_SCHEME true)
endif()

add_dependencies(tests
    buzz
    # bloom
    # lfo
    # deltick
    # texcube
    # tick
    # triangle
    # trancegate
    # royal

    # goodwin
    # zaval
)
target_compile_definitions(tests PRIVATE TEST_CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}")

add_test(NAME tests COMMAND tests)
